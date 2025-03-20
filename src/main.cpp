#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <iostream>
#include "../include/model/Node.hpp"
#include "../include/model/Beam.hpp"
#include "../include/physics/Simulator.hpp" // Add to existing includes
#include "../include/data/CSVHandler.hpp"

// Global structures
std::vector<Node> nodes;
std::vector<Beam> beams;

// Load test structure
void loadTestStructure() {
    nodes.emplace_back(0.0f, 0.0f, 0.0f).setFixed(true);
    nodes.emplace_back(2.0f, 0.0f, 0.0f);
    beams.emplace_back(&nodes[0], &nodes[1], 2e11f, 0.01f); // Steel beam
}

// Function to draw a sphere
void drawSphere(const glm::vec3& position, float radius) {
    // Implement sphere drawing logic here
    // This is a placeholder implementation
    glPushMatrix();
    glTranslatef(position.x, position.y, position.z);
    // Draw sphere using OpenGL functions
    // For example, using gluSphere or a custom sphere drawing function
    glPopMatrix();
}

// Function to draw a cylinder
void drawCylinder(const glm::vec3& start, const glm::vec3& end, float radius) {
    // Implement cylinder drawing logic here
    // This is a placeholder implementation
    glPushMatrix();
    // Calculate the cylinder's position and orientation
    // Draw cylinder using OpenGL functions
    glPopMatrix();
}

// Shader sources (will move to separate files later)
const char* vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(0.8, 0.3, 0.2, 1.0); // Orange color
}
)glsl";

int main() {
    // SDL2 Initialization
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Window Creation
    SDL_Window* window = SDL_CreateWindow("C_Structures", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        800, 600, 
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    // OpenGL Context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GLContext glContext = SDL_GL_CreateContext(window);

    // GLEW Initialization
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW init failed!" << std::endl;
        return 1;
    }

    // Shader Compilation
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Load test structure
    loadTestStructure();

    // Create physics engine instance
    Simulator physicsEngine(nodes, beams); // Create physics engine instance

    // Apply test force (example)
    nodes[1].applyForce(glm::vec3(1000, 0, 0)); 

    // Solve and update
    physicsEngine.solveStaticForces(); // Solve static forces

    // Main loop
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
        }

        // Clear screen
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render nodes
        for (const auto& node : nodes) {
            glm::vec3 disp = physicsEngine.getNodeDisplacements()[&node - &nodes[0]]; // Get displacements
            drawSphere(node.getPosition() + disp, 0.1f); // Implement this
        }

        // Render beams with force visualization
        for (const auto& beam : beams) {
            float force = physicsEngine.getBeamForce(beam); // Add this method to Simulator
            glm::vec3 color = ForceRenderer::getBeamColor(force, ForceRenderer::MAX_STRESS);
            
            drawCylinder(beam.getStart()->getPosition(), 
                        beam.getEnd()->getPosition(), 
                        0.05f, 
                        color); // Use the color based on the force
        }

        // Render force vectors
        ForceRenderer::renderForceVectors(nodes);
        }
        glUseProgram(shaderProgram);
        
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
