#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


class Shader;

class Sprite {
public:
    Sprite(GLuint textureID, Shader* shader);

    ~Sprite();

    void setPosition(const glm::vec2& p);
    void setScale   (const glm::vec2& s);
    void setRotation(float r);

    void draw(const glm::mat4& projection) const;

private:
    GLuint    _VAO, _VBO;
    GLuint    _textureID;
    Shader*   _shader;
    glm::vec2 _position  = glm::vec2(0.0f, 0.0f);
    glm::vec2 _scale     = glm::vec2(1.0f, 1.0f);
    float     _rotation  = 0.0f;

    void initRenderData();
};
