"""
Automated robot controller
Program Inputs:
1. To start autonomous motion type m
2. To start experiment and avoind occlusions right click********************************
3. To follow robot path modify path variable controller_service.cpp
4. Specify occlusions

TO DO:
1. change random location to "belief state" new location
2. test predator canonical pursuit
3. load random location from robot world
4. fix 10_03
5. look at PID fix distance overshoot check normalize error correct
6. added pause and resume to avoid overshoot fix later
7. gamepad wrapper
"""

import sys
from cellworld import World, Display, Location, Agent_markers, Capture, Capture_parameters, Step, Timer, Cell_group_builder, Polygon, Polygon_list, Location_visibility
from cellworld_controller_service import ControllerClient
from cellworld_experiment_service import ExperimentClient
from random import choice
from time import sleep


class AgentData:
    """
    Class to initialize prey and predator objects
    """
    def __init__(self, agent_name: str):
        self.is_valid = None
        self.step = Step()
        self.step.agent_name = agent_name


def on_experiment_started(experiment):
    """
    To start experiment right click on map
    """
    print("Experiment started:", experiment)
    experiments[experiment.experiment_name] = experiment.copy()


def on_episode_started(experiment_name):
    global display
    print("New Episode: ", experiment_name)
    print("Occlusions: ", experiments[experiment_name].world.occlusions)
    # occlusions = Cell_group_builder.get_from_name("hexagonal", experiments[experiment_name].world.occlusions, "occlusions")
    # display.set_occlusions(occlusions)
    # print(occlusions)


def load_world():
    """
    Load world to display
    """
    global display
    global world

    occlusion = Cell_group_builder.get_from_name("hexagonal", occlusions + ".occlusions")
    world.set_occlusions(occlusion)
    display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)


def load_robot_world():
    """
    Load world with extra occlusions for where the robot cannot navigate
    """
    global robot_world
    global robot_visibility

    occlusion = Cell_group_builder.get_from_name("hexagonal", occlusions + ".occlusions.robot")
    robot_world.set_occlusions(occlusion)

    # visibility set up
    occlusion_locations = robot_world.cells.occluded_cells().get("location")            # all occludded locations in world
    occlusions_polygons = Polygon_list.get_polygons(occlusion_locations, robot_world.configuration.cell_shape.sides, robot_world.implementation.cell_transformation.size / 2, robot_world.implementation.space.transformation.rotation + robot_world.implementation.cell_transformation.rotation)  # polygon object
    robot_visibility = Location_visibility(occlusions_polygons) # create visibility object

def random_location():
    """
    Returns random open location in world (keep this for cases where there are no hidden locations)
    """
    location = choice(robot_world.cells.free_cells().get("location"))
    return location


def hidden_location():
    """
    Returns random hidden location in world
    """
    current_location = predator.step.location
    hidden_cells = robot_visibility.hidden_cells(current_location, robot_world.cells)
    try:    # find random hidden cell
        new_cell = choice(hidden_cells)
        new_cell_location = new_cell.location
    except:  # if no hidden locations
        new_cell_location = random_location()
    return new_cell_location


def on_step(step: Step):
    """

    """
    global behavior

    if step.agent_name == "predator":
        predator.is_valid = Timer(time_out)
        predator.step = step
        display.circle(step.location, 0.002, "blue")
        if behavior != ControllerClient.Behavior.Explore:
            controller.set_behavior(ControllerClient.Behavior.Explore)
            behavior = ControllerClient.Behavior.Explore
    else:
        prey.is_valid = Timer(time_out)
        prey.step = step
        controller.set_behavior(ControllerClient.Behavior.Pursue)


def on_click(event):
    """
    Assign destiantion by clicking on map
    Right click to start experiment
    """
    global current_predator_destination

    if event.button == 1:
        controller.resume()
        location = Location(event.xdata, event.ydata)
        cell_id = world.cells.find(location)
        destination_cell = world.cells[cell_id]
        print("CELL", destination_cell)
        if destination_cell.occluded:
            print("can't navigate to an occluded cell")
            return
        current_predator_destination = destination_cell.location
        controller.set_destination(destination_cell.location)
        display.circle(current_predator_destination, 0.01, "red")
    else:
        print("starting experiment")
        exp = experiment_service.start_experiment(                  # call start experiment
            prefix="PREFIX",
            suffix="SUFFIX",
            occlusions=occlusions,
            world_implementation="canonical",
            world_configuration="hexagonal",
            subject_name="SUBJECT",
            duration=10)
        print("Experiment Name: ", exp.experiment_name)
        r = experiment_service.start_episode(exp.experiment_name)   # call start episode
        print(r)


