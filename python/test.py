"""
Non auto tuner
"""

import sys
from cellworld import *
from cellworld_controller_service import ControllerClient
from cellworld_tracking import TrackingClient
from time import sleep
from json_cpp import JsonList
from tcp_messages import MessageClient, Message

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

class c(JsonObject):
    def __init__(self):
        self.left = 0
        self.right = 0
        self.speed = 0


class AgentData:
    """
    Class to initialize prey and predator objects
    """
    def __init__(self, agent_name: str):
        self.is_valid = None # timers for predator and prey updates
        self.step = Step()
        self.step.agent_name = agent_name


def load_world():
    """
    Load world to display
    """
    global display
    global world

    occlusion = Cell_group_builder.get_from_name("hexagonal", occlusions + ".occlusions")
    world.set_occlusions(occlusion)
    display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)


def on_step(step: Step):
    #print (step)
    """
    Updates steps and predator behavior
    """
    if step.agent_name == "prey":
        prey.is_valid = Timer(time_out) # pursue when prey is seen
        prey.step = step


def robot_tick_update(left_tick, right_tick):
    '''
    Sends values to robot
    '''
    global VALUES
    VALUES.left = left_tick
    VALUES.right = right_tick
    VALUES.speed = SPEED
    # robot agent update
    controller.set_agent_values(VALUES)

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


STRAIGHT1 = 0.11/2.34
STRAIGHT2 = 0.19/2.34
# STRAIGHT2 = 0.127/2.34
tick_guess_dict = { "sw": {'L': -990, 'R': 990},  # 150 ccw
                    "nw": {'L': -520, 'R': 520},  # 30 ccw
                    "n": {'L': 1020, 'R': 1020},   # straight
                    "ne": {'L': 520, 'R': -520},  # 30 cw
                    "se": {'L': 920, 'R': -920},  # 150 cw
                    "s": {'L': 1450, 'R': -1450},  # 180
                    "nn": {'L': 1720, 'R': 1720},  # long straight
                    "w": {'L': -740, 'R': 740},    # 90 ccw
                    "e": {'L': 710, 'R': -710}}  # 90 cw

# move characteristic dict - see diagram about measure angle in these pos
move_constants_dict = {"sw": {'r': 0,         'th': 120}, # 210
                       "nw": {'r': 0,         'th': 60},
                       "n": {'r': STRAIGHT1, 'th': 0},
                       "ne": {'r': 0,         'th': 60},
                       "se": {'r': 0,         'th': 120},
                       "s": {'r': 0,         'th': 180},
                       "nn": {'r': STRAIGHT2, 'th': 0},
                       "w": {'r': 0,         'th': 90},
                       "e": {'r': 0,         'th': 90}}
moves = ["sw", "nw", "n", "ne", "se", "s", "nn", "w", "e"]

# SET UP GLOBAL VARIABLES
SPEED = 1000 #3100
occlusions = "00_00"
time_out = 1.0      # step timer for predator and prey
display = None
VALUES = c()

# create world
world = World.get_from_parameters_names("hexagonal", "canonical")
load_world()

#  create predator and prey objects
prey = AgentData("prey")


# CONNECT TO TRACKER
tracker = TrackingClient()
if not tracker.connect("127.0.0.1"):
    print("failed to connect to the controller")
    exit(1)
#controller.set_request_time_out(10000)
tracker.subscribe()
tracker.set_throughput(5.0) # 5 per second
tracker.on_step = on_step

# CONNECT TO CONTROLLER AND PUT IT IN TUNE STATE
controller_timer = Timer(3.0)     # initialize controller timer variable
controller = ControllerClient()
if not controller.connect("127.0.0.1", 4590):
    print("failed to connect to the controller")
    exit(1)
controller.set_request_time_out(10000)
controller.subscribe()
if not controller.tune():
    print("WRONG STATE")
    exit()
sleep(0.1)

display.set_agent_marker("prey", Agent_markers.arrow())


current_step = prey.step
display.agent(step=prey.step, color="blue", size=10)
display.update()
sleep(0.1)

# ask for initial move
print(controller.is_move_done())
move = input("ENTER MOVE: ")
robot_tick_update(tick_guess_dict[move]['L'], tick_guess_dict[move]['R'])

running = True
while running:

    if controller.is_move_done():

        # get initial and final step
        previous_step = current_step
        current_step = prey.step

        # translation
        if (move == 'n' or move == 'nn'):
            SPEED = 2000
            distance = current_step.location.dist(previous_step.location)
            desired_distance = move_constants_dict[move]['r']
            print(f"Desired: {desired_distance}, Distance: {distance}")
            if desired_distance > distance:
                print(f'Increase - Current Tick Value: {tick_guess_dict[move]["L"]}')
            elif desired_distance < distance:
                print(f'Decrease - Current Tick Value: {tick_guess_dict[move]["L"]}')

        # rotation
        else:
            SPEED = 2000
            desired_angle = move_constants_dict[move]['th']
            left_tick_value = tick_guess_dict[move]["L"]
            if left_tick_value < 0:
                direction = "ccw"
            else:
                direction = "cw"

            angle = angle_difference(previous_step.rotation, current_step.rotation, direction)

            print(f"Desired: {desired_angle}, Measured Angle: {angle}")
            if desired_angle > angle:
                print(f'Increase - Current Tick Value: {abs(left_tick_value)}')
            elif desired_angle < angle:
                print(f'Decrease - Current Tick Value: {abs(left_tick_value)}')


        # ask for new move after current move is done
        move = input("ENTER MOVE: ")
        robot_tick_update(tick_guess_dict[move]['L'], tick_guess_dict[move]['R'])



    # plotting the current location of the predator and prey
    if prey.is_valid:
        display.agent(step=prey.step, color="blue", size=10)

    else:
        display.agent(step=prey.step, color="gray", size=10)


    display.update()
    sleep(0.1)


controller.unsubscribe()
controller.stop()

