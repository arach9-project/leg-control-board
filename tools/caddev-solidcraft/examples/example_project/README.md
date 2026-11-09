# Example project

Point the editable caddev dependency in `pyproject.toml` at your local checkout,
then run `uv sync` and `uv run caddev dev`.

Open `build/parts/example.step` or `build/assemblies/pair.step` in SolidCraft.
The pair assembly contains two independently placed instances of the same part.
Saving Python sources rebuilds affected exports. Refresh imports manually.
