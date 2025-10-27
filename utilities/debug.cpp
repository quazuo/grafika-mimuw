#include "debug.hpp"

#include <sstream>
#include <iostream>

void debugCallback(const GLenum source, const GLenum type, const GLuint id, const GLenum severity,
                   const GLsizei length, const GLchar *message, const void *userParam) {
    std::stringstream ss;
    ss << "[ERROR]\n\tsource: ";

    switch (source) {
        case GL_DEBUG_SOURCE_API:
            ss << "API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            ss << "WINDOW_SYSTEM";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            ss << "SHADER_COMPILER";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            ss << "THIRD_PARTY";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            ss << "APPLICATION";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            ss << "OTHER";
            break;
        default:
            break;
    }

    ss << "\n\ttype: ";

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            ss << "ERROR";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            ss << "DEPRECATED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            ss << "UNDEFINED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            ss << "PORTABILITY";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            ss << "PERFORMANCE";
            break;
        case GL_DEBUG_TYPE_MARKER:
            ss << "MARKER";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            ss << "PUSH_GROUP";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            ss << "POP_GROUP";
            break;
        case GL_DEBUG_TYPE_OTHER:
            ss << "OTHER";
            break;
        default:
            break;
    }

    ss << "\n\tid: " << id;
    ss << "\n\tseverity: ";

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            ss << "HIGH";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            ss << "MEDIUM";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            ss << "LOW";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            ss << "NOTIFICATION";
            break;
        default:
            break;
    }

    ss << "\n\tmessage: " << message << "\n";

    std::cout << ss.str();

    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
        throw std::runtime_error(ss.str());
    }
}
