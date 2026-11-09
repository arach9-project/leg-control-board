"""Immutable preview snapshots, with one local STEP payload per physical part."""
from dataclasses import dataclass
from hashlib import sha256
from pathlib import Path


IDENTITY = ((1., 0., 0., 0.), (0., 1., 0., 0.),
            (0., 0., 1., 0.), (0., 0., 0., 1.))


@dataclass(frozen=True)
class PreviewNode:
    key: tuple[str, ...]
    name: str
    placement: tuple[tuple[float, ...], ...] = IDENTITY
    step: str | None = None
    digest: str | None = None


@dataclass(frozen=True)
class PreviewScene:
    nodes: tuple[PreviewNode, ...]


def preview_scene(shape, root: Path, target: str) -> PreviewScene:
    """Snapshot successful exports together with resolved local placements.

    Instance IDs, rather than labels or child indices, keep UI identity stable.
    STEP headers contain timestamps and temporary export paths; only DATA affects
    geometry, names and appearance. Unchanged parts therefore produce no command.
    """
    payloads = {}
    nodes = []

    def visit(node, key, source):
        transform = node.location.wrapped.Transformation()
        placement = tuple(tuple(transform.Value(r, c) for c in range(1, 5))
                          for r in range(1, 4)) + (IDENTITY[3],)
        step = digest = None
        if not node.children:
            if source not in payloads:
                text = (root / 'build' / 'parts' / f'{source}.step').read_text(encoding='utf-8')
                _, marker, data = text.partition('DATA;')
                if not marker:
                    raise ValueError(f'Invalid STEP export for {source}: missing DATA section')
                payloads[source] = (text, sha256(data.encode()).hexdigest())
            step, digest = payloads[source]
        nodes.append(PreviewNode(key, node.label, placement, step, digest))
        for child in node.children:
            visit(child, key + (child._caddev_instance_id,), child._caddev_component_id)

    visit(shape, (target,), target)
    return PreviewScene(tuple(nodes))


def placement_delta(new, old):
    """Parent-frame rigid move: new × inverse(old), in row-major API order."""
    rotation = [[sum(new[r][k] * old[c][k] for k in range(3))
                 for c in range(3)] for r in range(3)]
    return [rotation[r] + [new[r][3] - sum(rotation[r][k] * old[k][3] for k in range(3))]
            for r in range(3)] + [list(IDENTITY[3])]
