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


def initialize():
    pass


def robot_tick_update(left_tick, right_tick):
    robot_client.set_left(300)
    robot_client.set_right(300)
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
            return a2 + math.pi *2 - a1
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

# Globals


# Variables
time_out = 1.0
robot_speed = 100

# Setup
occlusions = "20_05"
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

# Tasks
# 1. initialize robot
# 2. move tuner - dont have to have initialize done btw
# 3. actual controller
prev_error = 0
P = 1
D = 1
mode1 = False
while True:
    # get robot to start position
    while False:

        destination = get_center_location(predator.step.location)
        dist = predator.step.location.dist(destination)
        destination_theta = predator.step.location.atan(destination)# atan
        theta = to_radians(predator.step.rotation)
        error = angle_difference(theta, destination_theta) * turn_direction(theta, destination_theta)
        normal_error = normalize_error(error)
        error_derivative = (error - prev_error)
        prev_error = error
        adjustment = error * P + D * error_derivative
        #left_tick = normal_error *
        robot_tick_update(left_tick, right_tick)
        robot_mode_1(predator.step.location, destination)

    # move robot to init location

    destination = get_center_location(predator.step.location)
    dist = predator.step.location.dist(destination)
    destination_theta = predator.step.location.atan(destination)# atan
    theta = to_radians(predator.step.rotation)
    error = angle_difference(theta, destination_theta) * turn_direction(theta, destination_theta)
    normal_error = normalize_error(error)
    error_derivative = (error - prev_error)
    prev_error = error
    adjustment = error * P + D * error_derivative




    # display robot position
    if predator.is_valid:
        display.agent(step=predator.step, color="blue", size= 15)
    else:
        display.agent(step=predator.step, color="grey", size= 15)

    display.circle(start_destination, 0.01, 'red')

    display.update()
    sleep(0.1)

tracker.unsubscribe()

