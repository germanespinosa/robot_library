from tcp_messages import MessageClient, Message
from cellworld import *


class ControllerClient(MessageClient):

    def __init__(self):
        MessageClient.__init__(self)
        self.on_step = None
        self.on_world_update = None
        self.router.add_route("_step$", self.__process_step__, Step)
        self.router.add_route("world_update", self.__process_world_update__, World_info)

    def __process_step__(self, step):
        if self.on_step:
            self.on_step(step)

    def __process_world_update__(self, world_info):
        if self.on_world_update:
            self.on_world_update(world_info)

    def pause(self) -> bool:
        return self.send_request(Message("pause_predator")).get_body(bool)

    def resume(self) -> bool:
        return self.send_request(Message("resume_predator")).get_body(bool)

    def stop(self) -> bool:
        return self.send_request(Message("stop_predator")).get_body(bool)

    def set_destination(self, new_destination: Location) -> bool:
        return self.send_request(Message("set_destination", new_destination)).get_body(bool)

    def prey_acquired(self) -> bool:
        return self.send_request(Message("prey_acquired")).get_body(bool)

    def get_world_info(self) -> World_info:
        return self.send_request(Message("get_world_info")).get_body(World_info)
