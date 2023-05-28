# Pixitale
![Project Logo](godot/screenshot.png)


## Build
Lazy command from `cpp` folder: `scons -C godot custom_modules=../custom && ./godot/bin/godot.linuxbsd.editor.x86_64 --editor --path ../godot`. Change `godot.linuxbsd.editor.x86_64` to your Godot editor executable if not on linux. 
Pro tip: close the editor with `ctrl + c`.

### Building the editor
Run `scons custom_modules=../custom` from `cpp/godot` folder. After building, Godot editor will be in `cpp/godot/bin`. Add `target=release` for better performance at the cost of getting worst error message.

### compile_commands.json
If working with clangd run `scons custom_modules=../custom compiledb=yes` instead and move `cpp/godot/compile_commands.json` to `cpp/compile_commands.json`.