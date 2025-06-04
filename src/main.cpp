#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Sprite.h"

class Shader {
public:
    Shader(const char* vertexSrc, const char* fragmentSrc) {
        GLuint v = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(v, 1, &vertexSrc, nullptr);
        glCompileShader(v);
        GLint ok;
        glGetShaderiv(v, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char info[1024];
            glGetShaderInfoLog(v, 1024, nullptr, info);
            std::cout << "Vertex SHADER COMPILATION ERROR:\n" << info << std::endl;
        }

        GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(f, 1, &fragmentSrc, nullptr);
        glCompileShader(f);
        glGetShaderiv(f, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char info[1024];
            glGetShaderInfoLog(f, 1024, nullptr, info);
            std::cout << "Fragment SHADER COMPILATION ERROR:\n" << info << std::endl;
        }

        _id = glCreateProgram();
        glAttachShader(_id, v);
        glAttachShader(_id, f);
        glLinkProgram(_id);
        glGetProgramiv(_id, GL_LINK_STATUS, &ok);
        if (!ok) {
            char info[1024];
            glGetProgramInfoLog(_id, 1024, nullptr, info);
            std::cout << "SHADER PROGRAM LINK ERROR:\n" << info << std::endl;
        }

        glDeleteShader(v);
        glDeleteShader(f);
    }

    ~Shader() {
        glDeleteProgram(_id);
    }

    void use() const {
        glUseProgram(_id);
    }

    GLuint id() const {
        return _id;
    }

    void setMat4(const char* name, const glm::mat4& matrix) const {
        GLint loc = glGetUniformLocation(_id, name);
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(matrix));
    }

private:
    GLuint _id;
};

static const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
uniform mat4 model;
uniform mat4 projection;
void main() {
    gl_Position = projection * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

static const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D spriteTexture;
void main() {
    FragColor = texture(spriteTexture, TexCoord);
}
)";

GLuint loadTexture(const char* path, int* width, int* height) {
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    stbi_set_flip_vertically_on_load(true);

    int nrChannels;
    unsigned char* data = stbi_load(path, width, height, &nrChannels, 0);
    if (!data) {
        std::cout << "Erro ao carregar: " << path << std::endl;
        return 0;
    }
    GLenum format = (nrChannels == 4 ? GL_RGBA : GL_RGB);
    glTexImage2D(GL_TEXTURE_2D, 0, format, *width, *height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

Sprite::Sprite(GLuint textureID, Shader* shader)
    : _textureID(textureID), _shader(shader)
{
    initRenderData();
}

Sprite::~Sprite() {
    glDeleteVertexArrays(1, &_VAO);
    glDeleteBuffers(1, &_VBO);
}

void Sprite::setPosition(const glm::vec2& p) {
    _position = p;
}

void Sprite::setScale(const glm::vec2& s) {
    _scale = s;
}

void Sprite::setRotation(float r) {
    _rotation = r;
}

void Sprite::initRenderData() {
    float vertices[] = {
        -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.0f,  1.0f, 0.0f
    };

    glGenVertexArrays(1, &_VAO);
    glGenBuffers(1, &_VBO);
    glBindVertexArray(_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, _VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        5 * sizeof(float),
        (void*)0
    );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE,
        5 * sizeof(float),
        (void*)(3 * sizeof(float))
    );

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Sprite::draw(const glm::mat4& projection) const {
    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(_position, 0.0f));
    model = glm::translate(model, glm::vec3(0.5f * _scale.x, 0.5f * _scale.y, 0.0f));
    model = glm::rotate(model, glm::radians(_rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(-0.5f * _scale.x, -0.5f * _scale.y, 0.0f));
    model = glm::scale(model, glm::vec3(_scale, 1.0f));

    _shader->use();
    _shader->setMat4("projection", projection);
    _shader->setMat4("model", model);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _textureID);
    glUniform1i(glGetUniformLocation(_shader->id(), "spriteTexture"), 0);

    glBindVertexArray(_VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

int main() {
    if (!glfwInit()) {
        std::cout << "Falha ao inicializar GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Cena com Sprites", nullptr, nullptr);
    if (!window) {
        std::cout << "Falha ao criar janela GLFW\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Falha ao inicializar GLAD\n";
        glfwTerminate();
        return -1;
    }
    glViewport(0, 0, 800, 600);

    Shader shader(vertexShaderSource, fragmentShaderSource);

    int widthBG, heightBG, widthChar, heightChar;
    GLuint texBG   = loadTexture("../assets/tex/1.png", &widthBG, &heightBG);
    GLuint texChar = loadTexture("../assets/sprites/waterbear.png", &widthChar, &heightChar);
    if (!texBG || !texChar) {
        std::cout << "Falha ao carregar alguma textura\n";
        glfwTerminate();
        return -1;
    }

    glm::mat4 projection = glm::ortho(
        0.0f, 800.0f,
        0.0f, 600.0f,
       -1.0f,  1.0f
    );

    Sprite bg(texBG, &shader);
    Sprite character(texChar, &shader);

    float windowW = 800.0f;
    float windowH = 600.0f;

    // Background ocupa a tela inteira
    bg.setScale({windowW, windowH});
    bg.setPosition({windowW / 2.0f, windowH / 2.0f});

    // Boneco centralizado na tela
    character.setScale({(float)widthChar, (float)heightChar});
    character.setPosition({windowW / 2.0f, windowH / 2.0f});
    character.setRotation(0.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        bg.draw(projection);
        character.draw(projection);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteTextures(1, &texBG);
    glDeleteTextures(1, &texChar);
    glfwTerminate();
    return 0;
}