"""
Move Optimizer:
This code finds the number of ticks required to achieve desired robot movements
V4 excludes arc tuner for straight and rotation tuning

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
# TODO: might have to add move  number message to c class
import math
from math import sin, cos, pi, atan2, asin
from time import sleep
from cellworld import World, Display, Location, Agent_markers, Step, Timer, Cell_map, Coordinates
from cellworld_tracking import TrackingClient
from tcp_messages import MessageClient, Message
import sys
from cellworld_controller_service import ControllerClient
from json_cpp import JsonObject


# INPUT
moves = ["sw", "nw", "n", "ne", "se", "s", "nn", "w", "e"]
move = moves[0]
print("MOVE: ", move)

# GLOBALS
DIRECTION_X = 1
DIRECTION_Y = 1
MOVE_TUNED = False
PREVIOUS_LOCATION = None
VALUES = None
CORRECT_EXECUTION = 0
CONTROLLER_STATE = 1

class c(JsonObject):
    def __init__(self):
        self.left = 0
        self.right = 0
        self.speed = 0


class ControllerClient(MessageClient):
    class Behavior:
        Explore = 0
        Pursue = 1

    def __init__(self):
        MessageClient.__init__(self)
        self.on_step = None
        self.on_world_update = None
        self.router.add_route("_step$", self.__process_step__, Step)

    def __process_step__(self, step):
        if self.on_step:
            self.on_step(step)

    def __process_world_update__(self, world_info):
        if self.on_world_update:
            self.on_world_update(world_info)

    def tune(self) -> bool:
        return self.send_request(Message("tune")).get_body(bool)

    def pause(self) -> bool:
        return self.send_request(Message("pause")).get_body(bool)

    def resume(self) -> bool:
        return self.send_request(Message("resume")).get_body(bool)

    def stop(self) -> bool:
        return self.send_request(Message("stop")).get_body(bool)

    def set_destination(self, new_destination: Location) -> bool:
        return self.send_request(Message("set_destination", new_destination)).get_body(bool)

    def set_behavior(self, behavior: int) -> bool:
        return self.send_request(Message("set_behavior", behavior)).get_body(bool)

    def set_agent_values(self, values: JsonObject) -> int:
        return self.send_request(Message("set_agent_values", values)).get_body(int)

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


def angle_difference(a_prev, a_current, direction):
    '''
    Returns difference between start and current angle
    Angle returned will be <= 360
    '''
    a1 = a_prev
    a2 = a_current
    if direction == "cw":
        if a2 >= a1:
            return a2 - a1
        else:
            return a2 + 360 - a1
    elif direction == "ccw":
        if a2 <= a1:
            return a1 - a2
        else:
            return a1 + 360 - a2


def robot_tick_update(left_tick, right_tick, speed = 1000):
    '''
    Sends values to robot
    '''
    global VALUES
    VALUES.left = left_tick
    VALUES.right = right_tick
    VALUES.speed = speed
    # robot agent update
    controller.set_agent_values(VALUES)


def turn_robot():
    '''
    For keeping robot in bounds while tuning
    '''
    global DIRECTION_X
    global DIRECTION_Y
    limit = 0.7
    if DIRECTION_X > 0 and predator.step.location.x >= limit:
        DIRECTION_X = -1
        robot_tick_update(tick_guess_dict["s"]['L'], tick_guess_dict["s"]['R'])
        return True
    elif DIRECTION_X < 0 and predator.step.location.x <= -limit:
        DIRECTION_X = 1
        robot_tick_update(tick_guess_dict["s"]['L'], tick_guess_dict["s"]['R'])
        return True
    if DIRECTION_Y > 0 and predator.step.location.y >= limit:
        DIRECTION_Y = -1
        robot_tick_update(tick_guess_dict["s"]['L'], tick_guess_dict["s"]['R'])
        return True
    if DIRECTION_Y > 0 and predator.step.location.y <= -limit:
        DIRECTION_Y = 1
        robot_tick_update(tick_guess_dict["s"]['L'], tick_guess_dict["s"]['R'])
        return True

    return False


def tune_translation():
    """
    Update move straight tick dict based on error
    """
    global MOVE_TUNED
    global PREVIOUS_STEP
    global tick_guess_dict
    global CORRECT_EXECUTION
    P = 100
    STRAIGHT = move_constants_dict[move]['r']

    display.circle(predator.step.location, 0.01, "magenta")

    # measure state of robot after move
    current_step = predator.step

    # find error
    distance = current_step.location.dist(PREVIOUS_STEP.location)
    error = STRAIGHT - distance         # desired - actual
    #print(error, PREVIOUS_STEP.location, previous_location, current_step.location)


    # tick modification logic
    if abs(error) <= 0.0005: # 0.0005
        print(f'Correct Execution Count: {CORRECT_EXECUTION}')
        CORRECT_EXECUTION += 1
        if CORRECT_EXECUTION > 4:
            print(f"Desired Distance: {STRAIGHT}, Distance: {distance}, Ticks: {tick_guess_dict[move]}")
            print("MOVE 2 TUNING DONE")
            MOVE_TUNED = True
    else:
        CORRECT_EXECUTION = 0
        tick_guess_dict[move]['L'] += int(P * error)
        tick_guess_dict[move]['R'] += int(P * error)

        assert int(P * error) < 200, "DANGEROUS"
        assert tick_guess_dict[move]['L'] > 0, "WHY THE F IS IS NEG"

    if not MOVE_TUNED:
        robot_tick_update(tick_guess_dict[move]['L'], tick_guess_dict[move]['R'])
        print(f"Desired: {STRAIGHT}, Distance: {distance}, TICKS: {tick_guess_dict[move]['L']}")

    # record previous location
    PREVIOUS_STEP = current_step


def tune_rotation():
    """
    Assuming angle difference will be less than 360
    """
    global MOVE_TUNED
    global PREVIOUS_STEP
    global tick_guess_dict
    global CORRECT_EXECUTION
    TH = move_constants_dict[move]['th']

    P = 1
    if tick_guess_dict[move]['L'] > tick_guess_dict[move]['R']:
        rot_direction = "cw"
        fwd = 'L'
        bck = 'R'
    else:
        rot_direction = "ccw"
        fwd = 'R'
        bck = 'L'

    # measure state of robot after move
    current_step = predator.step

    # find error
    actual_angle = angle_difference(PREVIOUS_STEP.rotation, current_step.rotation, rot_direction)
    # assert actual_angle > 0, "something wrong with angle diff function"

    # tick modification logic
    if abs(TH - actual_angle) <= 1:
        print(f"Desired Angle: {TH}, Angle: {actual_angle}, Ticks: {tick_guess_dict[move]}")
        print(f"Correct Execution: {actual_angle}")
        CORRECT_EXECUTION += 1
        if CORRECT_EXECUTION > 5:
            print("DONE")
            MOVE_TUNED = True
    elif (TH > actual_angle):
        CORRECT_EXECUTION = 0
        tick_guess_dict[move][fwd] += 10
        tick_guess_dict[move][bck] -= 10
    else:
        CORRECT_EXECUTION = 0
        tick_guess_dict[move][fwd] -= 10
        tick_guess_dict[move][bck] += 10

    if not MOVE_TUNED:
        # TODO: fix tick print it prints tick adjustment not ticks that caused angle actual
        robot_tick_update(tick_guess_dict[move]['L'], tick_guess_dict[move]['R'])
        print(f"Desired Angle: {TH}, Angle: {actual_angle}, Ticks: {tick_guess_dict[move]}")

    # record previous location
    PREVIOUS_STEP = current_step


def initial_direction(rot):
    '''
    For turing robot to keep in bounds while tuning
    '''
    global DIRECTION_X
    global DIRECTION_Y

    if rot <= 90:
        DIRECTION_X = 1
        DIRECTION_Y = 1
    elif rot <= 180:
        DIRECTION_X = 1
        DIRECTION_Y = -1
    elif rot <= 270:
        DIRECTION_X = -1
        DIRECTION_Y = -1
    else:
        DIRECTION_X = -1
        DIRECTION_Y = 1
    print("DIRECTION ")
    print(DIRECTION_X, DIRECTION_Y)

def initial_move():
    global PREVIOUS_STEP
    previous_location = predator.step.location
    previous_rotation = predator.step.rotation
    initial_direction(previous_rotation)
    PREVIOUS_STEP = Step(agent_name=predator, location=previous_location, rotation=90)
    display.circle(PREVIOUS_STEP.location, 0.005, "red")
    robot_tick_update(tick_guess_dict[move]['L'], tick_guess_dict[move]['R'])

def on_keypress(event):
    """
    Sets up keyboard intervention
    """
    global CONTROLLER_STATE

    if event.key == "p":
        print("pause")
        controller.pause()
        CONTROLLER_STATE = 0

    if event.key == "t":
        print("resume")
        controller.tune()
        CONTROLLER_STATE = 1


# CONSTANTS
STRAIGHT1 = 0.11/2.34
STRAIGHT2 = 0.127/2.34
time_out = 2.0
robot_speed = 1000
VALUES = c()

# DICTIONARIES
# store number of ticks for each move
# TODO: may need to change embedded code to both wheels need to reach tick value to stop
# TODO: check into alpha calculation
tick_guess_dict = { "sw": {'L': -1000, 'R': 1000},  # 150 ccw
                    "nw": {'L': -100, 'R': 100},  # 30 ccw
                    "n": {'L': 1000, 'R': 1000},   # straight
                    "ne": {'L': 100, 'R': -100},  # 30 cw
                    "se": {'L': 100, 'R': -100},  # 150 cw
                    "s": {'L': 338, 'R': -338},  # 180
                    "nn": {'L': 1500, 'R': 1500},  # long straight
                    "w": {'L': -100, 'R': 100},    # 90 ccw
                    "e": {'L': 100, 'R': -100}}  # 90 cw

# move characteristic dict - see diagram about measure angle in these pos
move_constants_dict = {"sw": {'r': 0,         'th': 150}, # 210
                       "nw": {'r': 0,         'th': 330},
                       "n": {'r': STRAIGHT1, 'th': 0},
                       "ne": {'r': 0,         'th': 30},
                       "se": {'r': 0,         'th': 150},
                       "s": {'r': 0,         'th': 180},
                       "nn": {'r': STRAIGHT2, 'th': 0},
                       "w": {'r': 0,         'th': 270},
                       "e": {'r': 0,         'th': 90}}
# function association dict
function_dict = {   "sw": tune_rotation,
                    "nw": tune_rotation,
                    "n": tune_translation,
                    "ne": tune_rotation,
                    "se": tune_rotation,
                    "s": tune_rotation,
                    "nn": tune_translation,
                    'w': tune_rotation,
                    'e': tune_rotation}

prev_error = 0
D = 1
mode1 = False

##### Setup #####
# WORLD AND AGENT
occlusions = "00_00"
world = World.get_from_parameters_names("hexagonal", "canonical", occlusions)
display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)
map = Cell_map(world.configuration.cell_coordinates)
predator = AgentData("predator")
predator = AgentData("predator")
display.set_agent_marker("predator", Agent_markers.arrow())
cid_keypress = display.fig.canvas.mpl_connect('key_press_event', on_keypress)  # keypress setup


# CONTROLLER CLIENT
controller_timer = Timer(3.0)     # initialize controller timer variable
controller = ControllerClient()
if not controller.connect("127.0.0.1", 4590):
    print("failed to connect to the controller")
    exit(1)
controller.set_request_time_out(10000)
controller.subscribe()
controller.on_step = on_step


if not controller.tune():
    print("WRONG STATE")
    exit()
sleep(0.1)

# INITIAL GUESS
sleep(0.2)
initial_move()
loop_count = 0

while True:
    if controller.is_move_done() and not MOVE_TUNED and CONTROLLER_STATE:
        print("MOVE DONE")
        # keeps robot in habitat if  near bounds
        if turn_robot():
            print("TURNING")
            # if needs to turn wait till move is done before sending another move
            while not controller.is_move_done():
                continue
        # for tuning moves 2 and 5 dont use tuner class can tune in one step
        function_dict[move]()
        loop_count += 1

    # # display robot position
    if predator.is_valid:
        display.agent(step=predator.step, color="blue", size= 15)
    else:
        display.agent(step=predator.step, color="grey", size= 15)

    display.update()
    sleep(0.5)

print("PROCESS ENDED")
controller.unsubscribe()










