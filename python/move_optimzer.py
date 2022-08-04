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
- save new ticks to file
- tune all  moves in sequence
- refine tuning to precise, optimal tick
'''


import math
from math import sin, cos, pi, atan2, asin
from time import sleep
from cellworld import World, Display, Location, Agent_markers, Step, Timer, Cell_map, Coordinates
from cellworld_tracking import TrackingClient
from tcp_messages import MessageClient, Message
import matplotlib.pyplot as plt

# GLOBALS
DIRECTION_X = 1
MOVE_TUNED = False
PREVIOUS_LOCATION = None


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


def get_center_location(current_location):
    """
    Given current location finds the closests center location (center of a cell)
    """
    min_distance = 1
    for new_loc in world.cells.get('location'):
        distance = current_location.dist(new_loc)
        if distance < min_distance:
            min_distance = distance
            min_location = new_loc

    return min_location

def normalize_radians(angle):
    """
    Confines angle to [0, 2pi] range
    """
    while angle < 0:
        angle += 2 * math.pi
    while angle > 2*math.pi:
        angle -= 2 * math.pi
    return angle

def angle_difference(a1, a2):
    """
    Return smallest angle difference
    """

    a1 = normalize_radians(a1)
    a2 = normalize_radians(a2)
    if a1 > a2:
        dth = a1 - a2
        if dth < math.pi:
            return dth
        else:
            return a2 + math.pi * 2 - a1
    else:
        dth = a2 - a1
        if dth < math.pi:
            return dth
        else:
            return a1 + math.pi * 2 - a2

def turn_direction(a1, a2):
    a1 = normalize_radians(a1)
    a2 = normalize_radians(a2)
    if a1 > a2:
        dth = a1 - a2
        if dth < math.pi:
            return 1  # assuming robot is a1 cw
        else:
            return -1  # ccw
    else:
        dth = a2 - a1
        if dth < math.pi:
            return -1  # ccw
        else:
            return 1  # cw


def normalize_error(error):
    # dont really get this why mult by pi
    pi_error = math.pi * error
    return 1/(pi_error**2 +1)


def robot_mode_1(position, destination):
    """
    This is initialization mode it sets the robot up to execute 6 moves from adequate start position
    In real world might need to put a tolerance on this
    """
    if position == destination:
        return True
    else:
        return False


def turn_robot():
    global DIRECTION_X
    if DIRECTION_X > 0 and predator.step.location.x > 0.9:
        DIRECTION_X = -1
        predator.move_done = False
        robot_tick_update(tick_guess_dict["m6"]['L'], tick_guess_dict["m6"]['R'])
    elif DIRECTION_X < 0 and predator.step.location.x < -0.9:
        DIRECTION_X = 1
        predator.move_done = False
        robot_tick_update(tick_guess_dict["m6"]['L'], tick_guess_dict["m6"]['R'])

def move_done(move_number):
    print("move ", move_number, "done")
    # display.circle(predator.step.location)
    # display.update()

    predator.move_state = move_number
    # when this function is called move state changes
    predator.move_done = True

def get_location(x,y):
    return world.cells[map[Coordinates(x,y)]].location

def tune_move6():
    pass

def tune_move3():
    global MOVE_TUNED
    global PREVIOUS_LOCATION

    display.circle(predator.step.location, 0.01, "magenta")
    predator.move_done = False
    # take measurement
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


####################### MOVES 1245 #####################################################################################
class Tuner:
    def __init__(self, move):
        self.r_d = move_constants_dict[move]['r']     # m
        self.th_d = move_constants_dict[move]['th']   # degrees
        self.move = move
        self.center_location = 0

    def get_center_location(self, current_step):
        """
        Finds the origin of the polar coordinate reference frame
        """
        th = current_step.rotation * pi/180
        x = current_step.location.x - (self.r_d * cos(th))
        y = current_step.location.y - (self.r_d * -sin(th))   # fix this based on turn direction
        center_location = Location(x, y)
        return center_location

    def find_ratio(self, current_step, move_dict):
        """
        Checks radius to adjust tick ratio

        """
        ratio_accuracy = 0.0001 # will change this to error

        # distance from center to robot - remember center location is updated each trial
        r_actual = self.center_location.dist(current_step.location)

        outer_key = max(move_dict)
        inner_key = min(move_dict)

        # check if results are "close enough"
        if ((r_actual < (self.r_d + ratio_accuracy)) and (r_actual > (self.r_d - ratio_accuracy))):
            print("STEP 1 DONE!")
            print(f"outer ticks = {tick_guess_dict[self.move][outer_key]}, inner ticks = {tick_guess_dict[self.move][inner_key]}")
            predator.move_done = True
            return True

        # inner wheel too many ticks
        elif r_actual > self.r_d:
            tick_guess_dict[self.move][outer_key] += 20

        # outer wheel too many ticks
        else:
            tick_guess_dict[self.move][inner_key] += 20

        print(f'radius desired: {self.r_d}, radius actual: {r_actual}')
        return False

    def find_alpha(self, current_step, previous_step, move_dict):
        outer_key = max(move_dict)
        inner_key = min(move_dict)
        tick_ratio = tick_guess_dict[self.move][inner_key]/tick_guess_dict[self.move][outer_key]

        alpha_accuracy = 1
        d = current_step.location.dist(previous_step.location)/2
        #r = self.center_location.dist(current_step.location)    # TODO: IMPROVE PRECISION
        r = self.r_d
        print(f'd: {d}, r: {r}')
        alpha = 2 * asin(d/r)
        alpha = alpha * (180/pi)

        # check if results are "close enough"
        if ((alpha < (self.th_d + alpha_accuracy)) and (alpha > (self.th_d - alpha_accuracy))):
            print("STEP 2 DONE!")
            print(f"Desired: {self.th_d}, Actual: {alpha}")
            print(f"outer ticks = {tick_guess_dict[self.move]['L'] }, inner ticks = {tick_guess_dict[self.move]['R'] }")
            return True

        # not enough ticks
        elif alpha < self.th_d:
            tick_guess_dict[self.move][outer_key] += 30
            tick_guess_dict[self.move][inner_key] = int(tick_guess_dict[self.move][outer_key] * tick_ratio)

        # too many ticks
        else:
            tick_guess_dict[self.move][outer_key] -= 30
            tick_guess_dict[self.move][inner_key] = int(tick_guess_dict[self.move][outer_key] * tick_ratio)

        print(f"Desired: {self.th_d}, Actual: {alpha}")
        print(f"outer ticks = {tick_guess_dict[self.move]['L'] }, inner ticks = {tick_guess_dict[self.move]['R'] }")
        return False
########################################################################################################################




# CONSTANTS
R1 = 0.0635/2.34
R2 = R1 + 0.11/2.34
TH1 = 120
TH2 = 60
TH3 = 180
STRAIGHT = 0.11
time_out = 2.0
robot_speed = 100


# GLOBALS
# the desired change in x,y,th for each move
# delta_dict = { "m1" :  np.array([,y1, 120]),
#                "m2" :  np.array([x2, y2, 60]),
#                "m3" :  np.array([23, 23, 0]),
#                "m4" :  np.array([x2, -y2, -60]),
#                "m5" :  np.array([x1, -y1, -120]),
#                "m6" :  np.array([0, 0, 180])}
# initial tick guess
tick_guess_dict =  {"m1": {'L': 212, 'R': 815},# 70, 260
                    "m2": {'L': 128, 'R': 306},
                    "m3": {'L': 216, 'R': 216},
                    "m4": {'L': 306, 'R': 128},
                    "m5": {'L': 323, 'R':-33},
                    "m6": {'L': 840, 'R': -840},
                    "m7": {'L': 420, 'R': -420},
                    "m8": {'L': 216, 'R': 216}}
# move characteristic dict
move_constants_dict= {"m1": {'r': R1,       'th': TH1},
                      "m2": {'r': R2,       'th': TH2},
                      "m3": {'r': STRAIGHT, 'th': 0},
                      "m4": {'r': R2,       'th': -TH2},
                      "m5": {'r': R1,       'th': -TH1},
                      "m6": {'r': 0,        'th': 180},
                      "m7": {'r': 0,        'th': 90}}
# function association dict
function_dict =  {  "m1": 0,
                    "m2": 0,
                    "m3": tune_move3,
                    "m4": 0,
                    "m5": 0,
                    "m6": 0,
                    "m7": 0 }
moves = ["m1", "m2", "m3", "m4", "m5", "m6", "m7"]
move = moves[0]


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
# robot client broadcasts when the move is done
robot_client.move_done = move_done
robot_client.subscribe()

# Tasks
# 1. initialize robot
# 2. move tuner - dont have to have initialize done btw
# 3. actual controller
prev_error = 0
P = 1
D = 1
mode1 = False


# MOVE TUNER
previous_theta = 0
direction = 'cw'
count = 0           # tuning is adequate if meet count requirement
step0_done = False
# current_theta = 0
TH = 90 # angle to tune
theta_accuracy = 0
PREVIOUS_LOCATION = get_location(0,0)
previous_step = Step(agent_name=predator, location=PREVIOUS_LOCATION, rotation=90)
STRAIGHT = (0.11/2)/2.34
distance_accuracy = 0




P = 100  #500 #1000
loop_count = 0
dist_list = []
count_list = []


# INITIALIZE TUNER
if move!= moves[2] or move!= moves[5]:
    tuner_object = Tuner(move)
    step1_done = True
    step2_done = False

# Try initial guess
assert tuner_object.r_d == R1
tuner_object.center_location = tuner_object.get_center_location(previous_step) # based on previous location
robot_tick_update(tick_guess_dict[move]['L'], tick_guess_dict[move]['R'])

display.circle(PREVIOUS_LOCATION, 0.005, "red")
display.circle(tuner_object.center_location, 0.005, "cyan")

while True:
    # turn predator around to keep it in bounds
    if predator.move_done:
        turn_robot()

    # send new ticks if previous move is done
    # if predator.move_done and not MOVE_TUNED:
    #     predator.move_done = False
    #     # function_dict[move]()

        # measure state of robot after move
        #current_location = predator.step.location

    while not step1_done and predator.move_done:
        predator.move_done = False

        # measure state of robot after move
        current_step = predator.step
        display.circle(current_step.location, 0.005, "red")



        # STEP 1: change tick ratio based on new state distance from center
        step1_done = tuner_object.find_ratio(current_step, tick_guess_dict[move])

        # update center
        # previous_step = current_step
        # tuner_object.center_location = tuner_object.get_center_location(previous_step)
        # display.circle(tuner_object.center_location, 0.005, "cyan")

        # execute new move
        if not step1_done:
            # update center
            previous_step = current_step
            tuner_object.center_location = tuner_object.get_center_location(previous_step)
            display.circle(tuner_object.center_location, 0.005, "cyan")
            robot_tick_update(tick_guess_dict[move]['L'], tick_guess_dict[move]['R'])


    while not step2_done and step1_done and predator.move_done:
        predator.move_done = False

        # STEP1 is done == ratio found
        # STEP2 : change # ticks based on alpha
        current_step = predator.step
        display.circle(current_step.location, 0.005, "magenta")

        step2_done = tuner_object.find_alpha(current_step, previous_step, tick_guess_dict[move])

        # update center location
        previous_step = current_step
        #tuner_object.center_location = tuner_object.get_center_location(previous_step) # should not need this since tick ratio was found

        # execute new move
        if not step2_done:
            robot_tick_update(tick_guess_dict[move]['L'], tick_guess_dict[move]['R'])






    # display robot position
    if predator.is_valid:
        display.agent(step=predator.step, color="blue", size= 15)
        # pass
    else:
        display.agent(step=predator.step, color="grey", size= 15)

    display.update()
    sleep(0.5)

tracker.unsubscribe()

