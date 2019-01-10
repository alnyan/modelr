#version 430 core
#extension GL_ARB_bindless_texture : require

in vec2 mSourceTexCoord;
out vec3 color;

uniform sampler2D mTexture;
uniform sampler2D mDepthTexture;
uniform float mTime;

uniform mat4 mProjectionMatrix;
uniform mat4 mCameraMatrix;

uniform vec4 mScreenDimensions;

const float gamma = 2.2;

void main() {
    vec3 colorHdr = texture(mTexture, mSourceTexCoord).rgb;
    vec3 toneMapped = colorHdr / (colorHdr + vec3(1));
    toneMapped = pow(toneMapped, vec3(1 / gamma));

    color = toneMapped;
}
