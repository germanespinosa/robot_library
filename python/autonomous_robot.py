"""
https://github.com/germanespinosa/tcp-messages/tree/master/python/resources
https://github.com/germanespinosa/json-cpp/tree/master/python/resources
"""


from cellworld import World, Timer, Display, Space, Location
from tcp_messages import Message, MessageClient
from cellworld import *
from json_cpp import JsonObject, JsonList
from cellworld_tracking import TrackingClient
import random
import time

def prey_visible():
    print("k")

    #destination_cell = # location of prey

    # if desitination cell occluded return none
    # if destination_cell.occluded:
    #     print("can't navigate to an occluded cell")
    #     return
    #
    # # otherwise send destination to controller
    # controller.connection.send(Message("set_destination", destination_cell.location))
    # while not controller.messages.contains("set_destination_result"):
    #     pass






# world
occlusions = "04_05"

# connect to controller
controller = MessageClient()
controller.router.add_route("prey seen", prey_visible())
controller.connect("127.0.0.1", 4520)
controller.subscribe()

# connect to tracker ... remove this eventually
tracker = TrackingClient()
tracker.connect("127.0.0.1") # port is 4510 see tracking client
tracker.subscribe() # get broadcasted messages

# def prey_seen_func(jsonList):
#     jsonList.prey location

# create world objects
world = World.get_from_parameters_names("hexagonal", "cv", occlusions)
src_space = world.implementation.space

mice_world = World.get_from_parameters_names("hexagonal", "mice", occlusions)
dst_space = mice_world.implementation.space



while not tracker.contains_agent_state("predator"):
    pass

display = Display(world, fig_size=(9, 8), animated=True)

# find current location of predator
robot = tracker.current_states["predator"].copy() # ROBOT {"time_stamp":55.7808,"agent_name":"predator","frame":557,"coordinates":{"x":-3,"y":9},"location":{"x":459.522,"y":934.431},"rotation":115.453,"data":""}





def predator_forage(world):
    # go to random NON visible cell but if see mouse change desination
    # set new destination once move (0,0)
    move = random.randint(0,330)
    destination_cell = world.cells[move]

    if destination_cell.occluded:
        print("can't navigate to an occluded cell")
        return

    # SEND NEW DESITNATION
    controller.connection.send(Message("set_destination", destination_cell.location))
    while not controller.messages.contains("set_destination_result"):
        pass





def on_keypress(event):
    '''
    Pause robot with p key
    '''
    if event.key == "p":
        print("Keyboard Interrupt Recieved -- Pausing Controller")
        controller.connection.send(Message("pause_controller"))

        while not controller.messages.contains("pause_controller_result"):
            print(controller.messages)
            pass
cid_keypress = display.fig.canvas.mpl_connect('key_press_event', on_keypress)


t = Timer(12000)
while t:


    predator_forage(world)
    robot = tracker.current_states["predator"].copy()
    display.agent(step=tracker.current_states["predator"], color="red")
    rotation = robot.rotation
    display.update()




controller.unregister_consumer()
