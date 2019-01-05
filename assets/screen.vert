#version 420 core

layout(location = 0) in vec3 mVertex;
layout(location = 1) in vec2 mTexCoord;

out vec2 mSourceTexCoord;

void main() {
    mSourceTexCoord = mTexCoord;

    gl_Position = vec4(mVertex, 1);
}
