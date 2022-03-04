from tcp_messages import MessageClient, Message
from cellworld import Location, to_degrees


class PreySimulator(MessageClient):
    def __init__(self, auto_rotation=True):
        self.auto_rotation = auto_rotation
        self.last_location = None
        MessageClient.__init__(self)

    def connect(self):
        return MessageClient.connect(self, "127.0.0.1", 4630)

    def update_location(self, prey_location: Location):
        self.send_message(Message("prey_location", prey_location))
        if self.auto_rotation:
            if self.last_location:
                prey_rotation = to_degrees(self.last_location.atan(prey_location))
                self.update_rotation(prey_rotation)
        self.last_location = prey_location

    def update_rotation(self, prey_rotation: float):
        self.send_message(Message("prey_rotation", prey_rotation))


