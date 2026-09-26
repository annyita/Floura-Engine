#version 460 core

#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_bindless_texture : require

out vec4 FragColor;

in vec2 texCoord;
//in vec3 camPositon;
uniform vec3 camPos;

uniform highp sampler2D screentexture;

// geometry
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D depthMap;
uniform sampler2D gSpecular;
uniform sampler2D gVelocity;
uniform sampler2D gEmission;

// history
uniform sampler2D hColour;
uniform sampler2D hDepthTexture;
uniform sampler2D hNormal;

uniform mat4 cameraMatrix;
uniform mat4 inverseViewMatrix;
uniform mat4 inverseProjection;
uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform float time;
uniform int frame;

uniform float FarPlane;
uniform float NearPlane;

uniform mat4 lightProjection;

uniform uint64_t BlueNoiseHandle;
uniform uint64_t bayerMatrixHandle;

uniform sampler2D shadowMap;

uniform uint64_t cmMainHandle;

uniform int indirectSamples;
uniform bool doSSR;
uniform bool doReflections;

uniform uint64_t SB_Handle;

bool doMatCap = false;

float linearizeDepth(float depth, float NP, float FP){
    return (2.0 * NP * FP) / (FP + NP - (depth * 2.0 - 1.0) * (FP - NP));
}

const int numBinarySearchSteps = 5;

vec3 BinarySearch(inout vec3 dir, inout vec3 hitCoord, inout float dDepth){
    float depth;

    vec4 projectedCoord;

    for(int i = 0; i < numBinarySearchSteps; i++)
    {

        projectedCoord = projectionMatrix * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

        depth = texture(gPosition, projectedCoord.xy).z;


        dDepth = hitCoord.z - depth;

        dir *= 0.5;
        if(dDepth > 0.0)
        hitCoord += dir;
        else
        hitCoord -= dir;
    }

    projectedCoord = projectionMatrix * vec4(hitCoord, 1.0);
    projectedCoord.xy /= projectedCoord.w;
    projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

    return vec3(projectedCoord.xy, depth);
}

#define rayThickness 0.1

vec4 RayCast(in vec3 dir, inout vec3 hitCoord, out float dDepth, int maxSteps, out float hit, in float step){
    hitCoord += dir* 0.1;

    dir *= step;
    float depth;
    vec4 projectedCoord = vec4(0.0);
    
    for (int i = 1; i < maxSteps; i++)
    {
        hitCoord += dir;
        
        projectedCoord = projectionMatrix * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

        if (projectedCoord.x < 0.0 || projectedCoord.x > 1.0 || projectedCoord.y < 0.0 || projectedCoord.y > 1.0) {
            break;
        }

        vec3 sampledWorldPos = texture(gPosition, projectedCoord.xy).xyz;

        vec3 sampledViewPos = (viewMatrix * vec4(sampledWorldPos, 1.0)).xyz;

        if (length(sampledWorldPos) < 0.001) {
            continue;
        }
        dDepth = hitCoord.z - sampledViewPos.z;


        if((dir.z - dDepth) < rayThickness){
            if(dDepth <= 0.0)
            {
                vec4 Result;
                //Result = vec4(projectedCoord.xy, dDepth, 1.0);
                Result = vec4(BinarySearch(dir, hitCoord, dDepth), 1.0);
                hit = 1.0;

                return Result;
            }
        }
    }
    hit = 0.0;
    return vec4(projectedCoord.xy, dDepth, 1.0);
}

#define Scale vec3(.8, .8, .8)
#define K 19.19

//https://www.shadertoy.com/view/ttc3zr thank you for the hash function
uvec3 murmurHash33(uvec3 src) {
    const uint M = 0x5bd1e995u;
    uvec3 h = uvec3(1190494759u, 2147483647u, 3559788179u);
    src *= M; src ^= src>>24u; src *= M;
    h *= M; h ^= src.x; h *= M; h ^= src.y; h *= M; h ^= src.z;
    h ^= h>>13u; h *= M; h ^= h>>15u;
    return h;
}

