# Archimedies Engine

Game Engine with toolkits for most normal things.

The goal is that everything can be accessed by the [Scripting Engine](), so that each person can their own
editor expirence.

The Engine works with like usual with a layer based system something is called a "Layer" and something  called and "Overlay".
Inside the LayerStack, Layers are prioritized over Overlay's. This allows for the engine gui to be Layer that gets events first.
And Some debug window to be an Overlay, then renders on top, but recieves mouse events last.

Keep the focus on the Viewport the "Game Screen" is the focus everything can be turned off or on.
Opened panels are always Dockable to the main window, but they would not always have to take up space in the viewport.

## TODO:

- [ ] Cora Integration
- [x] Shader Library
- [ ] Animation
- [ ] 2D Renderer
- [ ] Audio
- [ ] Gamepad Input
- [ ] Text Rendering
- [ ] 3D Models
- [ ] Scene Serialization
- [x] ImGui Overlay
- [ ] Asset Manager
- [x] Layer Stack
- [ ] Scripting
- [ ] Editor UI -- Started
- [ ] Multi-Material Meshes
- [ ] Skybox
- [ ] Lighting
- [x] Directional Light
- [ ] Shadows
- [ ] ImGui Viewport
- [ ] Post-Processing
- [ ] Make sure Delta-Time is real deltatime.
- [ ] ImGuizmo - or Gizmos ourselves.