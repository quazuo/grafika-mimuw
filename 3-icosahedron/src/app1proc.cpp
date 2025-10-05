// #include <stdlib.h>
// #include <stdio.h>
// #include <ctype.h>
// #include "utilities/openglheader.h"
//
// #include "utilities/utilities.h"
// #include "app1proc.hpp"
//
// GLuint program_id;
// GLuint trbi, trbuf;
// GLint trbsize, trbofs[3];
// int option = 2;
//
// GLuint icos_vao, icos_vbo[3];
//
// void LoadMyShaders(void) {
//     static const char *filename[] =
//             {"main.vert", "main.frag"};
//     static const GLchar *UTBNames[] =
//             {"TransBlock", "TransBlock.mm", "TransBlock.vm", "TransBlock.pm"};
//     GLuint shader_id[2];
//     GLuint ind[3];
//     int i;
//
//     shader_id[0] = CompileShaderFiles(GL_VERTEX_SHADER, 1, &filename[0]);
//     shader_id[1] = CompileShaderFiles(GL_FRAGMENT_SHADER, 1, &filename[1]);
//     program_id   = LinkShaderProgram(2, shader_id, NULL);
//     trbi         = glGetUniformBlockIndex(program_id, UTBNames[0]);
//     glGetActiveUniformBlockiv(program_id, trbi,
//                               GL_UNIFORM_BLOCK_DATA_SIZE, &trbsize);
//     glGetUniformIndices(program_id, 3, &UTBNames[1], ind);
//     glGetActiveUniformsiv(program_id, 3, ind, GL_UNIFORM_OFFSET, trbofs);
//     glUniformBlockBinding(program_id, trbi, 0);
//     glGenBuffers(1, &trbuf);
//     glBindBufferBase(GL_UNIFORM_BUFFER, 0, trbuf);
//     glBufferData(GL_UNIFORM_BUFFER, trbsize, NULL, GL_DYNAMIC_DRAW);
//     for (i = 0; i < 2; i++)
//         glDeleteShader(shader_id[i]);
//     ExitIfGLError("LoadMyShaders");
// } /*LoadMyShaders*/
//
// void InitModelMatrix() {
//     GLfloat m[16];
//
//     M4x4Identf(m);
//     glBindBuffer(GL_UNIFORM_BUFFER, trbuf);
//     glBufferSubData(GL_UNIFORM_BUFFER, trbofs[0], 16 * sizeof(GLfloat), m);
//     ExitIfGLError("InitModelMatrix");
// }
//
// void InitViewMatrix() {
//     GLfloat m[16];
//
//     M4x4Translatef(m, 0.0, 0.0, -10.0);
//     glBindBuffer(GL_UNIFORM_BUFFER, trbuf);
//     glBufferSubData(GL_UNIFORM_BUFFER, trbofs[1], 16 * sizeof(GLfloat), m);
//     ExitIfGLError("InitViewMatrix");
// }
//
// void ConstructIcosahedronVAO() {
// #define A 0.52573115
// #define B 0.85065085
//     static const GLfloat vertpos[12][3] =
//     {
//         {-A, 0.0, -B}, {A, 0.0, -B}, {0.0, -B, -A}, {-B, -A, 0.0},
//         {-B, A, 0.0}, {0.0, B, -A}, {A, 0.0, B}, {-A, 0.0, B},
//         {0.0, -B, A}, {B, -A, 0.0}, {B, A, 0.0}, {0.0, B, A}
//     };
//     static const GLubyte vertcol[12][3] =
//     {
//         {255, 0, 0}, {255, 127, 0}, {255, 255, 0}, {127, 255, 0}, {0, 255, 0}, {0, 255, 127},
//         {0, 255, 255}, {0, 127, 255}, {0, 0, 255}, {127, 0, 255}, {255, 0, 255}, {255, 0, 127}
//     };
//     static const GLubyte vertind[62] =
//     {
//         0, 1, 2, 0, 3, 4, 0, 5, 1, 9, 2, 8, 3, /* lamana, od 0 */
//         7, 4, 11, 5, 10, 9, 6, 8, 7, 6, 11, 7,
//         1, 10, 6,                 /* lamana, od 25 */
//         2, 3, 4, 5, 8, 9, 10, 11, /* 4 odcinki, od 28 */
//         0, 1, 2, 3, 4, 5, 1,      /* wachlarz, od 36 */
//         6, 7, 8, 9, 10, 11, 7,    /* wachlarz, od 43  */
//         1, 9, 2, 8, 3, 7, 4, 11, 5, 10, 1, 9
//     }; /* tasma, od 50 */
//
//     glGenVertexArrays(1, &icos_vao);
//     glBindVertexArray(icos_vao);
//     glGenBuffers(3, icos_vbo);
//     glBindBuffer(GL_ARRAY_BUFFER, icos_vbo[0]);
//     glBufferData(GL_ARRAY_BUFFER,
//                  12 * 3 * sizeof(GLfloat), vertpos, GL_STATIC_DRAW);
//     glEnableVertexAttribArray(0);
//     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
//                           3 * sizeof(GLfloat), (GLvoid *) 0);
//     glBindBuffer(GL_ARRAY_BUFFER, icos_vbo[1]);
//     glBufferData(GL_ARRAY_BUFFER,
//                  12 * 3 * sizeof(GLubyte), vertcol, GL_STATIC_DRAW);
//     glEnableVertexAttribArray(1);
//     glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE,
//                           3 * sizeof(GLubyte), (GLvoid *) 0);
//     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, icos_vbo[2]);
//     glBufferData(GL_ELEMENT_ARRAY_BUFFER,
//                  62 * sizeof(GLubyte), vertind, GL_STATIC_DRAW);
//     glBindVertexArray(0);
//     ExitIfGLError("ConstructIcosahedronVAO");
// }
//
// void InitMyWorld(int argc, char *argv[], int width, int height) {
//     LoadMyShaders();
//     InitModelMatrix();
//     InitViewMatrix();
//     ConstructIcosahedronVAO();
//     ResizeMyWorld(width, height);
// }
//
// void DrawIcosahedron(int opt) {
//     glUseProgram(program_id);
//     glBindVertexArray(icos_vao);
//     switch (opt) {
//         case 0: /* wierzcholki */
//             glPointSize(5.0);
//             glDrawArrays(GL_POINTS, 0, 12);
//             break;
//         case 1: /* krawedzie */
//             glDrawElements(GL_LINE_STRIP, 25,
//                            GL_UNSIGNED_BYTE, (GLvoid *) 0);
//             glDrawElements(GL_LINE_STRIP, 3,
//                            GL_UNSIGNED_BYTE, (GLvoid *) (25 * sizeof(GLubyte)));
//             glDrawElements(GL_LINES, 8,
//                            GL_UNSIGNED_BYTE, (GLvoid *) (28 * sizeof(GLubyte)));
//             break;
//         default: /* sciany */
//             glDrawElements(GL_TRIANGLE_FAN, 7,
//                            GL_UNSIGNED_BYTE, (GLvoid *) (36 * sizeof(GLubyte)));
//             glDrawElements(GL_TRIANGLE_FAN, 7,
//                            GL_UNSIGNED_BYTE, (GLvoid *) (43 * sizeof(GLubyte)));
//             glDrawElements(GL_TRIANGLE_STRIP, 12,
//                            GL_UNSIGNED_BYTE, (GLvoid *) (50 * sizeof(GLubyte)));
//             break;
//     }
//     glBindVertexArray(0);
//     ExitIfGLError("DrawIcosahdron");
// }
//
// void ResizeMyWorld(int width, int height) {
//     GLfloat m[16];
//     float lr;
//
//     glViewport(0, 0, width, height); /* klatka jest calym oknem */
//     lr = 0.5533 * (float) width / (float) height;
//     M4x4Frustumf(m, NULL, -lr, lr, -0.5533, 0.5533, 5.0, 15.0);
//     glBindBuffer(GL_UNIFORM_BUFFER, trbuf);
//     glBufferSubData(GL_UNIFORM_BUFFER, trbofs[2], 16 * sizeof(GLfloat), m);
//     ExitIfGLError("ResizeMyWorld");
// } /*ResizeMyWorld*/
//
// void RedrawMyWorld(void) {
//     glClearColor(1.0, 1.0, 1.0, 1.0);
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//     glEnable(GL_DEPTH_TEST);
//     DrawIcosahedron(option);
//     glFlush();
//     ExitIfGLError("RedrawMyWorld");
// } /*RedrawMyWorld*/
//
// void DeleteMyWorld(void) {
//     glUseProgram(0);
//     glDeleteProgram(program_id);
//     glDeleteBuffers(1, &trbuf);
//     glDeleteVertexArrays(1, &icos_vao);
//     glDeleteBuffers(3, icos_vbo);
// #ifdef DEBUG_BUFFERS_ALLOCATION
//   DumpBufferIdentifiers ();
// #endif
//     ExitIfGLError("DeleteMyWorld");
// } /*DeleteMyWorld*/
//
// char ProcessCharCommand(char charcode) {
//     int oldoption;
//
//     oldoption = option;
//     switch (toupper(charcode)) {
//         case 'W': option = 0;
//             break; /* przelaczamy na wierzcholki */
//         case 'K': option = 1;
//             break; /* przelaczamy na krawedzie */
//         case 'S': option = 2;
//             break;             /* przelaczamy na sciany */
//         default: return false; /* ignorujemy wszystkie inne klawisze */
//     }
//     return option != oldoption;
// } /*ProcessCharCommand*/