// 3 outputs, 3 inputs
vec3 hash33(vec3 src) {
    uvec3 h = murmurHash33(floatBitsToUint(src));
    return uintBitsToFloat(h & 0x007fffffu | 0x3f800000u) - 1.0;
}

#define ssr_maxstep 64
#define ssr_stepsize 1.0f
#define ssr_reflectionSpecularFalloffExponent 3.0f

vec3 fresnelSchlick(float cosTheta, vec3 F0) { return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); }

vec4 ssr(vec3 ARM, vec3 position, vec3 normal, int maxSteps, float step, highp vec2 velocity){
    float Metallic = ARM.b;
    float rough = ARM.g;

    //bool fallback;

    if(Metallic < 0.01) return vec4(0.0);
    
    vec3 viewPos    = (viewMatrix * vec4(position, 1.0)).xyz;
    vec3 viewNormal = mat3(viewMatrix) * normal;

    vec3 V = normalize(viewPos);
    vec3 R = reflect(V, viewNormal);
    
    vec3 hitPos = viewPos;
    float dDepth;
    float hit = 0.0;

    //return vec4(V, 1.0);
    //return vec4(normalize(viewPos) * 0.5 + 0.5, 1.0);

    vec3 F0 = vec3(0.04);
    F0      = mix(F0, texture(gAlbedoSpec, texCoord.xy).rgb, Metallic);
    vec3 Fresnel = fresnelSchlick(max(dot(normalize(viewNormal), normalize(V)), 0.0), F0);


    vec3 wp = vec3(vec4(viewPos, 1.0) * viewMatrix);
    vec3 t = vec3(time,time +1,time +3);
    //vec3 jitt = mix(vec3(0.0), vec3(hash31(rough)), rough); // time
    vec3 jitt = mix(vec3(0.0), vec3(hash33(wp + t)), rough); // time

    //vec4 csCoords = RayCast(R, hitPos, dDepth, 32);
    //vec4 csCoords = RayCast(R, hitPos, dDepth, 32, hit, 0.10);

    vec3 rayDir = normalize(R + jitt) * step;

    ivec2 size = ivec2(textureSize(gNormal, 0)); // get size of tex
        
    vec4 csCoords = RayCast(rayDir, hitPos, dDepth, maxSteps, hit, step);
    csCoords.xy -= velocity;
    
    if (hit == 1.0){
        //vec4 csCoords = RayCast(vec3(jitt) + R, hitPos, dDepth, maxSteps, hit, step);

        vec2 dCoords = smoothstep(0.2, 0.6, abs(vec2(0.5, 0.5) - csCoords.xy));
        //vec2 dCoords = smoothstep(0.2, 0.6, abs(vec2(0.5, 0.5) - texCoord.xy));

        float screenEdgefactor = clamp(1.0 - (dCoords.x + dCoords.y), 0.0, 1.0);

        float ReflectionMultiplier = pow(Metallic, ssr_reflectionSpecularFalloffExponent) *
        screenEdgefactor *
        -R.z;

        vec3 SSR = textureLod(hColour, csCoords.xy, 0).rgb * clamp(ReflectionMultiplier, 0.0, 0.9) * Fresnel;//hColour
        return vec4(clamp(SSR,0.0, 1.0), ReflectionMultiplier);//Metallic
    }
    return vec4(0.0);
}

// (thanks learnopengl)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / max(denom, 0.0001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 metRough(vec3 albedo, vec3 iNormal, vec3 viewVector, out vec3 nFer, out float nMet, out vec3 irradiance, vec3 ARM){
    // textures
    vec3 metallicRoughness = ARM; // metalic
    float rough = metallicRoughness.g; // g
    float met =  metallicRoughness.b; // b
    nMet = met;

    // vectors
    vec3 normal = iNormal;
    //vec3 normal = normalize(Normal);
    vec3 v = normalize(viewVector);
    vec3 r = reflect(v, normal);
    float nDotV = max(dot(normal, -v), 0.0001);

    samplerCube cmSamp = samplerCube(cmMainHandle);

    // LOD based Roughness (irradiance probes);
    float maxLod = float(textureQueryLevels(cmSamp) - 1);
    float lod = rough * maxLod;
    lod = min(lod, 10.0);

    vec3 reflectionColour = textureLod(cmSamp, r, lod).rgb;
    irradiance = textureLod(cmSamp, normal, maxLod).rgb;

    // fresnel
    float reflectivity = 0.04f;
    vec3 nF = mix(vec3(reflectivity), albedo, met);
    nFer = nF;
    vec3 F = nF + (max(vec3(1.0 - rough), nF) - nF) * pow(clamp(1.0 - nDotV, 0.0, 1.0), 5.0);

    // visibility factor 
    float G = GeometrySmith(normal, -v, r, rough);

    //return reflectionColour * F;
    return reflectionColour * F * G;
}

