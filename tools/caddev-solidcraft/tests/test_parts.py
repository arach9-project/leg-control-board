from pathlib import Path
import tempfile
import unittest

from caddev import CadPart, ComponentGeometry, FeatureGeometry, Assembly, Instance
from caddev.exporter import build_component, build_assembly, write_step
from caddev.preview_scene import preview_scene
from build123d import Align, Box, Location, import_step


class PartsTests(unittest.TestCase):
    def test_dimensions_cache_and_override(self):
        class Block(CadPart):
            calls = 0
            def build(self):
                self.calls += 1
                return Box(2,3,4)
        block = Block()
        self.assertEqual((block.width,block.depth,block.height),(2,3,4))
        block.shape.move(Location((100,0,0)))
        self.assertAlmostEqual(block.bounds.center().X,0)
        self.assertEqual(block.calls,1)
        class Nominal(Block):
            @property
            def height(self): return 5.0
        nominal=Nominal()
        self.assertEqual(nominal.height,5)
        self.assertEqual(nominal.bounds.size.Z,4)

    def test_rotated_multibody_part_and_nested_instance(self):
        raw = Box(2,3,4,align=(Align.MIN,Align.MIN,Align.MIN)).moved(Location((1,2,3)))
        rotation = Location((10,20,30),(180,0,0))
        geometry = ComponentGeometry(features={
            'flipped': FeatureGeometry(raw,name='Flipped body',location=rotation),
            'plain': FeatureGeometry(Box(1,1,1),name='Plain body'),
        })
        part = build_component(geometry,'Axle')
        self.assertFalse(part.shape.children)
        self.assertEqual(len(part.shape.solids()),2)
        expected=raw.moved(rotation).bounding_box()
        actual=part.shape.solids()[0].bounding_box()
        self.assertLess((expected.min-actual.min).length,1e-7)
        self.assertLess((expected.max-actual.max).length,1e-7)
        self.assertAlmostEqual(raw.bounding_box().min.Z,3)
        assembly=Assembly('gearbox',[Instance('axle','p',location=Location((50,0,0),(0,90,0)))])
        target=build_assembly(assembly,{'p':part})
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'rotated.step';write_step(target,path)
            read=import_step(path)
            self.assertEqual(len(read.children),1)
            self.assertFalse(read.children[0].children)
            self.assertEqual(len(read.solids()),2)
            self.assertLess((read.bounding_box().min-target.shape.bounding_box().min).length,1e-6)
            self.assertLess((read.bounding_box().max-target.shape.bounding_box().max).length,1e-6)
            text=path.read_text()
            self.assertEqual(text.count(' = PRODUCT('),2)
            self.assertIn("MANIFOLD_SOLID_BREP('Flipped body'",text)

    def test_body_names_invalidate_preview(self):
        a=build_component(ComponentGeometry.from_shape(Box(1,2,3),name='A'),'part')
        b=build_component(ComponentGeometry.from_shape(Box(1,2,3),name='B'),'part')
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = root/'build/parts/part.step'
            write_step(a, path)
            before = preview_scene(a.shape, root, 'part')
            write_step(b, path)
            after = preview_scene(b.shape, root, 'part')
            self.assertNotEqual(before.nodes[0].digest, after.nodes[0].digest)

    def test_class_can_contain_another_class_as_a_body(self):
        class Plate(CadPart):
            def build(self):return Box(2,3,4)
        part=build_component(ComponentGeometry(features={'plate':FeatureGeometry(Plate())}),'part')
        self.assertAlmostEqual(part.shape.volume,24)


if __name__=='__main__':unittest.main()
