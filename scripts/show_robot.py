import socket
import time
import json
sock = socket.socket()
host = "127.0.0.1" # Agent_ifo provider (location, rotation)
port = 5000
sock.connect((host, port))
time.sleep(.1)

request = json.dumps({"command": "get_agent_info", "content": ""}).encode('utf-8')

def get_agent_info():
    sock.send(request)
    time.sleep(.1)
    data = sock.recv(8192)
    while True:
        datas = data.decode('utf-8')
        response = json.loads(datas[:-1])
        if "command" in response and response["command"] == "set_agent_info":
            return json.loads(response["content"])


import math
import matplotlib.pyplot as plt
from world import World


world = World("hexa_10_05_mice")
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

tx = []
ty = []

agent_info = get_agent_info()
tx.append(agent_info["location"]["x"])
ty.append(agent_info["location"]["y"])
rotation = 120 - agent_info["theta"] * 180 / math.pi

trajectory, = ax.plot(tx,ty, 'r-') # Returns a tuple of line objects, thus the comma
agent_body, = ax.plot(tx[-1], ty[-1], marker="o", c="lightblue", markersize=20)
agent_dir, = ax.plot(tx[-1], ty[-1], marker=(3, 0, 0), c="blue", markersize=15)


while agent_info:
    agent_info = get_agent_info()
    print(agent_info)
    tx.append(agent_info["location"]["x"])
    ty.append(agent_info["location"]["y"])
    rotation = 120 - agent_info["theta"] * 180 / math.pi
    trajectory.set_xdata(tx)
    trajectory.set_ydata(ty)
    agent_dir.set_xdata(tx[-1])
    agent_dir.set_ydata(ty[-1])
    agent_dir.set_marker((3, 0, rotation))

    agent_body.set_xdata(tx[-1])
    agent_body.set_ydata(ty[-1])

    fig.canvas.draw()
    fig.canvas.flush_events()
    plt.pause(.001)
