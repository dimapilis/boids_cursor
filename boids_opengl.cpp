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

#define PI 3.14159265359
#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 600
#define BOIDSCOUNT 300

using namespace std;

// Vector class (3D)
class vec3df {
public:
    float x, y, z;
    vec3df() {x = 0; y = 0; z = 0;}
    vec3df(float px, float py, float pz) {
        x = px;
        y = py;
        z = pz;
    }
    
    vec3df operator+ (vec3df o) { return vec3df(x+o.x, y+o.y, z+o.z); }
    vec3df operator- (vec3df o) { return vec3df(x-o.x, y-o.y, z-o.z); }
    vec3df operator* (float b) { return vec3df(x*b, y*b, z*b); }
    vec3df operator/ (float b) { return vec3df(x/b, y/b, z/b); }
    float length() { return sqrt(x*x + y*y + z*z); }
};

vec3df normalize(vec3df a) { 
    return a / a.length(); 
}

float dotproduct(vec3df a, vec3df b) { 
    return a.x*b.x + a.y*b.y + a.z*b.z; 
}

vec3df crossproduct(vec3df a, vec3df b) {
    return vec3df(a.y*b.z - a.z*b.y,
                  a.z*b.x - a.x*b.z,
                  a.x*b.y - a.y*b.x);
}

float distBetween(vec3df a, vec3df b) {
    return sqrt((b.x-a.x)*(b.x-a.x) + (b.y-a.y)*(b.y-a.y) + (b.z-a.z)*(b.z-a.z));
}

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

// Global variables
int GW, GH;

// Lighting globals
GLfloat light_pos[4] = {1.0, 1.0, 1000.0, 1.0};
GLfloat light_amb[4] = {0.6, 0.6, 0.6, 1.0};
GLfloat light_diff[4] = {0.6, 0.6, 0.6, 1.0};
GLfloat light_spec[4] = {0.8, 0.8, 0.8, 1.0};

struct materialStruct {
    GLfloat ambient[4];
    GLfloat diffuse[4];
    GLfloat specular[4];
    GLfloat shininess[1];
};

materialStruct blueMaterial = {
    {0.0, 0.0, 0.0, 1.0},
    {(float)104/255, (float)206/255, (float)205/255, 1.0},
    {0.0, 0.0, 0.0, 1.0},
    {0.0}
};

materialStruct orangeMaterial = {
    {0.0, 0.0, 0.0, 1.0},
    {(float)230/255, (float)152/255, (float)97/255, 1.0},
    {0.0, 0.0, 0.0, 1.0},
    {0.0}
};

// Flock information
int flockPopulation;
vector<Boid> flockList;
// Flock influences
int m1, m2, m3;

// Predator boid
Boid ball;
float modelAngle;

// Animation variables
bool wingRise;
float upperWingAngle;
float lowerWingAngle;
bool pauseScene;
float bodyHeight;
bool lightIsEnabled;

// Box boundaries
float xMin = -250.0;
float xMax = 250.0;
float yMin = -250.0;
float yMax = 250.0;
float zMin = 250.0;
float zMax = 700.0; 

// Sets up a specific material
void materials(materialStruct materials) {
    glMaterialfv(GL_FRONT, GL_AMBIENT, materials.ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materials.diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, materials.specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, materials.shininess);
}

void init_lighting() {
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diff);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_amb);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_spec);
    glShadeModel(GL_FLAT);
}

void pos_light() {
    // Set the light's position
    glMatrixMode(GL_MODELVIEW);
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
}

float randPoint(float min, float max) {
    return ((float(rand()) / float(RAND_MAX)) * (max - min)) + min;
}

void setup(int reqPop) {
    // Create the flock of boids
    flockPopulation = reqPop;
    Boid b;
    srand(time(NULL));
    for(int i = 0; i < flockPopulation; ++i) {
        // Random locations
        b.position.x = randPoint(xMin, xMax);
        b.position.y = randPoint(yMin, yMax);
        b.position.z = randPoint(zMin, zMax);
        b.oldposition = b.position;
        b.velocity.x = 0.0;
        b.velocity.y = 0.0;
        b.velocity.z = 0.0;
        // Rotating vectors
        b.direction.x = 0.0;
        b.direction.y = 0.0;
        b.direction.z = 1.0;
        b.rotation.x = 0.0;
        b.rotation.y = 0.0;
        b.rotation.z = 0.0;
        b.angle = 0;
        // Non-uniform wing movement
        b.upperWingAngle = randPoint(0.0, 67.5);
        b.lowerWingAngle = (b.upperWingAngle/67.5)*90.0-45.0;
        if(b.upperWingAngle < 33.75) {
            b.wingRise = true;
        }
        else {
            b.wingRise = false;
        }
        flockList.push_back(b);
    }
} 

