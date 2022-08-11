'''
TO DO:
1. mode 1 initialize robot
    a. PD to desired angle  to get to destination
    b. PD to move fwd
    c. or combine like old controller
2. mode 2 tune moves
    a. from center of cell to start pos
    b. 6 moves
3. controller


- if tick num increase then decrease oscillation in modified value --> decrease step
'''


import math
from time import sleep
from cellworld import World, Display, Location, Agent_markers, Capture, Capture_parameters, Step, Timer, Cell_group_builder, to_radians, to_degrees, Location_list, Cell_group, Cell_map, Coordinates
from cellworld_tracking import TrackingClient
from tcp_messages import MessageClient, Message
from cellworld_controller_service import ControllerClient


class AgentData:
    """
    Stores steps (info from tracker) for each agent
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

def get_location(x, y):
    return world.cells[map[Coordinates(x, y)]].location


# CONSTANTS
time_out = 1.0
robot_speed = 1800


# GLOBALS
tick_guess_dict =  {"m0": {'L': 1, 'R': 600}, # 44, 660
                    "m1": {'L': 231, 'R': 535},
                    "m2": {'L': 432, 'R': 432},
                    "m3": {'L': 535, 'R': 231},
                    "m4": {'L': 815, 'R': 212},
                    "m5": {'L': 450, 'R': -450},
                    "m6": {'L': 420, 'R': -420},
                    "m7": {'L': 216, 'R': 216}}   # initialize 11/2

moves = ["m0", "m1", "m2", "m3", "m4", "m5", "m6","m7"]
move = moves[2]




# Setup
occlusions = "00_00"
world = World.get_from_parameters_names("hexagonal", "canonical", occlusions)
display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)
map = Cell_map(world.configuration.cell_coordinates)
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
# print(f'destination: {destination}')
# print(controller.set_destination(destination))



# Try move
# controller.set_agent_values(100,100,100)
display.circle(predator.step.location, 0.005, "cyan")
a = 1
while True:



    # display robot position
    if predator.is_valid:
        display.agent(step=predator.step, color="blue", size= 15)
    else:
        display.agent(step=predator.step, color="grey", size= 15)

    display.update()
    sleep(0.5)

tracker.unsubscribe()

