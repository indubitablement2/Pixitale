# Pixitale
![Project Logo](image.jpg)

## Editor Build
See https://docs.godotengine.org/en/stable/contributing/development/compiling/index.html for requirements.
Commands to easily build and launch the editor as well as update compile_commands.json (for use with clangd) are in: `custom/.vscode/tasks.json`. These commands are meant to run from `custom/` folder.

### Notes
Need to move these into the editor or design wiki.

- Generation performance: Generation passes should abort as early as possible when it has nothing to do. Eg. When generating surface terrain, check if anywhere near the surface before iterating over each cells or doing expensive operations. Generating noise is especially expensive.
- Generation randomness: To generate independently of time, use rand methods from `GridChunkIter` which will output the same value based on chunk coordinate and uses from previous generation passes.
- Sharing data between generation passes: Add it as metadata to `GridChunkIter`. The same iterator is used throughout the chunk generation. It is just reset to start at `(0,0)` before each pass.
- Multiplayer and determinism: The grid has too much data to be sent to clients in real time. Therefore it is only sent once along with successive mutations in order. A mutation is deterministic if it only takes inputs from deterministic sources or is synchronized through methods from `GridApi.add_grid_edit_method()`. These methods are initiated from the server and sent to clients to be executed with the same inputs at the same `Grid.tick` in the same order. Desync sources include:
Using gdscript's `randb` and friends instead of analogous methods exposed on `Grid`.
Trigonometric methods on float (sin, cos, tan) and anything which relies on them. +, -, *, /, sqrt and all of int are Ok.
Everything else (enemies, projectile, etc.) uses the standard authoritative server approach to networking.