vec3df flockCentering(Boid bj, int j) {
    vec3df pcj;
    Boid b;
    
    for(int i = 0; i < flockPopulation; ++i) {
        if(j != i) {
            b = flockList.at(i);
            pcj = pcj + b.position;
        }
    }
    
    pcj = pcj / (flockPopulation - 1);
    
    return (pcj - bj.position) / 100.0;
}

vec3df collisionAvoidance(Boid bj, int j) {
    vec3df c;
    Boid b;
    
    for(int i = 0; i < flockPopulation; ++i) {
        if(j != i) {
            b = flockList.at(i);
            if(distBetween(b.position, bj.position) < 10.0) {
                c = c - (b.position - bj.position);
            }
        }
    }
    
    return c;
}

vec3df velocityMatching(Boid bj, int j) {
    vec3df pvj;
    Boid b;
    
    for(int i = 0; i < flockPopulation; ++i) {
        if(j != i) {
            b = flockList.at(i);
            pvj = pvj + b.velocity;
        }
    }
    
    pvj = pvj / (flockPopulation - 1);
    
    return (pvj - bj.velocity) / 8;
}

vec3df limit_velocity(Boid b) {
    float vlim = 10.0;
    
    if(b.velocity.length() > vlim) {
        b.velocity = (b.velocity / b.velocity.length()) * vlim;
    }
    
    return b.velocity;
}

