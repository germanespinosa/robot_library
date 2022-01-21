from cellworld import World, Timer, Display, Space, Location
from tcp_messages import Message, MessageClient
from cellworld_tracking import TrackingClient

controller = MessageClient()
controller.connect("127.0.0.1", 4520)

tracker = TrackingClient()
tracker.connect("127.0.0.1")
#tracker.filter = Filter(.8, 10, 5)
tracker.register_consumer()

t = Timer(12000)

# occlusions = "10_05"
occlusions = "00_00"

world = World.get_from_parameters_names("hexagonal", "cv", occlusions)
src_space = world.implementation.space

mice_world = World.get_from_parameters_names("hexagonal", "mice", occlusions)
dst_space = mice_world.implementation.space

while not tracker.contains_agent_state("predator"):
    pass

display = Display(world, fig_size=(9, 8), animated=True)

robot = tracker.current_states["predator"].copy()

def onclick(event):
    location = Space.transform_to(Location(event.xdata, event.ydata), src_space, dst_space)  # event.x, event.y
    cell_id = mice_world.cells.find(location)
    destination_cell = mice_world.cells[cell_id]
    print("CELL", destination_cell)
    if destination_cell.occluded:
        print("can't navigate to an occluded cell")
        return
    controller.connection.send(Message("set_destination", destination_cell.location))
    while not controller.messages.contains("set_destination_result"):
        pass


cid = display.fig.canvas.mpl_connect('button_press_event', onclick)

def on_keypress(event):
    # if event.key == "ctrl+c":
    #     print("Keyboard Interrupt Recieved -- Stopping Controller")
    #     controller.connection.send(Message("stop_controller"))
    #
    #     # also stop the timer t / use a different setup for the main control loop
    #     #ASK: should this resend message on communciation failure? not much elese we can do...
    #
    #     while not controller.messages.contains("stop_controller_result"):
    #         pass

    if event.key == "p":
        print("Keyboard Interrupt Recieved -- Pausing Controller")
        controller.connection.send(Message("pause_controller"))

        while not controller.messages.contains("pause_controller_result"):
            print(controller.messages)
            pass


cid_keypress = display.fig.canvas.mpl_connect('key_press_event', on_keypress)



while t:
    robot = tracker.current_states["predator"].copy()
    display.agent(step=tracker.current_states["predator"], color="red")
    rotation = robot.rotation
    display.update()


tracker.unregister_consumer()
