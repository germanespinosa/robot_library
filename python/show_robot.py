from time import sleep
import sys
from cellworld import World, Display, Location, Agent_markers, Capture, Capture_parameters, Step, Timer, Cell_group_builder
from controller import ControllerClient
from cellworld_experiment_service import ExperimentClient


time_out = 1.0


class AgentData:
    def __init__(self, agent_name: str):
        self.is_valid = None
        self.step = Step()
        self.step.agent_name = agent_name


display = None
world = None
occlusions = Cell_group_builder()


def on_experiment_started(experiment):
    print("Experiment started:", experiment)
    experiments[experiment.experiment_name] = experiment.copy()


world_changed = False


def on_episode_started(experiment_name):
    global occlusions
    global world_changed
    print("New Episode!!!", experiment_name)
    occlusions = Cell_group_builder.get_from_name("hexagonal", experiments[experiment_name].world.occlusions, "occlusions")
    world_changed = True


experiment_service = ExperimentClient()
experiment_service.on_experiment_started = on_experiment_started
experiment_service.on_episode_started = on_episode_started
if not experiment_service.connect("127.0.0.1"):
    print("Failed to connect to experiment service")
    exit(1)
experiment_service.set_request_time_out(5000)
experiment_service.subscribe()

experiments = {}

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


behavior = -1


def on_step(step: Step):
    global behavior
    if step.agent_name == "predator":
        predator.is_valid = Timer(time_out)
        predator.step = step
        if behavior != ControllerClient.Behavior.Explore:
            controller.set_behavior(ControllerClient.Behavior.Explore)
            behavior = ControllerClient.Behavior.Explore
    else:
        prey.is_valid = Timer(time_out)
        prey.step = step
        if behavior != ControllerClient.Behavior.Pursue:
            controller.set_behavior(ControllerClient.Behavior.Pursue)
            behavior = ControllerClient.Behavior.Pursue

# connect to controller

controller = ControllerClient()
if not controller.connect("127.0.0.1", 4590):
    print("failed to connect to the controller")
    exit(1)
controller.set_request_time_out(1000000)
controller.subscribe()
controller.on_step = on_step


world = World.get_from_parameters_names("hexagonal", "canonical")
display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)

capture = Capture(Capture_parameters(2.0, 90.0), world)


def on_click(event):
    if event.button == 1:
        location = Location(event.xdata, event.ydata)  # event.x, event.y
        cell_id = world.cells.find(location)
        destination_cell = world.cells[cell_id]
        if destination_cell.occluded:
            print("can't navigate to an occluded cell")
            return
        t = Timer()
        controller.set_destination(destination_cell.location)
        print(t.to_seconds() * 1000)
    else:
        display.set_occlusions(occlusions)


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
    if world_changed:
        display.set_occlusions(occlusions)
        world_changed = False

    if prey.is_valid:
        display.agent(step=prey.step, color="green", size=10)
    else:
        display.agent(step=prey.step, color="gray", size=10)

    if predator.is_valid:
        display.agent(step=predator.step, color="blue", size=10)
    else:
        display.agent(step=predator.step, color="gray", size=10)
    display.update()
    sleep(.1)

controller.unsubscribe()
controller.stop()
