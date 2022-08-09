"""
Move Optimizer:
This code finds the number of ticks required to achieve desired robot movements

Code:
STEP 1:
stores current location of the robot - PREVIOUS LOCATION
displays location
executes move (initial guess in tick_guess_dict)
STEP 2:
waits for move to finish
checks that robot will not go out of bounds - fix if it will
measures state of robot - current_location
find the error (desired - (current - previous))
updates the tick_guess_dict based on error
stores the current_location as - PREVIOUS_LOCATION
executes the move again

this is an inifinite loop until move error is small enough
"""

import math
from math import sin, cos, pi, atan2, asin
from time import sleep
from cellworld import World, Display, Location, Agent_markers, Step, Timer, Cell_map, Coordinates
from cellworld_tracking import TrackingClient
from tcp_messages import MessageClient, Message
import sys

# GLOBALS
DIRECTION_X = 1
MOVE_TUNED = False
PREVIOUS_LOCATION = None


class Robot_client(MessageClient):

    def __init__(self):
        MessageClient.__init__(self)

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

    def is_move_done(self) -> bool:
        return self.send_request(Message("is_move_done")).get_body(bool)


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
    return world.cells[map[Coordinates(x,y)]].location


def robot_tick_update(left_tick, right_tick):
    robot_client.set_left(left_tick)
    robot_client.set_right(right_tick)
    robot_client.set_speed(robot_speed)
    robot_client.update()


def turn_robot():
    global DIRECTION_X
    if DIRECTION_X > 0 and predator.step.location.x > 0.9:
        DIRECTION_X = -1
        robot_tick_update(tick_guess_dict["m6"]['L'], tick_guess_dict["m6"]['R'])
        return True
    elif DIRECTION_X < 0 and predator.step.location.x < -0.9:
        DIRECTION_X = 1
        robot_tick_update(tick_guess_dict["m6"]['L'], tick_guess_dict["m6"]['R'])
        return True
    return False

def tune_move3():
    """
    Update move straight tick dict based on error
    """
    global MOVE_TUNED
    global PREVIOUS_LOCATION
    global tick_guess_dict

    display.circle(predator.step.location, 0.01, "magenta")

    # measure state of robot after move
    current_location = predator.step.location

    # find error
    distance = current_location.dist(PREVIOUS_LOCATION)
    error = STRAIGHT - distance         # desired - actual

    # tick modification logic
    if abs(error) <= 0.0001:
        print(f"Distance: {distance}, Ticks: {tick_guess_dict['m3']}")
        print("DONE")
        MOVE_TUNED = True
    else:
        tick_guess_dict[move]['L'] += P * error
        tick_guess_dict[move]['R'] += P * error

        robot_tick_update(tick_guess_dict[move]['L'], tick_guess_dict[move]['R'])
        print(f"Desired: {STRAIGHT}, Distance: {distance}, TICKS: {tick_guess_dict[move]['L']}")

    # record previous location
    PREVIOUS_LOCATION = current_location


# CONSTANTS
R1 = (0.0635/2)/2.34
R2 = R1 + (0.127/2)/2.34
TH1 = 120
TH2 = 60
TH3 = 180
STRAIGHT = 0.11
time_out = 2.0
robot_speed = 100

# DICTIONARIES
# store number of ticks for each move
tick_guess_dict =  {"m0": {'L': 212, 'R': 815},
                    "m1": {'L': 231, 'R': 535},  # 497, 1135
                    "m2": {'L': 100, 'R': 100}, # 216
                    "m3": {'L': 306, 'R': 128},
                    "m4": {'L': 323, 'R':-33},
                    "m5": {'L': 840, 'R': -840},
                    "m6": {'L': 420, 'R': -420},
                    "m7": {'L': 216, 'R': 216}} # init
# move characteristic dict
move_constants_dict= {"m0": {'r': R1,       'th': TH1},
                      "m1": {'r': R2,       'th': TH2},
                      "m2": {'r': STRAIGHT, 'th': 0},
                      "m3": {'r': R2,       'th': -TH2},
                      "m4": {'r': R1,       'th': -TH1},
                      "m5": {'r': 0,        'th': 180},
                      "m6": {'r': 0,        'th': 90}}
# function association dict
function_dict =  {  "m0": 0,
                    "m1": 0,
                    "m2": tune_move3,
                    "m3": 0,
                    "m4": 0,
                    "m5": 0,
                    "m6": 0 }
moves = ["m0", "m1", "m2", "m3", "m4", "m5", "m6"]

# Variables
move = moves[2]
prev_error = 0
P = 100
D = 1
mode1 = False


##### Setup #####
# WORLD AND AGENT
occlusions = "00_00"
world = World.get_from_parameters_names("hexagonal", "canonical", occlusions)
display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)
map = Cell_map(world.configuration.cell_coordinates)
predator = AgentData("predator")
display.set_agent_marker("predator", Agent_markers.arrow())


# TRACKER CLIENT
tracker = TrackingClient()
if tracker.connect("127.0.0.1"):
    print("connected to tracker")
else:
    print("failed to connect to tracker")
tracker.set_request_time_out(5000)
tracker.subscribe()
tracker.set_throughput(5)
tracker.on_step = on_step

# ROBOT CLIENT -> to send ticks  - change to real robot ip
robot_ip = sys.argv[1]
robot_client = Robot_client()
if robot_client.connect("127.0.0.1", 6300):
    print("connected to robot! yay")
else:
    print("failed to connect to robot! bummer")
    exit(1)
robot_client.subscribe()

# TUNER
# if move!= moves[2] or move!= moves[5]:
#     tuner_object = Tuner(move)
#     step1_done = True
#     step2_done = False

# INITIAL GUESS
PREVIOUS_LOCATION = get_location(0, 0)
display.circle(PREVIOUS_LOCATION, 0.005, "red")
robot_tick_update(tick_guess_dict[move]['L'], tick_guess_dict[move]['R'])

# display.circle(tuner_object.center_location, 0.005, "cyan")

a = 1
while True:
    if robot_client.is_move_done() and not MOVE_TUNED:
        print("MOVE DONE")
        # keeps robot in habitat if  near bounds
        if turn_robot():
            print("TURNING")
            # if needs to turn wait till move is done before sending another move
            while not robot_client.is_move_done():
                continue

        function_dict[move]()
    #     # measure state of robot after move
    #     # current_location = predator.step.location

    # display robot position
    if predator.is_valid:
        display.agent(step=predator.step, color="blue", size= 15)
    else:
        display.agent(step=predator.step, color="grey", size= 15)

    display.update()
    sleep(0.5)

tracker.unsubscribe()
robot_client.unsubscirbe()

#









