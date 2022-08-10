"""
Created for simulation atm
- Decides and destination commands
- Receives tracker information and displays state of robot
"""

import math
from math import sin, cos, pi, atan2, asin
from time import sleep
from cellworld import World, Display, Location, Agent_markers, Step, Timer, Cell_map, Coordinates
from cellworld_tracking import TrackingClient
from cellworld_controller_service import ControllerClient
from tcp_messages import MessageClient, Message


class AgentData:
    """
    Stores information about the agents
    """
    def __init__(self, agent_name: str):
        self.is_valid = None
        self.step = Step()                      # step object - step is a class in experiment.py
        self.step.agent_name = agent_name
        self.move_state = None
        self.move_done = False


def on_step(step):
    """
    Updates steps and predator behavior
    """
    if step.agent_name == "predator":
        predator.is_valid = Timer(time_out)
        predator.step = step


def move_done(move_number):
    print(f"move {move_number} done")
    predator.move_state = move_number
    predator.move_done = True


def get_location(x, y):
    return world.cells[map[Coordinates(x, y)]].location


def on_click(event):
    global current_predator_destination
    if event.button == 1:
        location = Location(event.xdata, event.ydata)
        cell_id = world.cells.find(location)
        destination_cell = world.cells[cell_id]
        current_predator_destination = destination_cell.location
        controller.set_destination(destination_cell.location)
        display.circle(current_predator_destination, 0.01, "orange")
        print(location)
        controller_timer.reset()


# GLOBALS
current_predator_destination = None


# SETUP
# world
occlusions = "00_00"
world = World.get_from_parameters_names("hexagonal", "canonical", occlusions)
display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)
map = Cell_map(world.configuration.cell_coordinates)
# agent
predator = AgentData("predator")
display.set_agent_marker("predator", Agent_markers.arrow())


# CONTROLLER CLIENT
controller_timer = Timer(3.0)     # initialize controller timer variable

controller = ControllerClient()
if not controller.connect("127.0.0.1", 4590):
    print("failed to connect to the controller")
    exit(1)
controller.set_request_time_out(10000)
controller.subscribe()
controller.on_step = on_step

destination = get_location(2, 0)
print(f'destination: {destination}')
print(controller.set_destination(destination))

# CLICK SETUP
cid1 = display.fig.canvas.mpl_connect('button_press_event', on_click)

# VARIABLES
time_out = 1.0
cell_size = world.implementation.cell_transformation.size

current_predator_destination = destination

running = True

while running:
    # TODO: note check coord instead of location
    # need to know when predator reaches destination
    # in controller need to know when a move has been fully executed - ratio should not! change till full exectuion of move
    # if not controller_timer:

    # returns destination if it is navigable
    # print(controller.set_destination(destination))
    if not controller_timer:
        controller.set_destination(current_predator_destination)
        controller_timer.reset()

    if predator.is_valid:
        display.agent(step=predator.step, color="blue", size= 15)
    else:
        display.agent(step=predator.step, color="grey", size= 15)

    display.update()
    sleep(0.5)







