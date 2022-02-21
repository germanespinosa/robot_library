import sys
from cellworld import World, Display, Location, Agent_markers, Capture, Capture_parameters, Step, Timer
from controller import ControllerClient
from cellworld_experiment_service import ExperimentClient
import matplotlib
time_out = 1.0


class AgentData:
    def __init__(self, agent_name: str):
        self.is_valid = None
        self.step = Step()
        self.step.agent_name = agent_name

experiment_service = ExperimentClient()
experiment_service.connect("127.0.0.1")
experiment_service.set_request_time_out(5000)
experiments = {}


def on_experiment_started(experiment):
    print("Experiment started:", experiment)
    experiments[experiment.experiment_name] = experiment.copy()

display = None
world = None

def load_world(occlusions):
    global display
    global world
    world = World.get_from_parameters_names("hexagonal", "canonical", occlusions)
    display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)


def on_episode_started(experiment_name):
    print (experiment_name)
    #load_world(experiments[experiment_name].world.occlusions)


experiment_service.on_experiment_started = on_experiment_started
experiment_service.on_episode_started = on_episode_started

experiment_service.subscribe()

if "-e" in sys.argv:
    e = experiment_service.start_experiment(prefix="PREFIX",
                                            suffix="SUFFIX",
                                            subject_name="SUBJECT",
                                            world_configuration="hexagonal",
                                            world_implementation="vr",
                                            occlusions="10_05",
                                            duration=10)
    print(e)

predator = AgentData("predator")
prey = AgentData("prey")


def on_step(step: Step):
    if step.agent_name == "predator":
        predator.is_valid = Timer(time_out)
        predator.step = step
        controller.set_behavior(ControllerClient.Behavior.Explore)
    else:
        prey.is_valid = Timer(time_out)
        prey.step = step
        controller.set_behavior(ControllerClient.Behavior.Pursue)

# connect to controller


controller = ControllerClient()
if not controller.connect("127.0.0.1", 4590):
    print("failed to connect to the controller")
    exit(1)
controller.set_request_time_out(10000)
controller.subscribe()
controller.on_step = on_step



load_world("10_00")

capture = Capture(Capture_parameters(2.0, 90.0),world)


def on_click(event):
    if event.button == 1:
        location = Location(event.xdata, event.ydata)  # event.x, event.y
        cell_id = world.cells.find(location)
        destination_cell = world.cells[cell_id]
        print("CELL", destination_cell)
        if destination_cell.occluded:
            print("can't navigate to an occluded cell")
            return
        controller.set_destination (destination_cell.location)
    else:
        print("starting experiment")
        occlusions = "10_05"
        exp = experiment_service.start_experiment(
            prefix="PREFIX",
            suffix="SUFFIX",
            occlusions=occlusions,
            world_implementation="canonical",
            world_configuration="hexagonal",
            subject_name="SUBJECT",
            duration=10)
        print ("EX", exp.experiment_name)
        r = experiment_service.start_episode(exp.experiment_name)
        print(r)

cid1 = display.fig.canvas.mpl_connect('button_press_event', on_click)
running = True


def on_keypress(event):
    global running
    if event.key == "p":
        controller.pause()
    if event.key == "r":
        controller.resume()
    if event.key == "q":
        running = False


cid_keypress = display.fig.canvas.mpl_connect('key_press_event', on_keypress)

display.set_agent_marker("predator", Agent_markers.arrow())

display.set_agent_marker("prey", Agent_markers.arrow())

while running:
    if prey.is_valid:
        display.agent(step=prey.step, color="blue", size=10)
    else:
        display.agent(step=prey.step, color="gray", size=10)

    if predator.is_valid:
        display.agent(step=predator.step, color="blue", size=10)
    else:
        display.agent(step=predator.step, color="gray", size=10)
    display.update()


controller.unsubscribe()
controller.stop()
