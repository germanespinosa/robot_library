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
        self.move_done = False


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
        # self.move_done = None
        # self.router.add_route("move_done", self.__move_done__, int)
    #
    # def __move_done__(self, move_number):
    #     if self.move_done:
    #         self.move_done(move_number)

    def set_left(self, v: int) -> bool:
        return self.send_request(Message("set_left", v)).get_body(bool)

    def set_right(self, v: int) -> bool:
        return self.send_request(Message("set_right", v)).get_body(bool)

    def set_speed(self, v: int) -> bool:
        return self.send_request(Message("set_speed", v)).get_body(bool)

    def update(self) -> bool:
        return self.send_request(Message("update")).get_body(bool)

    def is_move_done(self) -> bool:
        return self.send_request(Message("is_move_done")).get_body(bool)



def initialize():
    pass


def robot_tick_update(left_tick, right_tick):
    robot_client.set_left(left_tick)
    robot_client.set_right(right_tick)
    robot_client.set_speed(robot_speed)
    robot_client.update()

def move_done(move_number):
    print("move ", move_number, "done")
    predator.move_state = move_number
    # when this function is called move state changes
    predator.move_done = True



# CONSTANTS
time_out = 1.0
robot_speed = 100


# GLOBALS
tick_guess_dict =  {"m1": {'L': 212, 'R': 815},
                    "m2": {'L': 231, 'R': 535},
                    "m3": {'L': 432, 'R': 432},
                    "m4": {'L': 535, 'R': 231},
                    "m5": {'L': 815, 'R': 212},
                    "m6": {'L': 450, 'R': -450},
                    "m7": {'L': 420, 'R': -420},
                    "m8": {'L': 216, 'R': 216}}   # initialize 11/2

moves = ["m1", "m2", "m3", "m4", "m5", "m6", "m7","m8"]
move = moves[3]




# Setup
occlusions = "00_00"
world = World.get_from_parameters_names("hexagonal", "canonical", occlusions)
display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)
map = Cell_map(world.configuration.cell_coordinates)
predator = AgentData("predator")
display.set_agent_marker("predator", Agent_markers.arrow())

# subscribe to tracker to receive robot step updates
tracker = TrackingClient()
if tracker.connect("192.168.137.155"):
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

robot_client.subscribe()



# Try move
# robot_tick_update(tick_guess_dict[moves[6]]['L'], tick_guess_dict[moves[6]]['R'])


robot_tick_update(tick_guess_dict[moves[7]]['L'], tick_guess_dict[moves[7]]['R'])
a = 1
while True:

    # if robot_client.is_move_done() and a == 1:
    #     print("MOVE DONE")
    #     robot_tick_update(tick_guess_dict[moves[5]]['L'], tick_guess_dict[moves[5]]['R'])
    #     a = 0
    # print(predator.step.rotation)

    # display robot position
    if predator.is_valid:
        display.agent(step=predator.step, color="blue", size= 15)
    else:
        display.agent(step=predator.step, color="grey", size= 15)

    display.update()
    sleep(0.5)

tracker.unsubscribe()

