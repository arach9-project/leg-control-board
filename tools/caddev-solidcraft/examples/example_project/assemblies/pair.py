from build123d import Location
from caddev import Assembly, Instance

pair = Assembly(
    id='pair',
    instances=[
        Instance('first', 'example'),
        Instance('second', 'example', location=Location((30, 0, 0))),
    ],
)
