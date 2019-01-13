#version 430 core

layout(location=S_ATTRIB_VERTEX) in vec3 mVertex;

layout(std140,binding=2) uniform mLight0Buffer {
    mat4 mLightProjection[4];
    mat4 mLightMatrix;
    vec4 mLight0Pos;
};

uniform int mRenderCascade;
uniform mat4 mModelMatrix;
uniform float mLifetime;

void main() {
    float scaleFactor = mLifetime * 0.5 + 0.1;
    mat3 scale = mat3(
        vec3(scaleFactor, 0, 0),
        vec3(0, scaleFactor, 0),
        vec3(0, 0, scaleFactor)
    );

    vec3 camUp = vec3(mLightMatrix[0].y, mLightMatrix[1].y, mLightMatrix[2].y);
    vec3 camRight = vec3(mLightMatrix[0].x, mLightMatrix[1].x, mLightMatrix[2].x);

    vec4 vew = mModelMatrix * vec4(scale * (camRight * mVertex.x + camUp * mVertex.y), 1);

    gl_Position = mLightProjection[mRenderCascade] * mLightMatrix * vew;
}