vec3df bound_position(Boid b) {
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

vec3df tend_to_place(Boid b) {
    vec3df place = ball.position;
    
    return (place - b.position) / 100.0;
}

void updateBoids() {
    vec3df v1, v2, v3, v4, v5;
    Boid b;
    vec3df avgDir;
    
    for(int i = 0; i < flockPopulation; ++i) {
        b = flockList.at(i);
        v1 = flockCentering(b, i) * m1;
        v2 = collisionAvoidance(b, i);
        v3 = velocityMatching(b, i);
        v4 = bound_position(b);
        v5 = tend_to_place(b) * m2;
        flockList.at(i).velocity = flockList.at(i).velocity + v1 + v2 + v3 + v4 + v5;
        vec3df t = v1 + v2 + v3 + v4;
        flockList.at(i).velocity = limit_velocity(flockList.at(i)) * m3;
        flockList.at(i).oldposition = flockList.at(i).position;
        flockList.at(i).position = flockList.at(i).position + flockList.at(i).velocity;
        flockList.at(i).direction = flockList.at(i).position - flockList.at(i).oldposition;
        avgDir = avgDir + flockList.at(i).direction;
    }
    
    avgDir = avgDir / flockPopulation;

    for(int i = 0; i < flockPopulation; i++) {
        vec3df olddir = flockList.at(i).direction;
        vec3df newdir = avgDir*100 - flockList.at(i).oldposition;
        flockList.at(i).avgdirection = avgDir*100;
        vec3df cross = crossproduct(olddir, newdir);
        flockList.at(i).rotation = cross;
        float radians = dotproduct(olddir, newdir);
        radians = acos(radians/(olddir.length()*newdir.length()));
        flockList.at(i).angle = radians*(180/PI);
    }
} 

void drawWing() {
    vec3df v1, v2, v3, v4, n;
    
    // Bottom face
    v1.x = -2.5; v1.y = -0.5; v1.z = -3.0;
    v2.x = 2.5; v2.y = -0.5; v2.z = -3.0;
    v3.x = 2.5; v3.y = -0.5; v3.z = 3.0;
    v4.x = -2.5; v4.y = -0.5; v4.z = 3.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Left face
    v1.x = -2.5; v1.y = -0.5; v1.z = -3.0;
    v2.x = -2.5; v2.y = 0.5; v2.z = -3.0;
    v3.x = -2.5; v3.y = 0.5; v3.z = 3.0;
    v4.x = -2.5; v4.y = -0.5; v4.z = 3.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Right face
    v1.x = 2.5; v1.y = -0.5; v1.z = -3.0;
    v2.x = 2.5; v2.y = 0.5; v2.z = -3.0;
    v3.x = 2.5; v3.y = 0.5; v3.z = 3.0;
    v4.x = 2.5; v4.y = -0.5; v4.z = 3.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Front face
    v1.x = -2.5; v1.y = -0.5; v1.z = 3.0;
    v2.x = -2.5; v2.y = 0.5; v2.z = 3.0;
    v3.x = 2.5; v3.y = 0.5; v3.z = 3.0;
    v4.x = 2.5; v4.y = -0.5; v4.z = 3.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Back face
    v1.x = -2.5; v1.y = -0.5; v1.z = -3.0;
    v2.x = -2.5; v2.y = 0.5; v2.z = -3.0;
    v3.x = 2.5; v3.y = 0.5; v3.z = -3.0;
    v4.x = 2.5; v4.y = -0.5; v4.z = -3.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Top face
    v1.x = -2.5; v1.y = 0.5; v1.z = -3.0;
    v2.x = 2.5; v2.y = 0.5; v2.z = -3.0;
    v3.x = 2.5; v3.y = 0.5; v3.z = 3.0;
    v4.x = -2.5; v4.y = 0.5; v4.z = 3.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
}

void drawBody() {    
    vec3df v1, v2, v3, v4, n;
    
    // Bottom face
    v1.x = -3.0; v1.y = -1.0; v1.z = -4.0;
    v2.x = 3.0; v2.y = -1.0; v2.z = -4.0;
    v3.x = 3.0; v3.y = -1.0; v3.z = 4.0;
    v4.x = -3.0; v4.y = -1.0; v4.z = 4.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Left face
    v1.x = -3.0; v1.y = -1.0; v1.z = -4.0;
    v2.x = -3.0; v2.y = 1.0; v2.z = -4.0;
    v3.x = -3.0; v3.y = 1.0; v3.z = 4.0;
    v4.x = -3.0; v4.y = -1.0; v4.z = 4.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Right face
    v1.x = 3.0; v1.y = -1.0; v1.z = -4.0;
    v2.x = 3.0; v2.y = 1.0; v2.z = -4.0;
    v3.x = 3.0; v3.y = 1.0; v3.z = 4.0;
    v4.x = 3.0; v4.y = -1.0; v4.z = 4.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Front face
    v1.x = -3.0; v1.y = -1.0; v1.z = 4.0;
    v2.x = -3.0; v2.y = 1.0; v2.z = 4.0;
    v3.x = 3.0; v3.y = 1.0; v3.z = 4.0;
    v4.x = 3.0; v4.y = -1.0; v4.z = 4.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Back face
    v1.x = -3.0; v1.y = -1.0; v1.z = -4.0;
    v2.x = -3.0; v2.y = 1.0; v2.z = -4.0;
    v3.x = 3.0; v3.y = 1.0; v3.z = -4.0;
    v4.x = 3.0; v4.y = -1.0; v4.z = -4.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Top face
    v1.x = -3.0; v1.y = 1.0; v1.z = -4.0;
    v2.x = 3.0; v2.y = 1.0; v2.z = -4.0;
    v3.x = 3.0; v3.y = 1.0; v3.z = 4.0;
    v4.x = -3.0; v4.y = 1.0; v4.z = 4.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
} 

void drawHead() {    
    vec3df v1, v2, v3, v4, n;
    
    // Bottom face
    v1.x = -3.0; v1.y = -1.0; v1.z = -2.0;
    v2.x = 3.0; v2.y = -1.0; v2.z = -2.0;
    v3.x = 3.0; v3.y = -1.0; v3.z = 2.0;
    v4.x = -3.0; v4.y = -1.0; v4.z = 2.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Left face
    v1.x = -3.0; v1.y = -1.0; v1.z = -2.0;
    v2.x = -3.0; v2.y = 1.0; v2.z = -2.0;
    v3.x = -3.0; v3.y = 1.0; v3.z = 0.0;
    v4.x = -3.0; v4.y = -1.0; v4.z = 2.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Right face
    v1.x = 3.0; v1.y = -1.0; v1.z = -2.0;
    v2.x = 3.0; v2.y = 1.0; v2.z = -2.0;
    v3.x = 3.0; v3.y = 1.0; v3.z = 0.0;
    v4.x = 3.0; v4.y = -1.0; v4.z = 2.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Front face
    v1.x = -3.0; v1.y = -1.0; v1.z = 2.0;
    v2.x = -3.0; v2.y = 1.0; v2.z = 0.0;
    v3.x = 3.0; v3.y = 1.0; v3.z = 0.0;
    v4.x = 3.0; v4.y = -1.0; v4.z = 2.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Back face
    v1.x = -3.0; v1.y = -1.0; v1.z = -2.0;
    v2.x = -3.0; v2.y = 1.0; v2.z = -2.0;
    v3.x = 3.0; v3.y = 1.0; v3.z = -2.0;
    v4.x = 3.0; v4.y = -1.0; v4.z = -2.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Top face
    v1.x = -3.0; v1.y = 1.0; v1.z = -2.0;
    v2.x = 3.0; v2.y = 1.0; v2.z = -2.0;
    v3.x = 3.0; v3.y = 1.0; v3.z = 0.0;
    v4.x = -3.0; v4.y = 1.0; v4.z = 0.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
}

void drawTail() {    
    vec3df v1, v2, v3, v4, n;
    
    // Bottom face
    v1.x = -3.0; v1.y = -0.5; v1.z = -2.0;
    v2.x = 3.0; v2.y = -0.5; v2.z = -2.0;
    v3.x = 3.0; v3.y = -0.5; v3.z = 2.0;
    v4.x = -3.0; v4.y = -0.5; v4.z = 2.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Left face
    v1.x = -3.0; v1.y = -0.5; v1.z = -2.0;
    v2.x = -3.0; v2.y = 0.5; v2.z = -2.0;
    v3.x = -3.0; v3.y = 0.5; v3.z = 2.0;
    v4.x = -3.0; v4.y = -0.5; v4.z = 2.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Right face
    v1.x = 3.0; v1.y = -0.5; v1.z = -2.0;
    v2.x = 3.0; v2.y = 0.5; v2.z = -2.0;
    v3.x = 3.0; v3.y = 0.5; v3.z = 2.0;
    v4.x = 3.0; v4.y = -0.5; v4.z = 2.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Front face
    v1.x = -3.0; v1.y = -0.5; v1.z = 2.0;
    v2.x = -3.0; v2.y = 0.5; v2.z = 2.0;
    v3.x = 3.0; v3.y = 0.5; v3.z = 2.0;
    v4.x = 3.0; v4.y = -0.5; v4.z = 2.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Back face
    v1.x = -3.0; v1.y = -0.5; v1.z = -2.0;
    v2.x = -3.0; v2.y = 0.5; v2.z = -2.0;
    v3.x = 3.0; v3.y = 0.5; v3.z = -2.0;
    v4.x = 3.0; v4.y = -0.5; v4.z = -2.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
    
    // Top face
    v1.x = -3.0; v1.y = 0.5; v1.z = -2.0;
    v2.x = 3.0; v2.y = 0.5; v2.z = -2.0;
    v3.x = 3.0; v3.y = 0.5; v3.z = 2.0;
    v4.x = -3.0; v4.y = 0.5; v4.z = 2.0;
    n = v1 + v2 + v3 + v4;
    n = n / 4.0;
    n = normalize(n);
    glNormal3f(n.x, n.y, n.z);
    if(lightIsEnabled) {
        glBegin(GL_POLYGON);
    }
    else {
        glBegin(GL_LINE_LOOP);
    }
    glVertex3f(v1.x, v1.y, v1.z);
    glVertex3f(v2.x, v2.y, v2.z);
    glVertex3f(v3.x, v3.y, v3.z);
    glVertex3f(v4.x, v4.y, v4.z);
    glEnd();
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
    materials(orangeMaterial);
    glPushMatrix();
    glTranslatef(ball.position.x, ball.position.y, ball.position.z);
    glRotatef(modelAngle, 0.0, 1.0, 0.0);
    glColor3f(0.0, 1.0, 0.0);
    drawBoid(0, false);
    glPopMatrix();
    
    vec3df avgPos;
    vec3df avgDir;
    
    // Drawing boids
    materials(blueMaterial);
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
    avgDir = avgDir * 100;
    
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
    pos_light();
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
            ball.position.x += -20;
            break;
        case SDLK_d:
            ball.position.x += 20;
            break;
        case SDLK_w:
            ball.position.y += 20;
            break;
        case SDLK_s:
            ball.position.y += -20;
            break;
        case SDLK_z:
            ball.position.z += 20;
            break;
        case SDLK_x:
            ball.position.z += -20;
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
            if(upperWingAngle >= 67.5) {
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
                if(flockList.at(i).upperWingAngle >= 67.5) {
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
    GW = 900;
    GH = 600;
    ball.position.x = ball.position.y = ball.position.z = 0.0;
    m1 = m3 = 1;
    m2 = 0;
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
    init_lighting();
    
    // Setup flock population
    setup(BOIDSCOUNT);
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