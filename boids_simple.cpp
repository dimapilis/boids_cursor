#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>
#include <thread>
#include <SDL2/SDL.h>

#define PI 3.14159265359
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define NUM_BOIDS 30

struct Vector2D {
    double x, y;
    
    Vector2D(double x = 0, double y = 0) : x(x), y(y) {}
    
    Vector2D operator+(const Vector2D& other) const {
        return Vector2D(x + other.x, y + other.y);
    }
    
    Vector2D operator-(const Vector2D& other) const {
        return Vector2D(x - other.x, y - other.y);
    }
    
    Vector2D operator*(double scalar) const {
        return Vector2D(x * scalar, y * scalar);
    }
    
    double magnitude() const {
        return std::sqrt(x * x + y * y);
    }
    
    Vector2D normalize() const {
        double mag = magnitude();
        if (mag > 0) {
            return Vector2D(x / mag, y / mag);
        }
        return Vector2D(0, 0);
    }
    
    double distance(const Vector2D& other) const {
        return (*this - other).magnitude();
    }
};

struct Boid {
    Vector2D position;
    Vector2D velocity;
    Vector2D acceleration;
    
    Boid(double x, double y) : position(x, y) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> angle_dist(0, 2 * PI);
        std::uniform_real_distribution<> speed_dist(1.0, 3.0);
        
        double angle = angle_dist(gen);
        double speed = speed_dist(gen);
        velocity = Vector2D(std::cos(angle) * speed, std::sin(angle) * speed);
        acceleration = Vector2D(0, 0);
    }
    
    void update(double dt) {
        velocity = velocity + acceleration * dt;
        
        // Limit maximum speed
        double maxSpeed = 5.0;
        if (velocity.magnitude() > maxSpeed) {
            velocity = velocity.normalize() * maxSpeed;
        }
        
        position = position + velocity * dt;
        acceleration = Vector2D(0, 0);
        
        // Wrap around screen boundaries
        if (position.x < 0) position.x = WINDOW_WIDTH;
        if (position.x > WINDOW_WIDTH) position.x = 0;
        if (position.y < 0) position.y = WINDOW_HEIGHT;
        if (position.y > WINDOW_HEIGHT) position.y = 0;
    }
    
    void applyForce(const Vector2D& force) {
        acceleration = acceleration + force;
    }
};

class BoidsSimulation {
private:
    std::vector<Boid> boids;
    double separationRadius = 25.0;
    double alignmentRadius = 50.0;
    double cohesionRadius = 50.0;
    double maxForce = 0.2;
    double separationWeight = 1.5;
    double alignmentWeight = 1.0;
    double cohesionWeight = 1.0;
    
public:
    BoidsSimulation(int numBoids = NUM_BOIDS) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> x_dist(0, WINDOW_WIDTH);
        std::uniform_real_distribution<> y_dist(0, WINDOW_HEIGHT);
        
        for (int i = 0; i < numBoids; ++i) {
            boids.emplace_back(x_dist(gen), y_dist(gen));
        }
    }
    
    Vector2D separation(const Boid& boid) {
        Vector2D steer(0, 0);
        int count = 0;
        
        for (const auto& other : boids) {
            double distance = boid.position.distance(other.position);
            if (distance > 0 && distance < separationRadius) {
                Vector2D diff = boid.position - other.position;
                diff = diff.normalize() * (1.0 / distance);
                steer = steer + diff;
                count++;
            }
        }
        
        if (count > 0) {
            steer = steer * (1.0 / count);
            steer = steer.normalize() * maxForce;
        }
        
        return steer;
    }
    
    Vector2D alignment(const Boid& boid) {
        Vector2D avgVelocity(0, 0);
        int count = 0;
        
        for (const auto& other : boids) {
            double distance = boid.position.distance(other.position);
            if (distance > 0 && distance < alignmentRadius) {
                avgVelocity = avgVelocity + other.velocity;
                count++;
            }
        }
        
        if (count > 0) {
            avgVelocity = avgVelocity * (1.0 / count);
            avgVelocity = avgVelocity.normalize() * maxForce;
        }
        
        return avgVelocity;
    }
    
    Vector2D cohesion(const Boid& boid) {
        Vector2D centerOfMass(0, 0);
        int count = 0;
        
        for (const auto& other : boids) {
            double distance = boid.position.distance(other.position);
            if (distance > 0 && distance < cohesionRadius) {
                centerOfMass = centerOfMass + other.position;
                count++;
            }
        }
        
        if (count > 0) {
            centerOfMass = centerOfMass * (1.0 / count);
            Vector2D desired = centerOfMass - boid.position;
            desired = desired.normalize() * maxForce;
            return desired;
        }
        
        return Vector2D(0, 0);
    }
    
    void update(double dt) {
        for (auto& boid : boids) {
            Vector2D sep = separation(boid) * separationWeight;
            Vector2D ali = alignment(boid) * alignmentWeight;
            Vector2D coh = cohesion(boid) * cohesionWeight;
            
            boid.applyForce(sep);
            boid.applyForce(ali);
            boid.applyForce(coh);
        }
        
        for (auto& boid : boids) {
            boid.update(dt);
        }
    }
    
    const std::vector<Boid>& getBoids() const {
        return boids;
    }
};

