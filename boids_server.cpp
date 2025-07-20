#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>
#include <thread>
#include <string>
#include <sstream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

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

std::string createHTTPResponse(const std::string& content, const std::string& contentType = "text/html") {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Content-Length: " << content.length() << "\r\n";
    response << "\r\n";
    response << content;
    return response.str();
}

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    BoidsSimulation simulation(30);
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind socket
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        return 1;
    }
    
    // Listen for connections
    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Error listening" << std::endl;
        return 1;
    }
    
    std::cout << "Boids server running on http://localhost:8080" << std::endl;
    
    while (true) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            std::cerr << "Error accepting connection" << std::endl;
            continue;
        }
        
        char buffer[1024] = {0};
        read(clientSocket, buffer, 1024);
        
        std::string request(buffer);
        std::string response;
        
        if (request.find("GET /api/boids") != std::string::npos) {
            // Update simulation
            simulation.update(0.016); // ~60 FPS
            
            // Return JSON data
            response = createHTTPResponse(simulation.getJSONState(), "application/json");
        } else if (request.find("GET /") != std::string::npos) {
            // Serve HTML file
            std::string htmlContent = readFile("index.html");
            if (htmlContent.empty()) {
                htmlContent = "<h1>Boids Simulation</h1><p>index.html not found</p>";
            }
            response = createHTTPResponse(htmlContent, "text/html");
        } else {
            response = createHTTPResponse("404 Not Found", "text/plain");
        }
        
        send(clientSocket, response.c_str(), response.length(), 0);
        close(clientSocket);
    }
    
    close(serverSocket);
    return 0;
} 