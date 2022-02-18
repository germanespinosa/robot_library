from cellworld import World, Display, Location, Agent_markers, Capture, Capture_parameters, Step, Timer
from controller import ControllerClient


time_out = 1.0

class AgentData:
    def __init__(self, agent_name: str):
        self.is_valid = None
        self.step = Step()
        self.step.agent_name = agent_name


predator = AgentData("predator")
prey = AgentData("prey")


def on_step(step: Step):
    if step.agent_name == "predator":
        predator.is_valid = Timer(time_out)
        predator.step = step
    else:
        prey.is_valid = Timer(time_out)
        prey.step = step

# connect to controller


controller = ControllerClient()
if not controller.connect("127.0.0.1", 4590):
    print("failed to connect to the controller")
    exit(1)

controller.subscribe()
controller.on_step = on_step

occlusions = "10_05"
world = World.get_from_parameters_names("hexagonal", "canonical", occlusions)

display = Display(world, fig_size=(9, 8), animated=True)

capture = Capture(Capture_parameters(2.0, 90.0),world)


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

display.set_agent_marker("predator", Agent_markers.arrow())

display.set_agent_marker("prey", Agent_markers.arrow())

while running:
    if prey.is_valid:
        display.agent(step=prey.step, color="blue")
    else:
        display.agent(step=prey.step, color="gray")

    if predator.is_valid:
        display.agent(step=predator.step, color="blue")
    else:
        display.agent(step=predator.step, color="gray")
    display.update()


controller.unsubscribe()
controller.stop()
