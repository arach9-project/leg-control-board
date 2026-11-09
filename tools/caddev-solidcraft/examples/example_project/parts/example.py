from build123d import Box, Part


def build() -> Part:
    part =  Box(20, 20, 10) + Box(5,5,100)
    return part
