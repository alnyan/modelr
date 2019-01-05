#version 420 core

in vec2 mSourceTexCoord;
out vec3 color;

uniform sampler2D mTexture;
uniform sampler2D mDepthTexture;

const float fogDensity = 0.5f;
const float far = 100;
const float near = 0.1;

void main() {
    float z = texture(mDepthTexture, mSourceTexCoord).r;
    float d = ((far + near) / (2 * (far - near)) - (near * far / (far - near)) + 1 / 2);
    float fogF = exp(d * fogDensity);
    color = (1 - fogF) * vec3(1) + fogF * texture(mTexture, mSourceTexCoord).rgb;
    //color = vec3(1 - fragDepth);
    //color = mix(texture(mTexture, mSourceTexCoord).rgb, vec3(1), fragDepth);
    //color = texture(mTexture, mSourceTexCoord).rgb;
}
