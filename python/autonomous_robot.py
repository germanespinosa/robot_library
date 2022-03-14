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
from matplotlib.backend_bases import MouseButton


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


def load_world():
    global display
    global world
    global visibility
    #occlusion = Cell_group_builder.get_from_name("hexagonal", occlusions + ".occlusions.robot")
    occlusion = Cell_group_builder.get_from_name("hexagonal", occlusions + ".occlusions")
    world.set_occlusions(occlusion)
    display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)
    # visibility set up
    # occlusion_locations = world.cells.occluded_cells().get("location") # all ocludded LOCATIONS in world
    # occlusions_polygons = Polygon_list.get_polygons(occlusion_locations, world.configuration.cell_shape.sides, world.implementation.cell_transformation.size / 2, world.implementation.space.transformation.rotation + world.implementation.cell_transformation.rotation) # ploygon object
    # visibility = Location_visibility(occlusions_polygons) # create visiblilty object


def load_robot_world():
    """
    Locations where the robot cannot navigste are occluded in this world
    """
    global robot_world
    global robot_visibility
    occlusion = Cell_group_builder.get_from_name("hexagonal", occlusions + ".occlusions.robot")
    robot_world.set_occlusions(occlusion)
    #display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)

    # visibility set up
    occlusion_locations = robot_world.cells.occluded_cells().get("location") # all ocludded LOCATIONS in world
    occlusions_polygons = Polygon_list.get_polygons(occlusion_locations, robot_world.configuration.cell_shape.sides, robot_world.implementation.cell_transformation.size / 2, robot_world.implementation.space.transformation.rotation + robot_world.implementation.cell_transformation.rotation) # ploygon object
    robot_visibility = Location_visibility(occlusions_polygons) # create visiblilty object


def on_episode_started(experiment_name):
    print(experiment_name)
    #load_world(experiments[experiment_name].world.occlusions)


def random_location():
    """
    Returns random open location in world
    """
    location = choice(robot_world.cells.free_cells().get("location"))
    return location


def hidden_location():
    """
    Returns random hidden location in world
    """
    current_location = predator.step.location
    hidden_cells = robot_visibility.hidden_cells(current_location, robot_world.cells)
    try:
        new_cell = choice(hidden_cells)
        new_cell_location = new_cell.location
    except:
        # if no hidden locations
        new_cell_location = random_location()
    return new_cell_location


def on_step(step: Step):
    global behavior
    if step.agent_name == "predator":
        predator.is_valid = Timer(time_out)
        predator.step = step
        display.circle(step.location, 0.001, "cyan")
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
        controller.resume()
        location = Location(event.xdata, event.ydata)  # event.x, event.y
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


def on_keypress(event):
    global running
    global current_predator_destination
    if event.key == "p":
        print("pause")
        controller.pause()
    if event.key == "r":
        print("resume")
        controller.resume()
    if event.key == "q":
        controller.pause()
        running = False
    if event.key == "m":
        global controller_timer
        # set initial destination and timer
        print("m")
        controller.resume()
        controller_timer = Timer(5.0)
        current_predator_destination = hidden_location()
        controller.set_destination(current_predator_destination)
        display.circle(current_predator_destination, 0.01, "red")



# GLOBALS
time_out = 1.0  # step timeout value
display = None
robot_visibility = None
world = World.get_from_parameters_names("hexagonal", "canonical")
robot_world = World.get_from_parameters_names("hexagonal", "canonical")
occlusions = "10_03" # global


# set globals - initial destination, behavior
load_world()
load_robot_world()
cell_size = world.implementation.cell_transformation.size


#  create predator and prey objects
predator = AgentData("predator")
prey = AgentData("prey")

# set initial desitnation - current predator location
current_predator_destination = predator.step.location
behavior = -1

# connect to experiment server
experiment_service = ExperimentClient()
experiment_service.on_experiment_started = on_experiment_started
experiment_service.on_episode_started = on_episode_started
if not experiment_service.connect("127.0.0.1"):
    print("Failed to connect to experiment service")
    exit(1)
experiment_service.set_request_time_out(5000)
experiment_service.subscribe()                  # having issues subscribing to exp service


experiments = {}
if "-e" in sys.argv:
    e = experiment_service.start_experiment(prefix="PREFIX",
                                            suffix="SUFFIX",
                                            subject_name="SUBJECT",
                                            world_configuration="hexagonal",
                                            world_implementation="vr",
                                            occlusions=occlusions,         # world config
                                            duration=10)
    print(e)


# initialize controller timer variable
controller_timer = 1

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
    print(controller_timer)
    # check predator distance from destination
    if current_predator_destination.dist(predator.step.location) < (cell_size * 1.5) and controller_timer != 1: # make this cell length
        controller.pause()
        current_predator_destination = hidden_location()
        controller.set_destination(current_predator_destination)
        controller_timer.reset()                                  # reset timer
        display.circle(current_predator_destination, 0.01, "red")
        print("DIST", current_predator_destination.dist(predator.step.location), cell_size * 1.5)
        print("NEW DESTINATION", current_predator_destination)
        controller.resume()

    # creating distance tolerance to avoid overshooting desitionation (account for inertia)
    elif current_predator_destination.dist(predator.step.location) < (cell_size * 1.5):
        controller.pause()
        current_predator_destination = predator.step.location

    # check for timeout and resend desitination
    if not controller_timer:
        controller.set_destination(current_predator_destination) # resend destination
        controller_timer.reset()
        print("RESEND DESTINATION", current_predator_destination)



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


## add gamepad code
# when hit button on gamepad stop autonomus control and switch to joystick
# hit button again to switch back to autonomous