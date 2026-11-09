"""Synchronize a disposable SolidCraft scene without reopening its document."""
from dataclasses import dataclass, replace
import json

from .preview_scene import PreviewNode, placement_delta


class PreviewRejected(RuntimeError):
    """An update was rejected and the previous preview has been restored."""


@dataclass
class LiveNode:
    node: PreviewNode
    component: int
    occurrence: int
    feature: int | None = None


class SceneSync:
    def __init__(self, call, max_line, rejected_errors=(PreviewRejected, ValueError)):
        self.call = call
        self.max_line = max_line
        self.live = {}
        self.rejected_errors = rejected_errors

    def command(self, command, params):
        result = self.call('engine.execute', {'command': command, 'params': params})
        # timeline.add reports feature errors in a successful protocol response.
        if result.get('error'):
            raise PreviewRejected(result['error'])
        return result

    def batch(self, commands):
        """Apply ordinary changes in one UI frame, splitting only large requests."""
        pending, size = [], 0

        def flush():
            result = self.call('engine.batch', {'commands': pending, 'rollback': True})
            if result.get('failed'):
                raise PreviewRejected(str(result['failed']))
            for item in result.get('results', []):
                if item.get('error'):
                    raise PreviewRejected(item['error'])

        for command in commands:
            length = len(json.dumps(command).encode()) + 2
            if length > self.max_line - 4096:
                raise PreviewRejected('Part exceeds SolidCraft’s control limit; last preview retained')
            if pending and (size + length > self.max_line - 4096 or len(pending) == 10000):
                flush()
                pending, size = [], 0
            pending.append(command)
            size += length
        if pending:
            flush()

    def update(self, scene):
        desired = {node.key: node for node in scene.nodes}
        if len(desired) != len(scene.nodes):
            raise ValueError('Duplicate preview instance paths')
        changed = [node for node in scene.nodes if node.key not in self.live or
                   self.live[node.key].node.digest != node.digest or
                   self.live[node.key].node.name != node.name or
                   self.live[node.key].node.placement != node.placement]
        removed = self.live.keys() - desired.keys()
        if not changed and not removed:
            return False
        # Check individual payloads before altering anything in the viewer.
        for node in changed:
            if node.step and len(json.dumps(node.step).encode()) > self.max_line - 4096:
                raise PreviewRejected(f'{node.name} exceeds SolidCraft’s control limit; last preview retained')
        checkpoint = self.command('edit.checkpoint', {})['checkpoint']
        live = {key: replace(value) for key, value in self.live.items()}
        try:
            commands = []
            # Deleting a parent also deletes its descendants.
            for key in sorted(removed, key=len):
                if key[:-1] not in removed:
                    commands.append({'command': 'component.delete',
                                     'params': {'component': live[key].component}})
                del live[key]
            self.batch(commands)
            commands = []
            for node in changed:
                entry = live.get(node.key)
                if entry is None:
                    parent = live[node.key[:-1]].component if len(node.key) > 1 else 0
                    created = self.command('component.create', {
                        'name': node.name, 'parent': parent, 'activate': False, 'grounded': False})
                    entry = LiveNode(PreviewNode(node.key, node.name),
                                     created['component'], created['occurrence'])
                    live[node.key] = entry
                    # Place new empty components before adding their geometry so
                    # they never flash at the origin between control replies.
                    if node.placement != entry.node.placement:
                        self.command('occurrence.move', {
                            'occurrence': entry.occurrence, 'space': 'local',
                            'matrix': placement_delta(node.placement, entry.node.placement),
                        })
                        entry.node = replace(entry.node, placement=node.placement)
                old = entry.node
                if old.name != node.name:
                    commands.append({'command': 'component.rename',
                                     'params': {'component': entry.component, 'name': node.name}})
                    if entry.feature is not None:
                        commands.append({'command': 'timeline.rename',
                                         'params': {'feature': entry.feature, 'name': node.name}})
                if node.step is None and entry.feature is not None:
                    commands.append({'command': 'timeline.delete', 'params': {'features': [entry.feature]}})
                    entry.feature = None
                elif node.step is not None and entry.feature is None:
                    added = self.command('timeline.add', {'feature': {
                        'type': 'import', 'name': node.name, 'component': entry.component,
                        'file': f'{node.key[-1]}.step', 'step': node.step,
                    }})
                    entry.feature = added['feature']
                elif node.digest != old.digest:
                    commands.append({'command': 'timeline.edit', 'params': {
                        'feature': entry.feature,
                        'set': {'step': node.step, 'body_names': []},
                    }})
                if old.placement != node.placement:
                    commands.append({'command': 'occurrence.move', 'params': {
                        'occurrence': entry.occurrence, 'space': 'local',
                        'matrix': placement_delta(node.placement, old.placement),
                    }})
                entry.node = node
            self.batch(commands)
        except self.rejected_errors:
            # A failed edit must not switch to a full import or permanently disable
            # live updates. Restore both geometry and undo history; retry next save.
            try:
                self.command('edit.restore_checkpoint', {'checkpoint': checkpoint})
                self.command('edit.restore_checkpoint', {'checkpoint': checkpoint, 'forget': True})
            except Exception as exc:
                raise RuntimeError('SolidCraft could not restore the previous preview') from exc
            raise
        try:
            self.command('edit.restore_checkpoint', {'checkpoint': checkpoint, 'forget': True})
        except Exception as exc:
            raise RuntimeError('SolidCraft could not finish the preview transaction') from exc
        self.live = live
        return True
