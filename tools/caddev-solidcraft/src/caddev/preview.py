"""A dedicated, disposable SolidCraft window controlled over its local API."""
from __future__ import annotations

import configparser
import json
import os
from pathlib import Path
import secrets
import shlex
import shutil
import socket
import subprocess
import threading
import time
from queue import Empty, Queue

from .log import info, warning
from .preview_scene import PreviewScene
from .preview_sync import SceneSync, PreviewRejected


MAX_LINE = 16 * 1024 * 1024


class ControlError(RuntimeError):
    """A request definitively rejected by the app (not a transport timeout)."""


def find_executable(explicit: str | None = None) -> str:
    candidate = explicit or os.environ.get('SOLIDCRAFT_EXECUTABLE') or shutil.which('solidcraft')
    if candidate:
        resolved = shutil.which(candidate) or str(Path(candidate).expanduser())
        if Path(resolved).is_file() and os.access(resolved, os.X_OK):
            return resolved
        raise RuntimeError(f'SolidCraft executable not found: {candidate}')
    # AppImages installed by desktop integration need not be on PATH.
    applications = Path(os.environ.get('XDG_DATA_HOME', Path.home() / '.local/share')) / 'applications'
    for path in sorted(applications.glob('*solidcraft*.desktop')):
        parser = configparser.ConfigParser(interpolation=None)
        try:
            parser.read(path)
            command = shlex.split(parser['Desktop Entry']['Exec'])[0]
            if Path(command).is_file() and os.access(command, os.X_OK):
                return command
        except (KeyError, IndexError, ValueError, configparser.Error):
            continue
    raise RuntimeError('SolidCraft not found. Set SOLIDCRAFT_EXECUTABLE to the app or AppImage path.')


class ControlClient:
    def __init__(self, port: int, token: str):
        self.socket = socket.create_connection(('127.0.0.1', port), timeout=1)
        self.socket.settimeout(65)
        self.stream = self.socket.makefile('rb')
        self.token = token
        self.sequence = 0

    def close(self):
        try:
            self.socket.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass
        self.stream.close()
        self.socket.close()

    def _request(self, method, params):
        self.sequence += 1
        request = dict(id=self.sequence, method=method, params=params, token=self.token)
        encoded = (json.dumps(request) + '\n').encode()
        if len(encoded) > MAX_LINE:
            raise ValueError('SolidCraft control request exceeds the 16 MiB limit')
        self.socket.sendall(encoded)
        line = self.stream.readline(MAX_LINE + 1)
        if not line or len(line) > MAX_LINE:
            raise RuntimeError('SolidCraft disconnected or returned an oversized response')
        reply = json.loads(line)
        if reply.get('id') != self.sequence:
            raise RuntimeError('Unexpected SolidCraft response ID')
        return reply

    def call(self, method, params, stopped):
        reply = self._request(method, params)
        # A timeout marked running is still executing. Never resend the mutation.
        while reply.get('running'):
            if stopped.wait(0.25):
                raise RuntimeError('Preview stopped')
            pending = self._request('control.pending', {})
            if not pending.get('ok'):
                raise RuntimeError(pending.get('error', 'SolidCraft pending request failed'))
            state = pending.get('result', {})
            if state.get('done'):
                reply = state['reply']
            elif state.get('pending') is None:
                raise RuntimeError('SolidCraft lost the outstanding request; preview not retried')
        if not reply.get('ok'):
            raise ControlError(reply.get('error', 'SolidCraft request failed'))
        return reply.get('result', {})


