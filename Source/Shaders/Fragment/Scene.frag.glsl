#version 460 core

in vec3 vWorldPosition;
in vec3 vWorldNormal;
in vec3 vWorldTangent;
in vec2 vTextureCoordinate;

uniform sampler2D uDiffuseMap;
uniform sampler2D uNormalMap;
uniform sampler2D uPackedMap;
uniform bool uHasDiffuse;
uniform bool uHasNormalMap;
uniform bool uHasPackedMap;
uniform vec3 uMaterialColor;
uniform vec3 uCameraPosition;
uniform vec3 uLightPosition;
uniform vec3 uLightColor;
uniform float uLightFalloffDistance;
uniform float uEmissive;
uniform float uAmbientFloor;
uniform float uDirectLightScale;

out vec4 fragmentColor;

void main() {
    vec4 diffuseSample = uHasDiffuse ? texture(uDiffuseMap, vTextureCoordinate) : vec4(1.0);
    if (diffuseSample.a < 0.04) {
        discard;
    }

    vec3 albedo = diffuseSample.rgb * uMaterialColor;
    vec3 normal = normalize(vWorldNormal);
    if (uHasNormalMap) {
        vec3 tangent = normalize(vWorldTangent - normal * dot(vWorldTangent, normal));
        vec3 bitangent = normalize(cross(normal, tangent));
        mat3 tangentFrame = mat3(tangent, bitangent, normal);
        normal = normalize(tangentFrame * (texture(uNormalMap, vTextureCoordinate).rgb * 2.0 - 1.0));
    }

    float ambientOcclusion = 1.0;
    float roughness = 0.62;
    float metallic = 0.0;
    if (uHasPackedMap) {
        vec3 packed = texture(uPackedMap, vTextureCoordinate).rgb;
        ambientOcclusion = packed.r;
        roughness = clamp(packed.g, 0.08, 1.0);
        metallic = packed.b;
    }

    vec3 toLight = uLightPosition - vWorldPosition;
    float lightDistance = length(toLight);
    vec3 lightDirection = normalize(toLight);
    float distanceFade = 1.0 - smoothstep(uLightFalloffDistance * 0.35, uLightFalloffDistance, lightDistance);
    float attenuation = distanceFade /
                        (1.0 + 0.09 * lightDistance + 0.032 * lightDistance * lightDistance);
    float downwardSpread = mix(0.72, 1.0, smoothstep(-0.05, 0.8, lightDirection.y));
    vec3 lightColor = uLightColor * attenuation * downwardSpread;
    float diffuse = max(dot(normal, lightDirection), 0.0);
    vec3 viewDirection = normalize(uCameraPosition - vWorldPosition);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);
    float specularExponent = mix(96.0, 7.0, roughness);
    float specular = pow(max(dot(normal, halfwayDirection), 0.0), specularExponent);
    vec3 reflectance = mix(vec3(0.04), albedo, metallic);
    float ambientLevel = max(0.08, uAmbientFloor);
    vec3 ambient = albedo * vec3(ambientLevel, ambientLevel * 0.90, ambientLevel * 0.78) * ambientOcclusion;
    vec3 direct = (albedo * lightColor * diffuse +
                   reflectance * lightColor * specular * (1.0 - roughness * 0.65)) * uDirectLightScale;
    vec3 emissive = albedo * uEmissive;
    fragmentColor = vec4(ambient + direct + emissive, diffuseSample.a);
}