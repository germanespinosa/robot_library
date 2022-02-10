from cellworld import World, Timer, Display, Space, Location
from tcp_messages import Message, MessageClient
from cellworld_tracking import TrackingClient
from controller import ControllerClient

# connect to controller
controller = ControllerClient()
controller.connect("127.0.0.1", 4590)
controller.subscribe()

tracker = TrackingClient()
tracker.connect("127.0.0.1")
tracker.subscribe()

occlusions = "10_05"
world = World.get_from_parameters_names("hexagonal", "canonical", occlusions)

while not tracker.contains_agent_state("predator"):
    pass

display = Display(world, fig_size=(9, 8), animated=True)

robot = tracker.current_states["predator"].copy()

def on_click(event):
    location = Location(event.xdata, event.ydata)  # event.x, event.y
    cell_id = world.cells.find(location)
    destination_cell = world.cells[cell_id]
    print("CELL", destination_cell)
    if destination_cell.occluded:
        print("can't navigate to an occluded cell")
        return
    controller.set_destination (destination_cell.location)



cid = display.fig.canvas.mpl_connect('button_press_event', on_click)

running = True

def on_keypress(event):
    global running
    if event.key == "p":
        controller.pause()
    if event.key == "r":
        controller.resume()
    if event.key == "q":
        running = False


cid_keypress = display.fig.canvas.mpl_connect('key_press_event', on_keypress)

while running:
    robot = tracker.current_states["predator"].copy()
    display.agent(step=tracker.current_states["predator"], color="red")
    rotation = robot.rotation
    display.update()

controller.stop()
tracker.unregister_consumer()
