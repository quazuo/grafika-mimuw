#ifndef SHADER_H
#define SHADER_H

#include <filesystem>
#include <vector>
#include <map>

#include <GL/glew.h>
#include <glm/glm.hpp>

class GLShaders {
    std::map<std::string, GLint> uniformIDs {};

protected:
    GLuint programID {};

    GLShaders();

public:
    virtual ~GLShaders();

    GLShaders(const GLShaders& other) = delete;

    GLShaders& operator=(const GLShaders& other) = delete;

    GLuint getID() const { return programID; }

    void enable() const;

    void setUniform(const std::string& name, GLint value);

    void setUniform(const std::string& name, const glm::ivec2& value);

    void setUniform(const std::string& name, const glm::ivec3& value);

    void setUniform(const std::string& name, const glm::ivec4& value);

    void setUniform(const std::string& name, float value);

    void setUniform(const std::string& name, const glm::vec2& value);

    void setUniform(const std::string& name, const glm::vec3& value);

    void setUniform(const std::string& name, const glm::vec4& value);

    void setUniform(const std::string& name, const glm::mat4& value);

    void setUniform(const std::string& name, const std::vector<GLint>& value);

    void setUniform(const std::string& name, const std::vector<float>& value);

private:
    GLint getUniformID(const std::string &name);

protected:
    GLuint compileShader(GLuint shaderKind, const std::filesystem::path &path) const;

    void linkProgram() const;
};

class GLGraphicsShaders final : public GLShaders {
public:
    GLGraphicsShaders(const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath);

    virtual ~GLGraphicsShaders() = default;
};

class GLComputeShader final : public GLShaders {
public:
    GLComputeShader(const std::filesystem::path &shaderPath);

    virtual ~GLComputeShader() = default;
};

#endif //SHADER_H
