"""
Automated robot controller
Program Inputs:
1. To start autonomous motion type m
2. To start experiment and avoind occlusions right click********************************
3. To follow robot path modify path variable controller_service.cpp
4. Specify occlusions

Program timers:
1. predator step updates
2. prey step updates
3. controller - destination timer

Mutable Variables:
1. occlusions

TO DO:
1. change random location to "belief state" new location
2. test predator canonical pursuit
5. look at PID fix distance overshoot check normalize error correct
6. added pause and resume to avoid overshoot fix later
7. PREYS LOCATION NOT CANNONICAL FIX THIS!!!
"""

import sys
from cellworld import World, Display, Location, Agent_markers, Capture, Capture_parameters, Step, Timer, Cell_group_builder, Polygon, Polygon_list, Location_visibility, Cell_map, Coordinates
from cellworld_controller_service import ControllerClient
from cellworld_experiment_service import ExperimentClient
from random import choice
from time import sleep


class AgentData:
    """
    Class to initialize prey and predator objects
    """
    def __init__(self, agent_name: str):
        self.is_valid = None # timers for predator and prey updates
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
    global map

    occlusion = Cell_group_builder.get_from_name("hexagonal", occlusions + ".occlusions")
    world.set_occlusions(occlusion)
    display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)
    map = Cell_map(world.configuration.cell_coordinates)

def get_location(x,y):
    return world.cells[map[Coordinates(x,y)]].location


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

# first = False
# #test_locations = [Location(.1, .1), Location(.9, .9)]
# test_locations = [get_location(-10,-10), get_location(10,10)]
# record_data = False
def random_location():
    global first
    global record_data
    """
    Returns random open location in robot_world (keep this for cases where there are no hidden locations)
    """
    first = not first
    if first:
        return test_locations[0]
    else:
        record_data = True
        return test_locations[1]


def hidden_location():
    return random_location()


def on_step(step: Step):
    """
    Updates steps and predator behavior
    """
    global behavior

    if step.agent_name == "predator":
        if record_data:
            with open("log.txt", "a") as f:
                f.write(str(step.location.x) + "," + str(step.location.y) + "\n")
        predator.is_valid = Timer(time_out)
        predator.step = step
        display.circle(step.location, 0.002, "royalblue")    # plot predator path (steps)
        if behavior != ControllerClient.Behavior.Explore:
            controller.set_behavior(ControllerClient.Behavior.Explore) # explore when prey not seen
            behavior = ControllerClient.Behavior.Explore
    else:
        prey.is_valid = Timer(time_out) # pursue when prey is seen
        prey.step = step
        controller.set_behavior(ControllerClient.Behavior.Pursue)


def on_click(event):
    """
    Assign destination by clicking on map
    Right-click to start experiment
    """
    global current_predator_destination
    global destination_list

    if event.button == 1:
        controller.resume()
        location = Location(event.xdata, event.ydata)
        cell_id = world.cells.find(location)
        destination_cell = world.cells[cell_id]
        #QQQQQQQQQQprint("CELL", destination_cell)
        if destination_cell.occluded:
            print("can't navigate to an occluded cell")
            return
        current_predator_destination = destination_cell.location
        controller.set_destination(destination_cell.location)
        destination_list.append(destination_cell.location)
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
    """
    Sets up keyboard intervention
    """
    global running
    global current_predator_destination
    global controller_timer
    global destination_list
    global controller_state

    if event.key == "p":
        print("pause")
        controller.pause()
        controller_state = 0
    if event.key == "r":
        print("resume")
        controller.resume()
        controller_state = 1
    if event.key == "q":
        print("quit")
        controller.pause()
        running = False
    if event.key == "m":
        print("auto")
        controller.resume()                                     # change controller state to Playing
        controller_timer = Timer(5.0)                           # set initial destination and timer
        current_predator_destination = hidden_location()        # assign new destination
        controller.set_destination(current_predator_destination)
        destination_list.append(current_predator_destination)
        display.circle(current_predator_destination, 0.01, "red")



# SET UP GLOBAL VARIABLES
occlusions = "00_00"
inertia_buffer = 1.8 #1.8 # 1.5
time_out = 1.0      # step timer for predator and preyQ

display = None
map = None
robot_visibility = None
controller_state = 1 # resume = 1, pause = 0
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
destination_list = []       # keeps track any NEW destinations
behavior = -1                                          # Explore or Pursue

# test logic ###############################################################################
first = False
#test_locations = [Location(.1, .1), Location(.9, .9)]
test_locations = [get_location(-9,-9), get_location(9,9)]
record_data = False


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
#controller_timer = 1  # initialize controller timer variable
controller_timer = 1     # initialize controller timer variable
controller = ControllerClient()
if not controller.connect("127.0.0.1", 4590):
    print("failed to connect to the controller")
    exit(1)
controller.set_request_time_out(10000)
controller.subscribe()
controller.on_step = on_step


# INITIALIZE KEYBOARD & CLICK INTERRUPTS
cid1 = display.fig.canvas.mpl_connect('button_press_event', on_click)
cid_keypress = display.fig.canvas.mpl_connect('key_press_event', on_keypress)

# ADD PREDATOR AND PREY TO WORLD
display.set_agent_marker("predator", Agent_markers.arrow())
display.set_agent_marker("prey", Agent_markers.arrow())


running = True
while running:
# add inertia buffer logic
    if prey.is_valid:
        inertia_buffer = 1.8
    else:
        inertia_buffer = 1


    # check predator distance from destination and send new on if reached
    if current_predator_destination.dist(predator.step.location) < (cell_size * inertia_buffer) and controller_timer != 1:
        controller.pause()                                           # prevents overshoot - stop robot omce close enough to destination
        current_predator_destination = hidden_location()             # assign new destination
        controller.set_destination(current_predator_destination)     # set destination
        destination_list.append(current_predator_destination)
        controller_timer.reset()                                     # reset controller timer
        display.circle(current_predator_destination, 0.01, "red")
        #print("NEW DESTINATION: ", current_predator_destination)
        controller.resume()                                          # Resume controller (unpause)

    # create distance tolerance to account for inertia
    elif current_predator_destination.dist(predator.step.location) < (cell_size * inertia_buffer):
        controller.pause()
        current_predator_destination = predator.step.location  # assign destination to current predator location (artificially reach goal when "close enough")

    # check for controller timeout and resend current destination
    if not controller_timer:
        controller.set_destination(current_predator_destination)  # resend destination
        controller_timer.reset()
        #print("RESEND DESTINATION: ", current_predator_destination)

    # check if prey was seen
    if prey.is_valid and controller_state:
        print("PREY SEEN")
        controller.pause()
        current_predator_destination = prey.step.location
        controller.set_destination(current_predator_destination)      # if prey is visible set new destination to prey location
        destination_list.append(current_predator_destination)
        display.circle(prey.step.location, 0.01, "blue")
        print(prey.step.location, predator.step.location)
        controller.resume()
        #controller_timer.reset()

    # plotting the current location of the predator and prey
    if prey.is_valid:
        display.agent(step=prey.step, color="green", size=10)

    else:
        display.agent(step=prey.step, color="gray", size=10)

    if predator.is_valid:
        display.agent(step=predator.step, color="blue", size=10)

    else:
        display.agent(step=predator.step, color="gray", size=10)

    # remove old destinations from map
    if len(destination_list) > 1:
        display.circle(destination_list[0], 0.008, "white")
        destination_list.remove(destination_list[0])

    display.update()
    sleep(0.1)


controller.unsubscribe()
controller.stop()

