import sys
from cellworld import World, Display, Location, Agent_markers, Capture, Capture_parameters, Step, Timer
from controller import ControllerClient
from cellworld_experiment_service import ExperimentClient
from random import choice


class AgentData:
    def __init__(self, agent_name: str):
        self.is_valid = None
        self.step = Step()
        self.step.agent_name = agent_name


def on_experiment_started(experiment):
    global experiments
    print("Experiment started:", experiment)
    experiments[experiment.experiment_name] = experiment.copy()


def load_world(occlusions):
    global display
    global world
    world = World.get_from_parameters_names("hexagonal", "canonical", occlusions)
    display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)


def on_episode_started(experiment_name):
    print (experiment_name)
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

    global current_preditor_destination



    if step.agent_name == "predator":
        predator.is_valid = Timer(time_out)
        predator.step = step
        #print("Predator Location", predator.step, "Destination", current_preditor_destination)

        # check distance between current and desired location
        if current_preditor_destination.dist(predator.step.location) < 0.6: # make this cell length
            current_preditor_destination = random_location()
            controller.set_destination(current_preditor_destination)
            print("NEW DESTINATION", current_preditor_destination)
        # if predator still moving to desired location sends the desired location again
        else:
            controller.set_destination(current_preditor_destination)    # every time step is called send destination until distance is small enough
        print(step.agent_name)

        controller.set_behavior(ControllerClient.Behavior.Explore)
    else:
        print(step.agent_name)
        prey.is_valid = Timer(time_out)
        prey.step = step
        current_preditor_destination = prey.step.destination
        print("ATTACK", current_preditor_destination)
        controller.set_destination(current_preditor_destination)      # if prey is visible set new destination to prey location
        controller.set_behavior(ControllerClient.Behavior.Pursue)


def on_click(event):
    global current_preditor_destination
    if event.button == 1:
        location = Location(event.xdata, event.ydata)  # event.x, event.y
        cell_id = world.cells.find(location)
        destination_cell = world.cells[cell_id]
        print("CELL", destination_cell)
        if destination_cell.occluded:
            print("can't navigate to an occluded cell")
            return
        current_preditor_destination = destination_cell.location
        controller.set_destination(destination_cell.location)
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
        print("EX", exp.experiment_name)
        r = experiment_service.start_episode(exp.experiment_name)
        print(r)


def on_keypress(event):
    global running
    if event.key == "p":
        controller.pause()
    if event.key == "r":
        controller.resume()
    if event.key == "q":
        running = False


predator = AgentData("predator")
prey = AgentData("prey")

time_out = 1.0
display = None
world = None
load_world("00_00")

# set initial destination
current_predator_destination = random_location()

experiment_service = ExperimentClient()
experiment_service.connect("127.0.0.1")
experiment_service.set_request_time_out(5000)
experiments = {}
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


# predator = AgentData("predator")
# prey = AgentData("prey")
# current_preditor_destination = predator.step.location
# print("PPPPPPPPPPPPPPPPPPPPPP", current_preditor_destination)



# connect to controller
controller = ControllerClient()
if not controller.connect("127.0.0.1", 4590):
    print("failed to connect to the controller")
    exit(1)
controller.set_request_time_out(10000)
controller.subscribe()
controller.on_step = on_step


capture = Capture(Capture_parameters(2.0, 90.0), world)

cid1 = display.fig.canvas.mpl_connect('button_press_event', on_click)
running = True
cid_keypress = display.fig.canvas.mpl_connect('key_press_event', on_keypress)

# controller.set_destination(current_predator_destination)
# predator.step

display.set_agent_marker("predator", Agent_markers.arrow())
display.set_agent_marker("prey", Agent_markers.arrow())

# # check prey
# prey.is_valid = 9
# prey.step.location = Location(x=0.5, y=0.5)

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
