# Archimedies Engine

Game Engine with toolkits for most normal things.

The goal is that everything can be accessed by the [Scripting Engine](), so that each person can their own
editor expirence.

The Engine works with like usual with a layer based system something is called a "Layer" and something  called and "Overlay".
Inside the LayerStack, Layers are prioritized over Overlay's. This allows for the engine gui to be Layer that gets events first.
And Some debug window to be an Overlay, then renders on top, but recieves mouse events last.

Keep the focus on the Viewport the "Game Screen" is the focus everything can be turned off or on.
Opened panels are always Dockable to the main window, but they would not always have to take up space in the viewport.

Remember to `git clone --recursive`

## TODO:

- [ ] DirectX12 -- Started Work on This!
- [x] Cora Integration
- [x] Shader Library
- [ ] Animation
- [ ] 2D Renderer
- [ ] 2.5D Native Support
- [ ] 2D Physics
- [x] Audio
	- [ ] 3D Audio
- [ ] Audio UI
	- [ ] Audio File Icon
	- [ ] Audio UI
	- [ ] Audio Indicator int the editor.
- [x] Content Browser
- [x] Image Thumbnails
- [ ] Gamepad Input
	- XInput
- [ ] Text Rendering
- [x] 3D Models
- [ ] Scene Serialization
- [x] Optimize Math lib
- [x] ImGui Overlay
- [ ] Asset Manager
- [x] Layer Stack
- [ ] Figure out native modding 
	- Headless engine maybe?
- [ ] Scripting
	- C#
		- [ ] Audio API is technically done we just need to make it work in C#.
	- Rust
- [ ] Press `F` to focus camera on object 
- [ ] Editor UI -- Started
- [ ] Multi-Material Meshes
- [ ] Skybox
- [ ] Signals
- [ ] Lighting
- [ ] Single Source Multiplayer
	- [ ] Netcode
- [x] Directional Light
- [ ] Real Time Collab [ Maybe ]()
- [ ] Time Travel Debugging for the game process.
	- [ ] ??
- [ ] Native Visual Scripting
- [ ] Shadows 
- [ ] Game UI
- [ ] Game Code
- [ ] Post-Processing
- [x] Make sure Delta-Time is real deltatime.
- [ ] ImGuizmo - or Gizmos ourselves.
- [ ] Port Scripting engine away from Sandbox
- [ ] Add some kind of execution queue.
	- [ ] Background Importers
- [ ] Entity Name should be editable.
	
# Bug

- Viewport Drag and drop takes whole screen menu bar is not longer interactable.