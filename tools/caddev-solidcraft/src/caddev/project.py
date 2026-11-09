"""Declarations for independently exported parts and assemblies."""
from __future__ import annotations

import importlib
import re
from dataclasses import dataclass, field
from typing import Callable, Any

from build123d import Location


@dataclass
class Component:
    id: str
    module: str
    factory: str = 'build'
    name: str | None = None

    def load_factory(self) -> Callable[[], Any]:
        factory = getattr(importlib.import_module(self.module), self.factory)
        from .parts import CadPart
        if isinstance(factory, CadPart):
            return lambda: factory
        if not callable(factory):
            raise TypeError(f'{self.module}.{self.factory} is not callable')
        return factory

    def build(self):
        return self.load_factory()()


@dataclass
class Mate:
    source_datum: str
    target_instance: str
    target_datum: str
    offset: Location = field(default_factory=Location)


@dataclass
class Instance:
    id: str
    component: str
    location: Location | None = None
    mate: Mate | None = None
    name: str | None = None


@dataclass
class Assembly:
    id: str
    instances: list[Instance] = field(default_factory=list)
    name: str | None = None
    datums: dict[str, Location] = field(default_factory=dict)


@dataclass
class Project:
    components: list[Component] = field(default_factory=list)
    assemblies: list[Assembly] = field(default_factory=list)

    def validate(self) -> None:
        targets = self.components + self.assemblies
        ids = [t.id for t in targets]
        if len(ids) != len(set(ids)):
            raise ValueError('Duplicate target IDs')
        for id in ids:
            if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]*', id):
                raise ValueError(f'Invalid target ID: {id!r}')
        assemblies = {a.id: a for a in self.assemblies}
        for a in self.assemblies:
            instances = {i.id: i for i in a.instances}
            if len(instances) != len(a.instances):
                raise ValueError(f'{a.id}: duplicate instance IDs')
            if not instances:
                raise ValueError(f'{a.id}: assembly is empty')
            for i in a.instances:
                if not isinstance(i.id, str) or not i.id.strip():
                    raise ValueError(f"{a.id}: instance ID must be nonempty")
                if i.component not in ids:
                    raise ValueError(f'{a.id}/{i.id}: unknown target {i.component}')
                if i.location is not None and i.mate is not None:
                    raise ValueError(f'{a.id}/{i.id}: choose location or mate')
                if i.mate and i.mate.target_instance not in instances:
                    raise ValueError(f'{a.id}/{i.id}: unknown mate target {i.mate.target_instance}')
            _acyclic({i.id: [i.mate.target_instance] if i.mate else [] for i in a.instances})
        _acyclic({a.id: [i.component for i in a.instances if i.component in assemblies]
                  for a in self.assemblies})


def _acyclic(graph: dict[str, list[str]]) -> None:
    active, done = set(), set()
    def visit(node):
        if node in active:
            raise ValueError(f'Cyclic dependency at {node}')
        if node in done:
            return
        active.add(node)
        for child in graph[node]:
            visit(child)
        active.remove(node)
        done.add(node)
    for node in graph:
        visit(node)
