# caddev

Build parts in build123d, define assemblies in Python, and preview them live in
SolidCraft. STEP exports also work independently with other CAD viewers.

## Install and run

Requires Python 3.11+, build123d, and watchfiles. From your CAD project:

```sh
uv add --editable /path/to/caddev
uv run caddev build       # one-shot export; nonzero exit on failure
uv run caddev dev         # initial build, then watch Python files
uv run caddev status      # list targets and last successful files
```

`caddev` with no arguments starts the export-only watcher. To automatically open
and refresh a dedicated SolidCraft preview, use `uv run caddev dev --preview`.
Exports remain available in `build/parts/<id>.step` and
`build/assemblies/<id>.step` regardless of preview mode.

Create a project from an environment where caddev is available:

```sh
uv run caddev create my-project
cd my-project
just run
```

The template includes `project.py`, `parts/`, `assemblies/`, `geometry/`, `utils/`,
an example part and pair assembly, a README, and a `justfile`. `just run` invokes
`uv run caddev dev --preview auto`, installing dependencies and starting the watcher
and a dedicated SolidCraft preview. Requires
[just](https://github.com/casey/just) and uv on your PATH. When running caddev from
this checkout, the generated project automatically uses it as an editable uv
source. Update that path when sharing the project or moving the checkout.
`caddev init` remains an alias for `create`. Existing nonempty directories are
never overwritten. `caddev add part bracket` creates a factory stub and registers
it in the literal `Project(components=[...])` list.

## Live SolidCraft preview

```sh
just run                 # first assembly, or first part if there are no assemblies
just run housing         # a specific registered part or assembly ID
uv run caddev dev        # export-only mode, no viewer
```

`just run` launches its own disposable SolidCraft window, authenticated through
SolidCraft's localhost control API. After a successful build of the selected
target, caddev maintains persistent components and an import feature per physical
part instance. Only parts whose STEP geometry or appearance changed receive a
`timeline.edit`. Placement changes move their existing occurrences directly.
Body additions, removals, names and colors update within the part; assembly additions
and removals change only the affected branches. Stable Python instance IDs keep the
other components, camera and browser state in place. Saves that produce identical
geometry and placements send no update, even if the STEP timestamp changed.

The preview never reopens the document to apply an edit. Ordinary edits are batched
into one UI update; large assemblies can be sent in smaller batches. If SolidCraft
rejects an edit, caddev restores the previous preview and retries on the next save.
A single part exceeding the control channel's 16 MiB request limit retains the last
preview and reports an error; its normal STEP export is still available. Updates
snapshot part payloads and placements together and keep only the latest queued save.

This uses SolidCraft's [timeline editing API](https://docs.solidcraft.io/reference/commands.html).
It does not stream native build123d objects or modify your existing editing windows.

Python is authoritative: edits made inside the preview window are replaced on
refresh, and the window closes when caddev stops. Use your normal SolidCraft
window for persistent editing. Geometry still needs to build, parse, and tessellate;
complex changes are not instantaneous. The preview worker runs separately so it does not block builds,
and queued updates are coalesced to the latest one. Failed builds retain the last
good preview. Viewer failures are logged without stopping STEP exports.

SolidCraft must support `--control` (tested with the 0.20.0 Linux AppImage).
caddev checks `SOLIDCRAFT_EXECUTABLE`, then PATH, then Linux desktop entries for
installed AppImages. To select it explicitly:

```sh
uv run caddev dev --preview housing --solidcraft /path/to/SolidCraft.AppImage
```

App output is in `build/solidcraft-preview.log`. The first successful selected
export starts the app; subsequent source saves update it. A STEP opened manually
outside this managed preview remains a normal, non-live import.

## Class-based parts and dimensions

New projects and `caddev add part` generate a `CadPart` subclass and a shared lazy
`part` instance. Implement `build()` to return a build123d shape or
`ComponentGeometry`. Register the shared instance with `factory="part"`:

```python
# parts/spacer.py
from build123d import Align, Box
from caddev import CadPart

class Spacer(CadPart):
    def __init__(self, height=10):
        self._height = height

    @property
    def height(self):
        return self._height

    def build(self):
        return Box(20, 20, self.height,
                   align=(Align.CENTER, Align.CENTER, Align.MIN))

part = Spacer()
```

```python
# project.py
from caddev import Component, Project
project = Project(components=[
    Component(id="spacer", module="parts.spacer", factory="part"),
])
```

`part.width`, `part.depth`, and `part.height` default to X, Y, and Z extents in the
part's coordinate frame. Override any with `@property` to expose a nominal design
dimension. `part.bounds` always reports actual geometry. An overridden dimension
changes the geometry only if `build()` uses it, as above.

Geometry is built lazily once per object. `part.shape` returns an independent copy;
`part.geometry` is its cached construction result and should not be mutated.
Treat constructor inputs as immutable and create a new instance to change them.
`part.datums` exposes a copy of its named placement frames. Sharing the module's
`part` instance lets dimension queries and exports reuse the same construction.
You may also register a zero-argument class with `factory="Spacer"`. Existing
zero-argument function factories remain supported with the default `factory="build"`.

For stacking bottom-aligned parts, use `Location((0, 0, lower.height))` for the
upper part. For arbitrary origins, use
`Location((0, 0, lower.bounds.max.Z - upper.bounds.min.Z + gap))`. This aligns
extents at the origin; compose it with the lower instance's placement when needed.
The generated pair assembly demonstrates stacking with `part.height`.

### Multiple bodies within a part

```python
from build123d import Box, Location
from caddev import CadPart, ComponentGeometry, FeatureGeometry, Appearance

class Bracket(CadPart):
    def build(self):
        return ComponentGeometry(
            features={
                "plate": FeatureGeometry(
                    shape=Box(20, 30, 4), name="Top plate",
                    location=Location((0, 0, 10), (180, 0, 0)),
                    appearance=Appearance(color=(0.8, 0.2, 0.1)),
                ),
                "base": FeatureGeometry(shape=Box(30, 40, 4), name="Base"),
            },
            datums={"mount": Location((0, 0, 4))},
        )

part = Bracket()
```

Each `FeatureGeometry` exports as a named **body inside its part**, rather than a
nested assembly component. Disconnected solids in one feature become numbered
bodies. They are not fused, so overlapping geometry remains separate.
Feature shapes may also be other `CadPart` objects.

A feature's complete translation and rotation are composed with its shape's
existing placement and baked into its body geometry exactly once. This includes
rotations such as `(180, 0, 0)`. Instance placements then position the whole part
within the assembly. Datums remain in the part's coordinate frame.

STEP uses millimeters and Z up. Body names and RGB colors are retained; engineering
materials, shininess, transparency, and datum metadata are not guaranteed to
round-trip. IDs must contain letters, digits, underscores, or hyphens, start with a
letter or digit, and be unique across registered parts and assemblies.

## Assemblies

Put definitions under `assemblies/` and import them into `Project(assemblies=[...])`.
Instances reference registered part or assembly IDs:

```python
from build123d import Location
from caddev import Assembly, Instance, Mate

fixture = Assembly(
    id="fixture",
    instances=[
        Instance("base", "bracket", location=Location()),
        Instance("upper", "bracket", mate=Mate(
            source_datum="mount",
            target_instance="base",
            target_datum="mount",
            offset=Location((0, 0, 10), (180, 0, 0)),
        )),
    ],
    datums={"origin": Location()},
)
```

This example assumes the registered bracket exposes the `mount` datum above. An instance accepts either
`location` or `mate`; omitting both places it at the origin. Instance IDs are
unique within an assembly. `name` optionally overrides an instance's display label.
Nested assemblies expose their own assembly-local `datums` for mating.

Mates align coordinate frames using:

```text
placement = target_placement × target_datum × offset × inverse(source_datum)
```

The offset is expressed in the target datum frame. Identity offset aligns frames;
there is no automatic facing reversal. Add an explicit rotation when required.
Instances can be declared in any order. Cyclic assembly references, cyclic mates,
unknown targets/datums, and conflicting placements produce errors. This is static
placement, not a motion solver or a mechanical constraint system.

STEP exports preserve the assembly tree, names, RGB colors, feature placements,
and exact solid geometry in millimeters with Z up. Datums, engineering materials,
shininess, and transparency remain Python metadata and are not guaranteed to
round-trip through STEP. `Appearance` and `Material` remain available for existing
projects. STEP is currently the only output format.

## Rebuild behavior

The watcher debounces source edits and tracks transitive local Python imports.
A changed factory or helper rebuilds affected parts and dependent assemblies.
Unchanged successful shapes are reused, and each required factory runs once per
rebuild. Assembly source edits rebuild that assembly and its dependents.

Changes to `project.py`, target registration, or the source-file inventory cause a
full rebuild. Dynamic imports or syntax errors also trigger a conservative full
rebuild. Keep dependencies as ordinary Python imports for selective rebuilding;
non-Python assets and external package changes are not watched, so restart caddev
after changing those inputs. Environments, caches, and `build/` are ignored.

Every output is replaced atomically only after successful export. A failed target
keeps its last-good file; dependent assemblies are blocked rather than exported
with stale input. Other independent targets can succeed. The watcher survives
errors and retries failed targets on the next source edit. Publication is atomic
per file, not across the entire project. Files for removed/renamed targets are
left untouched; `status` lists only currently registered targets. Presence does
not prove freshness.

## Development

```sh
uv sync
uv run python -m unittest discover -s tests -v
```

A runnable part and repeated-instance assembly are in `examples/step_project`.

The core modules are project declarations, the incremental build session, static
dependency tracking, source loading, the STEP exporter, and the file watcher.
CAD projects execute Python code with your user privileges; build trusted sources.
