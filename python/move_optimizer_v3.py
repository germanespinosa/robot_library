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
# TODO: add event handler for joystick
# TODO: manage joystick events in robot_agent
import math
from math import sin, cos, pi, atan2, asin
from time import sleep
from cellworld import World, Display, Location, Agent_markers, Step, Timer, Cell_map, Coordinates
from cellworld_tracking import TrackingClient
from tcp_messages import MessageClient, Message
import sys
from cellworld_controller_service import ControllerClient
from json_cpp import JsonObject


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


class Tuner:
    """
    For moves 0134
    """
    def __init__(self, move):
        self.r_d = move_constants_dict[move]['r']     # (m) radius from pivot location
        self.th_d = move_constants_dict[move]['th']   # (degrees) inner angle s = th * r
        self.move = move
        self.center_location = 0              # pivot location computed each iteration based on start location of robot

    def get_center_location(self, current_step):
        """
        Finds the origin of the polar coordinate reference frame - pivot point
        """
        th = current_step.rotation * pi/180
        x = current_step.location.x - (self.r_d * cos(th))
        y = current_step.location.y - (self.r_d * -sin(th))   # fix this based on turn direction
        center_location = Location(x, y)
        return center_location

    def find_ratio(self, current_step, move_dict):
        """
        STEP 1:
        Checks distance from pivot point and adjusts tick value to achieve desired radius

        :param current_step: current state of robot
        :param move_dict: tick_guess_dict[move] so dictionary with key = L or R, value: tick number
        :return: bool indicating whether ratio has been found
        """
        ratio_accuracy = 0.00001     # will change this to error

        # distance from center to robot - remember center location is updated each trial
        r_actual = self.center_location.dist(current_step.location)

        outer_key = max(move_dict)
        inner_key = min(move_dict)

        # check if results are "close enough"
        if ((r_actual < (self.r_d + ratio_accuracy)) and (r_actual > (self.r_d - ratio_accuracy))):
            print("STEP 1 DONE!")
            print(f'radius desired: {self.r_d}, radius actual: {r_actual}')
            print(f"outer ticks = {tick_guess_dict[self.move][outer_key]}, inner ticks = {tick_guess_dict[self.move][inner_key]}")
            print(" ")
            # predator.move_done = True
            return True

        # inner wheel too many ticks
        elif r_actual > self.r_d:
            tick_guess_dict[self.move][outer_key] += 5 #20

        # outer wheel too many ticks
        else:
            tick_guess_dict[self.move][inner_key] += 5 #20

        print(f"outer ticks = {tick_guess_dict[self.move][outer_key]}, inner ticks = {tick_guess_dict[self.move][inner_key]}")
        print(f'radius desired: {self.r_d}, radius actual: {r_actual}')
        return False

    def find_alpha(self, current_step, previous_step, move_dict):
        """
        STEP 2:
        Adjust ticks based on desired arc length while keeping desired tick ratio found in STEP1

        :param current_step: current state of robot
        :param previous_step: state of robot before move executed/tested
        :param move_dict: tick_guess_dict[move] so dictionary with key = L or R, value: tick number
        :return: bool indicating whether desired arc length has been achieved
        """
        alpha_accuracy = 1

        outer_key = max(move_dict)
        inner_key = min(move_dict)
        tick_ratio = tick_guess_dict[self.move][inner_key]/tick_guess_dict[self.move][outer_key]    # step 1 result

        # compute arc length angle right triangle
        d = current_step.location.dist(PREVIOUS_STEP.location)/2
        r = self.center_location.dist(current_step.location)    # TODO: IMPROVE PRECISION was making this r_d
        print(f'd: {d}, r: {r}')
        try:
            alpha = 2 * asin(d/r)
            alpha = alpha * (180/pi)
        except:
            print("CHECK RADIUS")
            alpha = 2 * asin(d/self.r_d)
            alpha = alpha * (180/pi)


        # check if results are "close enough"
        if ((alpha < (self.th_d + alpha_accuracy)) and (alpha > (self.th_d - alpha_accuracy))):
            print("STEP 2 DONE!")
            print(f"Desired: {self.th_d}, Actual: {alpha}")
            print(f"outer ticks = {tick_guess_dict[self.move]['L'] }, inner ticks = {tick_guess_dict[self.move]['R'] }")
            return True

        # not enough ticks
        elif alpha < self.th_d:
            tick_guess_dict[self.move][outer_key] += 2
            tick_guess_dict[self.move][inner_key] = int(tick_guess_dict[self.move][outer_key] * tick_ratio)

        # too many ticks
        else:
            tick_guess_dict[self.move][outer_key] -= 2
            tick_guess_dict[self.move][inner_key] = int(tick_guess_dict[self.move][outer_key] * tick_ratio)

        print(f"Desired: {self.th_d}, Actual: {alpha}")
        print(f"outer ticks = {tick_guess_dict[self.move]['L'] }, inner ticks = {tick_guess_dict[self.move]['R'] }")
        return False


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
    a1 = a_prev
    a2 = a_current
    if direction == "cw":
        if a2 >= a1:
            return a2 - a1
        else:
            return a2 + 360 - a1


