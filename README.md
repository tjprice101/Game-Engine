# Game Engine

A production-quality 2D game engine built with OpenGL 4.5, supporting Terraria-style survival games, RPGs, and clicker/tycoon games.

## Features

- **Multi-pass deferred rendering**: Sky → Occluder → Shadow → GBuffer → Lighting → Particles → Post-Process → UI
- **Dynamic 2D lighting**: 1D polar shadow maps, up to 64 point lights with soft shadows and normal-mapped diffuse
- **Post-processing stack**: Dual-Kawase bloom, ACES tone mapping, LUT color grading, CRT scanlines, chromatic aberration, film grain, vignette, FXAA
- **Sparse-set ECS**: O(1) component add/get/remove, cache-friendly iteration
- **Particle system**: 16,384 GPU-instanced particles, 10+ presets (fire, smoke, sparks, rain, explosion...)
- **Procedural world generation**: Biome-aware terrain, caves, ores, trees, fluid simulation
- **Structure generation**: Dungeons, underground cabins, mineshafts
- **Multi-layer tilemaps**: Wall / Main / Decoration layers, 40 tile types
- **Immediate-mode UI**: Hotbar, health/mana bars, inventory, tooltips, floating damage numbers
- **Camera system**: Trauma-based screen shake, spring follow, deadzone, letterbox, parallax layers
- **Spritesheet animation**: Clip-based state machine with event callbacks

## Requirements

- CMake 3.20+
- C++20 compiler (MSVC 2022, GCC 12+, Clang 14+)
- OpenGL 4.5 capable GPU

Dependencies are fetched automatically via CMake FetchContent:
- GLFW 3.3.10
- GLM 1.0.1
- GLAD2 (OpenGL 4.5 Core)
- STB (image loading)
- miniaudio 0.11.21

## Build

```bash
cmake -B build -S .
cmake --build build --config Release
./build/bin/GameEngine
```

## Controls

| Key | Action |
|-----|--------|
| WASD / Arrow Keys | Move |
| Space | Jump |
| Left Click | Mine tile |
| Right Click | Place tile |
| 1-9, 0 | Select hotbar slot |
| E | Open/close inventory |
| Ctrl + Scroll | Zoom |
| Escape | Quit |

## Architecture

```
src/
├── core/          # Window, Input, Timer, EventBus
├── ecs/           # ECS.h (sparse-set), Components.h
├── game/          # AnimationSystem, CameraController, UISystem, Player, Inventory
├── renderer/      # RenderPipeline, LightSystem, PostProcess, SpriteBatch, ParticleSystem
└── world/         # World, Chunk, BiomeSystem, TileRegistry, StructureGen, Noise

assets/shaders/    # GLSL 4.50 shaders for all render passes
```
