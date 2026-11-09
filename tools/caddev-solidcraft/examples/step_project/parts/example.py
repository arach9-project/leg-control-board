from build123d import Align, Box
from caddev import CadPart


class Example(CadPart):
    @property
    def height(self) -> float:
        return 10.0

    def build(self):
        return Box(20, 20, self.height, align=(Align.CENTER, Align.CENTER, Align.MIN))


part = Example()