void drawBoid(SDL_Renderer* renderer, const Boid& boid) {
    double angle = std::atan2(boid.velocity.y, boid.velocity.x);
    
    // Calculate bird points (triangle shape)
    double size = 8.0;
    SDL_Point points[3];
    
    // Bird body (triangle pointing in direction of movement)
    points[0].x = static_cast<int>(boid.position.x + size * std::cos(angle));
    points[0].y = static_cast<int>(boid.position.y + size * std::sin(angle));
    
    points[1].x = static_cast<int>(boid.position.x + size * 0.5 * std::cos(angle + 2.5));
    points[1].y = static_cast<int>(boid.position.y + size * 0.5 * std::sin(angle + 2.5));
    
    points[2].x = static_cast<int>(boid.position.x + size * 0.5 * std::cos(angle - 2.5));
    points[2].y = static_cast<int>(boid.position.y + size * 0.5 * std::sin(angle - 2.5));
    
    // Set color (light blue)
    SDL_SetRenderDrawColor(renderer, 79, 195, 247, 255);
    
    // Draw bird body
    SDL_RenderDrawLines(renderer, points, 3);
    SDL_RenderDrawLine(renderer, points[2].x, points[2].y, points[0].x, points[0].y);
    
    // Draw wing
    SDL_Point wingPoints[3];
    wingPoints[0].x = static_cast<int>(boid.position.x + size * 0.3 * std::cos(angle));
    wingPoints[0].y = static_cast<int>(boid.position.y + size * 0.3 * std::sin(angle));
    
    wingPoints[1].x = static_cast<int>(boid.position.x + size * 0.6 * std::cos(angle + 1.8));
    wingPoints[1].y = static_cast<int>(boid.position.y + size * 0.6 * std::sin(angle + 1.8));
    
    wingPoints[2].x = static_cast<int>(boid.position.x + size * 0.6 * std::cos(angle - 1.8));
    wingPoints[2].y = static_cast<int>(boid.position.y + size * 0.6 * std::sin(angle - 1.8));
    
    // Set wing color (darker blue)
    SDL_SetRenderDrawColor(renderer, 41, 182, 246, 255);
    SDL_RenderDrawLines(renderer, wingPoints, 3);
    SDL_RenderDrawLine(renderer, wingPoints[2].x, wingPoints[2].y, wingPoints[0].x, wingPoints[0].y);
    
    // Draw eye
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    int eyeX = static_cast<int>(boid.position.x + size * 0.7 * std::cos(angle));
    int eyeY = static_cast<int>(boid.position.y + size * 0.7 * std::sin(angle));
    SDL_RenderDrawPoint(renderer, eyeX, eyeY);
}

int main() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create window
    SDL_Window* window = SDL_CreateWindow("Boids Flocking Simulation", 
                                        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create simulation
    BoidsSimulation simulation(NUM_BOIDS);
    
    // Main loop
    bool quit = false;
    SDL_Event event;
    
    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    quit = true;
                }
            }
        }
        
        // Update simulation
        simulation.update(1.0 / 60.0); // 60 FPS
        
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 26, 26, 46, 255); // Dark blue background
        SDL_RenderClear(renderer);
        
        // Draw boids
        for (const auto& boid : simulation.getBoids()) {
            drawBoid(renderer, boid);
        }
        
        // Update screen
        SDL_RenderPresent(renderer);
        
        // Cap frame rate
        SDL_Delay(16); // ~60 FPS
    }
    
    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
} 