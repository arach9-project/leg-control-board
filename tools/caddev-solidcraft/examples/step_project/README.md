# Example project

Run `just run` to build STEP files and preview the pair assembly live in SolidCraft.
Run `just run example` to preview the individual part instead. SolidCraft is found
on PATH or through its installed Linux desktop entry; alternatively set
`SOLIDCRAFT_EXECUTABLE` to the app or AppImage path.

Python controls this disposable preview window. Saving sources refreshes the
selected export automatically; Ctrl+C closes the preview. Keep persistent edits
in a separate normal SolidCraft window. `uv run caddev dev` runs export-only mode.

The pair assembly contains two independently placed instances of the same part.
Outputs are in `build/parts/example.step` and `build/assemblies/pair.step`.
