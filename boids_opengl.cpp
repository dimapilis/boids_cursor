#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>
#include <thread>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>

// Constants
#define PI 3.14159265359
#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 600
#define BOIDSCOUNT 300

// Simulation parameters
#define MAX_WING_ANGLE 67.5
#define WING_ANGLE_THRESHOLD 33.75
#define MAX_VELOCITY 10.0
#define COLLISION_RADIUS 10.0
#define COHESION_FACTOR 100.0
#define ALIGNMENT_FACTOR 8.0

using namespace std;

// ============================================================================
// Vector Mathematics
// ============================================================================

class vec3df {
public:
    float x, y, z;
    
    vec3df() : x(0), y(0), z(0) {}
    vec3df(float px, float py, float pz) : x(px), y(py), z(pz) {}
    
    vec3df operator+ (const vec3df& o) const { 
        return vec3df(x + o.x, y + o.y, z + o.z); 
    }
    vec3df operator- (const vec3df& o) const { 
        return vec3df(x - o.x, y - o.y, z - o.z); 
    }
    vec3df operator* (float b) const { 
        return vec3df(x * b, y * b, z * b); 
    }
    vec3df operator/ (float b) const { 
        return vec3df(x / b, y / b, z / b); 
    }
    
    float length() const { 
        return sqrt(x * x + y * y + z * z); 
    }
    
    vec3df normalize() const {
        float len = length();
        return len > 0 ? *this / len : vec3df();
    }
};

// Vector utility functions
vec3df normalize(const vec3df& a) { 
    return a.normalize(); 
}

float dotproduct(const vec3df& a, const vec3df& b) { 
    return a.x * b.x + a.y * b.y + a.z * b.z; 
}

vec3df crossproduct(const vec3df& a, const vec3df& b) {
    return vec3df(a.y * b.z - a.z * b.y,
                  a.z * b.x - a.x * b.z,
                  a.x * b.y - a.y * b.x);
}

float distBetween(const vec3df& a, const vec3df& b) {
    vec3df diff = b - a;
    return diff.length();
}

// ============================================================================
// Boid Structure
// ============================================================================

struct Boid {
    vec3df avgdirection;
    vec3df oldposition;
    vec3df position;
    vec3df direction;
    vec3df rotation;
    float angle;
    vec3df velocity;
    bool wingRise;
    float upperWingAngle;
    float lowerWingAngle;
    float bodyHeight;
};

// ============================================================================
// Global Variables
// ============================================================================

// Window dimensions
int GW = WINDOW_WIDTH, GH = WINDOW_HEIGHT;

// Lighting configuration
GLfloat light_pos[4] = {1.0, 1.0, 1000.0, 1.0};
GLfloat light_amb[4] = {0.6, 0.6, 0.6, 1.0};
GLfloat light_diff[4] = {0.6, 0.6, 0.6, 1.0};
GLfloat light_spec[4] = {0.8, 0.8, 0.8, 1.0};

// Material definitions
struct Material {
    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    GLfloat shininess[1];
};

Material blueMaterial = {
    {0.0, 0.0, 0.0, 1.0},
    {104.0f/255, 206.0f/255, 205.0f/255, 1.0},
    {0.0, 0.0, 0.0, 1.0},
    {0.0}
};

Material orangeMaterial = {
    {0.0, 0.0, 0.0, 1.0},
    {230.0f/255, 152.0f/255, 97.0f/255, 1.0},
    {0.0, 0.0, 0.0, 1.0},
    {0.0}
};

// Flock data
int flockPopulation;
vector<Boid> flockList;

// Behavior weights
int m1 = 1, m2 = 0, m3 = 1;  // cohesion, attraction, velocity

// Predator/attractor
Boid predator;
float modelAngle = 0.0;

// Animation state
bool wingRise = true;
float upperWingAngle = 0.0;
float lowerWingAngle = -45.0;
bool pauseScene = false;
float bodyHeight = 0.0;
bool lightIsEnabled = true;