class SolidCraftPreview:
    def __init__(self, root: Path, executable: str | None = None):
        self.root = root.resolve()
        self.executable = find_executable(executable)
        self.queue = Queue()
        self.stopped = threading.Event()
        self.process = None
        self.client = None
        self.log = None
        self.opened = False
        self.path = None
        self.scene_sync = None
        self.thread = threading.Thread(target=self._work, name='solidcraft-preview', daemon=True)
        self.thread.start()

    def submit(self, path: Path, scene: PreviewScene):
        # Payloads and placements were captured together before the next build.
        update = (path.resolve(), scene)
        while True:
            try:
                self.queue.get_nowait()
            except Empty:
                break
        self.queue.put(update)

    def _start(self):
        # New app and token: never attach to or overwrite the user's editing session.
        with socket.socket() as probe:
            probe.bind(('127.0.0.1', 0))
            port = probe.getsockname()[1]
        token = secrets.token_hex(32)
        control = self.root / 'build' / '.solidcraft-control'
        control.mkdir(parents=True, exist_ok=True, mode=0o700)
        self.log = (self.root / 'build' / 'solidcraft-preview.log').open('a')
        env = dict(os.environ, SOLIDCRAFT_CONTROL_TOKEN=token, SOLIDCRAFT_CONTROL_DIR=str(control))
        self.process = subprocess.Popen([self.executable, '--control', str(port)],
                                        cwd=self.root, env=env, stdout=self.log, stderr=self.log)
        deadline = time.monotonic() + 20
        while not self.stopped.is_set() and time.monotonic() < deadline:
            if self.process.poll() is not None:
                raise RuntimeError('SolidCraft exited; see build/solidcraft-preview.log')
            try:
                self.client = ControlClient(port, token)
                return
            except OSError:
                self.stopped.wait(0.1)
        raise RuntimeError('SolidCraft control channel did not start; see build/solidcraft-preview.log')

    def _refresh(self, path, scene):
        if self.client is None:
            self._start()
        if self.scene_sync is None:
            self.scene_sync = SceneSync(
                lambda method, params: self.client.call(method, params, self.stopped), MAX_LINE,
                rejected_errors=(ControlError, PreviewRejected, ValueError))
        try:
            changed = self.scene_sync.update(scene)
        except (ControlError, PreviewRejected, ValueError) as exc:
            warning(f'SolidCraft update rejected; last preview retained: {exc}')
            return
        if not self.opened:
            self.client.call('ui.view', {'view': 'fit'}, self.stopped)
        self.opened = True
        self.path = path
        if changed:
            info(f'SolidCraft preview updated in place: {path.name}')

    def _work(self):
        while not self.stopped.is_set():
            try:
                update = self.queue.get(timeout=0.2)
            except Empty:
                continue
            # Coalesce saves while the viewer is applying a previous build.
            while True:
                try:
                    update = self.queue.get_nowait()
                except Empty:
                    break
            try:
                self._refresh(*update)
            except Exception as exc:
                if not self.stopped.is_set():
                    warning(f'SolidCraft preview: {exc}. STEP exports remain available.')
                # Do not resend commands with an uncertain outcome into this session.
                self._dispose()
                self.opened = False
                self.path = None
                self.scene_sync = None

    def _dispose(self):
        if self.client:
            self.client.close()
            self.client = None
        if self.process:
            if self.process.poll() is None:
                self.process.terminate()
                try:
                    self.process.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    self.process.kill()
                    self.process.wait()
            self.process = None
        if self.log:
            self.log.close()
            self.log = None

    def close(self):
        self.stopped.set()
        # Wake a blocked control read by terminating only our disposable process.
        if self.process and self.process.poll() is None:
            self.process.terminate()
        self.thread.join(timeout=5)
        if not self.thread.is_alive():
            self._dispose()


def preview_path(project, root: Path, target: str) -> Path:
    if target == 'auto':
        targets = project.assemblies or project.components
        if not targets:
            raise ValueError('No targets available to preview')
        target = targets[0].id
    for folder, targets in [('parts', project.components), ('assemblies', project.assemblies)]:
        if any(t.id == target for t in targets):
            return root / 'build' / folder / f'{target}.step'
    raise ValueError(f'Unknown preview target: {target}')
