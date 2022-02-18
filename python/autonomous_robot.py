"""
https://github.com/germanespinosa/tcp-messages/tree/master/python/resources
https://github.com/germanespinosa/json-cpp/tree/master/python/resources
"""

from controller import ControllerClient

from cellworld import World, Timer, Display, Space, Location
from tcp_messages import Message, MessageClient
from cellworld import *
from json_cpp import JsonObject, JsonList
from cellworld_tracking import TrackingClient
import random
import time

def MyFunc(step:Step):
    global predator_step
    global prey_step
    if step.agent_name == "prey":
        prey_step = step
    else:
        predator_step = step








# connect to controller
controller = ControllerClient()
controller.connect("127.0.0.1", 4590)
controller.subscribe()


# create world object
occlusions = "10_05"
world = World.get_from_parameters_names("hexagonal", "canonical", occlusions)
display = Display(world, fig_size=(9, 8), animated=True)

# store previous steps
prev_prey_step = prey_step
prev_predator_step = predator_step

controller.on_step = MyFunc
running = True
while running:
    controller.on_step




    controller.on_step(display)
    print(controller.prey_acquired())  # returns bool if prey is aquired



    #print(controller.)
    #controller.on_step
    # robot = tracker.current_states["predator"].copy()
    # display.agent(step=tracker.current_states["predator"], color="red")
    # rotation = robot.rotation
    # display.update()

controller.stop()