// Boundary box
const float xMin = -250.0, xMax = 250.0;
const float yMin = -250.0, yMax = 250.0;
const float zMin = 250.0, zMax = 700.0;

// ============================================================================
// Utility Functions
// ============================================================================

float randPoint(float min, float max) {
    return ((float(rand()) / float(RAND_MAX)) * (max - min)) + min;
}

void setMaterial(const Material& material) {
    glMaterialfv(GL_FRONT, GL_AMBIENT, material.ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, material.diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, material.specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, material.shininess);
}

void initLighting() {
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diff);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_amb);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_spec);
    glShadeModel(GL_FLAT);
}

void updateLightPosition() {
    glMatrixMode(GL_MODELVIEW);
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
}

// ============================================================================
// Flock Initialization
// ============================================================================

void setupFlock(int population) {
    flockPopulation = population;
    srand(time(NULL));
    
    for(int i = 0; i < flockPopulation; ++i) {
        Boid boid;
        
        // Random initial position
        boid.position = vec3df(randPoint(xMin, xMax), 
                               randPoint(yMin, yMax), 
                               randPoint(zMin, zMax));
        boid.oldposition = boid.position;
        boid.velocity = vec3df(0.0, 0.0, 0.0);
        
        // Initial direction and rotation
        boid.direction = vec3df(0.0, 0.0, 1.0);
        boid.rotation = vec3df(0.0, 0.0, 0.0);
        boid.angle = 0;
        
        // Wing animation state
        boid.upperWingAngle = randPoint(0.0, MAX_WING_ANGLE);
        boid.lowerWingAngle = (boid.upperWingAngle / MAX_WING_ANGLE) * 90.0 - 45.0;
        boid.wingRise = (boid.upperWingAngle < WING_ANGLE_THRESHOLD);
        
        flockList.push_back(boid);
    }
}

// ============================================================================
// Flock Behavior
// ============================================================================

vec3df flockCentering(const Boid& bj, int j) {
    vec3df pcj;
    
    for(int i = 0; i < flockPopulation; ++i) {
        if(j != i) {
            pcj = pcj + flockList.at(i).position;
        }
    }
    
    pcj = pcj / (flockPopulation - 1);
    
    return (pcj - bj.position) / COHESION_FACTOR;
}

vec3df collisionAvoidance(const Boid& bj, int j) {
    vec3df c;
    
    for(int i = 0; i < flockPopulation; ++i) {
        if(j != i) {
            if(distBetween(flockList.at(i).position, bj.position) < COLLISION_RADIUS) {
                c = c - (flockList.at(i).position - bj.position);
            }
        }
    }
    
    return c;
}

vec3df velocityMatching(const Boid& bj, int j) {
    vec3df pvj;
    
    for(int i = 0; i < flockPopulation; ++i) {
        if(j != i) {
            pvj = pvj + flockList.at(i).velocity;
        }
    }
    
    pvj = pvj / (flockPopulation - 1);
    
    return (pvj - bj.velocity) / ALIGNMENT_FACTOR;
}

vec3df limit_velocity(Boid& b) {
    if(b.velocity.length() > MAX_VELOCITY) {
        b.velocity = b.velocity.normalize() * MAX_VELOCITY;
    }
    
    return b.velocity;
}

vec3df bound_position(const Boid& b) {
    vec3df v;
    
    if(b.position.x < xMin) {
        v.x = 3.0;
    }
    else if(b.position.x > xMax) {
        v.x = -3.0;
    }
    if(b.position.y < yMin) {
        v.y = 3.0;
    }
    else if(b.position.y > yMax) {
        v.y = -3.0;
    }
    if(b.position.z < zMin) {
        v.z = 3.0;
    }
    else if(b.position.z > zMax) {
        v.z = -3.0;
    }
    
    return v;
}

vec3df tend_to_place(const Boid& b) {
    vec3df place = predator.position;
    
    return (place - b.position) / COHESION_FACTOR;
}

// ============================================================================
// Boid Update
// ============================================================================