def robot_tick_update(left_tick, right_tick, speed = 1700):
    global VALUES
    VALUES.left = left_tick
    VALUES.right = right_tick;
    VALUES.speed = speed
    # robot agent update
    controller.set_agent_values(VALUES)


def turn_robot():
    global DIRECTION_X
    global DIRECTION_Y
    limit = 0.7
    if DIRECTION_X > 0 and predator.step.location.x >= limit:
        DIRECTION_X = -1
        robot_tick_update(tick_guess_dict["m5"]['L'], tick_guess_dict["m5"]['R'])
        return True
    elif DIRECTION_X < 0 and predator.step.location.x <= -limit:
        DIRECTION_X = 1
        robot_tick_update(tick_guess_dict["m5"]['L'], tick_guess_dict["m5"]['R'])
        return True
    if DIRECTION_Y > 0 and predator.step.location.y >= limit:
        DIRECTION_Y = -1
        robot_tick_update(tick_guess_dict["m5"]['L'], tick_guess_dict["m5"]['R'])
        return True
    if DIRECTION_Y > 0 and predator.step.location.y <= -limit:
        DIRECTION_Y = 1
        robot_tick_update(tick_guess_dict["m5"]['L'], tick_guess_dict["m5"]['R'])
        return True

    return False


def tune_move2():
    """
    Update move straight tick dict based on error
    """
    global MOVE_TUNED
    global PREVIOUS_STEP
    global tick_guess_dict
    global CORRECT_EXECUTION
    P = 1000

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


def tune_move5():
    """
    Assuming angle difference will be less than 360
    """
    global MOVE_TUNED
    global PREVIOUS_STEP
    global tick_guess_dict
    global CORRECT_EXECUTION

    P = 1

    if tick_guess_dict[move]['L'] > tick_guess_dict[move]['R']:
        rot_direction = "cw"
        fwd = 'L'
        bck = 'R'
    else:
        rot_direction = "ccw"
        fwd = 'L'
        bck = 'R'

    # measure state of robot after move
    current_step = predator.step

    # find error
    actual_angle = angle_difference(PREVIOUS_STEP.rotation, current_step.rotation, rot_direction)
    # assert actual_angle > 0, "something wrong with angle diff function"

    # tick modification logic
    if abs(TH3 - actual_angle) <= 1:
        print(f"Desired Angle: {TH3}, Angle: {actual_angle}, Ticks: {tick_guess_dict[move]}")
        print(f"Correct Execution: {actual_angle}")
        CORRECT_EXECUTION += 1
        if CORRECT_EXECUTION > 5:
            print("DONE")
            MOVE_TUNED = True
    elif (TH3 > actual_angle):
        CORRECT_EXECUTION = 0
        tick_guess_dict[move][fwd] += 1
        tick_guess_dict[move][bck] -= 1
    else:
        CORRECT_EXECUTION = 0
        tick_guess_dict[move][fwd] -= 1
        tick_guess_dict[move][bck] += 1

    if not MOVE_TUNED:
        robot_tick_update(tick_guess_dict[move]['L'], tick_guess_dict[move]['R'])
        print(f"Desired Angle: {TH3}, Angle: {actual_angle}, Ticks: {tick_guess_dict[move]}")

    # record previous location
    PREVIOUS_STEP = current_step


