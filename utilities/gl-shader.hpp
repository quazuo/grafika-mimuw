#ifndef SHADER_H
#define SHADER_H

#include <filesystem>
#include <unordered_map>
#include <vector>
#include <map>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <sstream>

class GLShaders {
    GLuint programID;

    std::map<std::string, GLint> uniformIDs {};

public:
    GLShaders(const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath);

    GLuint getID() const { return programID; }

    void enable() const;

    void setUniform(const std::string& name, GLint value);

    void setUniform(const std::string& name, float value);

    void setUniform(const std::string& name, const glm::vec2& value);

    void setUniform(const std::string& name, const glm::vec3& value);

    void setUniform(const std::string& name, const glm::vec4& value);

    void setUniform(const std::string& name, const glm::mat4& value);

    void setUniform(const std::string& name, const std::vector<GLint>& value);

    void setUniform(const std::string& name, const std::vector<float>& value);

private:
    GLint getUniformID(const std::string &name);

    GLuint compileShader(GLuint shaderKind, const std::filesystem::path &path) const;

    void linkProgram(GLuint vertexShader, GLuint fragmentShader);
};

#endif //SHADER_H