void updateBoids() {
    vec3df v1, v2, v3, v4, v5;
    
    for(int i = 0; i < flockPopulation; ++i) {
        Boid& b = flockList.at(i);
        
        v1 = flockCentering(b, i) * m1;
        v2 = collisionAvoidance(b, i);
        v3 = velocityMatching(b, i);
        v4 = bound_position(b);
        v5 = tend_to_place(b) * m2;
        
        b.velocity = b.velocity + v1 + v2 + v3 + v4 + v5;
        b.velocity = limit_velocity(b) * m3;
        
        b.oldposition = b.position;
        b.position = b.position + b.velocity;
        b.direction = b.position - b.oldposition;
    }
    
    // Update average direction and rotation
    vec3df avgDir;
    for(int i = 0; i < flockPopulation; ++i) {
        avgDir = avgDir + flockList.at(i).direction;
    }
    avgDir = avgDir / flockPopulation;

    for(int i = 0; i < flockPopulation; ++i) {
        vec3df olddir = flockList.at(i).direction;
        vec3df newdir = avgDir * COHESION_FACTOR - flockList.at(i).oldposition;
        flockList.at(i).avgdirection = avgDir * COHESION_FACTOR;
        flockList.at(i).rotation = crossproduct(olddir, newdir);
        flockList.at(i).angle = dotproduct(olddir, newdir) / (olddir.length() * newdir.length());
        flockList.at(i).angle = acos(flockList.at(i).angle) * (180.0 / PI);
    }
}

// ============================================================================
// Drawing Functions
// ============================================================================

// Helper function to draw a rectangular face
void drawFace(const vec3df& v1, const vec3df& v2, const vec3df& v3, const vec3df& v4) {
    vec3df normal = (v1 + v2 + v3 + v4) / 4.0;
    normal = normal.normalize();
    glNormal3f(normal.x, normal.y, normal.z);
    
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    } else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
}

void drawWing() {
    // Wing dimensions
    const float width = 2.5, height = 0.5, depth = 3.0;
    
    // Bottom face
    drawFace(vec3df(-width, -height, -depth), vec3df(width, -height, -depth),
             vec3df(width, -height, depth), vec3df(-width, -height, depth));
    
    // Left face
    drawFace(vec3df(-width, -height, -depth), vec3df(-width, height, -depth),
             vec3df(-width, height, depth), vec3df(-width, -height, depth));
    
    // Right face
    drawFace(vec3df(width, -height, -depth), vec3df(width, height, -depth),
             vec3df(width, height, depth), vec3df(width, -height, depth));
    
    // Front face
    drawFace(vec3df(-width, -height, depth), vec3df(-width, height, depth),
             vec3df(width, height, depth), vec3df(width, -height, depth));
    
    // Back face
    drawFace(vec3df(-width, -height, -depth), vec3df(-width, height, -depth),
             vec3df(width, height, -depth), vec3df(width, -height, -depth));
    
    // Top face
    drawFace(vec3df(-width, height, -depth), vec3df(width, height, -depth),
             vec3df(width, height, depth), vec3df(-width, height, depth));
}

void drawBody() {    
    // Body dimensions
    const float width = 3.0, height = 1.0, depth = 4.0;
    
    // Bottom face
    drawFace(vec3df(-width, -height, -depth), vec3df(width, -height, -depth),
             vec3df(width, -height, depth), vec3df(-width, -height, depth));
    
    // Left face
    drawFace(vec3df(-width, -height, -depth), vec3df(-width, height, -depth),
             vec3df(-width, height, depth), vec3df(-width, -height, depth));
    
    // Right face
    drawFace(vec3df(width, -height, -depth), vec3df(width, height, -depth),
             vec3df(width, height, depth), vec3df(width, -height, depth));
    
    // Front face
    drawFace(vec3df(-width, -height, depth), vec3df(-width, height, depth),
             vec3df(width, height, depth), vec3df(width, -height, depth));
    
    // Back face
    drawFace(vec3df(-width, -height, -depth), vec3df(-width, height, -depth),
             vec3df(width, height, -depth), vec3df(width, -height, -depth));
    
    // Top face
    drawFace(vec3df(-width, height, -depth), vec3df(width, height, -depth),
             vec3df(width, height, depth), vec3df(-width, height, depth));
} 

