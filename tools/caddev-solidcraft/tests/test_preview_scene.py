from dataclasses import replace
from pathlib import Path
import tempfile
import unittest
from unittest.mock import Mock

from build123d import Box, Location
from caddev import Assembly, Instance
from caddev.exporter import build_component, build_assembly, write_step
from caddev.preview import ControlError, MAX_LINE
from caddev.preview_scene import PreviewNode, PreviewScene, preview_scene, placement_delta
from caddev.preview_sync import SceneSync, PreviewRejected


class SceneTests(unittest.TestCase):
    def test_snapshot_reuses_part_payloads_and_preserves_nested_local_rotations(self):
        part = build_component(Box(2, 3, 4), 'Plate')
        sub = build_assembly(Assembly('sub', [Instance('plate', 'p',
                            location=Location((1, 2, 3), (180, 0, 0)))]), {'p': part})
        assembly = build_assembly(Assembly('a', [Instance('first', 'sub'),
                                  Instance('second', 'sub', location=Location((20, 0, 0)))]), {'sub': sub})
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = root/'build/parts/p.step'
            write_step(part, path)
            snapshot = preview_scene(assembly.shape, root, 'a')
            first, second = snapshot.nodes[2], snapshot.nodes[4]
            self.assertEqual(first.key, ('a', 'first', 'plate'))
            self.assertEqual(second.key, ('a', 'second', 'plate'))
            self.assertIs(first.step, second.step)
            self.assertAlmostEqual(first.placement[2][2], -1)
            self.assertAlmostEqual(snapshot.nodes[3].placement[0][3], 20)
            # Fresh export headers do not trigger any geometry updates.
            old = path.read_text()
            path.write_text(old.replace('FILE_NAME(', "FILE_NAME(/* new timestamp */", 1))
            again = preview_scene(assembly.shape, root, 'a')
            self.assertEqual(first.digest, again.nodes[2].digest)
            path.write_text('invalid')
            self.assertIsNotNone(snapshot.nodes[2].step)  # Immutable saved payload.

    def test_noncommuting_placement_delta(self):
        def matrix(location):
            t = location.wrapped.Transformation()
            return tuple(tuple(t.Value(r, c) for c in range(1, 5)) for r in range(1, 4)) + ((0, 0, 0, 1),)
        old = Location((10, 20, 30), (30, 45, 70))
        new = Location((-4, 7, 12), (180, 0, 90))
        actual = placement_delta(matrix(new), matrix(old))
        expected = matrix(new * old.inverse())
        for a, b in zip(actual, expected):
            for x, y in zip(a, b): self.assertAlmostEqual(x, y)


class SyncTests(unittest.TestCase):
    def setUp(self):
        self.sequence = 0
        def call(method, params):
            if method == 'engine.batch':
                return {'results': [{} for _ in params['commands']]}
            self.sequence += 1
            command = params['command']
            if command == 'edit.checkpoint': return {'checkpoint': self.sequence}
            if command == 'component.create':
                return {'component': self.sequence, 'occurrence': self.sequence + 1000}
            if command == 'timeline.add': return {'feature': self.sequence}
            return {}
        self.call = Mock(side_effect=call)
        self.sync = SceneSync(self.call, MAX_LINE, (ControlError, PreviewRejected, ValueError))
        self.a = PreviewNode(('root', 'a'), 'A', step='STEP A', digest='A')
        self.b = PreviewNode(('root', 'b'), 'B', step='STEP B', digest='B')
        self.scene = PreviewScene((PreviewNode(('root',), 'Root'), self.a, self.b))
        self.sync.update(self.scene)
        self.call.reset_mock()

    def commands(self):
        return [command for call in self.call.call_args_list for command in
                (call.args[1]['commands'] if call.args[0] == 'engine.batch' else [call.args[1]])]

    def test_changed_part_only_and_noop_save(self):
        old_ids = {key: (value.component, value.feature) for key, value in self.sync.live.items()}
        changed = replace(self.a, step='new dimension, body, color or name', digest='new')
        scene = PreviewScene((self.scene.nodes[0], changed, self.b))
        self.sync.update(scene)
        edits = [c for c in self.commands() if c['command'] == 'timeline.edit']
        self.assertEqual(len(edits), 1)
        self.assertEqual(edits[0]['params']['feature'], old_ids[self.a.key][1])
        self.assertEqual(edits[0]['params']['set']['body_names'], [])
        self.assertEqual(old_ids, {key: (v.component, v.feature) for key, v in self.sync.live.items()})
        self.call.reset_mock()
        self.assertFalse(self.sync.update(scene))
        self.call.assert_not_called()

    def test_move_and_rename_do_not_resend_geometry(self):
        changed = replace(self.a, name='Renamed', placement=((1,0,0,4),(0,-1,0,5),(0,0,-1,6),(0,0,0,1)))
        self.sync.update(PreviewScene((self.scene.nodes[0], changed, self.b)))
        commands = [c['command'] for c in self.commands()]
        self.assertIn('occurrence.move', commands)
        self.assertIn('component.rename', commands)
        self.assertNotIn('timeline.edit', commands)
        self.assertNotIn('doc.open', commands)

    def test_add_remove_keep_other_component_and_feature(self):
        before = self.sync.live[self.a.key]
        added = replace(self.b, key=('root', 'c'), name='C')
        self.sync.update(PreviewScene((self.scene.nodes[0], self.a, added)))
        self.assertEqual(before, self.sync.live[self.a.key])
        commands = [c['command'] for c in self.commands()]
        self.assertEqual(commands.count('component.delete'), 1)
        self.assertEqual(commands.count('component.create'), 1)
        self.assertEqual(commands.count('timeline.add'), 1)

    def test_rejected_edit_rolls_back_then_next_save_recovers(self):
        normal = self.call.side_effect
        def fail(method, params):
            if method == 'engine.batch': return {'failed': {'error': 'bad solid'}}
            return normal(method, params)
        self.call.side_effect = fail
        scene = PreviewScene((self.scene.nodes[0], replace(self.a, step='bad', digest='bad'), self.b))
        with self.assertRaises(PreviewRejected): self.sync.update(scene)
        self.assertEqual(self.sync.live[self.a.key].node, self.a)
        restores = [c for c in self.commands() if c['command'] == 'edit.restore_checkpoint']
        self.assertEqual(len(restores), 2)
        self.call.side_effect = normal
        self.assertTrue(self.sync.update(scene))

    def test_transport_failure_does_not_resend_or_rollback_unknown_outcome(self):
        normal = self.call.side_effect
        def fail(method, params):
            if method == 'engine.batch': raise TimeoutError('unknown outcome')
            return normal(method, params)
        self.call.side_effect = fail
        with self.assertRaises(TimeoutError):
            self.sync.update(PreviewScene((self.scene.nodes[0], replace(self.a, digest='new'), self.b)))
        self.assertNotIn('edit.restore_checkpoint', [c['command'] for c in self.commands()])

    def test_oversized_part_keeps_last_preview_without_reload(self):
        self.sync.max_line = 4200
        changed = replace(self.a, step='x'*500, digest='large')
        with self.assertRaises(PreviewRejected):
            self.sync.update(PreviewScene((self.scene.nodes[0], changed, self.b)))
        self.call.assert_not_called()
        self.assertEqual(self.sync.live[self.a.key].node, self.a)


if __name__ == '__main__': unittest.main()
