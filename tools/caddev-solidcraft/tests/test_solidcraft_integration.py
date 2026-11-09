"""Opt-in app tests: CADDEV_TEST_SOLIDCRAFT=/path/to/AppImage python -m unittest ..."""
from dataclasses import replace
import os
from pathlib import Path
import tempfile
import unittest

from build123d import Box, Location, import_step
from caddev import CadPart, ComponentGeometry, FeatureGeometry, Assembly, Instance
from caddev.exporter import build_component, build_assembly, write_step
from caddev.preview import SolidCraftPreview
from caddev.preview_scene import PreviewScene, preview_scene


@unittest.skipUnless(os.environ.get('CADDEV_TEST_SOLIDCRAFT'), 'requires an explicit SolidCraft executable')
class SolidCraftTests(unittest.TestCase):
    def test_persistent_parts_bodies_nested_rotations_and_failure_recovery(self):
        with tempfile.TemporaryDirectory(prefix='caddev-live-test-') as directory:
            root = Path(directory)
            viewer = SolidCraftPreview(root, os.environ['CADDEV_TEST_SOLIDCRAFT'])
            path = root/'build/assemblies/assembly.step'
            calls = []

            def scene(size=2, extra=False, move=10):
                class Plate(CadPart):
                    def build(self):
                        bodies = {'plate': FeatureGeometry(Box(size, 3, 4))}
                        if extra:
                            bodies['lid'] = FeatureGeometry(Box(1, 1, 1), location=Location((0, 0, 10)))
                        return ComponentGeometry(features=bodies)
                part = build_component(Plate(), 'Plate')
                other = build_component(Box(1, 2, 3), 'Other')
                write_step(part, root/'build/parts/p.step')
                write_step(other, root/'build/parts/q.step')
                sub = build_assembly(Assembly('sub', [
                    Instance('first', 'p', location=Location((0, 0, 5), (180, 0, 0))),
                    Instance('second', 'p', location=Location((0, 20, 0))),
                ]), {'p': part})
                result = build_assembly(Assembly('assembly', [
                    Instance('sub', 's', location=Location((move, 0, 0), (0, 0, 90))),
                    Instance('unchanged', 'q'),
                ]), {'s': sub, 'q': other})
                return preview_scene(result.shape, root, 'assembly'), result

            def command(name, params):
                return viewer.client.call('engine.execute', {'command': name, 'params': params}, viewer.stopped)

            def mutations():
                return [c for method, params in calls for c in
                        (params['commands'] if method == 'engine.batch' else
                         [params] if method == 'engine.execute' else [])]

            try:
                initial, _ = scene()
                viewer._start()
                actual_call = viewer.client.call
                def traced(method, params, stopped):
                    calls.append((method, params))
                    return actual_call(method, params, stopped)
                viewer.client.call = traced
                viewer._refresh(path, initial)
                ids = {key: (v.component, v.feature) for key, v in viewer.scene_sync.live.items()}
                camera = viewer.client.call('ui.camera', {}, viewer.stopped)
                calls.clear()
                changed, target = scene(4, True)
                viewer._refresh(path, changed)
                self.assertEqual([c['command'] for c in mutations()].count('timeline.edit'), 2)
                self.assertEqual(ids, {key: (v.component, v.feature) for key, v in viewer.scene_sync.live.items()})
                doc = viewer.client.call('document.inspect', {'measure': True}, viewer.stopped)
                self.assertEqual(len(doc['bodies']), 5)
                self.assertEqual(sorted(b['volume_mm3'] for b in doc['bodies']), [1, 1, 6, 48, 48])
                self.assertEqual(camera, viewer.client.call('ui.camera', {}, viewer.stopped))
                self.assertFalse(any(c['command'] in ('doc.open', 'file.insert_step') for c in mutations()))
                calls.clear()
                identical, _ = scene(4, True)
                viewer._refresh(path, identical)
                self.assertEqual(calls, [])
                moved, target = scene(4, True, 35)
                viewer._refresh(path, moved)
                self.assertEqual([c['command'] for c in mutations()].count('occurrence.move'), 1)
                self.assertNotIn('timeline.edit', [c['command'] for c in mutations()])
                # Check world geometry exported by the app, including both levels
                # of rotation and feature-local body placement.
                output = root/'build/from-solidcraft.step'
                command('file.export', {'path': str(output), 'format': 'step'})
                read = import_step(output)
                expected, actual = target.shape.bounding_box(), read.bounding_box()
                self.assertLess((expected.min - actual.min).length, 1e-5)
                self.assertLess((expected.max - actual.max).length, 1e-5)
                self.assertEqual(len(read.solids()), len(target.shape.solids()))
                bad = list(moved.nodes)
                bad[2] = replace(bad[2], step='invalid STEP', digest='bad')
                viewer._refresh(path, PreviewScene(tuple(bad)))
                self.assertEqual(viewer.scene_sync.live[bad[2].key].node, moved.nodes[2])
                recovered, _ = scene(5, True, 35)
                viewer._refresh(path, recovered)
                self.assertEqual(ids, {key: (v.component, v.feature) for key, v in viewer.scene_sync.live.items()})
                self.assertEqual(camera, viewer.client.call('ui.camera', {}, viewer.stopped))
            finally:
                viewer.close()


if __name__ == '__main__': unittest.main()