void drawHead() {    
    // Head dimensions (pointed front)
    const float width = 3.0, height = 1.0, depth = 2.0;
    
    // Bottom face
    drawFace(vec3df(-width, -height, -depth), vec3df(width, -height, -depth),
             vec3df(width, -height, depth), vec3df(-width, -height, depth));
    
    // Left face (pointed)
    drawFace(vec3df(-width, -height, -depth), vec3df(-width, height, -depth),
             vec3df(-width, height, 0.0), vec3df(-width, -height, depth));
    
    // Right face (pointed)
    drawFace(vec3df(width, -height, -depth), vec3df(width, height, -depth),
             vec3df(width, height, 0.0), vec3df(width, -height, depth));
    
    // Front face (pointed)
    drawFace(vec3df(-width, -height, depth), vec3df(-width, height, 0.0),
             vec3df(width, height, 0.0), vec3df(width, -height, depth));
    
    // Back face
    drawFace(vec3df(-width, -height, -depth), vec3df(-width, height, -depth),
             vec3df(width, height, -depth), vec3df(width, -height, -depth));
    
    // Top face (pointed)
    drawFace(vec3df(-width, height, -depth), vec3df(width, height, -depth),
             vec3df(width, height, 0.0), vec3df(-width, height, 0.0));
}

void drawTail() {    
    // Tail dimensions
    const float width = 3.0, height = 0.5, depth = 2.0;
    
    // Bottom face
    drawFace(vec3df(-width, -height, -depth), vec3df(width, -height, -depth),
             vec3df(width, -height, depth), vec3df(-width, -height, depth));
    
    // Left face
    drawFace(vec3df(-width, -height, -depth), vec3df(-width, height, -depth),
             vec3df(-width, height, depth), vec3df(-width, -height, depth));
    
    // Right face
    drawFace(vec3df(width, -height, -depth), vec3df(width, height, -depth),
             vec3df(width, height, depth), vec3df(width, -height, depth));
    
    // Front face
    drawFace(vec3df(-width, -height, depth), vec3df(-width, height, depth),
             vec3df(width, height, depth), vec3df(width, -height, depth));
    
    // Back face
    drawFace(vec3df(-width, -height, -depth), vec3df(-width, height, -depth),
             vec3df(width, height, -depth), vec3df(width, -height, -depth));
    
    // Top face
    drawFace(vec3df(-width, height, -depth), vec3df(width, height, -depth),
             vec3df(width, height, depth), vec3df(-width, height, depth));
}

