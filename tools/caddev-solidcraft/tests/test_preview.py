import json
from pathlib import Path
import tempfile
import threading
import unittest
from unittest.mock import Mock, patch
from queue import Queue
from build123d import Box, Compound, Location

from caddev import Assembly, Component, Instance, Project
from caddev.builder import BuildReport
from caddev.preview import ControlClient, SolidCraftPreview, preview_path, find_executable, ControlError
from caddev.commands.dev import dev
from caddev.preview_scene import PreviewNode, PreviewScene
from caddev.preview_sync import PreviewRejected


class PreviewTests(unittest.TestCase):
    def test_target_selection(self):
        project = Project([Component('part','unused')], [Assembly('pair',[Instance('one','part')])])
        self.assertEqual(preview_path(project,Path('/project'),'auto'),Path('/project/build/assemblies/pair.step'))
        self.assertEqual(preview_path(project,Path('/project'),'part'),Path('/project/build/parts/part.step'))
        with self.assertRaises(ValueError): preview_path(project,Path('/project'),'missing')

    def test_authenticated_protocol_and_pending_reply(self):
        fake = Mock()
        fake.makefile.return_value.readline.side_effect = [
            json.dumps({'id':1,'ok':False,'running':True}).encode()+b'\n',
            json.dumps({'id':2,'ok':True,'result':{'pending':1,'done':True,
                'reply':{'id':1,'ok':True,'result':{'updated':True}}}}).encode()+b'\n',
        ]
        with patch('caddev.preview.socket.create_connection',return_value=fake):
            client=ControlClient(1234,'private')
            result=client.call('engine.execute',{'command':'doc.open'},threading.Event())
            self.assertEqual(result,{'updated':True})
            requests=[json.loads(c.args[0]) for c in fake.sendall.call_args_list]
            self.assertEqual([r['method'] for r in requests],['engine.execute','control.pending'])
            self.assertTrue(all(r['token']=='private' for r in requests))
            client.close()

    def viewer(self):
        viewer = SolidCraftPreview.__new__(SolidCraftPreview)
        viewer.root = Path('/project')
        viewer.stopped = threading.Event()
        viewer.opened = True
        viewer.path = Path('/project/build/parts/part.step')
        viewer.client = Mock()
        viewer.scene_sync = Mock()
        return viewer

    def test_refresh_never_opens_document_or_resets_camera(self):
        viewer = self.viewer()
        scene = PreviewScene((PreviewNode(('part',), 'Part'),))
        viewer._refresh(viewer.path, scene)
        viewer.scene_sync.update.assert_called_once_with(scene)
        viewer.client.call.assert_not_called()
        viewer.opened = False
        viewer._refresh(viewer.path, scene)
        viewer.client.call.assert_called_once_with('ui.view', {'view':'fit'}, viewer.stopped)

    def test_rejection_keeps_session_and_next_save_retries(self):
        viewer = self.viewer()
        scene = PreviewScene(())
        viewer.scene_sync.update.side_effect = [PreviewRejected('invalid part'), True]
        viewer._refresh(viewer.path, scene)
        viewer._refresh(viewer.path, scene)
        self.assertEqual(viewer.scene_sync.update.call_count, 2)
        viewer.client.call.assert_not_called()
        viewer.scene_sync.update.side_effect = TimeoutError('unknown outcome')
        with self.assertRaises(TimeoutError): viewer._refresh(viewer.path, scene)

    def test_snapshot_and_latest_pending_update(self):
        viewer = self.viewer()
        viewer.queue = Queue()
        path = Path('/project/build/part.step')
        old = PreviewScene((PreviewNode(('part',), 'Old', step='old STEP', digest='old'),))
        latest = PreviewScene((PreviewNode(('part',), 'New', step='new STEP', digest='new'),))
        viewer.submit(path, old)
        viewer.submit(path, latest)
        self.assertEqual(viewer.queue.qsize(), 1)
        self.assertEqual(viewer.queue.get(), (path, latest))

    def test_large_request_is_rejected_before_send(self):
        fake = Mock()
        with patch('caddev.preview.socket.create_connection', return_value=fake), \
             patch('caddev.preview.MAX_LINE', 100):
            client = ControlClient(1234, 'private')
            with self.assertRaises(ValueError):
                client._request('engine.execute', {'step': 'x' * 101})
            fake.sendall.assert_not_called()
            client.close()

    def test_only_successful_selected_exports_refresh(self):
        root=Path('/project')
        session=Mock()
        session.project=Project([Component('part','unused'),Component('other','unused')])
        session.cache = {'part': Mock(shape=Box(1,2,3))}
        session.rebuild.side_effect=[BuildReport(['part']),BuildReport(['other']),BuildReport([],{'part':'failed'})]
        def watch(root,callback):
            callback({root/'other.py'}); callback({root/'part.py'})
        with patch('caddev.commands.dev.BuildSession',return_value=session), \
             patch('caddev.commands.dev.SolidCraftPreview') as preview, \
             patch('caddev.commands.dev.preview_scene', return_value=PreviewScene(())), \
             patch('caddev.commands.dev.watch_project',side_effect=watch):
            dev(root,preview='part')
            preview.return_value.submit.assert_called_once_with(root/'build/parts/part.step', PreviewScene(()))
            preview.return_value.close.assert_called_once()

    def test_discovery_failure_is_actionable(self):
        with tempfile.TemporaryDirectory() as directory:
            with patch.dict('os.environ',{'XDG_DATA_HOME':directory,'SOLIDCRAFT_EXECUTABLE':''}), \
                 patch('caddev.preview.shutil.which',return_value=None):
                with self.assertRaisesRegex(RuntimeError,'SOLIDCRAFT_EXECUTABLE'):
                    find_executable()


if __name__=='__main__': unittest.main()
