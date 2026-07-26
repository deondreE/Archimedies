# Archimedies Engine

Game Engine with toolkits for most normal things.

The goal is that everything can be accessed by the [Scripting Engine](), so that each person can their own
editor expirence.

The Engine works with like usual with a layer based system something is called a "Layer" and something  called and "Overlay".
Inside the LayerStack, Layers are prioritized over Overlay's. This allows for the engine gui to be Layer that gets events first.
And Some debug window to be an Overlay, then renders on top, but recieves mouse events last.

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
- [ ] ImGui Overlay
- [ ] Asset Manager
- [x] Layer Stack
- [ ] Scripting
- [ ] Editor UI
- [ ] Multi-Material Meshes
- [ ] Skybox
- [ ] Lighting
- [x] Directional Light
- [ ] Shadows
- [ ] Post-Processing