void drawBoid(int i, bool inFlock) {
    // Translating body and wings
    glPushMatrix();
    if(inFlock) {
        glTranslatef(0.0, flockList.at(i).bodyHeight, 0.0);
    }
    else {
        glTranslatef(0.0, bodyHeight, 0.0);
    }

    // Draw right wing
    glPushMatrix();
    glTranslatef(3.0, 0.0, 0.0);
    if(inFlock) {
        glRotatef(flockList.at(i).upperWingAngle, 0.0, 0.0, 1.0);
    }
    else {
        glRotatef(upperWingAngle, 0.0, 0.0, 1.0);
    }
    glTranslatef(2.5, 0.0, 0.0);
    glPushMatrix();
    glTranslatef(2.5, 0.0, 0.0);
    if(inFlock) {
        glRotatef(flockList.at(i).lowerWingAngle, 0.0, 0.0, 1.0);
    }
    else {
        glRotatef(lowerWingAngle, 0.0, 0.0, 1.0);
    }
    glTranslatef(2.5, 0.0, 0.0);
    drawWing();
    glPopMatrix();
    drawWing();
    glPopMatrix();
    
    // Draw left wing
    glPushMatrix();
    glTranslatef(-3.0, 0.0, 0.0);
    if(inFlock) {
        glRotatef(-flockList.at(i).upperWingAngle, 0.0, 0.0, 1.0);
    }
    else {
        glRotatef(-upperWingAngle, 0.0, 0.0, 1.0);
    }
    glTranslatef(-2.5, 0.0, 0.0);
    glPushMatrix();
    glTranslatef(-2.5, 0.0, 0.0);
    if(inFlock) {
        glRotatef(-flockList.at(i).lowerWingAngle, 0.0, 0.0, 1.0);
    }
    else {
        glRotatef(-lowerWingAngle, 0.0, 0.0, 1.0);
    }
    glTranslatef(-2.5, 0.0, 0.0);
    drawWing();
    glPopMatrix();
    drawWing();
    glPopMatrix();
    
    // Draw body
    drawBody();
    
    // Draw head
    glPushMatrix();
    glTranslatef(0.0, 0.0, 6.0);
    glRotatef(-lowerWingAngle, 1.0, 0.0, 0.0);
    drawHead();
    glPopMatrix();
    
    // Draw tail
    glPushMatrix();
    glTranslatef(0.0, 0.0, -6.0);
    glRotatef(lowerWingAngle, 1.0, 0.0, 0.0);
    drawTail();
    glPopMatrix();

    glPopMatrix();
} 

void drawAll() {
    // Drawing predator
    setMaterial(orangeMaterial);
    glPushMatrix();
    glTranslatef(predator.position.x, predator.position.y, predator.position.z);
    glRotatef(modelAngle, 0.0, 1.0, 0.0);
    glColor3f(0.0, 1.0, 0.0);
    drawBoid(0, false);
    glPopMatrix();
    
    vec3df avgPos;
    vec3df avgDir;
    
    // Drawing boids
    setMaterial(blueMaterial);
    for(int i = 0; i < flockList.size(); ++i) {
        glPushMatrix();
        Boid boid = flockList.at(i);
        glTranslatef(boid.position.x, boid.position.y, boid.position.z);
        avgPos = avgPos + boid.position;
        avgDir = avgDir + boid.direction;
        glRotatef(boid.angle, 0.0, boid.rotation.y, boid.rotation.z);
        glColor3f((float)0/255, (float)0/255, (float)0/255);
        drawBoid(i, true);
        glPopMatrix();
    }
    
    avgPos = avgPos / flockPopulation;
    avgDir = avgDir / flockPopulation;
    avgDir = avgDir * COHESION_FACTOR; // Scale for display
    
    for(int i = 0; i < flockPopulation && !lightIsEnabled; i++){
        Boid s = flockList.at(i);
        glColor3f(0.0, 1.0, 0.0);
        glBegin(GL_LINES);
        glVertex3f(s.position.x, s.position.y, s.position.z); // Origin of the line
        glVertex3f(s.position.x+(s.direction.x*2), s.position.y+(s.direction.y*2), s.position.z+(s.direction.z*2)); // Ending point of the line
        glEnd();
    }
}

void display(SDL_Window* window) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    // Set up the camera
    gluLookAt(0.0, 0.0, 800.0, 
              0.0, 0.0, 0.0,
              0.0, 1.0, 0.0);
    updateLightPosition();
    drawAll();
    
    SDL_GL_SwapWindow(window);
}

void reshape(int w, int h) {
    GW = w;
    GH = h;
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (float)w/h, .01, 1500.0);
    glViewport(0, 0, w, h);
    glMatrixMode(GL_MODELVIEW);
}

