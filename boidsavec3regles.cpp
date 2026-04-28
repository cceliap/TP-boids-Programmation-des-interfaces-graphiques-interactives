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

struct Boid {
    float px, py;
    float vx, vy;
};

Boid createRandomBoid() {
    Boid b;
    b.px = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    b.py = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    float angle = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f;
    float speed = 0.2f + ((float)rand() / RAND_MAX) * 0.2f;
    b.vx = cosf(angle) * speed;
    b.vy = sinf(angle) * speed;
    return b;
}

// Limite la vitesse d'un boid à maxSpeed
void limitSpeed(Boid& b, float maxSpeed) {
    float speed = sqrtf(b.vx * b.vx + b.vy * b.vy);
    if (speed > maxSpeed) {
        b.vx = (b.vx / speed) * maxSpeed;
        b.vy = (b.vy / speed) * maxSpeed;
    }
}

// Vérifie si le voisin j est visible depuis i (angle mort dans le dos)
bool isVisible(const Boid& i, const Boid& j, float blindAngle) {
    float dx = j.px - i.px;
    float dy = j.py - i.py;
    float angleToJ = atan2f(dy, dx);
    float myAngle = atan2f(i.vy, i.vx);
    float diff = angleToJ - myAngle;
    // Normalise entre -PI et PI
    while (diff > 3.14159f) diff -= 2.0f * 3.14159f;
    while (diff < -3.14159f) diff += 2.0f * 3.14159f;
    return fabsf(diff) < (3.14159f - blindAngle / 2.0f);
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

    // Paramètres des zones (modifiables plus tard via curseurs)
    float rRepulsion = 0.05f;  // zone de répulsion
    float rAlignment = 0.15f;  // zone d'alignement
    float rCohesion = 0.30f;  // zone de cohésion
    float blindAngle = 1.0f;   // angle mort (radians)
    float maxSpeed = 0.5f;   // vitesse max
    float mass = 1.0f;

    int nbBoids = 50;
    std::vector<Boid> boids(nbBoids);
    for (int i = 0; i < nbBoids; i++)
        boids[i] = createRandomBoid();

    float lastTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float currentTime = (float)glfwGetTime();
        float dt = currentTime - lastTime;
        lastTime = currentTime;

        // Calcul des forces pour chaque boid
        std::vector<Boid> newBoids = boids;

        for (int i = 0; i < nbBoids; i++) {
            Boid& b = boids[i];

            // Forces
            float fRepX = 0, fRepY = 0;  // séparation
            float fAliX = 0, fAliY = 0;  // alignement
            float fCohX = 0, fCohY = 0;  // cohésion

            int countRep = 0, countAli = 0, countCoh = 0;

            for (int j = 0; j < nbBoids; j++) {
                if (i == j) continue;
                Boid& n = boids[j];

                float dx = n.px - b.px;
                float dy = n.py - b.py;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist == 0) continue;

                if (!isVisible(b, n, blindAngle)) continue;

                if (dist < rRepulsion) {
                    // Séparation : force inverse, plus forte si proche
                    fRepX += -(dx / dist) * (1.0f / dist);
                    fRepY += -(dy / dist) * (1.0f / dist);
                    countRep++;
                }
                else if (dist < rAlignment) {
                    // Alignement : adopter la vitesse moyenne
                    fAliX += n.vx;
                    fAliY += n.vy;
                    countAli++;
                }
                else if (dist < rCohesion) {
                    // Cohésion : aller vers le centre
                    fCohX += dx;
                    fCohY += dy;
                    countCoh++;
                }
            }

            float ax = 0, ay = 0; // accélération totale

            if (countRep > 0) {
                ax += (fRepX / countRep) * 0.5f;
                ay += (fRepY / countRep) * 0.5f;
            }
            else if (countAli > 0) {
                float vaX = fAliX / countAli;
                float vaY = fAliY / countAli;
                ax += (vaX - b.vx) / dt * 0.1f;
                ay += (vaY - b.vy) / dt * 0.1f;
            }

            if (countCoh > 0) {
                ax += (fCohX / countCoh) * 0.3f;
                ay += (fCohY / countCoh) * 0.3f;
            }

            // Mise à jour vitesse et position
            newBoids[i].vx = b.vx + ax * dt;
            newBoids[i].vy = b.vy + ay * dt;
            limitSpeed(newBoids[i], maxSpeed);

            newBoids[i].px = b.px + newBoids[i].vx * dt;
            newBoids[i].py = b.py + newBoids[i].vy * dt;

            // Rebond sur les bords
            if (newBoids[i].px > 1.0f || newBoids[i].px < -1.0f) newBoids[i].vx = -newBoids[i].vx;
            if (newBoids[i].py > 1.0f || newBoids[i].py < -1.0f) newBoids[i].vy = -newBoids[i].vy;
        }

        boids = newBoids;

        // Dessin
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);

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
