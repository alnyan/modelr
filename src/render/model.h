#pragma once
#include <GL/glew.h>
#include <vector>

struct Part {
    GLuint begin, size;
    int materialIndex;
};

struct Model {
    std::vector<Part> parts;
    //GLuint begin, size;
    //int materialIndex;
};