def tune_move0134():
    global MOVE_TUNED
    global PREVIOUS_STEP
    global tick_guess_dict
    global STEP1_DONE
    global STEP2_DONE

    # measure state of robot after move
    current_step = predator.step
    display.circle(current_step.location, 0.005, "red")

    if not STEP1_DONE:
        # STEP 1: change tick ratio based on new state distance from center
        STEP1_DONE = tuner_object.find_ratio(current_step, tick_guess_dict[move])

        # execute new move
        if not STEP1_DONE:
            # update center
            PREVIOUS_STEP = current_step
            tuner_object.center_location = tuner_object.get_center_location(PREVIOUS_STEP)
            display.circle(tuner_object.center_location, 0.005, "cyan")
            robot_tick_update(tick_guess_dict[move]['L'], tick_guess_dict[move]['R'])

    if not STEP2_DONE and STEP1_DONE:
        STEP2_DONE = tuner_object.find_alpha(current_step, PREVIOUS_STEP, tick_guess_dict[move])
        # update center location
        # TODO: look into this is it causing problems added it pretty latep
        PREVIOUS_STEP = current_step
        tuner_object.center_location = tuner_object.get_center_location(PREVIOUS_STEP)
        display.circle(tuner_object.center_location, 0.005, "cyan")
        # execute new move
        if not STEP2_DONE:
            robot_tick_update(tick_guess_dict[move]['L'], tick_guess_dict[move]['R'])

    if STEP1_DONE and STEP2_DONE:
        return True
    else:
        False


def initial_direction(rot):
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
R1 = (0.0635/2)/2.34
R2 = R1 + (0.127/2)/2.34
TH1 = 120
TH2 = 60
TH3 = 180
STRAIGHT = (0.11)/2.34
time_out = 2.0
robot_speed = 800
VALUES = c()

# DICTIONARIES
# store number of ticks for each move
# TODO: may need to change embedded code to both wheels need to reach tick value to stop
# TODO: check into alpha calculation
tick_guess_dict =  {"m0": {'L': 5, 'R': 600},
                    "m1": {'L': 73, 'R': 284},     # 497, 1135
                    "m2": {'L': 256, 'R': 256},     # straight
                    "m3": {'L': 284, 'R': 73},
                    "m4": {'L': 600, 'R': 5},
                    "m5": {'L': 338, 'R': -338}, # 180
                    "m6": {'L': 200, 'R': -420},
                    "m7": {'L': 216, 'R': 216}}     # init
# move characteristic dict
move_constants_dict= {"m0": {'r': R1,       'th': TH1},
                      "m1": {'r': R2,       'th': TH2},
                      "m2": {'r': STRAIGHT, 'th': 0},
                      "m3": {'r': R2,       'th': -TH2},
                      "m4": {'r': R1,       'th': -TH1},
                      "m5": {'r': 0,        'th': 180},
                      "m6": {'r': 0,        'th': 90}}
# function association dict
function_dict =  {  "m0": tune_move0134,
                    "m1": tune_move0134,
                    "m2": tune_move2,
                    "m3": tune_move0134,
                    "m4": tune_move0134,
                    "m5": tune_move5,
                    "m6": 0 }
moves = ["m0", "m1", "m2", "m3", "m4", "m5", "m6"]

# Variables
move = moves[2]
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

# TUNER
if move != moves[2] and move != moves[5]:
    tuner_object = Tuner(move)
    STEP1_DONE = False
    STEP2_DONE = False
    tuner_object.center_location = tuner_object.get_center_location(PREVIOUS_STEP)
    display.circle(PREVIOUS_STEP.location, 0.005, "red")
    display.circle(tuner_object.center_location, 0.005, "cyan")

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










