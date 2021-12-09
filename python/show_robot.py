import time

import matplotlib.pyplot as plt

from cellworld_py import *
from agent_tracking_py import Agent_tracking, Filter
import subprocess
import matplotlib as mpl

tracker = Agent_tracking()
tracker.filter = Filter(.8, 10, 5)
tracker.register_consumer()

t = Timer(12000)

occlusions = "10_05"

world = World.get_from_parameters_names("hexagonal", "cv", "10_05")
src_space = world.implementation.space

mice_world = World.get_from_parameters_names("hexagonal", "mice", "10_05")
dst_space = mice_world.implementation.space

while not tracker.contains_agent_state("predator"):
    pass

display = Display(world, animated=True)

robot = tracker.current_states["predator"].copy()

# tx = []
# ty = []

# tx.append(robot.location.x)
# ty.append(robot.location.y)
# trajectory, = display.ax.plot(tx, ty, 'r-')# Returns a tuple of line objects, thus the comma

def onclick(event):
    location = Space.transform_to(Location(event.xdata, event.ydata), src_space, dst_space)
    cell_id = mice_world.cells.find(location)
    destination_cell = mice_world.cells[cell_id]
    print("CELL", destination_cell)
    if destination_cell.occluded:
        print("can't navigate to an occluded cell")
        return
    p = subprocess.Popen(["wsl", "-e", "/mnt/c/Research/robot/robot_library/cmake-build-debug/controller", "-o", occlusions, "-d", str(destination_cell.location), "-n", ".001", "-fd", ".001", "-br", ".001"])

cid = display.fig.canvas.mpl_connect('button_press_event', onclick)


while t:
    robot = tracker.current_states["predator"].copy()
    display.agent(step=tracker.current_states["predator"], color="dimgray", show_trajectory=True)
    # tx.append(robot.location.x)
    # ty.append(robot.location.y)
    rotation = robot.rotation
    # trajectory.set_xdata(tx)
    # trajectory.set_ydata(ty)
    display.update()

tracker.unregister_consumer()
