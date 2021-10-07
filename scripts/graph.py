import web_resources as wr
import networkx as nx
from map import Map

class Graph:
    def __init__(self, world, graph_name=None):
        if graph_name:
            self._connections = wr.get_resource("graph", world.name + "." + graph_name)
        else:
            map = Map(world)
            self._connections = [[] for x in range(len(world.cells))]
            for cell in world.cells:
                src = cell["id"]
                src_coord = cell["coordinates"]
                if cell["occluded"] == 0:
                    for conn in world.connection_pattern:
                        dst_coord = {"x": src_coord["x"] + conn["x"], "y": src_coord["y"] + conn["y"]}
                        dst_index = map.find(dst_coord)
                        if dst_index >= 0:
                            if world.cells[dst_index]["occluded"] == 0:
                                dst = world.cells[dst_index]["id"]
                                self.connect(src, dst, False)

    def connect(self, src, dst, bi=True):
        if not self.is_connected(src, dst):
            self._connections[src].append(dst)
        if bi and not self.is_connected(dst, src):
            self._connections[dst].append(src)

    def is_connected(self, src, dst):
        if src >= len(self._connections):
            return False
        return dst in self._connections[src]

    def to_nxgraph (self):
        nxgraph = nx.Graph()
        nxgraph.add_nodes_from(list(range(len(self._connections))))
        for src, conns in enumerate(self._connections):
            for dst in conns:
                nxgraph.add_edge(src, dst)
        return nxgraph