import web_resources as wr
from graph import Graph
import numpy as np


def is_coord(coord):
    return type(coord) is dict and "x" in coord and "y" in coord and type(coord["x"]) is int and type(coord["y"]) is int

def coord_sum(c1,c2):
    if is_coord(c1) and is_coord(c2):
        return {"x": c1["x"] + c2["x"] , "y": c1["y"] + c2["y"]}
    else:
        return None

class World:
    def __init__(self, world_name):
        world_json = wr.get_resource("world", world_name)
        self.name = world_json["name"]
        self.cells = world_json["cells"]
        minx, maxx = self.cells[0]["coordinates"]["x"], self.cells[0]["coordinates"]["x"]
        miny, maxy = self.cells[0]["coordinates"]["y"], self.cells[0]["coordinates"]["x"]
        for cell in self.cells:
            if cell["coordinates"]["x"] < minx:
                miny = cell["coordinates"]["x"]
            if cell["coordinates"]["x"] > maxx:
                maxx = cell["coordinates"]["x"]
            if cell["coordinates"]["y"] < miny:
                miny = cell["coordinates"]["y"]
            if cell["coordinates"]["y"] > maxy:
                maxy = cell["coordinates"]["y"]
        self.dimensions = {"width": 1 + maxx - minx, "height": 1 + maxy - miny}
        self.coordinates = [{"x": minx, "y": miny}, {"x": maxx, "y": maxy}]
        self.connection_pattern = world_json["connection_pattern"]
        self._index = -np.ones((self.dimensions["width"], self.dimensions["height"]), np.integer)
        index = 0
        for cell in self.cells:
            self._index[(cell["coordinates"]["x"] - self.coordinates[0]["x"], cell["coordinates"]["y"] - self.coordinates[0]["y"])] = index
            index += 1
        # self.visibility = Graph(self, ".visibility")
        self.graph = Graph(self)

    def is_valid(self, coordinates):
        if not is_coord(coordinates):
            return False
        if coordinates["x"] < self.coordinates[0]["x"] or coordinates["x"] > self.coordinates[1]["x"] or coordinates["y"] < self.coordinates[0]["y"] or coordinates["y"] > self.coordinates[1]["y"]:
            return False
        return True

    def find(self, coordinates):
        if not self.is_valid(coordinates):
            return None
        return self._index[(coordinates["x"]-self.coordinates[0]["x"], coordinates["y"]-self.coordinates[0]["y"])]

    def cell(self, coordinates):
        cell_id = self.find(coordinates)
        if cell_id is None:
            return cell_id
        return self.cells[cell_id]


