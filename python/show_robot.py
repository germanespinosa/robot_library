from time import sleep
import sys
from cellworld import World, Display, Location, Agent_markers, Capture, Capture_parameters, Step, Timer, Cell_group_builder
from cellworld_controller_service import ControllerClient
from cellworld_experiment_service import ExperimentClient
from prey_simulator import PreySimulator
from matplotlib.backend_bases import MouseButton

time_out = 1.0


class AgentData:
    def __init__(self, agent_name: str):
        self.is_valid = None
        self.step = Step()
        self.step.agent_name = agent_name


display = None
world = None
occlusions = Cell_group_builder()

prey_simulator = PreySimulator()
if not prey_simulator.connect():
    print("failed to connect to prey simulator")
    exit(1)


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

def on_capture(frame: int):
    print("you've been captured!!")


experiment_service = ExperimentClient()
experiment_service.on_experiment_started = on_experiment_started
experiment_service.on_episode_started = on_episode_started
experiment_service.on_capture = on_capture

if not experiment_service.connect():
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
    if step.agent_name == "predator":
        predator.is_valid = Timer(time_out)
        predator.step = step
    else:
        prey.is_valid = Timer(time_out)
        prey.step = step

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

local_prey_location = Location()

def set_prey_location(location: Location):
    global local_prey_location
    local_prey_location = location
    prey_simulator.update_location(location)
    print("updating prey location to ", location)


def on_click(event):
    location = Location(event.xdata, event.ydata)  # event.x, event.y
    if event.button == MouseButton.LEFT:
        cell_id = world.cells.find(location)
        destination_cell = world.cells[cell_id]
        if destination_cell.occluded:
            print("can't navigate to an occluded cell")
            return
        t = Timer()
        controller.set_destination(destination_cell.location)
        print(t.to_seconds() * 1000)
    elif event.button == MouseButton.RIGHT:
        # display.set_occlusions(occlusions)
        set_prey_location(location)

def on_key(event):
    print(event)


cid1 = display.fig.canvas.mpl_connect('button_press_event', on_click)
cid2 = display.fig.canvas.mpl_connect('key_press_event', on_key)
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


def setBehavior(new_behavior):
    global behavior
    if behavior != new_behavior:
        controller.set_behavior(new_behavior)
        behavior = new_behavior

experiment = experiment_service.start_experiment("TEST","TEST","hexagonal","canonical","10_05","TEST", 2)

wait = Timer(1)
print("Episode starts in 3...", end="")
while not wait.time_out():
    pass
print("2...", end="")
while not wait.time_out():
    pass
print("1...", end="")
while not wait.time_out():
    pass
print("GO", end="")
set_prey_location(Location(0, .5))

experiment_service.start_episode(experiment.experiment_name)

while running:
    if world_changed:
        display.set_occlusions(occlusions)
        world_changed = False

    if prey.is_valid:
        setBehavior(ControllerClient.Behavior.Pursue)
        display.agent(step=prey.step, color="red", size=15)
    else:
        setBehavior(ControllerClient.Behavior.Explore)
        local_prey_step = prey.step
        local_prey_step.location = local_prey_location
        display.agent(step=local_prey_step, color="green", size=15)

    if predator.is_valid:
        display.agent(step=predator.step, color="blue", size=15)
    else:
        display.agent(step=predator.step, color="gray", size=15)
    display.update()
    sleep(.1)

controller.unsubscribe()
controller.stop()
