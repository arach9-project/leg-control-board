import contextlib
import io
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import signal
import unittest
from unittest.mock import patch

from caddev import (Assembly, Component, ComponentGeometry, FeatureGeometry,
                    Appearance, Instance, Mate, Project)
from caddev.builder import BuildSession
from caddev.exporter import build_component, build_assembly, write_step
from caddev.commands.init import init_project
from caddev.commands.add import add_part
from caddev.commands.dev import dev
from caddev.loader import unload_project_modules
from caddev.watcher import source_filter
from build123d import Box, Location, import_step


class GeometryTests(unittest.TestCase):
    def test_feature_step_roundtrip(self):
        raw = Box(2, 3, 4)
        geometry = ComponentGeometry(features={
            'body': FeatureGeometry(raw, name='Red body', location=Location((10, 0, 0)),
                                    appearance=Appearance(color=(1, 0, 0))),
            'other': FeatureGeometry(Box(1, 1, 1), name='Other', location=Location((-10, 0, 0)))})
        target = build_component(geometry, 'Example')
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / 'part.step'
            write_step(target, path)
            result = import_step(path)
            self.assertEqual(len(result.solids()), 2)
            self.assertAlmostEqual(result.volume, 25)
            self.assertAlmostEqual(result.bounding_box().min.X, -10.5)
            self.assertAlmostEqual(result.bounding_box().max.X, 11)
            self.assertEqual(result.label, 'Example')
            self.assertIn("'Red body'", path.read_text())
            self.assertFalse(result.children)  # Bodies are not child parts.
            self.assertIn("MANIFOLD_SOLID_BREP('Red body'", path.read_text())
            self.assertIn("DRAUGHTING_PRE_DEFINED_COLOUR('red')", path.read_text())
        self.assertAlmostEqual(raw.center().X, 0)

    def test_nested_repeated_and_mates(self):
        datum = Location((2, 1, 0), (0, 0, 30))
        target = build_component(ComponentGeometry.from_shape(Box(1,2,3), datums={'d': datum}), 'Part')
        anchor = Location((10, 20, 30), (0, 90, 0))
        offset = Location((0,0,5), (180,0,0))
        a = Assembly('a', [Instance('fixed', 'p', location=anchor),
                           Instance('mated', 'p', mate=Mate('d', 'fixed', 'd', offset))],
                     datums={'origin': Location()})
        Project([Component('p','unused')], [a]).validate()
        built = build_assembly(a, {'p': target})
        expected = anchor * datum * offset * datum.inverse()
        actual = built.shape.children[1].location
        delta = expected.inverse() * actual
        self.assertLess(delta.position.length, 1e-7)
        self.assertLess(delta.orientation.length, 1e-7)
        outer = Assembly('outer', [Instance('one','a'), Instance('two','a',location=Location((100,0,0)))])
        nested = build_assembly(outer, {'a': built})
        with tempfile.TemporaryDirectory() as d:
            path = Path(d)/'nested.step'; write_step(nested,path)
            read = import_step(path)
            self.assertEqual(len(read.solids()), 4)
            self.assertEqual([c.label for c in read.children], ['one','two'])
            self.assertEqual(len(read.children[0].children), 2)
            self.assertAlmostEqual(sum(s.volume for s in read.solids()), 24)
            original_box, imported_box = nested.shape.bounding_box(), read.bounding_box()
            self.assertLess((original_box.min - imported_box.min).length, 1e-6)
            self.assertLess((original_box.max - imported_box.max).length, 1e-6)
        self.assertLess(target.shape.location.position.length, 1e-7)

    def test_invalid_graphs_and_datums(self):
        part = Component('p','unused')
        cases = [Project([part,part]), Project([Component('../bad','unused')]),
                 Project([part],[Assembly('a',[Instance('i','missing')])]),
                 Project([part],[Assembly('a',[Instance('i','p'),Instance('i','p')])]),
                 Project([part],[Assembly('a',[Instance('i','p', Location(), Mate('d','i','d'))])]),
                 Project([part],[Assembly('a',[Instance('i','p',mate=Mate('d','i','d'))])]),
                 Project([part],[Assembly('a',[Instance('i','b')]),Assembly('b',[Instance('i','a')])])]
        for project in cases:
            with self.subTest(project=project), self.assertRaises(ValueError): project.validate()
        with self.assertRaisesRegex(ValueError,'missing datum'):
            build_assembly(Assembly('a',[Instance('i','p'),Instance('j','p',mate=Mate('x','i','x'))]),
                           {'p':build_component(Box(1,1,1),'p')})

    def test_atomic_failure(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d)/'a.step'; path.write_text('previous')
            with patch('caddev.exporter.export_step', return_value=False), self.assertRaises(RuntimeError):
                write_step(build_component(Box(1,1,1),'p'),path)
            self.assertEqual(path.read_text(),'previous')
            self.assertEqual(list(Path(d).iterdir()),[path])


class SessionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.put('fixtureparts/__init__.py','')
        self.put('helper.py','SIZE = 1\n')
        self.put('fixtureparts/a.py','from build123d import Box\nfrom helper import SIZE\ndef build(): return Box(SIZE, 1, 1)\n')
        self.put('fixtureparts/b.py','from build123d import Box\ndef build(): return Box(2, 1, 1)\n')
        self.put('layout.py',"from caddev import Assembly, Instance\na = Assembly('assembly',[Instance('first','a')])\n")
        self.project = "from caddev import Project, Component\nfrom layout import a\nproject = Project([Component('a','fixtureparts.a'), Component('b','fixtureparts.b')], [a])\n"
        self.put('project.py',self.project)
        self.session = BuildSession(self.root)

    def tearDown(self):
        unload_project_modules(self.root)
        if str(self.root) in sys.path: sys.path.remove(str(self.root))
        self.temp.cleanup()

    def put(self,name,text):
        path = self.root/name; path.parent.mkdir(parents=True,exist_ok=True); path.write_text(text)
        return path

    def rebuild(self,changed=None):
        report = self.session.rebuild(changed)
        self.assertFalse(report.errors, report.errors)
        return set(report.built)

    def test_selective_shared_and_rapid_saves(self):
        self.assertEqual(self.rebuild(),{'a','b','assembly'})
        helper = self.root/'helper.py'; original_stat=helper.stat()
        helper.write_text('SIZE = 3\n')
        os.utime(helper, ns=(original_stat.st_atime_ns, original_stat.st_mtime_ns))
        self.assertEqual(self.rebuild({helper}),{'a','assembly'})
        self.assertAlmostEqual(self.session.cache['a'].shape.volume,3)
        helper.write_text('SIZE = 4\n')
        os.utime(helper, ns=(original_stat.st_atime_ns, original_stat.st_mtime_ns))
        self.assertEqual(self.rebuild({helper}),{'a','assembly'})
        self.assertAlmostEqual(self.session.cache['a'].shape.volume,4)
        self.assertEqual(self.rebuild({self.root/'fixtureparts/b.py'}),{'b'})

    def test_assembly_change_and_project_change(self):
        self.rebuild()
        p=self.put('layout.py',"from caddev import Assembly, Instance\na = Assembly('assembly',[Instance('second','b')])\n")
        self.assertEqual(self.rebuild({p}),{'assembly'})
        self.assertEqual(self.rebuild({self.root/'project.py'}),{'a','b','assembly'})

    def test_failure_recovery_and_deletion(self):
        self.rebuild()
        output=self.root/'build/assemblies/assembly.step'; before=output.read_bytes()
        p=self.put('fixtureparts/a.py','def build(): raise RuntimeError("broken")\n')
        report=self.session.rebuild({p})
        self.assertEqual(set(report.errors),{'a','assembly'})
        self.assertEqual(output.read_bytes(),before)
        self.assertNotIn('a',self.session.cache)
        self.put('fixtureparts/a.py','from build123d import Box\ndef build(): return Box(4,1,1)\n')
        self.assertEqual(self.rebuild({p}),{'a','assembly'})
        p.unlink()
        report=self.session.rebuild({p})
        self.assertIn('a',report.errors)
        self.put('fixtureparts/a.py','from build123d import Box\ndef build(): return Box(4,1,1)\n')
        self.assertEqual(self.rebuild({p}),{'a','b','assembly'})

    def test_invalid_initial_project_and_dynamic_import(self):
        p=self.put('project.py','invalid syntax!')
        self.assertIn('project',self.session.rebuild().errors)
        self.put('project.py',self.project)
        self.assertEqual(self.rebuild({p}),{'a','b','assembly'})
        p=self.put('fixtureparts/b.py','from importlib import import_module\nfrom build123d import Box\ndef build(): return Box(import_module("helper").SIZE,1,1)\n')
        self.assertEqual(self.rebuild({p}),{'a','b','assembly'})
        self.assertEqual(self.rebuild({self.root/'helper.py'}),{'a','b','assembly'})

    def test_watcher_filters_and_shutdown(self):
        self.assertTrue(source_filter(self.root,str(self.root/'fixtureparts/a.py')))
        for path in ['build/a.py','.venv/x.py','.git/a.py','a.step']:
            self.assertFalse(source_filter(self.root,str(self.root/path)))
        with patch('caddev.commands.dev.watch_project',side_effect=KeyboardInterrupt), contextlib.redirect_stdout(io.StringIO()):
            dev(self.root)

    def test_cli_exit_codes_and_status(self):
        env=dict(os.environ, PYTHONPATH=str(Path(__file__).resolve().parents[1]/'src'))
        result=subprocess.run([sys.executable,'-m','caddev.main','build'],cwd=self.root,env=env,capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        result=subprocess.run([sys.executable,'-m','caddev.main','status'],cwd=self.root,env=env,capture_output=True,text=True)
        self.assertEqual(result.returncode,0); self.assertIn('present',result.stdout)
        self.put('project.py','broken!')
        result=subprocess.run([sys.executable,'-m','caddev.main','build'],cwd=self.root,env=env,capture_output=True,text=True)
        self.assertEqual(result.returncode,1)

    def test_live_watcher_recovers_from_initial_error(self):
        self.put('project.py', 'not valid python!')
        env = dict(os.environ, PYTHONPATH=str(Path(__file__).resolve().parents[1] / 'src'),
                   PYTHONUNBUFFERED='1', PYTHONDONTWRITEBYTECODE='1')
        log_path = self.root / 'watch.log'
        with log_path.open('w') as log:
            process = subprocess.Popen([sys.executable, '-m', 'caddev.main', 'dev'],
                                       cwd=self.root, env=env, stdout=log, stderr=log)
            def wait_for(predicate):
                deadline = time.monotonic() + 15
                while time.monotonic() < deadline:
                    if predicate():
                        return
                    if process.poll() is not None:
                        self.fail(log_path.read_text())
                    time.sleep(0.05)
                self.fail('Watcher timeout: ' + log_path.read_text())
            try:
                wait_for(lambda: 'Watching Python sources' in log_path.read_text())
                # Let watchfiles finish establishing its OS watch before the edit.
                time.sleep(0.2)
                self.put('project.py', self.project)
                output = self.root / 'build/parts/a.step'
                wait_for(output.exists)
                previous = output.stat().st_mtime_ns
                self.put('helper.py', 'SIZE = 8\n')
                wait_for(lambda: output.stat().st_mtime_ns != previous)
                self.assertAlmostEqual(import_step(output).volume, 8)
                process.send_signal(signal.SIGINT)
                self.assertEqual(process.wait(timeout=5), 0)
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()

    def test_create_cli_buildable_template(self):
        import tomllib
        env = dict(os.environ, PYTHONPATH=str(Path(__file__).resolve().parents[1] / 'src'),
                   PYTHONDONTWRITEBYTECODE='1')
        for command in ['create', 'init']:
            root = self.root / command
            result = subprocess.run([sys.executable, '-m', 'caddev.main', command, str(root)],
                                    cwd=self.root, env=env, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            config = tomllib.loads((root / 'pyproject.toml').read_text())
            self.assertEqual(config['tool']['uv']['sources']['caddev']['path'],
                             str(Path(__file__).resolve().parents[1]))
            result = subprocess.run([sys.executable, '-m', 'caddev.main', 'build'],
                                    cwd=root, env=env, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertTrue((root / 'build/parts/example.step').exists())
            self.assertTrue((root / 'build/assemblies/pair.step').exists())
            original = (root / 'project.py').read_bytes()
            result = subprocess.run([sys.executable, '-m', 'caddev.main', command, str(root)],
                                    cwd=self.root, env=env, capture_output=True, text=True)
            self.assertEqual(result.returncode, 1)
            self.assertEqual((root / 'project.py').read_bytes(), original)

    def test_scaffold_and_add(self):
        root=self.root/'new'
        with contextlib.redirect_stdout(io.StringIO()):
            init_project(root,project_name='new')
            add_part(root,name='Second Part')
        self.assertTrue((root/'parts/second_part.py').exists())
        source=(root/'project.py').read_text()
        self.assertIn('assemblies=[pair]',source)
        self.assertIn('uv run caddev dev', (root/'justfile').read_text())
        self.assertTrue((root/'assemblies/pair.py').exists())
        compile(source,'project.py','exec')
        with self.assertRaises(RuntimeError): add_part(root,name='Second Part')
        with self.assertRaises(ValueError): add_part(root,name='123')


if __name__ == '__main__': unittest.main()
