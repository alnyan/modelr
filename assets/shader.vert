#version 430 core
#extension GL_ARB_shader_draw_parameters : enable

layout(location = 0) in vec3 mVertex;
layout(location = 1) in vec2 mTexCoord;
layout(location = 2) in vec3 mNormal;
layout(location = 3) in vec3 mTangent;
layout(location = 4) in vec3 mBitangent;

layout(std140,binding=0) uniform mSceneParams {
    mat4 mProjectionMatrix;
    mat4 mCameraMatrix;
    vec4 mCameraPosition;
    vec4 mCameraDestination;
};

layout(std430,binding=1) buffer mModelParams {
    mat4 mModelMatrices[];
};

out vec3 mSourceVertex;
out vec3 mSourceNormal;
out vec2 mSourceTexCoord;
out vec3 mSourceTangent;
out vec3 mSourceBitangent;
out vec3 mClipSpacePos;

void main() {
    mat4 mModelMatrix = mModelMatrices[gl_DrawIDARB];

    mSourceVertex = (mModelMatrix * vec4(mVertex, 1.0)).xyz;
    mSourceNormal = mNormal;
    mSourceTexCoord = mTexCoord;
    mSourceTangent = mTangent;
    mSourceBitangent = mBitangent;

    gl_Position = mProjectionMatrix * mCameraMatrix * mModelMatrix * vec4(mVertex, 1.0);
    mClipSpacePos = gl_Position.xyz;
}