void Reflect(vec3 albedo,  vec3 iNormal, vec3 viewVector, out vec3 diffuse, out vec3 specular, vec3 ARM)
{

    //if (doReflect && depth < maxDist)

        float met = 0;
        vec3 nF = vec3(0.0f);
        vec3 irradiance = vec3(0.0f);
        specular = metRough(albedo,iNormal,viewVector, nF, met, irradiance, ARM);

        vec3 nDiffuse = vec3(0.0f);
        nDiffuse = albedo * (1.0f - nF);
        nDiffuse *= (1.0f - met);

        diffuse = irradiance * nDiffuse;
        //diffuse = nDiffuse;
}

void main(){

    //vec3 positionTemp = texture(gPosition, texCoord).rgb;
    //FragColor = vec4(vec3(positionTemp), 1.0f); return;
    vec4 albedo = texture(gAlbedoSpec, texCoord);
    vec3 colour = texture(screentexture, texCoord).rgb;
    //FragColor = vec4(vec3(albedo.rgb), 1.0f); return;
    //FragColor = vec4(vec3(colour), 1.0f); return;
    
    if (albedo.a <= 0.0){
        FragColor = vec4(colour, 1.0f); return;
    }

    float depth = texture(depthMap, texCoord).r;

    //early z cutoff
    if (depth >= 0.99999){
        FragColor = vec4(colour, 1.0f);
        return;
    }

    vec3 normal = texture(gNormal, texCoord).rgb;
    //                      AO  Rough  MAT
    //vec3 ARM = vec3(0.0, 1.0, 1.0); // placeholder
    vec3 ARM = texture(gSpecular, texCoord).rgb;// placeholder
    //ARM.g = 0.0;
    //ARM.b = 1.0;
    //gSpecular  

    float displacement = texture(gNormal, texCoord).a;

    vec3 position = texture(gPosition, texCoord).rgb;

    //FragColor = vec4(position, 1.0); return;

    vec3 viewVector = position - camPos;

    vec3 emission = texture(gEmission, texCoord).rgb;
    highp vec2 velocity = texture(gVelocity, texCoord).rg;
    
    vec4 reflections = vec4(0.0);
    
    if (doMatCap){
        vec3 v = normalize(viewVector);
        vec3 r = reflect(v, normal);

        vec3 reflectionColour = textureLod(samplerCube(cmMainHandle), r, 0).rgb;
        
        FragColor.rgb = reflectionColour;
        return;
    }

    if (doSSR) reflections = ssr(ARM, position, normal, ssr_maxstep, ssr_stepsize, velocity);

    vec3 specular = vec3(0.0f);
    vec3 diffuse  = vec3(0.0f);
    
    if (reflections.a < 1.0 && doReflections){
        Reflect(albedo.rgb, normal, viewVector, diffuse, specular, ARM);

        reflections.rgb = mix(diffuse + specular, reflections.rgb, reflections.a);
        //reflections.rgb = diffuse + specular;
    }

    
    //SB_Handle
    //FragColor = vec4(vec3(ndbColour), 1.0f);
    
    vec3 final = colour + reflections.rgb + emission;
    
    //vec3 ndbColour = mix(vec3(1.0, 0.0, 0.0), reflections.rgb, reflections.a);
    //FragColor = vec4(vec3(ndbColour), 1.0f);
    FragColor = vec4(vec3(final), 1.0f);
    //FragColor = vec4(vec3(final + nEmissionblur), 1.0f);
    
}