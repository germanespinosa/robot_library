from cellworld import *

world = World.get_from_parameters_names("hexagonal", "canonical", "04_03")



display = Display(world, animated=True)


display.set_agent_marker("predator", Agent_markers.arrow())


speed = .01

goal = Location(10, .48)

occlusions_mass = .001
occlusions = world.cells.occluded_cells()

predator_location = Location(0, .5)
predator_rotation = 90


forces = []

forces.append((goal, 100))

for occlusion in occlusions:
    occlusion_center = occlusion.location
    forces.append((occlusion_center, -occlusions_mass))


def get_vector(speed: float) -> Location:
    global forces
    result = Location(0, 0)
    for location, mass in forces:
        distance = predator_location.dist(location)
        gravity = mass / (distance ** 2)
        result.move(predator_location.atan(location), gravity)
    return result * (speed / result.mod())


while True:
    change = get_vector(speed)
    new_predator_location = predator_location + change
    predator_rotation = to_degrees(predator_location.atan(new_predator_location))
    predator_location = new_predator_location
    display.agent(location=predator_location, rotation=predator_rotation, agent_name="predator", color="blue", size=30 )
    display.update()

