#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <cstdlib>

const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    uniform vec2 position;
    uniform float angle;
    void main() {
        float c = cos(angle);
        float s = sin(angle);
        vec2 rotated = vec2(
            aPos.x * c - aPos.y * s,
            aPos.x * s + aPos.y * c
        );
        gl_Position = vec4(rotated + position, 0.0, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
)";

// Structure qui représente un boid
struct Boid {
    float px, py; // position
    float vx, vy; // vitesse
};

// Crée un boid avec position et vitesse aléatoires
Boid createRandomBoid() {
    Boid b;
    b.px = ((float)rand() / RAND_MAX) * 2.0f - 1.0f; // entre -1 et 1
    b.py = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    b.vx = ((float)rand() / RAND_MAX) * 0.6f - 0.3f; // entre -0.3 et 0.3
    b.vy = ((float)rand() / RAND_MAX) * 0.6f - 0.3f;
    return b;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Boids", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glViewport(0, 0, 800, 600);

    // Compilation shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float vertices[] = {
        0.0f,  0.02f,
       -0.01f, -0.02f,
        0.01f, -0.02f
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Création de 50 boids aléatoires
    int nbBoids = 50;
    std::vector<Boid> boids(nbBoids);
    for (int i = 0; i < nbBoids; i++)
        boids[i] = createRandomBoid();

    float lastTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float currentTime = (float)glfwGetTime();
        float dt = currentTime - lastTime;
        lastTime = currentTime;

        // Mise à jour de chaque boid
        for (auto& b : boids) {
            b.px += b.vx * dt;
            b.py += b.vy * dt;
            if (b.px > 1.0f || b.px < -1.0f) b.vx = -b.vx;
            if (b.py > 1.0f || b.py < -1.0f) b.vy = -b.vy;
        }

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Dessin de chaque boid
        for (const auto& b : boids) {
            float angle = atan2f(b.vx, b.vy);
            glUniform2f(glGetUniformLocation(shaderProgram, "position"), b.px, b.py);
            glUniform1f(glGetUniformLocation(shaderProgram, "angle"), angle);
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
