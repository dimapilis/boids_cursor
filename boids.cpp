#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>

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
        std::uniform_real_distribution<> angle_dist(0, 2 * M_PI);
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
        if (position.x < 0) position.x = 800;
        if (position.x > 800) position.x = 0;
        if (position.y < 0) position.y = 600;
        if (position.y > 600) position.y = 0;
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
    
public:
    BoidsSimulation(int numBoids = 30) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> x_dist(0, 800);
        std::uniform_real_distribution<> y_dist(0, 600);
        
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
            Vector2D sep = separation(boid);
            Vector2D ali = alignment(boid);
            Vector2D coh = cohesion(boid);
            
            sep = sep * 1.5;  // Separation weight
            ali = ali * 1.0;  // Alignment weight
            coh = coh * 1.0;  // Cohesion weight
            
            boid.applyForce(sep);
            boid.applyForce(ali);
            boid.applyForce(coh);
        }
        
        for (auto& boid : boids) {
            boid.update(dt);
        }
    }
    
    std::string getJSONState() {
        std::ostringstream json;
        json << "{\"boids\":[";
        
        for (size_t i = 0; i < boids.size(); ++i) {
            if (i > 0) json << ",";
            json << "{\"x\":" << boids[i].position.x 
                 << ",\"y\":" << boids[i].position.y
                 << ",\"vx\":" << boids[i].velocity.x
                 << ",\"vy\":" << boids[i].velocity.y << "}";
        }
        
        json << "]}";
        return json.str();
    }
};

int main() {
    BoidsSimulation simulation(30);
    
    std::cout << "Content-Type: text/html\r\n\r\n";
    
    std::ifstream htmlFile("index.html");
    if (htmlFile.is_open()) {
        std::string line;
        while (std::getline(htmlFile, line)) {
            std::cout << line << std::endl;
        }
        htmlFile.close();
    } else {
        // Fallback HTML if file doesn't exist
        std::cout << R"(
<!DOCTYPE html>
<html>
<head>
    <title>Boids Simulation</title>
    <style>
        body { margin: 0; background: #1a1a2e; overflow: hidden; }
        canvas { display: block; }
        .info {
            position: absolute;
            top: 10px;
            left: 10px;
            color: white;
            font-family: Arial, sans-serif;
            font-size: 14px;
            background: rgba(0,0,0,0.5);
            padding: 10px;
            border-radius: 5px;
        }
    </style>
</head>
<body>
    <div class="info">Boids Flocking Simulation - 30 Birds</div>
    <canvas id="canvas"></canvas>
    <script>
        const canvas = document.getElementById('canvas');
        const ctx = canvas.getContext('2d');
        
        canvas.width = window.innerWidth;
        canvas.height = window.innerHeight;
        
        let boids = [];
        
        function drawBoid(x, y, vx, vy) {
            const angle = Math.atan2(vy, vx);
            const speed = Math.sqrt(vx*vx + vy*vy);
            
            ctx.save();
            ctx.translate(x, y);
            ctx.rotate(angle);
            
            // Bird body (triangle)
            ctx.fillStyle = '#4fc3f7';
            ctx.beginPath();
            ctx.moveTo(8, 0);
            ctx.lineTo(-4, -3);
            ctx.lineTo(-4, 3);
            ctx.closePath();
            ctx.fill();
            
            // Bird wing
            ctx.fillStyle = '#29b6f6';
            ctx.beginPath();
            ctx.moveTo(0, 0);
            ctx.lineTo(-2, -6);
            ctx.lineTo(2, -4);
            ctx.closePath();
            ctx.fill();
            
            // Bird tail
            ctx.fillStyle = '#0277bd';
            ctx.beginPath();
            ctx.moveTo(-4, 0);
            ctx.lineTo(-8, -2);
            ctx.lineTo(-8, 2);
            ctx.closePath();
            ctx.fill();
            
            ctx.restore();
        }
        
        function animate() {
            ctx.fillStyle = 'rgba(26, 26, 46, 0.1)';
            ctx.fillRect(0, 0, canvas.width, canvas.height);
            
            boids.forEach(boid => {
                drawBoid(boid.x, boid.y, boid.vx, boid.vy);
            });
            
            requestAnimationFrame(animate);
        }
        
        function updateBoids() {
            fetch('/cgi-bin/boids')
                .then(response => response.json())
                .then(data => {
                    boids = data.boids;
                })
                .catch(error => console.error('Error:', error));
        }
        
        animate();
        setInterval(updateBoids, 50);
    </script>
</body>
</html>
        )" << std::endl;
    }
    
    return 0;
} 