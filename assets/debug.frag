#version 430 core

layout(location = 1) in vec3 mColor;

out vec3 color;

void main() {
    color = mColor;
}
