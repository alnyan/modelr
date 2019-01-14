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
const float motionBlurCoeff = 0.005;

vec4 blur5(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) {
	vec4 color = vec4(0.0);
	vec2 off1 = vec2(1.3333333333333333) * direction;
	color += texture2D(image, uv) * 0.29411764705882354;
	color += texture2D(image, uv + (off1 / resolution)) * 0.35294117647058826;
	color += texture2D(image, uv - (off1 / resolution)) * 0.35294117647058826;
	return color;
}

void main() {
	float d = texture2D(mDepthTexture, mSourceTexCoord).x;
	vec2 vel;

	if (d < 1) {
		vel = texture2D(mVelocityTexture, mSourceTexCoord).xy;
	} else {
		vel = vec2(0, 0);
	}

	vec4 texV = blur5(mTexture, mSourceTexCoord, vec2(1, 1), vec2(motionBlurCoeff * vel.x, motionBlurCoeff * vel.y));
    vec3 colorHdr = texV.rgb;
    vec3 toneMapped = colorHdr / (colorHdr + vec3(1));
    toneMapped = pow(toneMapped, vec3(1 / gamma));

	color = toneMapped;
}
