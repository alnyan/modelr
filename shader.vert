#version 330 core

layout(location = 0) in vec3 mVertex;
layout(location = 1) in vec2 mTexCoord;
layout(location = 2) in vec3 mNormal;
layout(location = 3) in vec3 mTangent;
layout(location = 4) in vec3 mBitangent;

uniform mat4 mProjectionMatrix;
uniform mat4 mCameraMatrix;
uniform mat4 mModelMatrix;

out vec3 mSourceVertex;
out vec3 mSourceNormal;
out vec2 mSourceTexCoord;
out vec3 mSourceTangent;
out vec3 mSourceBitangent;

void main() {
    mSourceVertex = (mModelMatrix * vec4(mVertex, 1.0)).xyz;
    mSourceNormal = mNormal;
    mSourceTexCoord = mTexCoord;
    mSourceTangent = mTangent;
    mSourceBitangent = mBitangent;

    gl_Position = mProjectionMatrix * mCameraMatrix * mModelMatrix * vec4(mVertex, 1.0);
}
