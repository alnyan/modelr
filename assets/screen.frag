#version 430 core
#extension GL_ARB_bindless_texture : require

in vec2 mSourceTexCoord;
out vec3 color;

uniform sampler2D mTexture;
uniform sampler2D mDepthTexture;
uniform sampler2D mVelocityTexture;

uniform float mTime;

uniform mat4 mProjectionMatrix;
uniform mat4 mCameraMatrix;

uniform vec4 mScreenDimensions;

const float gamma = 2.2;

void main() {
    vec4 texV = texture2D(mTexture, mSourceTexCoord);
    vec3 colorHdr = texV.rgb;
    vec3 toneMapped = colorHdr / (colorHdr + vec3(1));
    toneMapped = pow(toneMapped, vec3(1 / gamma));

	color = toneMapped;
}
