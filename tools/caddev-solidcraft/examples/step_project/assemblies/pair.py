from build123d import Location
from caddev import Assembly, Instance
from parts.example import part


pair = Assembly(
    id="pair",
    instances=[
        Instance("first", "example"),
        Instance("second", "example", location=Location((0, 0, part.height))),
    ],
)
