import numpy as np
from world import World


class Heat_map:

  def __init__(self, world):
    self.world = world
    self.size = (world.dimensions["height"], world.dimensions["width"])
    self.values = np.zeros(self.size)
    self.minx = world.coordinates[0]["x"]
    self.miny = world.coordinates[0]["y"]

  def get_index(self, id_or_coordinates):
    coordinates = {}
    if type(id_or_coordinates) is int:
      coordinates = self.world.cells[id_or_coordinates]["coordinates"]
    else:
      coordinates = id_or_coordinates
    if self.world.is_valid(coordinates):
      return (coordinates["y"] - self.miny, coordinates["x"] - self.minx)
    else:
      return None

  def __getitem__(self, id_or_coordinates):
    index = self.get_index(id_or_coordinates)
    if index:
      return self.values[index]
    else:
      return None

  def __setitem__(self, id_or_coordinates, value):
    index = self.get_index(id_or_coordinates)
    if index:
      self.values[index] = value
