#version 420 core

in vec2 mSourceTexCoord;
out vec3 color;

uniform sampler2D mTexture;
uniform sampler2D mDepthTexture;
uniform float mTime;

uniform mat4 mProjectionMatrix;
uniform mat4 mCameraMatrix;

uniform vec4 mScreenDimensions;

void main() {
    float d = texture2D(mDepthTexture, mSourceTexCoord).x;
    float d_n = 2.0 * d - 1.0;
    float d_e = 2.0 * 0.1f * 100.0f / (100.0f + 0.1f - d_n * (100.0f - 0.1f));
    //color = texture(mDepthTexture, mSourceTexCoord).rgb;
    color = vec3(d_e / 100);
    color = texture(mTexture, mSourceTexCoord).rgb;;
}
