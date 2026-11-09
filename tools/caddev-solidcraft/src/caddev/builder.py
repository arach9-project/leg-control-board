"""Incremental build session; failed targets are never reused as current inputs."""
from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
import sys

from .dependencies import Dependencies
from .exporter import BuiltTarget, build_assembly, build_component, write_step
from .loader import fresh_sources, reload_project
from .part_metadata import write_metadata


@dataclass
class BuildReport:
    built: list[str] = field(default_factory=list)
    errors: dict[str, str] = field(default_factory=dict)


class BuildSession:
    def __init__(self, root: Path):
        self.root = root.resolve()
        self.cache: dict[str, BuiltTarget] = {}
        self.dependencies = {}
        self.graph = None
        self.signature = None
        self.project = None
        self.failed = set()
        self.assembly_signatures = {}

    def rebuild(self, changed: set[Path] | None = None) -> BuildReport:
        report = BuildReport()
        try:
            with fresh_sources(self.root):
                project = reload_project(self.root)
                graph = Dependencies(self.root)
                signature = repr((project.components, [a.id for a in project.assemblies]))
                full = (changed is None or self.graph is None or graph.uncertain
                        or self.graph.uncertain or signature != self.signature
                        or self.root / 'project.py' in changed
                        or graph.files != self.graph.files)
                dependencies = {c.id: graph.module_paths(c.module) for c in project.components}
                for a in project.assemblies:
                    origins = {Path(m.__file__).resolve() for m in list(sys.modules.values())
                               if getattr(m, '__file__', None) and m.__name__ != 'caddev_user_project'
                               and any(v is a for v in list(vars(m).values()))}
                    dependencies[a.id] = graph.closure(origins & graph.files)
                affected = set(dependencies) if full else {
                    id for id, paths in dependencies.items()
                    if (paths | self.dependencies.get(id, set())) & changed}
                assembly_signatures = {a.id: repr(a) for a in project.assemblies}
                affected |= {id for id, signature in assembly_signatures.items()
                             if self.assembly_signatures.get(id) != signature}
                self.assembly_signatures = assembly_signatures
                affected |= self.failed
                # Assembly dependencies are propagated to a fixed point.
                while True:
                    more = {a.id for a in project.assemblies
                            if any(i.component in affected for i in a.instances)}
                    if more <= affected:
                        break
                    affected |= more
                ids = set(dependencies)
                self.cache = {id: value for id, value in self.cache.items()
                              if id in ids and id not in affected}
                self.project, self.graph = project, graph
                self.signature, self.dependencies = signature, dependencies
                self.failed &= ids
                for c in project.components:
                    if c.id not in affected:
                        continue
                    try:
                        target = build_component(c.build(), c.name or c.id)
                        path = self.root / 'build' / 'parts' / f'{c.id}.step'
                        write_step(target, path)
                        write_metadata(target, c.id, path)
                        self.cache[c.id] = target
                        self.failed.discard(c.id)
                        report.built.append(c.id)
                    except Exception as exc:
                        self.failed.add(c.id)
                        report.errors[c.id] = str(exc)
                assemblies = {a.id: a for a in project.assemblies}
                attempted = set()
                def build(id):
                    if id not in affected or id in attempted:
                        return
                    attempted.add(id)
                    a = assemblies[id]
                    for i in a.instances:
                        if i.component in assemblies:
                            build(i.component)
                    try:
                        missing = [i.component for i in a.instances if i.component not in self.cache]
                        if missing:
                            raise ValueError(f'Unavailable dependencies: {", ".join(missing)}')
                        target = build_assembly(a, self.cache)
                        write_step(target, self.root / 'build' / 'assemblies' / f'{id}.step')
                        self.cache[id] = target
                        self.failed.discard(id)
                        report.built.append(id)
                    except Exception as exc:
                        self.failed.add(id)
                        report.errors[id] = str(exc)
                for id in assemblies:
                    build(id)
        except Exception as exc:
            report.errors['project'] = str(exc)
            self.graph = None  # Recover conservatively after invalid configuration.
        return report
