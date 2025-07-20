#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>
#include <thread>
#include <SFML/Graphics.hpp>

#define PI 3.14159265359

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
        if (position.x < 0) position.x = 1200;
        if (position.x > 1200) position.x = 0;
        if (position.y < 0) position.y = 800;
        if (position.y > 800) position.y = 0;
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
    BoidsSimulation(int numBoids = 30) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> x_dist(0, 1200);
        std::uniform_real_distribution<> y_dist(0, 800);
        
        for (int i = 0; i < numBoids; ++i) {
            boids.emplace_back(x_dist(gen), y_dist(gen));
        }
    }
    
    void setWeights(double sep, double ali, double coh) {
        separationWeight = sep;
        alignmentWeight = ali;
        cohesionWeight = coh;
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

void drawBoid(sf::RenderWindow& window, const Boid& boid) {
    double angle = std::atan2(boid.velocity.y, boid.velocity.x);
    
    // Create bird shape (triangle)
    sf::ConvexShape bird;
    bird.setPointCount(3);
    bird.setPoint(0, sf::Vector2f(10, 0));
    bird.setPoint(1, sf::Vector2f(-5, -4));
    bird.setPoint(2, sf::Vector2f(-5, 4));
    
    // Set bird properties
    bird.setFillColor(sf::Color(79, 195, 247)); // Light blue
    bird.setOutlineColor(sf::Color(41, 182, 246)); // Darker blue
    bird.setOutlineThickness(1);
    
    // Position and rotate bird
    bird.setPosition(boid.position.x, boid.position.y);
    bird.setRotation(angle * 180 / PI);
    
    // Draw bird
    window.draw(bird);
    
    // Add wing
    sf::ConvexShape wing;
    wing.setPointCount(3);
    wing.setPoint(0, sf::Vector2f(0, 0));
    wing.setPoint(1, sf::Vector2f(-3, -8));
    wing.setPoint(2, sf::Vector2f(3, -6));
    
    wing.setFillColor(sf::Color(41, 182, 246)); // Darker blue
    wing.setPosition(boid.position.x, boid.position.y);
    wing.setRotation(angle * 180 / PI);
    
    window.draw(wing);
    
    // Add tail
    sf::ConvexShape tail;
    tail.setPointCount(3);
    tail.setPoint(0, sf::Vector2f(-5, 0));
    tail.setPoint(1, sf::Vector2f(-10, -3));
    tail.setPoint(2, sf::Vector2f(-10, 3));
    
    tail.setFillColor(sf::Color(2, 119, 189)); // Dark blue
    tail.setPosition(boid.position.x, boid.position.y);
    tail.setRotation(angle * 180 / PI);
    
    window.draw(tail);
    
    // Add eye
    sf::CircleShape eye(1);
    eye.setFillColor(sf::Color::Black);
    eye.setPosition(boid.position.x + 6 * std::cos(angle) - 1 * std::sin(angle) - 1,
                   boid.position.y + 6 * std::sin(angle) + 1 * std::cos(angle) - 1);
    
    window.draw(eye);
}

int main() {
    // Create window
    sf::RenderWindow window(sf::VideoMode(1200, 800), "Boids Flocking Simulation");
    window.setFramerateLimit(60);
    
    // Create simulation
    BoidsSimulation simulation(30);
    
    // Create background
    sf::RectangleShape background(sf::Vector2f(1200, 800));
    background.setFillColor(sf::Color(26, 26, 46)); // Dark blue background
    
    // Create text for info
    sf::Font font;
    if (!font.loadFromFile("/System/Library/Fonts/Arial.ttf")) {
        // Fallback font
        std::cout << "Warning: Could not load font, using default" << std::endl;
    }
    
    sf::Text infoText;
    infoText.setFont(font);
    infoText.setCharacterSize(16);
    infoText.setFillColor(sf::Color::White);
    infoText.setPosition(10, 10);
    
    // Main loop
    sf::Clock clock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        
        // Update simulation
        float dt = clock.restart().asSeconds();
        simulation.update(dt);
        
        // Clear window
        window.clear();
        
        // Draw background
        window.draw(background);
        
        // Draw boids
        for (const auto& boid : simulation.getBoids()) {
            drawBoid(window, boid);
        }
        
        // Update info text
        infoText.setString("Boids Flocking Simulation - 30 Birds\nPress ESC to exit");
        
        // Draw info
        window.draw(infoText);
        
        // Display everything
        window.display();
    }
    
    return 0;
} 