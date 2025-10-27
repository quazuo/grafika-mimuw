#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "GL/glew.h"
#include "GLFW/glfw3.h"

void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                   const GLchar *message, const void *userParam);

#endif //DEBUG_HPP