def on_keypress(event):
    global running
    global current_predator_destination
    global controller_timer

    if event.key == "p":
        print("pause")
        controller.pause()
    if event.key == "r":
        print("resume")
        controller.resume()
    if event.key == "q":
        print("quit")
        controller.pause()
        running = False
    if event.key == "m":
        controller.resume()                                     #  change controller state to Playing
        controller_timer = Timer(5.0)                           # set initial destination and timer
        current_predator_destination = hidden_location()        # assign new destination
        controller.set_destination(current_predator_destination)
        display.circle(current_predator_destination, 0.01, "red")


# SET UP GLOBAL VARIABLES
occlusions = "10_03"
time_out = 1.0      # step timeout value

display = None
robot_visibility = None
# create world
world = World.get_from_parameters_names("hexagonal", "canonical")
robot_world = World.get_from_parameters_names("hexagonal", "canonical")
load_world()
load_robot_world()
cell_size = world.implementation.cell_transformation.size
#  create predator and prey objects
predator = AgentData("predator")
prey = AgentData("prey")
# set initial destination and behavior
current_predator_destination = predator.step.location  # initial predator destination

behavior = -1                                          # Explore or Pursue


# CONNECT TO EXPERIMENT SERVER
experiment_service = ExperimentClient()
experiment_service.on_experiment_started = on_experiment_started
experiment_service.on_episode_started = on_episode_started
if not experiment_service.connect("127.0.0.1"):
    print("Failed to connect to experiment service")
    exit(1)
experiment_service.set_request_time_out(5000)
experiment_service.subscribe()                  # having issues subscribing to exp service
experiments = {}


# CONNECT TO CONTROLLER
controller_timer = 1 # initialize controller timer variable
controller = ControllerClient()
if not controller.connect("127.0.0.1", 4590):
    print("failed to connect to the controller")
    exit(1)
controller.set_request_time_out(10000)
controller.subscribe()
controller.on_step = on_step


# INITIALIZE KEYBOARD & CLICK INTERUPTS
cid1 = display.fig.canvas.mpl_connect('button_press_event', on_click)
cid_keypress = display.fig.canvas.mpl_connect('key_press_event', on_keypress)

# ADD PREDATOR AND PREY TO WORLD
display.set_agent_marker("predator", Agent_markers.arrow())
display.set_agent_marker("prey", Agent_markers.arrow())


running = True
while running:

    # check predator distance from destination and send new on if reached
    if current_predator_destination.dist(predator.step.location) < (cell_size * 1.5) and controller_timer != 1:
        controller.pause()                                           # prevents overshoot - stop robot omce close enough to destination
        current_predator_destination = hidden_location()             # assign new destination
        controller.set_destination(current_predator_destination)     # set destination
        controller_timer.reset()                                     # reset controller timer
        display.circle(current_predator_destination, 0.01, "red")
        print("NEW DESTINATION: ", current_predator_destination)
        controller.resume()                                          # Resume controller (unpause)

    # create distance tolerance to account for inertia
    elif current_predator_destination.dist(predator.step.location) < (cell_size * 1.5):
        controller.pause()
        current_predator_destination = predator.step.location  # assign destination to current predator location (artificially reach goal when "close enough")

    # check for controller timeout and resend current destination
    if not controller_timer:
        controller.set_destination(current_predator_destination)  # resend destination
        controller_timer.reset()
        print("RESEND DESTINATION: ", current_predator_destination)

    # check if prey was seen
    if prey.is_valid:
        print("PREY SEEN")
        current_predator_destination = prey.step.location
        controller.set_destination(current_predator_destination)      # if prey is visible set new destination to prey location


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

