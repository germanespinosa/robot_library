import pygame
from datetime import datetime

class GamePad:
    BLACK = pygame.Color('black')
    WHITE = pygame.Color('white')

    class TextPrint(object):
        def __init__(self):
            self.reset()
            self.font = pygame.font.Font(None, 20)

        def tprint(self, screen, textString):
            textBitmap = self.font.render(textString, True, GamePad.BLACK)
            screen.blit(textBitmap, (self.x, self.y))
            self.y += self.line_height

        def reset(self):
            self.x = 10
            self.y = 10
            self.line_height = 15

        def indent(self):
            self.x += 10

        def unindent(self):
            self.x -= 10

    def __init__(self, joystick_number: int = 0):
        pygame.init()
        joystick_count = pygame.joystick.get_count()
        if joystick_count == 0:
            print("no Joystick found")
            exit(1)
        self.joystick = pygame.joystick.Joystick(joystick_number)
        self.joystick.init()

        self.screen = pygame.display.set_mode((400, 430))

        pygame.display.set_caption("My Game")
        self.done = False
        self.clock = pygame.time.Clock()
        pygame.joystick.init()
        self.textPrint = GamePad.TextPrint()

        self.axis_count = self.joystick.get_numaxes()
        self.name = self.joystick.get_name()
        self.axis = [0 for x in range(self.axis_count)]

        self.buttons_count = self.joystick.get_numbuttons()
        self.buttons = [0 for x in range(self.buttons_count)]

        self.hats_count = self.joystick.get_numhats()
        self.hats = [0 for x in range(self.hats_count)]

    def update(self) -> bool:
        for event in pygame.event.get(): # User did something.
            if event.type == pygame.QUIT: # If user clicked close.
                done = True # Flag that we are done so we exit this loop.
            elif event.type == pygame.JOYBUTTONDOWN:
                print("Joystick button pressed.")
            elif event.type == pygame.JOYBUTTONUP:
                print("Joystick button released.")
        self.screen.fill(GamePad.WHITE)
        self.textPrint.reset()
        self.joystick.init()

        try:
            jid = self.joystick.get_instance_id()
        except AttributeError:
            jid = self.joystick.get_id()
        self.textPrint.tprint(self.screen, "Joystick {}".format(jid))
        self.textPrint.indent()
        self.textPrint.tprint(self.screen, "Joystick name: {}".format(self.name))

        try:
            guid = self.joystick.get_guid()
        except AttributeError:
            pass
        else:
            self.textPrint.tprint(self.screen, "GUID: {}".format(guid))

        self.textPrint.tprint(self.screen, "Number of axes: {}".format(self.axis_count))
        self.textPrint.indent()
        for i in range(self.axis_count):
            self.axis[i] = self.joystick.get_axis(i)
            self.textPrint.tprint(self.screen, "Axis {} value: {}".format(i, self.axis[i]))
        self.textPrint.unindent()

        self.textPrint.tprint(self.screen, "Number of buttons: {}".format(self.buttons_count))
        self.textPrint.indent()
        for i in range(self.buttons_count):
            self.buttons[i] = self.joystick.get_button(i)
            self.textPrint.tprint(self.screen,
                                  "Button {:>2} value: {}".format(i, self.buttons[i]))
        self.textPrint.unindent()

        self.textPrint.tprint(self.screen, "Number of hats: {}".format(self.hats_count))
        self.textPrint.indent()
        for i in range(self.hats_count):
            self.hats[i] = self.joystick.get_hat(i)
            self.textPrint.tprint(self.screen, "Hat {} value: {}".format(i, str(self.hats[i])))
        self.textPrint.unindent()
        self.textPrint.unindent()
        pygame.display.flip()
        self.clock.tick(20)
        return not self.done

    def wait(self, time_out: float = 1):
        if self.done:
            return False
        start = datetime.now()
        while (datetime.now()-start).total_seconds() < time_out:
            if not self.update():
                return False
        return True

    def __del__(self):
        pygame.quit()


if __name__ == "__main__":
    gamepad = GamePad()
    while gamepad.update():
        pass