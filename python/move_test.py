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
from cellworld import World, Display, Location, Agent_markers, Capture, Capture_parameters, Step, Timer, Cell_group_builder, to_radians, to_degrees, Location_list, Cell_group, Cell_map
from cellworld_tracking import TrackingClient
from tcp_messages import MessageClient, Message


class AgentData:
    """
    Stores steps (info from tracker) for each agent
    """
    def __init__(self, agent_name: str):
        self.is_valid = None
        self.step = Step()                      # step object - step is a class in experiment.py
        self.step.agent_name = agent_name
        self.move_state = None
        self.move_done = None


def on_step(step):
    """
    Updates steps and predator behavior
    """
    if step.agent_name == "predator":
        predator.is_valid = Timer(time_out)
        predator.step = step


class Robot_client(MessageClient):

    def __init__(self):
        MessageClient.__init__(self)
        self.move_done = None
        self.router.add_route("move_done", self.__move_done__, int)

    def __move_done__(self, move_number):
        if self.move_done:
            self.move_done(move_number)

    def set_left(self, v: int) -> bool:
        return self.send_request(Message("set_left", v)).get_body(bool)

    def set_right(self, v: int) -> bool:
        return self.send_request(Message("set_right", v)).get_body(bool)

    def stop(self) -> bool:
        return self.send_request(Message("stop")).get_body(bool)

    def set_speed(self, v: int) -> bool:
        return self.send_request(Message("set_speed", v)).get_body(bool)

    def update(self) -> bool:
        return self.send_request(Message("update")).get_body(bool)


def initialize():
    pass


def robot_tick_update(left_tick, right_tick):
    robot_client.set_left(left_tick)
    robot_client.set_right(right_tick)
    robot_client.set_speed(robot_speed)
    robot_client.update()



# CONSTANTS
R1  = 0.0635/2
R2  = R1 + 0.127/2
TH1 = 120
TH2 = 60
TH3 = 180
STRAIGHT = 0.11
time_out = 1.0
robot_speed = 100


# GLOBALS
tick_guess_dict =  {"m1": {'L': 70, 'R': 260},
                    "m2": {'L': 128, 'R': 306},

                    "m3": {'L': 216, 'R': 216},
                    "m4": {'L': 306, 'R': 128},
                    "m5": {'L': 323, 'R':-33},
                    "m6": {'L': 267, 'R': -267},
                    "m7": {'L': 450, 'R': -450}}
# move characteristic dict
move_constants_dict= {"m1": {'r': R1,       'th': TH1},
                      "m2": {'r': R2,       'th': TH2},
                      "m3": {'r': STRAIGHT, 'th': 0},
                      "m4": {'r': R2,       'th': -TH2},
                      "m5": {'r': R1,       'th': -TH1},
                      "m6": {'r': 0,        'th': 180},
                      "m7": {'r': 0,        'th': 90}}
moves = ["m1", "m2", "m3", "m4", "m5", "m6", "m7"]
move = moves[2]




# Setup
occlusions = "00_00"
world = World.get_from_parameters_names("hexagonal", "canonical", occlusions)
display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)
map = Cell_map(world.configuration.cell_coordinates)
predator = AgentData("predator")
display.set_agent_marker("predator", Agent_markers.arrow())

# subscribe to tracker to receive robot step updates
tracker = TrackingClient()
if tracker.connect("127.0.0.1"):
    print("connected to tracker")
else:
    print("failed to connect to tracker")
tracker.set_request_time_out(5000)
tracker.subscribe()
tracker.set_throughput(5)
tracker.on_step = on_step

# connect to robot client to send ticks
robot_client = Robot_client()
if robot_client.connect("127.0.0.1", 6300):
    print("connected to robot! yay")
else:
    print("failed to connect to robot! bummer")
    exit(1)



# Try move
# robot_tick_update(tick_guess_dict[moves[6]]['L'], tick_guess_dict[moves[6]]['R'])


robot_tick_update(tick_guess_dict[moves[2]]['L'], tick_guess_dict[moves[2]]['R'])
robot_tick_update(tick_guess_dict[moves[0]]['L'], tick_guess_dict[moves[0]]['R'])


while True:

    # display robot position
    if predator.is_valid:
        display.agent(step=predator.step, color="blue", size= 15)
    else:
        display.agent(step=predator.step, color="grey", size= 15)

    display.update()
    sleep(0.5)

tracker.unsubscribe()

