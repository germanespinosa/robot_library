from time import sleep
import sys
from random import choice
from cellworld import World, Display, Location, Agent_markers, Capture, Capture_parameters, Step, Timer, Cell_group_builder, to_radians, to_degrees, Location_list, Cell_group
from cellworld_controller_service import ControllerClient
from cellworld_experiment_service import ExperimentClient
from cellworld_tracking import TrackingClient
from cellworld_tracking import TrackingService
from matplotlib.backend_bases import MouseButton
from tcp_messages import MessageClient, Message


class AgentData:
    """
    Stores steps (info from tracker) for each agent
    """
    def __init__(self, agent_name: str):
        self.is_valid = None
        self.step = Step()                      # step object - step is a class in experiment.py
        self.step.agent_name = agent_name


def on_step(step):
    """
    Updates steps and predator behavior
    """
    if step.agent_name == "predator":
        predator.is_valid = Timer(time_out)
        predator.step = step


class Robot_client(MessageClient):
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


# Globals

# Variables
time_out = 1.0

robot_client = Robot_client()
if robot_client.connect("127.0.0.1", 6300):
    print("connected to robot! yay")
else:
    print("failed to connect to robot! bummer")
    exit(1)


# Setup
occlusions = "20_05"
world = World.get_from_parameters_names("hexagonal", "canonical", occlusions)

display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)
predator = AgentData("predator")
display.set_agent_marker("predator", Agent_markers.arrow())

# subscribe to tracking service this will tell you updated location of robot base on ticks sent   --- if this does not work see experiment client
# tracker
tracker = TrackingClient()
if tracker.connect("127.0.0.1"):
    print("connected to tracker")
else:
    print("failed to connect to tracker")
tracker.set_request_time_out(5000)
tracker.subscribe()
tracker.set_throughput(5)
tracker.on_step = on_step


robot_client.set_left(300)
robot_client.set_right(100)
robot_client.set_speed(100)
robot_client.update()
sleep(0.1)

robot_client.set_left(300)
robot_client.set_right(100)
robot_client.set_speed(100)
robot_client.update()
sleep(0.1)

while True:
    if predator.is_valid:
        display.agent(step=predator.step, color="blue", size= 15)
        print(predator.step)
    else:
        display.agent(step=predator.step, color="grey", size= 15)




    display.update()
    sleep(0.1)

tracker.unsubscribe()

