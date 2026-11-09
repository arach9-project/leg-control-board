"""Persistent assembly tests. App test requires CADDEV_TEST_SOLIDCRAFT."""

import json
import os
from pathlib import Path
import tempfile
import time
import unittest
from build123d import Box, Location
from caddev import Assembly, Instance
from caddev.exporter import build_component, write_step
from caddev.part_metadata import write_metadata, read_part, matrix
from caddev.persistent import PersistentApp, Deferred, command


class MetadataTests(unittest.TestCase):
    def test_local_frames_and_mismatched_publication(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            target = build_component(Box(2, 3, 4), "part")
            target.datums = {"axis": Location((1, 2, 3), (0, 90, 0))}
            path = root / "build/parts/p.step"
            write_step(target, path)
            write_metadata(target, "p", path)
            data, _ = read_part(root, "p")
            self.assertEqual(data["datums"]["axis"], matrix(target.datums["axis"]))
            self.assertEqual(data["units"], "mm")
            path.write_text(path.read_text().replace("DATA;", "DATA;\n/*changed*/"))
            with self.assertRaisesRegex(ValueError, "inconsistent"):
                read_part(root, "p")


@unittest.skipUnless(os.environ.get("CADDEV_TEST_SOLIDCRAFT"), "requires SolidCraft desktop")
class PersistentIntegrationTests(unittest.TestCase):
    def test_gui_joint_survives_refresh_undo_save_reopen_and_reconnect(self):
        with tempfile.TemporaryDirectory(prefix="caddev-persistent-") as directory:
            root = Path(directory)
            app = PersistentApp(
                root, root / "fixture.solidcraft", os.environ["CADDEV_TEST_SOLIDCRAFT"]
            )
            process = None
            targets = {}

            def part(id, size, datum=None):
                target = build_component(Box(size, 3, 4), id)
                target.datums = {"axis": datum or Location((1, 2, 3), (0, 90, 0))}
                path = root / "build/parts" / f"{id}.step"
                write_step(target, path)
                write_metadata(target, id, path)
                targets[id] = target

            def click(name):
                until = time.monotonic() + 5
                while time.monotonic() < until:
                    handles = app.call("ui.handles", {})
                    handle = next((h for h in handles if h["name"] == name), None)
                    if handle:
                        app.call("ui.click", {"x": handle["x"], "y": handle["y"]})
                        time.sleep(0.2)
                        return
                    time.sleep(0.1)
                self.fail(f"Missing GUI handle {name}")

            try:
                part("base", 10)
                part("rotor", 2)
                app.start()
                process = app.process
                app.seed(
                    Assembly(
                        "fixture",
                        [
                            Instance("base", "base"),
                            Instance("rotor", "rotor", location=Location((0, 0, 8), (0, 0, 30))),
                        ],
                    ),
                    targets,
                    "base",
                )
                self.assertEqual(app.refresh(set(targets)), (0, []))
                # Named datum orientation/translation is stored in each part's local frame.
                app.execute(
                    "joint.create",
                    name="Datum check",
                    type="revolute",
                    a="base.axis",
                    b="rotor.axis",
                    flip=True,
                )
                datum_joint = app.execute("joint.inspect")["joints"][0]
                for side in ("a", "b"):
                    snap = datum_joint[side]["snap"]
                    for actual, expected in zip(snap["point"], [1, 2, 3]):
                        self.assertAlmostEqual(actual, expected)
                    for actual, expected in zip(snap["z"], [1, 0, 0]):
                        self.assertAlmostEqual(actual, expected)
                app.execute("edit.undo")
                # A joint created through the actual GUI dialog, not joint.create.
                app.execute(
                    "select.set",
                    items=[{"type": "body", "name": "base"}, {"type": "body", "name": "rotor"}],
                )
                app.call("ui.start", {"command": "joint.as_built"})
                with self.assertRaises(Deferred):
                    app.refresh(set(targets))
                click("dialog:Type")
                click("dialog:Type:Revolute")
                click("dialog:ok")
                joints = app.execute("joint.inspect")["joints"]
                self.assertEqual(len(joints), 1)
                self.assertEqual(joints[0]["type"], "revolute")
                jid = joints[0]["id"]
                app.execute("joint.drive", joint=jid, value=27)
                app.execute(
                    "joint.origin",
                    name="GUI origin",
                    occurrence=joints[0]["b"]["occurrence"],
                    point=[0, 0, 8],
                )
                app.execute(
                    "motion.study",
                    name="Spin",
                    steps=10,
                    keys=[{"joint": jid, "points": [[0, 0], [10, 90]]}],
                )
                motions = app.execute("motion.list")
                # A GUI-created shared occurrence gets the same updated geometry.
                app.execute(
                    "occurrence.copy", component=joints[0]["b"]["component"], translate=[20, 0, 0]
                )
                before = app.execute("component.list")
                joints_before = app.execute("joint.inspect")
                camera = app.call("ui.camera", {})
                part("rotor", 4)
                self.assertEqual(app.refresh(set(targets)), (1, []))
                after = app.execute("component.list")
                self.assertEqual(before, after)
                self.assertEqual(joints_before, app.execute("joint.inspect"))
                self.assertEqual(motions, app.execute("motion.list"))
                self.assertEqual(camera, app.call("ui.camera", {}))
                volumes = [
                    b["volume_mm3"]
                    for b in app.call("document.inspect", {"measure": True})["bodies"]
                ]
                self.assertEqual(sorted(volumes), [48, 120])
                rotor_component = next(c for c in after["components"] if c["name"] == "rotor")
                self.assertEqual(len(rotor_component["occurrences"]), 2)
                # A single feature edit carries its digest too: undo/redo is coherent.
                app.execute("edit.undo")
                self.assertEqual(app.execute("joint.inspect"), joints_before)
                app.execute("edit.redo")
                self.assertEqual(app.refresh(set(targets)), (0, []))
                # Invalid import: the entire batch rolls back without cross-frame restores.
                fid = next(
                    f["id"]
                    for f in app.call("document.inspect", {})["timeline"]
                    if f["name"].endswith(":rotor")
                )
                revision = app.call("document.inspect", {})["revision"]
                with self.assertRaisesRegex(ValueError, "previous geometry retained"):
                    app.batch([command("timeline.edit", feature=fid, set={"step": "broken"})])
                self.assertEqual(app.execute("joint.inspect"), joints_before)
                self.assertEqual(app.execute("component.list"), before)
                # Changed datum is blocked, rather than silently changing joint frames.
                part("rotor", 5, Location((2, 2, 3), (0, 90, 0)))
                count, warnings = app.refresh(set(targets))
                self.assertEqual(count, 0)
                self.assertTrue(any("datum frames changed" in x for x in warnings))
                part("rotor", 4)
                self.assertEqual(app.refresh(set(targets)), (0, []))
                # Missing registrations/build failures preserve existing components.
                self.assertEqual(app.refresh({"base"})[0], 0)
                self.assertEqual(app.execute("component.list"), before)
                app.execute("file.save")
                app.close()
                self.assertIsNone(process.poll(), "closing watcher must not terminate the app")
                app = PersistentApp(
                    root, root / "fixture.solidcraft", os.environ["CADDEV_TEST_SOLIDCRAFT"]
                )
                app.start()
                self.assertTrue(app.attached)
                self.assertEqual(app.refresh(set(targets)), (0, []))
                # Close/reopen through GUI document lifecycle, not part refresh.
                app.call("ui.documents", {"action": "close", "force": True})
                app.execute("doc.open", path=str(root / "fixture.solidcraft"))
                self.assertEqual(app.execute("joint.inspect"), joints_before)
                self.assertEqual(app.execute("motion.list"), motions)
                part("rotor", 6)
                self.assertEqual(app.refresh(set(targets)), (1, []))
                self.assertEqual(app.execute("joint.inspect"), joints_before)
            finally:
                app.close()
                # Only the isolated test process is disposed, never a user's document.
                if process and process.poll() is None:
                    process.terminate()
                    process.wait(timeout=10)


if __name__ == "__main__":
    unittest.main()
