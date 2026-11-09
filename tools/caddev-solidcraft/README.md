# caddev — persistent SolidCraft assembly fork

This copy keeps build123d part generation and STEP exports, and adds a persistent
SolidCraft editing mode. The original framework remains unchanged.

```sh
uv sync --locked
uv run caddev build
uv run caddev assembly --document assembly/gearbox.solidcraft
```

`Project` must register parts only. To migrate an existing static assembly once:

```sh
uv run caddev assembly --document assembly/gearbox.solidcraft \
  --seed migration.gearbox:gearbox --ground housing --seed-only
```

The seed refuses an existing document. The persistent command opens a saved design
or reconnects to the project-owned editing window. It does not terminate that window
on exit. Assembly pose, constraints, motion studies, and saves belong to SolidCraft.
The old `dev --preview` mode is still disposable; do not use it for the persistent
assembly. Its compatibility documentation is in `LEGACY_PREVIEW.md`.

Part export additionally writes `<id>.json` with schema version 1, stable part ID,
mm units, Z-up, row-major 4x4 local datum matrices (column-vector convention), and a
SHA-256 of the STEP DATA section. STEP and metadata are independently atomic files;
the reader verifies their hashes before importing, so an interrupted pair is refused.
STEP names and RGB colors are retained. Engineering material metadata does not
round-trip through STEP.

Bindings are stored under `io.caddev.parts` in document/component metadata, never
only in a Python process. Unique import-feature names and embedded hashes validate
bindings after reconnect, undo, and reopen. Existing shared occurrences receive the
updated definition. Independently copied definitions (Paste New) require rebinding;
invalid bindings are reported and skipped. Registration removal never deletes GUI
components. `--insert PART --name NAME` provides explicit insertion at the origin.
Stop the watcher before insertion to release its project lock.

Refreshes defer while interactive editing is reported by the UI. Source changes
coalesce naturally during builds; rejected imports use SolidCraft's same-request
batch rollback. No checkpoint is restored across GUI frames. A lost transport stops
the watcher, preserving the editor; restart to inspect current bindings and reconnect.
Save changes through SolidCraft. No background autosave is added by caddev.

## Verified compatibility and limits

Tested with SolidCraft 0.20.0. Datum-frame changes are blocked because that version
has no in-place endpoint/origin editing API; named origins are copied into joints.
Supporting live datum changes requires that API capability, not an unverified version
upgrade. Topology changes may invalidate face-attached constraints and are reported.
No whole-document reload is used as a refresh fallback. Batches larger than the
16 MiB control limit are rejected rather than split into partial updates.

## Tests

```sh
uv run python -m unittest discover -s tests -v
CADDEV_TEST_SOLIDCRAFT=/path/to/SolidCraft.AppImage \
  uv run python -m unittest discover -s tests -p test_persistent.py -v
```

The opt-in test creates a revolute joint through the actual GUI dialog, checks a
shared occurrence and motion study, refreshes geometry, tests rejected imports and
changed datums, then verifies undo/redo, reconnect, and save/reopen. It closes only
its isolated test app. Normal watcher shutdown leaves the user's editor open.