void handleKeyboard(SDL_Event& event) {
    switch(event.key.keysym.sym) {
        case SDLK_a:
            predator.position.x += -20;
            break;
        case SDLK_d:
            predator.position.x += 20;
            break;
        case SDLK_w:
            predator.position.y += 20;
            break;
        case SDLK_s:
            predator.position.y += -20;
            break;
        case SDLK_z:
            predator.position.z += 20;
            break;
        case SDLK_x:
            predator.position.z += -20;
            break;
        case SDLK_q:
            exit(EXIT_SUCCESS);
            break;
        case SDLK_u:
            // Switches between predator and bait
            m2++;
            if(m2 == 2) { m2 = -1; }
            break;
        case SDLK_i:
            // Scatters the flock
            m1 = -m1;
            break;
        case SDLK_o:
            // Unpauses animation
            m3 = 1;
            pauseScene = false;
            break;
        case SDLK_p:
            // Pauses animation
            m3 = 0;
            pauseScene = true;
            break;
        case SDLK_m:
            // Rotate model
            modelAngle += 45.0;
            if(modelAngle == 360.0) {
                modelAngle = 0.0;
            }
            break;
        case SDLK_l:
            lightIsEnabled = !lightIsEnabled;
            if(lightIsEnabled) {
                glEnable(GL_LIGHTING);
            }
            else {
                glDisable(GL_LIGHTING);
            }
            break;
    }
}

void idle() {
    if(!pauseScene) {
        if(wingRise) {
            upperWingAngle += 6.0;
            lowerWingAngle += 8.0;
            bodyHeight = lowerWingAngle/15;
            if(upperWingAngle >= MAX_WING_ANGLE) {
                wingRise = false;
            }
        }
        else {
            upperWingAngle -= 6.0;
            lowerWingAngle -= 8.0;
            bodyHeight = lowerWingAngle/15;
            if(upperWingAngle <= 0.0) {
                wingRise = true;
            }
        }
        
        for(int i = 0; i < flockList.size(); ++i) {
            if(flockList.at(i).wingRise) {
                flockList.at(i).upperWingAngle += 6.0;
                flockList.at(i).lowerWingAngle += 8.0;
                flockList.at(i).bodyHeight = flockList.at(i).lowerWingAngle/15;
                if(flockList.at(i).upperWingAngle >= MAX_WING_ANGLE) {
                    flockList.at(i).wingRise = false;
                }
            }
            else {
                flockList.at(i).upperWingAngle -= 6.0;
                flockList.at(i).lowerWingAngle -= 8.0;
                flockList.at(i).bodyHeight = flockList.at(i).lowerWingAngle/15;
                if(flockList.at(i).upperWingAngle <= 0.0) {
                    flockList.at(i).wingRise = true;
                }
            }
        }
        updateBoids();
    }
}

int main(int argc, char **argv) {
    // Global variable initializations
    GW = WINDOW_WIDTH;
    GH = WINDOW_HEIGHT;
    predator.position.x = predator.position.y = predator.position.z = 0.0;
    m1 = 1; m2 = 0; m3 = 1; // Reset weights
    modelAngle = 0.0;
    pauseScene = false;
    lightIsEnabled = true;
    
    // Animation variables
    wingRise = true;
    upperWingAngle = 0.0;
    lowerWingAngle = -45.0;
    bodyHeight = 0.0;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    
    // Create window
    SDL_Window* window = SDL_CreateWindow("Boids Simulator", 
                                        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT, 
                                        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // Create OpenGL context
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "OpenGL context could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    
    // OpenGL is ready to use
    
    // Setup 3D and lighting
    glClearColor(0.078, 0.078, 0.180, 1.0); // Dark blue background
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);
    initLighting();
    
    // Setup flock population
    setupFlock(BOIDSCOUNT);
    std::cout << "Boids simulation initialized with " << BOIDSCOUNT << " boids" << std::endl;
    
    // Initial reshape
    reshape(WINDOW_WIDTH, WINDOW_HEIGHT);
    
    // Main loop
    bool quit = false;
    SDL_Event event;
    
    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
            else if (event.type == SDL_KEYDOWN) {
                handleKeyboard(event);
            }
            else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    reshape(event.window.data1, event.window.data2);
                }
            }
        }
        
        // Update simulation
        idle();
        
        // Render
        display(window);
        
        // Cap frame rate
        SDL_Delay(16); // ~60 FPS
    }
    
    // Cleanup
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
} 