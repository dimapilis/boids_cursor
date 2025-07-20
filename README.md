# Boids Simulation - Cursor Recreation

This repository contains a recreation of my original boids (bird flocking) simulation senior project, rebuilt using Cursor AI. The project demonstrates emergent flocking behavior through the implementation of Craig Reynolds' boids algorithm.

## Project Overview

The boids simulation showcases how complex flocking behavior can emerge from simple rules:
- **Cohesion**: Boids move toward the center of nearby flock members
- **Separation**: Boids avoid collisions by steering away from nearby boids
- **Alignment**: Boids match velocity with nearby flock members

## Versions

### OpenGL Version (Recommended)
The main implementation uses OpenGL and SDL2 for 3D rendering:

- **File**: `boids_opengl.cpp`
- **Makefile**: `Makefile_opengl`
- **Documentation**: [README_opengl.md](README_opengl.md)

Features:
- 300 3D boids with detailed wing animations
- Realistic flocking behavior
- Interactive predator/attractor
- Dynamic lighting and materials
- Full keyboard controls

### Other Versions
- `boids_simple.cpp` - Simple 2D SDL2 implementation
- `boids_fixed.cpp` - Fixed version of the simple implementation
- `boids_server.cpp` - Server-based implementation
- `boids_local.cpp` - Local implementation

## Quick Start

### Prerequisites
- macOS with Homebrew
- SDL2: `brew install sdl2`
- OpenGL (built into macOS)

### Building and Running
```bash
# Compile the OpenGL version
make -f Makefile_opengl

# Run the simulation
./boids_opengl
```

### Controls
- **WASD/ZX**: Move predator/attractor
- **U**: Toggle predator behavior
- **I**: Toggle flock scattering
- **O/P**: Resume/pause animation
- **M**: Rotate model
- **L**: Toggle lighting
- **Q**: Quit

## Technical Details

### Original Project
This recreates my senior project from Cal Poly SLO (2010) that demonstrated emergent behavior in computer graphics. The original used GLUT for window management, while this version uses SDL2 for better cross-platform compatibility.

### Algorithm
The simulation implements the classic Craig Reynolds boids algorithm with three main forces that create emergent flocking behavior. Each boid considers its neighbors within specific radii for each behavior type.

### Rendering
The OpenGL version renders each boid as a detailed 3D model composed of geometric primitives, with realistic wing flapping animations and dynamic lighting.

## Development

This project was recreated using Cursor AI, demonstrating how modern AI tools can help reconstruct and improve legacy code while maintaining the original vision and behavior.

## License

Original work by Brent Dimapilis (2010). Recreated with Cursor AI (2024). 