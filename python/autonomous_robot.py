"""
Automated robot controller
To start autonomous driving must right click to start experiment

TO DO:
1. change random location to "belief state" new location
2. test predator pursuit
3. modify stop distance to cell size
"""

import sys
from cellworld import World, Display, Location, Agent_markers, Capture, Capture_parameters, Step, Timer, Cell_group_builder
from controller import ControllerClient
from cellworld_experiment_service import ExperimentClient
from random import choice
from time import sleep


class AgentData:
    def __init__(self, agent_name: str):
        self.is_valid = None
        self.step = Step()
        self.step.agent_name = agent_name


def on_experiment_started(experiment):
    """
    Starts experiment when right click
    """
    print("Experiment started:", experiment)
    experiments[experiment.experiment_name] = experiment.copy()


def on_episode_started(experiment_name):
    global display
    print("New Episode!!!", experiment_name)
    occlusions = Cell_group_builder.get_from_name("hexagonal", experiments[experiment_name].world.occlusions, "occlusions")
    display.set_occlusions(occlusions)
    print(occlusions)


def load_world(occlusions):
    global display
    global world
    world = World.get_from_parameters_names("hexagonal", "canonical", occlusions)
    display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)


def on_episode_started(experiment_name):
    print(experiment_name)
    #load_world(experiments[experiment_name].world.occlusions)


def random_location():
    """
    Returns random open location in world
    """
    location = choice(world.cells.get("location"))
    cell_id = world.cells.find(location)
    destination_cell = world.cells[cell_id]
    while destination_cell.occluded:
        location = choice(world.cells.get("location"))
        cell_id = world.cells.find(location)
        destination_cell = world.cells[cell_id]
    return location


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
        controller.set_behavior(ControllerClient.Behavior.Pursue)


def on_click(event):
    global current_predator_destination
    if event.button == 1:
        location = Location(event.xdata, event.ydata)  # event.x, event.y
        cell_id = world.cells.find(location)
        destination_cell = world.cells[cell_id]
        print("CELL", destination_cell)
        if destination_cell.occluded:
            print("can't navigate to an occluded cell")
            return
        current_predator_destination = destination_cell.location
        controller.set_destination(destination_cell.location)
    else:
        print("starting experiment")
        occlusions = "10_05"
        exp = experiment_service.start_experiment(  # call start experiment
            prefix="PREFIX",
            suffix="SUFFIX",
            occlusions=occlusions,
            world_implementation="canonical",
            world_configuration="hexagonal",
            subject_name="SUBJECT",
            duration=10)
        print("EX", exp.experiment_name)
        r = experiment_service.start_episode(exp.experiment_name)   # call strat episode
        print(r)
        # set initial destination
        controller.set_destination(current_predator_destination)
        display.circle(current_predator_destination, 0.01, "red")


def on_keypress(event):
    global running
    if event.key == "p":
        controller.pause()
    if event.key == "r":
        controller.resume()
    if event.key == "q":
        running = False


time_out = 1.0  # step timeout value
display = None
world = None

# set globals - initial destination, behavior
load_world("10_05")
current_predator_destination = random_location()
behavior = -1

#  create predator and prey objects
predator = AgentData("predator")
prey = AgentData("prey")

# connect to experiment server
experiment_service = ExperimentClient()
experiment_service.on_experiment_started = on_experiment_started
experiment_service.on_episode_started = on_episode_started
experiment_service.connect("127.0.0.1")
experiment_service.set_request_time_out(5000)
experiment_service.subscribe()
experiments = {}
if "-e" in sys.argv:
    e = experiment_service.start_experiment(prefix="PREFIX",
                                            suffix="SUFFIX",
                                            subject_name="SUBJECT",
                                            world_configuration="hexagonal",
                                            world_implementation="vr",
                                            occlusions="10_05",         # world config
                                            duration=10)
    print(e)


# resend destination timer
controller_timer = Timer(3.0)

# connect to controller
controller = ControllerClient()
if not controller.connect("127.0.0.1", 4590):
    print("failed to connect to the controller")
    exit(1)
controller.set_request_time_out(10000)
controller.subscribe()
controller.on_step = on_step

# initialize keyboard/click interrupts
cid1 = display.fig.canvas.mpl_connect('button_press_event', on_click)
cid_keypress = display.fig.canvas.mpl_connect('key_press_event', on_keypress)

# add predator and prey to world
display.set_agent_marker("predator", Agent_markers.arrow())
display.set_agent_marker("prey", Agent_markers.arrow())


running = True
while running:

    # check predator distance from destination
    if current_predator_destination.dist(predator.step.location) < 0.05: # make this cell length
        current_predator_destination = random_location()
        controller.set_destination(current_predator_destination)
        controller_timer.reset()                                  # reset timer
        display.circle(current_predator_destination, 0.01, "red")
        print("NEW DESTINATION", current_predator_destination)

    # check for timeout
    if not controller_timer:
        controller.set_destination(current_predator_destination) # resend destination
        controller_timer.reset()

    # check if prey was seen
    # if prey.is_valid:
    #     current_predator_destination = prey.step.destination
    #     print("ATTACK", current_predator_destination)
    #     controller.set_destination(current_predator_destination)      # if prey is visible set new destination to prey location


    # plotting the current location of the predator and prey
    if prey.is_valid:
        display.agent(step=prey.step, color="blue", size=10)

    else:
        display.agent(step=prey.step, color="gray", size=10)

    if predator.is_valid:
        display.agent(step=predator.step, color="blue", size=10)

    else:
        display.agent(step=predator.step, color="gray", size=10)

    display.update()
    sleep(0.1)


controller.unsubscribe()
controller.stop()
