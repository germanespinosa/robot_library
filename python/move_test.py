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
# from cellworld_controller_service import ControllerClient
from json_cpp import JsonObject
from tcp_messages import MessageClient, Message
from cellworld import *


class c(JsonObject):
    def __init__(self):
        self.left =100
        self.right = 100
        self.speed = 100

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

    def set_agent_values(self, values: JsonObject):
        return self.send_request(Message("set_agent_values", values)).get_body(int)

    def set_left_ticks(self, left_ticks: int) -> bool:
        return self.send_request(Message("set_left_ticks", left_ticks)).get_body(bool)

    def set_right_ticks(self, right_ticks: int) -> bool:
        return self.send_request(Message("set_right_ticks", right_ticks)).get_body(bool)

    def set_speed(self, speed: int) -> bool:
        return self.send_request(Message("set_speed", speed)).get_body(bool)

    def agent_move_number(self, move_number: int) -> bool:
        return self.send_request(Message("move_number", move_number)).get_body(bool)


    # def update(self) -> int:
    #     """
    #     in theory what to recieve move number back
    #     """
    #     return self.send_request(Message("agent_update")).get_body(int)



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


def robot_tick_update(left_tick, right_tick):
    controller.set_left(left_tick)
    controller.set_right(right_tick)
    controller.set_speed(robot_speed)
    controller.update()


# CONSTANTS
time_out = 1.0
robot_speed = 500


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

values = c()


# Try move
# print(controller.set_agent_values(values))
# robot_tick_update(100, 200)
move_number = 1
print(controller.tune())
controller.agent_move_number(move_number)
controller.set_left_ticks(100)
controller.set_right_ticks(100)
controller.set_speed(100)
# print(controller.update())  #TODO: figure out how to recieve proper value

display.circle(predator.step.location, 0.005, "cyan")
a = 1
while True:


    # display robot position
    if predator.is_valid:
        display.agent(step=predator.step, color="blue", size= 15)
    else:
        display.agent(step=predator.step, color="grey", size= 15)

    display.update()
    sleep(0.2)

tracker.unsubscribe()

