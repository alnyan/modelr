#version 420 core

in vec2 mSourceTexCoord;
out vec3 color;

uniform sampler2D mTexture;
uniform sampler2D mDepthTexture;
uniform float mTime;

void main() {
    color = texture(mTexture, mSourceTexCoord).rgb;
}
