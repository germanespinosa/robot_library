from time import sleep
import sys
from random import choice
from cellworld import World, Display, Location, Agent_markers, Capture, Capture_parameters, Step, Timer, Cell_group_builder, to_radians, to_degrees, Location_list, Cell_group
from cellworld_controller_service import ControllerClient
from cellworld_experiment_service import ExperimentClient
from matplotlib.backend_bases import MouseButton

from gamepad import GamePad


game_pad = GamePad()
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

free_cells = Cell_group()
spawn_locations = Location_list()

start_port = Location(0, .5)
end_port = Location(1, .5)
lead = .3


def on_episode_started(experiment_name):
    global occlusions
    global world_changed
    global free_cells
    print("New Episode!!!", experiment_name)
    occlusions = Cell_group_builder.get_from_name("hexagonal", experiments[experiment_name].world.occlusions, "occlusions")
    world.set_occlusions(occlusions)
    free_cells = world.cells.free_cells()
    spawn_locations.clear()
    for cell in free_cells:
        if start_port.dist(cell.location) > lead:
            spawn_locations.append(cell.location)
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

predator = AgentData("predator")


behavior = -1


def on_step(step: Step):
    if step.agent_name == "predator":
        predator.is_valid = Timer(time_out)
        predator.step = step

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


rotation_speed = 10.0
forward_speed = .01


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
        pass


def on_key(event):
    print(event)


last_destination_update = None
last_destination_sent = None


def set_destination(destination: Location):
    global last_destination_update
    global controller
    global last_destination_sent
    last_destination_update = Timer(2)
    controller.set_destination(destination)
    last_destination_sent = destination


def check_destination_timeout():
    global last_destination_update
    if last_destination_update.time_out():
        last_destination_update = Timer(2)
        print(last_destination_sent)
        controller.set_destination(last_destination_sent)



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

def reached(position: Location, destination:Location):
    return position.dist(destination) < world.implementation.cell_transformation.size


def setBehavior(new_behavior):
    global behavior
    if behavior != new_behavior:
        controller.set_behavior(new_behavior)
        behavior = new_behavior


experiment = experiment_service.start_experiment("TEST", "TEST", "hexagonal", "canonical", "21_05", "TEST", 2)

last_chasing_destination_timer = Timer(0)
last_exploring_destination_timer = Timer(0)
last_exploring_destination = Location()


while True:
    world_changed = False
    experiment_service.start_episode(experiment.experiment_name)
    while not world_changed:
        game_pad.wait(.1)
    display.set_occlusions(occlusions)

    #give the predator chance to move to the spawn location
    spawn_location = choice(spawn_locations)
    controller.set_destination(spawn_location)
    spawn_timer = Timer(1)
    display.circle(spawn_location, .01, "red")
    set_destination(spawn_location)
    while game_pad.update() and not reached(predator.step.location, spawn_location):
        display.agent(step=predator.step, color="blue", size=15)
        display.update()
        check_destination_timeout()

    display.circle(spawn_location, .01, "white")

    if last_exploring_destination_timer.time_out() or reached(predator.step.location, last_exploring_destination):
        display.circle(last_exploring_destination, .01, "white")
        last_exploring_destination = choice(free_cells).location
        set_destination(last_exploring_destination)
        last_exploring_destination_timer = Timer(10)
        display.circle(last_exploring_destination, .01, "blue")

    if predator.is_valid:
        display.agent(step=predator.step, color="blue", size=15)
    else:
        display.agent(step=predator.step, color="gray", size=15)
    display.update()
    check_destination_timeout()

    experiment_service.finish_episode()

controller.unsubscribe()
controller.stop()
