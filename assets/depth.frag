#version 430 core

layout(location = 0) out float fragDepth;
//out vec3 color;

void main() {
    fragDepth = gl_FragCoord.z;
    //color = vec3(gl_FragCoord.z, 0, 0);
}
