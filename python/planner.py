from cellworld import World, Display, Location, Agent_markers, Capture, Capture_parameters, Step, Timer, Cell_group_builder, Polygon, Location_visibility, Polygon_list
from controller import ControllerClient
from cellworld_experiment_service import ExperimentClient
from random import choice
from time import sleep
#
#
#
# def load_world(occlusions):
#     global display
#     global world
#     world = World.get_from_parameters_names("hexagonal", "canonical", occlusions) ################# fix this
#     display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)
#
#
# # load world
# load_world("10_05")
#
# occlusion_locations = world.cells.occluded_cells().get("location") # all oclludded LOCATIONS in world
# occlusions_polygons = Polygon_list.get_polygons(occlusion_locations, world.configuration.cell_shape.sides, world.implementation.cell_transformation.size / 2, world.implementation.space.transformation.rotation + world.implementation.cell_transformation.rotation) # ploygon object
# visibility = Location_visibility(occlusions_polygons) # create visiblilty object
#
#
#
#
# # current location
# loc = world.cells[0].location
# loc1 = world.cells[150].location
# display.circle(loc, .01, 'blue')
#
# l = visibility.is_visible(loc, loc1)
# print(l)
#
# l = visibility.visible_locations(loc, world)
# print(l)
#
#
# # for i in visibility.invisble_locations(loc, world):
# #     display.circle(world.cells.get("location")[i], .01, 'cyan')
# #
# # display.circle(loc, .01, 'blue')


def on_keypress(event):
    global running
    if event.key == "p":
       print("p")
    if event.key == "r":
       print("(r")
    if event.key == "q":
        running = False
    if event.key == "m":
        # set initial destination and timer
        print("m")

world = World.get_from_parameters_names("hexagonal", "canonical", "10_05") ################# fix this
display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)
cid_keypress = display.fig.canvas.mpl_connect('key_press_event', on_keypress)
running = True
while running:
    display.update()
    sleep(0.1)
    continue
