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
from cellworld import *
from cellworld_controller_service import ControllerClient
from cellworld_experiment_service import ExperimentClient
from random import choice, choices
from time import sleep
from json_cpp import JsonList

episode_in_progress = False
experiment_log_folder = "/habitat/logsV2"
current_experiment_name = ""

pheromone_charge = .25
pheromone_decay = 1.0
pheromone_max = 50

possible_destinations = Cell_group()
possible_destinations_weights = []

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


def on_episode_finished(m):
    global episode_in_progress, current_predator_destination, inertia_buffer
    controller.set_behavior(0)
    inertia_buffer = 1
    episode_in_progress = False
    current_predator_destination = choice(spawn_locations)
    controller.set_destination(current_predator_destination)     # set destination
    destination_list.append(current_predator_destination)
    if controller_timer != 1: # no idea why the timer would be an integer but whatevs
        controller_timer.reset()                                     # reset controller timer
    display.circle(current_predator_destination, 0.01, "red")
    #print("NEW DESTINATION: ", current_predator_destination)
    last_trajectory = Experiment.get_from_file(experiment_log_folder + "/" + current_experiment_name + "_experiment.json").episodes[-1].trajectories.get_agent_trajectory("prey")
    for step in last_trajectory:
        cell_index = possible_destinations.find(step.location)
        possible_destinations_weights[cell_index] = min(possible_destinations_weights[cell_index] + pheromone_charge, pheromone_max)
    for i in range(len(possible_destinations_weights)):
        possible_destinations_weights[i] = max(possible_destinations_weights[i] - pheromone_decay, 1)
    controller.resume()


def on_capture( frame:int ):
    global inertia_buffer
    controller.set_behavior(0)
    inertia_buffer = 1
    print ("PREY CAPTURED")


def on_episode_started(experiment_name):
    print("hi")
    global display, episode_in_progress, current_experiment_name
    current_experiment_name = experiment_name
    # episode_in_progress = True
    print("New Episode: ", experiment_name)
    print("Occlusions: ", experiments[experiment_name].world.occlusions)
    # occlusions = Cell_group_builder.get_from_name("hexagonal", experiments[experiment_name].world.occlusions, "occlusions")
    # display.set_occlusions(occlusions)
    # print(occlusions)

def on_prey_entered_arena():
    global episode_in_progress
    episode_in_progress = True

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
    Returns random open location in robot_world (keep this for cases where there are no hidden locations)
    """
    location = choice(robot_world.cells.free_cells().get("location")) #this is probably wrong?
    return location


def hidden_location():
    """
    Returns random hidden location in robot_world
    """
    current_location = predator.step.location
    #hidden_cells = robot_visibility.hidden_cells(current_location, robot_world.cells)
    #hidden_cells = robot_visibility.hidden_cells(current_location, possible_destinations)

    try:    # find random hidden cell
        #new_cell = choices(hidden_cells)
        new_cell = choices(possible_destinations, weights=possible_destinations_weights)
        new_cell_location = new_cell.location
    except:  # if no hidden locations
        new_cell_location = random_location()
    return new_cell_location


def on_step(step: Step):
    """
    Updates steps and predator behavior
    """
    global behavior

    if step.agent_name == "predator":
        predator.is_valid = Timer(time_out)
        predator.step = step
        #display.circle(step.location, 0.002, "royalblue")    # plot predator path (steps)
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


def get_spawn_locations(w: Cell_group):
    entrance = Location(0, .5)
    min_dist = .5
    spawn_locations = JsonList(list_type=Location)
    for cell in w:
        if cell.location.dist(entrance) >= min_dist:
            spawn_locations.append(cell.location)
    return spawn_locations


def get_possible_destinations(w: World) -> Cell_group:
    res = Cell_group()
    map = Cell_map(w.configuration.cell_coordinates)
    for cell in w.cells.free_cells():
        connection_count = 0
        for connection in w.configuration.connection_pattern:
            connection_coordinates = connection + cell.coordinates
            connection_index = map[connection_coordinates]
            if connection_index >= 0:
                if not w.cells[connection_index].occluded:
                    connection_count += 1
        if connection_count == 6:
            res.append(cell)
    return res

# SET UP GLOBAL VARIABLES
occlusions = sys.argv[1]

inertia_buffer = 1 #1.8 # 1.5
time_out = 1.0      # step timer for predator and preyQ
display = None
robot_visibility = None
controller_state = 1 # resume = 1, pause = 0
# create world
world = World.get_from_parameters_names("hexagonal", "canonical")
robot_world = World.get_from_parameters_names("hexagonal", "canonical")
load_world()
load_robot_world()
possible_destinations = get_possible_destinations(robot_world)
possible_destinations_weights = [1 for x in possible_destinations]
spawn_locations = get_spawn_locations(possible_destinations)
cell_size = world.implementation.cell_transformation.size
#  create predator and prey objects
predator = AgentData("predator")
prey = AgentData("prey")
# set initial destination and behavior
current_predator_destination = predator.step.location  # initial predator destination
destination_list = []       # keeps track any NEW destinations
behavior = -1                                          # Explore or Pursue


# CONNECT TO EXPERIMENT SERVER
experiment_service = ExperimentClient()
experiment_service.on_experiment_started = on_experiment_started
experiment_service.on_episode_started = on_episode_started
experiment_service.on_prey_entered_arena = on_prey_entered_arena
experiment_service.on_episode_finished = on_episode_finished
experiment_service.on_capture = on_capture

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
    # check predator distance from destination and send new on if reached
    if current_predator_destination.dist(predator.step.location) < (cell_size * inertia_buffer):
        controller.pause()                                           # prevents overshoot - stop robot omce close enough to destination
        controller.set_behavior(0)
        inertia_buffer = 1
        if controller_timer != 1:
            if episode_in_progress:
                current_predator_destination = hidden_location()             # assign new destination
                controller.set_destination(current_predator_destination)     # set destination
                destination_list.append(current_predator_destination)
                controller_timer.reset()                                     # reset controller timer
                display.circle(current_predator_destination, 0.01, "red")
                #print("NEW DESTINATION: ", current_predator_destination)
                controller.resume()                                          # Resume controller (unpause)
        # create distance tolerance to account for inertia
        else:
            current_predator_destination = predator.step.location  # assign destination to current predator location (artificially reach goal when "close enough")

    # check for controller timeout and resend current destination
    if not controller_timer:
        controller.set_destination(current_predator_destination)  # resend destination
        controller_timer.reset()
        #print("RESEND DESTINATION: ", current_predator_destination)

    # check if prey was seen
    if prey.is_valid and controller_state and episode_in_progress: # controller state allows pause to overrule pursue
        print("PREY SEEN")
        controller.pause()
        controller.set_behavior(1)
        inertia_buffer = 2
        current_predator_destination = prey.step.location
        controller.set_destination(current_predator_destination)      # if prey is visible set new destination to prey location
        destination_list.append(current_predator_destination)
        display.circle(prey.step.location, 0.01, "magenta")
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

    #cmap = plt.cm.Reds([x/max(possible_destinations_weights) for x in possible_destinations_weights])
    # for i, cell in enumerate(possible_destinations):
    #     display.cell(cell_id=cell.id, color=cmap[i])
    display.update()
    sleep(0.1)


controller.unsubscribe()
controller.stop()

