#version 460 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec2 aTextureCoordinate;

uniform mat4 uViewProjection;
uniform mat4 uModel;
uniform mat3 uNormalMatrix;
uniform vec2 uUvScale;

out vec3 vWorldPosition;
out vec3 vWorldNormal;
out vec3 vWorldTangent;
out vec2 vTextureCoordinate;

void main() {
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    vWorldPosition = worldPosition.xyz;
    vWorldNormal = normalize(uNormalMatrix * aNormal);
    vWorldTangent = normalize(mat3(uModel) * aTangent);
    vTextureCoordinate = aTextureCoordinate * uUvScale;
    gl_Position = uViewProjection * worldPosition;
}