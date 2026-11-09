from caddev import Component, Project
from assemblies.pair import pair

project = Project(
    components=[Component(id='example', name='Example Part', module='parts.example')],
    assemblies=[pair],
)
