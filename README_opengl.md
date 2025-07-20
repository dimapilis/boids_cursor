# OpenGL Boids Simulation

This is a recreation of the original boids (bird flocking) simulation using OpenGL and SDL2. The simulation features 300 3D boids with realistic wing flapping animations and flocking behavior.

## Features

- **300 3D Boids**: Each boid is rendered as a detailed 3D model with wings, body, head, and tail
- **Realistic Wing Animation**: Wings flap with non-uniform movement patterns
- **Flocking Behavior**: Boids follow the classic flocking rules:
  - **Cohesion**: Move toward the center of nearby boids
  - **Separation**: Avoid collisions with nearby boids
  - **Alignment**: Match velocity with nearby boids
- **Predator/Attractor**: A special boid that can attract or repel the flock
- **3D Lighting**: Dynamic lighting with material properties
- **Boundary Handling**: Boids are kept within a 3D box boundary

## Controls

### Movement Controls (Predator/Attractor)
- **W/A/S/D**: Move predator up/left/down/right
- **Z/X**: Move predator forward/backward

### Simulation Controls
- **Q**: Quit the simulation
- **U**: Toggle predator behavior (attractor/neutral/repeller)
- **I**: Toggle flock scattering (inverts cohesion behavior)
- **O**: Resume animation (unpause)
- **P**: Pause animation
- **M**: Rotate the predator model
- **L**: Toggle lighting (solid/wireframe mode)

## Technical Details

### Flocking Algorithm
The simulation implements the classic Craig Reynolds boids algorithm with three main forces:

1. **Flock Centering (Cohesion)**: Boids move toward the center of mass of nearby boids
2. **Collision Avoidance (Separation)**: Boids steer away from nearby boids to avoid collisions
3. **Velocity Matching (Alignment)**: Boids adjust their velocity to match nearby boids

### 3D Rendering
- Uses OpenGL for 3D rendering
- Each boid is composed of multiple geometric primitives
- Dynamic lighting with ambient, diffuse, and specular components
- Perspective projection with depth testing

### Animation
- Wing flapping uses sine-wave based animation
- Body height varies with wing position for realistic movement
- Non-uniform wing movement creates more natural flight patterns

## Building

To compile the simulation:

```bash
make
```

### Dependencies
- SDL2 (installed via Homebrew)
- OpenGL (built into macOS)
- GLUT (built into macOS)

## Original Code

This is a recreation of the original boids simulation by Brent Dimapilis from Cal Poly SLO (2010). The original used GLUT for window management, while this version uses SDL2 for better cross-platform compatibility while maintaining the same visual appearance and behavior.

## Configuration

The simulation uses the same parameters as the original:
- **300 boids** (BOIDSCOUNT)
- **3D boundary box**: -250 to 250 in X/Y, 250 to 700 in Z
- **Wing animation**: 67.5° maximum wing angle
- **Velocity limits**: 10.0 maximum speed
- **Collision radius**: 10.0 units for separation 