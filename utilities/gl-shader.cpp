#include "gl-shader.hpp"

#include <fstream>
#include <iostream>

GLShaders::GLShaders(const std::filesystem::path &vertexShaderPath, const std::filesystem::path &fragmentShaderPath) {
    programID = glCreateProgram();
    const GLuint vertexShaderID = compileShader(GL_VERTEX_SHADER, vertexShaderPath);
    const GLuint fragmentShaderID = compileShader(GL_FRAGMENT_SHADER, fragmentShaderPath);
    linkProgram(vertexShaderID, fragmentShaderID);
    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);
}

void GLShaders::enable() const {
    glUseProgram(programID);
}

void GLShaders::setUniform(const std::string &name, const GLint value) {
    const GLint uniformID = getUniformID(name);
    glUniform1i(uniformID, value);
}

void GLShaders::setUniform(const std::string &name, const float value) {
    const GLint uniformID = getUniformID(name);
    glUniform1f(uniformID, value);
}

void GLShaders::setUniform(const std::string &name, const glm::vec2 &value) {
    const GLint uniformID = getUniformID(name);
    glUniform2f(uniformID, value.x, value.y);
}

void GLShaders::setUniform(const std::string &name, const glm::vec3 &value) {
    const GLint uniformID = getUniformID(name);
    glUniform3f(uniformID, value.x, value.y, value.z);
}

void GLShaders::setUniform(const std::string &name, const glm::vec4 &value) {
    const GLint uniformID = getUniformID(name);
    glUniform4f(uniformID, value.x, value.y, value.z, value.w);
}

void GLShaders::setUniform(const std::string &name, const glm::mat4 &value) {
    const GLint uniformID = getUniformID(name);
    glUniformMatrix4fv(uniformID, 1, GL_FALSE, &value[0][0]);
}

void GLShaders::setUniform(const std::string &name, const std::vector<GLint> &value) {
    const GLint uniformID = getUniformID(name);
    glUniform1iv(uniformID, static_cast<GLint>(value.size()), value.data());
}

void GLShaders::setUniform(const std::string &name, const std::vector<float> &value) {
    const GLint uniformID = getUniformID(name);
    glUniform1fv(uniformID, static_cast<GLint>(value.size()), value.data());
}

GLint GLShaders::getUniformID(const std::string &name) {
    const auto it = uniformIDs.find(name);
    if (it != uniformIDs.end()) {
        return it->second;
    }

    const GLint id = glGetUniformLocation(programID, name.c_str());
    if (id == -1) {
        throw std::runtime_error("failed to get uniform with name: " + name);
    }

    uniformIDs.emplace(name, id);
    return id;
}

GLuint GLShaders::compileShader(const GLuint shaderKind, const std::filesystem::path &path) const {
    // create the shaders
    GLuint shaderID = glCreateShader(shaderKind);

    // read the shader code from the file
    std::ifstream shaderStream(path, std::ios::in);
    if (!shaderStream.is_open()) {
        const std::string errorMessage = "Impossible to open shader file: " + absolute(path).string();
        std::cerr << errorMessage;
        throw std::runtime_error(errorMessage);
    }

    std::stringstream sstr;
    sstr << shaderStream.rdbuf();
    std::string shaderCode = sstr.str();
    shaderStream.close();

    GLint result = GL_FALSE;
    int infoLogLength;

    // compile the shader
    std::cout << "Compiling shader: " << path.filename() << "\n";
    char const *vertexSourcePointer = shaderCode.c_str();
    glShaderSource(shaderID, 1, &vertexSourcePointer, nullptr);
    glCompileShader(shaderID);

    // check the shader
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
        std::vector<char> shaderErrorMessage(infoLogLength + 1);
        glGetShaderInfoLog(shaderID, infoLogLength, nullptr, &shaderErrorMessage[0]);
        const std::string errorMessage = "shader compilation failed: " + std::string(&shaderErrorMessage[0]);
        std::cerr << errorMessage;
        throw std::runtime_error(errorMessage);
    }

    return shaderID;
}

void GLShaders::linkProgram(const GLuint vertexShader, const GLuint fragmentShader) {
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);

    // check the program
    GLint result = GL_FALSE;
    int infoLogLength;

    glGetProgramiv(programID, GL_LINK_STATUS, &result);
    glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength > 0) {
        std::vector<char> programErrorMessage(infoLogLength + 1);
        glGetProgramInfoLog(programID, infoLogLength, nullptr, &programErrorMessage[0]);
        const std::string errorMessage = "shader linking failed: " + std::string(&programErrorMessage[0]);
        std::cerr << errorMessage;
        throw std::runtime_error(errorMessage);
    }
}
