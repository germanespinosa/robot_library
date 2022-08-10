"""
test random code here
"""

from cellworld import World, Display, Location, Agent_markers, Capture, Capture_parameters, Step, Timer, Cell_group_builder, to_radians, to_degrees, Location_list, Cell_group, Cell_map

# create world object
occlusions = "20_05"
world = World.get_from_parameters_names("hexagonal", "canonical", occlusions)

# list all cellworld locations
locations = world.cells.get("location")
print(locations)

# find the distance between 2 locations
location1 = locations[0]
location2 = locations[1]
distance = location1.dist(location2)
print(distance)

