from cellworld import World, Display, Location, Agent_markers, Capture, Capture_parameters, Step, Timer, Cell_group_builder
from controller import ControllerClient
from cellworld_experiment_service import ExperimentClient
from random import choice
from time import sleep


def load_world(occlusions):
    global display
    global world
    world = World.get_from_parameters_names("hexagonal", "canonical", occlusions) ################# fix this
    #display = Display(world, fig_size=(9.0*.75, 8.0*.75), animated=True)


load_world("10_05")
print(world)

print(world.implementation.cell_transformation.size)