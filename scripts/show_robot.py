import math

import matplotlib.pyplot as plt
from world import World
import numpy as np

world = World("hexa_00_00_mice")
plt.ion()
fig = plt.figure(figsize=(10, 9))
ax = fig.add_subplot(111)
ax.axes.xaxis.set_visible(False)
ax.axes.yaxis.set_visible(False)

cellx = [cell["location"]["x"] for cell in world.cells]
celly = [cell["location"]["y"] for cell in world.cells]
cellc = ["black" if cell["occluded"] else "white" for cell in world.cells]

xmin = min(cellx)
xmax = max(cellx)

cell_width = (xmax - xmin) / 20

ymin = min(celly)
ymax = max(celly)

cell_height = (ymax - ymin) / 15

ax.set_xlim(xmin=xmin - cell_width, xmax=xmax + cell_width)
ax.set_ylim(ymin=ymin - cell_height, ymax=ymax + cell_height)

centerx = (xmax-xmin) / 2
centery = (ymax-ymin) / 2

walls_drawing = ax.scatter([0.5], [0.5], c="white", alpha=1, marker=(6, 0, 90), s=300000, edgecolors="black", linewidths=1)
cells_drawing = ax.scatter(cellx, celly, c=cellc, alpha=1, marker=(6, 0, 0), s=850, edgecolors="lightgrey", linewidths=1)


lx = np.linspace(0, 1, 200)
ly = np.sin(lx * 8) / 3 + .5

trajectory, = ax.plot(lx[:1], ly[:1], 'r-') # Returns a tuple of line objects, thus the comma
agent_body, = ax.plot(ly[0], ly[0], marker="o", c="lightblue", markersize=20)
agent_dir, = ax.plot(ly[0], ly[0], marker=(3, 0, 0), c="blue", markersize=15)

for i in range(2, 200):
    trajectory.set_xdata(lx[:i])
    trajectory.set_ydata(ly[:i])
    agent_dir.set_xdata(lx[i])
    agent_dir.set_ydata(ly[i])
    agent_dir.set_marker((3, 0, 120 - math.atan2(lx[i]-lx[i-1], ly[i]-ly[i-1]) * 180 / math.pi))

    agent_body.set_xdata(lx[i])
    agent_body.set_ydata(ly[i])

    fig.canvas.draw()
    fig.canvas.flush_events()
    plt.pause(.001)






#import httpimport
#with httpimport.remote_repo(['world', 'web_resources', 'display', 'map', 'heat_map', 'graph'], 'https://raw.githubusercontent.com/germanespinosa/cellworld/master/python/'):

import matplotlib.pyplot as plt
import numpy as np
#
# x = np.linspace(0, 6*np.pi, 100)
# y = np.sin(x)
#
# # You probably won't need this if you're embedding things in a tkinter plot...
# plt.ion()
#
# fig = plt.figure()
# ax = fig.add_subplot(111)
# line1, = ax.plot(x, y, 'r-') # Returns a tuple of line objects, thus the comma
#
# for phase in np.linspace(0, 10*np.pi, 500):
#     line1.set_ydata(np.sin(x + phase))
#     fig.canvas.draw()
#     fig.canvas.flush_events()
#
# import math
# import time
#
# from world import World
# from heat_map import Heat_map
# from display import Display
# from IPython import display
# from matplotlib import pyplot as plt
# from IPython.display import clear_output
#
# world = World("hexa_00_00_mice")
# hm = Heat_map(world)
#
# plt.ion()
# d = Display(hm)
#
# while True:
#     for i in range(1000):
#         clear_output(wait=True)
#         d.prepare()
#         d.agents = []
#         d.add_agent((.5,.5), math.pi * 2 / 1000 * i, .1, "o", 20, "red", "robot")
#         d.fig.canvas.draw()
#         d.fig.canvas.flush_events()
#
#
#     # display.clear_output(wait=False)
#     # time.sleep(.5)
# d.show();
#
#
