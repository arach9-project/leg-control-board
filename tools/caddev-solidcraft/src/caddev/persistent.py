"""Persistent SolidCraft editing: Python owns imported geometry, never assembly pose.

0.20 cannot edit the snap of an existing joint. Datum changes are deliberately
rejected. No document reload, process termination, or cross-frame restore is used
for refreshing an editing session.
"""

from __future__ import annotations
import json
import os
import socket
import secrets
import subprocess
import time
from pathlib import Path
import threading
import uuid

from .part_metadata import atomic_json, read_part, matrix
from .preview import ControlClient, MAX_LINE, find_executable

KEY = "io.caddev.parts"


class Deferred(RuntimeError):
    """The editor is busy or no longer showing the bound document."""


class CompatibilityError(RuntimeError):
    pass


def command(command_id, **params):
    return {"command": command_id, "params": params}


class PersistentApp:
    def __init__(self, root, document, executable=None):
        self.root = Path(root).resolve()
        self.document = Path(document).resolve()
        self.stopped = threading.Event()
        self.client = None
        self.process = None
        self.log = None
        self.executable = find_executable(executable)
        self.runtime = self.root / "build" / ".solidcraft-session.json"
        self.attached = False

    def start(self):
        if self.runtime.exists():
            state = json.loads(self.runtime.read_text())
            if state["document"] != str(self.document):
                raise RuntimeError("Another document owns this project session")
            try:
                self.client = ControlClient(state["port"], state["token"])
                self.call("document.inspect", {})
                self.attached = True
            except OSError:
                self.client = None
                proc = Path(f"/proc/{state['pid']}/stat")
                if proc.exists() and proc.read_text().split()[21] == state["started"]:
                    raise RuntimeError(
                        "SolidCraft is still running but cannot be reached; leave its window intact and reconnect later"
                    )
        if self.client is None:
            # Reuse discovery/launch only, never the disposable worker or cleanup.
            self.launch()
            assert self.client is not None and self.process is not None
            port = self.client.socket.getpeername()[1]
            atomic_json(
                self.runtime,
                {
                    "port": port,
                    "token": self.client.token,
                    "pid": self.process.pid,
                    "started": Path(f"/proc/{self.process.pid}/stat").read_text().split()[21],
                    "document": str(self.document),
                },
            )
            self.runtime.chmod(0o600)
            if self.document.exists():
                self.execute("doc.open", path=str(self.document))
        specs = {c["id"] for c in self.call("engine.commands", {})}
        required = {
            "timeline.edit",
            "timeline.inspect",
            "component.metadata.set",
            "document.metadata.set",
            "joint.inspect",
            "component.list",
            "file.save",
        }
        if missing := required - specs:
            raise CompatibilityError(f"SolidCraft lacks required commands: {sorted(missing)}")

    def launch(self):
        # Launch a detached editing window; watcher exit must not close it.
        with socket.socket() as probe:
            probe.bind(("127.0.0.1", 0))
            port = probe.getsockname()[1]
        token = secrets.token_hex(32)
        control = self.root / "build" / ".solidcraft-control"
        control.mkdir(parents=True, exist_ok=True, mode=0o700)
        self.log = (self.root / "build" / "solidcraft-assembly.log").open("a")
        env = dict(os.environ, SOLIDCRAFT_CONTROL_TOKEN=token, SOLIDCRAFT_CONTROL_DIR=str(control))
        self.process = subprocess.Popen(
            [self.executable, "--control", str(port)],
            cwd=self.root,
            env=env,
            stdout=self.log,
            stderr=self.log,
            start_new_session=True,
        )
        deadline = time.monotonic() + 20
        while not self.stopped.is_set() and time.monotonic() < deadline:
            if self.process.poll() is not None:
                raise RuntimeError("SolidCraft exited; see build/solidcraft-assembly.log")
            try:
                self.client = ControlClient(port, token)
                return
            except OSError:
                self.stopped.wait(0.1)
        raise RuntimeError(
            "SolidCraft control channel did not start; see build/solidcraft-assembly.log"
        )

    def call(self, method, params):
        if self.client is None:
            raise RuntimeError("SolidCraft is not connected")
        return self.client.call(method, params, self.stopped)

    def execute(self, command_id, **params):
        result = self.call("engine.execute", command(command_id, **params))
        if result.get("error"):
            raise RuntimeError(result["error"])
        return result

    def batch(self, commands):
        if not commands:
            return
        params = {"commands": commands, "rollback": True}
        if len(json.dumps(params).encode()) > MAX_LINE - 4096:
            raise RuntimeError(
                "Refresh exceeds the 16 MiB atomic control limit; no changes applied"
            )
        result = self.call("engine.batch", params)
        if result.get("failed"):
            if not result.get("rolled_back"):
                raise RuntimeError(
                    f"Refresh failed without confirmed rollback: {result['failed']}; synchronization stopped"
                )
            raise ValueError(f"Refresh rejected; previous geometry retained: {result['failed']}")
        if any(r.get("error") for r in result.get("results", [])):
            raise RuntimeError(
                "SolidCraft returned a feature error; synchronization stopped for inspection"
            )

    def close(self):
        # User-owned window and unsaved edits survive stopping the Python process.
        if self.client:
            self.client.close()
            self.client = None
        if self.log:
            self.log.close()
            self.log = None

    def ready(self):
        ui = self.call("ui.inspect", {})
        if any(ui.get(k) for k in ("tool", "dialog", "file_busy", "file_picker_busy", "quitting")):
            raise Deferred("Waiting for the active SolidCraft tool/dialog to finish")
        if ui.get("preview", {}).get("active") or ui.get("preview", {}).get("busy"):
            raise Deferred("Waiting for the interactive preview to finish")
        doc = self.call("document.inspect", {})
        if doc.get("path") != str(self.document):
            raise Deferred("Select the bound gearbox document to resume synchronization")
        if doc.get("active_sketch") is not None or doc.get("marker") != len(doc["timeline"]):
            raise Deferred("Finish sketch editing and return the timeline to its end")
        return doc

    def seed(self, assembly, targets, ground):
        if self.document.exists():
            raise FileExistsError(f"Refusing to overwrite {self.document}")
        doc = self.call("document.inspect", {})
        if doc["timeline"] or doc.get("dirty"):
            raise RuntimeError("Seeding requires a new, empty SolidCraft window")
        from .exporter import build_assembly

        built = build_assembly(assembly, targets)
        positions = {n._caddev_instance_id: matrix(n.location) for n in built.shape.children}
        token = str(uuid.uuid4())
        self.execute("document.metadata.set", values={KEY: {"schema": 1, "id": token}})
        for instance in assembly.instances:
            if instance.component not in targets:
                raise ValueError("Seed currently accepts registered parts only")
            self.insert(
                instance.component,
                instance.name or instance.id,
                positions[instance.id],
                token,
                grounded=instance.id == ground,
            )
        self.document.parent.mkdir(parents=True, exist_ok=True)
        self.execute("file.save", path=str(self.document))
        self.call("ui.view", {"view": "fit"})

    def insert(self, part_id, name, placement, token, grounded=False):
        metadata, step = read_part(self.root, part_id)
        created = self.execute("component.create", name=name, parent=0, activate=False)
        cid, oid = created["component"], created["occurrence"]
        self.execute("occurrence.ground", occurrence=oid, grounded=False)
        self.execute("occurrence.move", occurrence=oid, matrix=placement)
        binding = str(uuid.uuid4())
        feature = self.execute(
            "timeline.add",
            feature={
                "type": "import",
                "name": f"caddev:{binding}:{name}",
                "component": cid,
                "file": f"caddev-{binding}-{metadata['geometry_hash']}.step",
                "step": step,
            },
        )["feature"]
        origins = {}
        # Datum frames are explicit part-local matrices; API inputs are world-space.
        for datum, local in metadata["datums"].items():
            world = [
                [sum(placement[r][k] * local[k][c] for k in range(4)) for c in range(4)]
                for r in range(4)
            ]
            origin_name = f"{name}.{datum}"
            self.execute(
                "joint.origin",
                name=origin_name,
                occurrence=oid,
                point=[world[r][3] for r in range(3)],
                z=[world[r][2] for r in range(3)],
                x=[world[r][0] for r in range(3)],
            )
            origins[datum] = origin_name
        self.execute(
            "component.metadata.set",
            component=cid,
            values={
                KEY: {
                    "schema": 1,
                    "document": token,
                    "part_id": part_id,
                    "binding": binding,
                    "feature": feature,
                    "feature_name": f"caddev:{binding}:{name}",
                    "datums": metadata["datums"],
                    "origins": origins,
                }
            },
        )
        if grounded:
            self.execute("occurrence.ground", occurrence=oid, grounded=True)

    def refresh(self, successful_parts):
        """Read current bindings on every update, including after undo/reopen.

        The digest travels in the import feature's file field in the SAME edit as
        the STEP payload, so undo cannot separate a digest from its geometry.
        """
        doc = self.ready()
        owner = doc["metadata"].get(KEY)
        if not owner or owner.get("schema") != 1:
            raise RuntimeError("Document is not a caddev persistent assembly; refusing to alter it")
        components = self.execute("component.list")
        if components.get("pending_moves"):
            raise Deferred("Capture or revert the pending component move first")
        origins = {o["name"] for o in self.execute("joint.list")["origins"]}
        commands, diagnostics = [], []
        for component in components["components"]:
            if component["id"] == 0:
                continue
            metadata = self.execute("component.metadata", component=component["id"])[
                "metadata"
            ].get(KEY)
            if not metadata:
                continue
            if metadata["document"] != owner["id"]:
                raise ValueError("Foreign component binding; synchronization refused")
            part_id = metadata["part_id"]
            if part_id not in successful_parts:
                diagnostics.append(
                    f"{part_id}: unavailable or failed build; existing geometry retained"
                )
                continue
            part, step = read_part(self.root, part_id)
            if part["datums"] != metadata["datums"]:
                diagnostics.append(
                    f"{part_id}: datum frames changed; SolidCraft 0.20 cannot retarget existing joint snaps. Refresh blocked for this part."
                )
                continue
            if missing := set(metadata["origins"].values()) - origins:
                diagnostics.append(
                    f"{part_id}: missing datum origins {sorted(missing)}; refresh blocked"
                )
                continue
            feature = self.execute("timeline.inspect", feature=metadata["feature"])
            prefix = f"caddev-{metadata['binding']}-"
            if (
                feature.get("name") != metadata["feature_name"]
                or feature["definition"].get("component") != component["id"]
                or not feature["definition"].get("file", "").startswith(prefix)
                or feature.get("rolled_back")
                or feature.get("suppressed")
            ):
                diagnostics.append(
                    f"{part_id}: import binding changed or suppressed; refresh blocked"
                )
                continue
            expected_file = prefix + part["geometry_hash"] + ".step"
            if feature["definition"]["file"] != expected_file:
                commands.append(
                    command(
                        "timeline.edit",
                        feature=metadata["feature_name"],
                        set={
                            "step": step,
                            "file": expected_file,
                            "body_names": [],
                        },
                    )
                )
        # Recheck after inspection; never restore a checkpoint across GUI frames.
        if commands:
            current = self.ready()
            if current["revision"] != doc["revision"]:
                raise Deferred("The document changed during inspection; retrying when idle")
            self.batch(commands)
            inspection = self.execute("joint.inspect")
            for joint in inspection.get("joints", []):
                if joint.get("errors") or joint.get("conflicts"):
                    diagnostics.append(
                        f"Joint {joint['name']} needs attention after geometry update: {joint.get('errors') or joint.get('conflicts')}"
                    )
        return len(commands), diagnostics
