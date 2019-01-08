#version 430 core

layout(location = 0) in vec3 mPosition;

//uniform mat4 mProjectionMatrix;
//uniform mat4 mCameraMatrix;
layout(std140,binding=0) uniform mSceneParams {
    mat4 mProjectionMatrix;
    mat4 mCameraMatrix;
    vec4 mCameraPosition;
    vec4 mCameraDestination;
};
layout(std430,binding=1) buffer mModelParams {
    mat4 mModelMatrices[];
};

void main() {
    gl_Position = mProjectionMatrix * mCameraMatrix * mModelMatrices[gl_InstanceID] * vec4(mPosition, 1);
}
