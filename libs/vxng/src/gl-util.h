#pragma once

#include <GL/glew.h>

// Shader loading utility programs

// From "SDL and Modern OpenGL" by LazyFoo
// https://lazyfoo.net/tutorials/SDL/51_SDL_and_modern_opengl/index.php

void printProgramLog(GLuint program);
void printShaderLog(GLuint shader);

typedef struct {
    GLuint count;
    GLuint instanceCount;
    GLuint firstIndex;
    int baseVertex;
    GLuint baseInstance;
} DrawElementsIndirectCommand;
