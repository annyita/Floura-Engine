#version 460 core

#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_bindless_texture : require
//#extension GL_KHR_shader_subgroup_ballot : enable
//#extension GL_KHR_shader_subgroup_arithmetic : enable
//#extension GL_KHR_shader_subgroup_vote : enable

//#extension GL_ARB_shader_clock  : enable

#define INF 3.40282347e+38f


//out vec4 FragColor;

in highp vec2 texCoord;

layout(location = 0) out vec4 oindirect;
layout(location = 1) out vec3 ospecular;
layout(location = 2) out vec4 oemission;
//layout(location = 3) out float swrtAO;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D depthMap;
uniform sampler2D gSpecular;
uniform sampler2D gEmission;
uniform highp sampler2D gVelocity;
//uniform sampler2D hAO;
uniform sampler2D swrtHDepth;
uniform float NearPlane;
uniform float FarPlane;

uniform mat4 viewMatrix;
uniform mat4 invViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 invProjectionMatrix;

uniform mat4 invHViewMatrix;
uniform mat4 invHProjectionMatrix;
uniform mat4 invCameraMatrix;

uniform vec3 cameraPosition;

uniform float time;
uniform int frame;

uniform vec3 directLightPos;
uniform vec3 directLightCol;
uniform bool doDirLight;
uniform float directAmbient;

uniform mat4 lightProjection;
uniform float dirSpecularLight;
uniform bool doDirSpecularLight;
uniform bool doReflect;
uniform sampler2D BlueNoiseHandle;
uniform samplerCube cmMainHandle;
uniform sampler2DShadow  shadowMap;
uniform int FilterRadius;
uniform int NumberOfSamples;
uniform float DirSMMaxBias;
uniform bool doDirShadowMap;

uniform vec3 sceneBoundPos;
uniform vec3 sceneBoundScale;

uniform bool doTemporalAccumulation;
uniform float temporalAccumulationBlendFactor;

uniform sampler2D hIndirect;
uniform sampler2D hEmission;
uniform sampler2D hSpecular;
uniform sampler2D presentImage;

uniform vec2 screenSize;
uniform vec3 skyColor;
// temp

struct Light{
    vec3 position;
    vec3 rotation;
    vec3 colour;
    float radius;
    int type;
};
// i seriously need a light ssbo....
uniform Light Lights[64];
uniform int lightCount;
// temp fields
float specularLight = 0.50f;

struct clipmapLevel{
    vec4 ps; // pos & scale
    uint64_t handle; // dist
    uint64_t handle2; // alb
    uint64_t handle3; // em
    uint64_t handle4; // metallic rough (not implemented)
    uint64_t handle5; // indirect cache
    uint64_t handle6; // emission cache?
    float thickness;
    uint dirty;
    uint pad;
    uint pad0;
};

struct MDF{
    vec4 position;  // uv.x
    vec4 extents;  // uv.y
    vec4 rootPosition;
    vec4 rootExtents;
    vec4 gPosition;
    vec4 gExtents;
//vec4 gRotation;
    mat4 globalTransform;

    uint64_t instanceUUID;
    sampler3D SDF_Handle; //sampler3D
    sampler3D UV_Handle;
    sampler2D ALB_Handle;
    sampler2D EM_Handle;
    sampler2D ARM_Handle;

    vec4 extraFactors;
    vec4 extraFactors2;
};

// mesh distance fields
layout(std430, binding = 11) buffer MDF_Buffer {
    MDF MDFS[];
};
// global distance fields
layout(std430, binding = 12) buffer GDF_Buffer {
    clipmapLevel GDFS[];
};

#define forceMirror 0// 0 = false, 1 = true, 2 = true + unaltered metallic
#define drawSDFSCENE 0
#define cascadeShape 0 // 0 = box, 1 = sphere
#define clipmapcount 4

#define r_steps 512
#define i_steps 512
#define r_maxdist 32.0f
#define i_maxdist 32.0f

#define doVXCache 1
#define  i_samples 1
#define doReflection 1
#define r_bounces 1
#define indirectMetallicMode 1 // 0 = false, 1 = true 

#define r_roughnessFloorEnabled 0
#define r_roughnessFloor 0.3f
float r_shadow_ambient = 0.0f; //0.07f;
float r_taBlendTreshhold = 0.1f;
float r_taBlendUnderTresh = 0.7f;

//float r_noiseThreshold = 1.0;
//float i_noiseThreshold  = 1.0;
float i_indirectBoost = 1.0f; // default at 1.0
float e_emissionBoost = 1.0f; // default at 1.0

float maxSDFDist = 64.0f;

const float eps = 0.001f;
float mindist = 0.001f;
//float originEplison = 0.2f;

// good for gdf
//const float eps = 0.05f;
//float mindist = 0.05f;
float originEplison = 0.5;
//const float eps = 0.01f;
//float mindist = 0.01f;
//float originEplison = 0.90f;
//#define r_bounces 1

struct hitresult{
    vec3 normal;
    vec3 hitpos;
    vec3 uvw;
    float distance;
    float totalDistance;
    float iterationsDBG;
    int cascade;
    bool isHit;
    //vec2 uv;
    //int hitIndex;
    //float totalDistanceTravelled;    
    //float maxDist;
    //float lowestDistance;
};


float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898, 78.233))) * 43758.5453);
}

uvec3 murmurHash31(uint src) {
    const uint M = 0x5bd1e995u;
    uvec3 h = uvec3(1190494759u, 2147483647u, 3559788179u);
    src *= M; src ^= src>>24u; src *= M;
    h *= M; h ^= src;
    h ^= h>>13u; h *= M; h ^= h>>15u;
    return h;
}

// 3 outputs, 1 input
vec3 hash31(float src) {
    uvec3 h = murmurHash31(floatBitsToUint(src));
    return uintBitsToFloat(h & 0x007fffffu | 0x3f800000u) - 1.0;
}

uint murmurHash13(uvec3 src) {
    const uint M = 0x5bd1e995u;
    uint h = 1190494759u;
    src *= M; src ^= src>>24u; src *= M;
    h *= M; h ^= src.x; h *= M; h ^= src.y; h *= M; h ^= src.z;
    h ^= h>>13u; h *= M; h ^= h>>15u;
    return h;
}

// 1 output, 3 inputs
float hash13(vec3 src) {
    uint h = murmurHash13(floatBitsToUint(src));
    return uintBitsToFloat(h & 0x007fffffu | 0x3f800000u) - 1.0;
}

float lumaFromRGB(vec3 rgb){
    vec3 weights = vec3(0.2126, 0.7152, 0.0722);
    float luminance = dot(rgb, weights);
    return luminance;
}

float sdBox(vec3 p, vec3 b){
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0f);
}

float sdCapsule( vec3 p, vec3 a, vec3 b, float r ){
    vec3 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h ) - r;
}

float sdSphere( vec3 p, float r ){
    return length(p) - r;
}

vec4 fOpUnionID4(vec4 res1, vec4 res2){
    return (res1.x < res2.x) ? res1 : res2;
}

vec3 transformP(vec3 p, mat4 matrix){
    vec4 np = matrix * vec4(p, 1.0);
    return np.xyz;
}

float texture3DSDF(vec3 p, vec3 s, sampler3D handle){
    vec3 distance = abs(p) - s;
    
   float outsideDistance = length(max(distance, 0.0));
    
    // map coords to 3d uv space
    vec3 uvw = (p / (s* 2.0)) + 0.5;
    
    float SDF = textureLod(handle, uvw, 0).r;
    
    if (outsideDistance > 0.0) return SDF + outsideDistance;
    
    return SDF;
}

vec3 texture3DSDFUV(vec3 p, vec3 s, sampler3D handle){
    vec3 distance = abs(p) - s;

    float outsideDistance = length(max(distance, 0.0));

    // map coords to 3d uv space
    vec3 uvw = (p / (s* 2.0)) + 0.5;

    vec3 SDF = textureLod(handle, uvw, 0).rgb;

    if (outsideDistance > 0.0) return vec3(SDF.r + outsideDistance, SDF.yz);

    return SDF;
}

vec3 nearestPointOnAABB(vec3 p, vec3 pos, const vec3 extents){
    return clamp(p, pos - extents, pos + extents);
}

vec4 gdfScene(vec3 p, out float nThickness, out int cascade){
    float nBD = INF;
    nThickness = 0.0;
    cascade = -1;
    //return vec4(INF, 0.0, 0.0, 0.0);
    //for (int i = 0; i < 1; i++ ){
    for (int i = 0; i < clipmapcount; i++ ){
        vec3 pos = GDFS[i].ps.xyz;
        vec3 scale = vec3(GDFS[i].ps.w);
        vec3 gp = p - pos;
        
        #if cascadeShape == 0
        float bd = sdBox(gp, scale);
        #elif cascadeShape == 1
        float bd = sdSphere(gp, scale.r);
        #endif
        if (bd > 0.0 || bd > -1.0 && bd < 0.0 && rand(texCoord + vec2(i) + time) < (bd + 1.0)){
        //if (bd > 0.0){
           nBD = min(nBD, bd);
            continue;
        }
        //if (rand(texCoord) < bd) continue;
        
        nThickness = GDFS[i].thickness;
        cascade = i;
        
        vec3 distance = abs(gp) - scale;

        float outsideDistance = length(max(distance, 0.0));

        // map coords to 3d uv space
        vec3 uvw = (gp / (scale* 2.0)) + 0.5;
        
        float SDF = textureLod(sampler3D(GDFS[i].handle), uvw, 0).r;
        //float spheredist = sdCapsule(p - cameraPosition, vec3(0.0, 0.5, 0.0), vec3(0.0, -1.3, 0.0), 0.5);
        //SDF = min(SDF, spheredist);
        //SDF.y = texelFetch(sampler3D(GDFS[i].handle), ivec3(uvw), 0).y;
        //
        //float ID = texelFetch(sampler3D(GDFS[i].handle), ivec3(uvw), 0).g;
        //vec2 SDF = vec2(dist, ID);
        if (outsideDistance > 0.0) return vec4(SDF + outsideDistance, uvw);

        return vec4(SDF, uvw);
    }
    
    return vec4(INF, 0.0, 0.0, 0.0);
}

vec3 CalculateNormalGDFOld( in vec3 p ){
    const vec2 h = vec2(eps,0);
    float t = 0.0; int ind = 0; 
    return normalize( vec3(
            gdfScene(p+h.xyy, t, ind).x - gdfScene(p-h.xyy, t, ind).x,
            gdfScene(p+h.yxy, t, ind).x - gdfScene(p-h.yxy, t, ind).x,
            gdfScene(p+h.yyx, t, ind).x - gdfScene(p-h.yyx, t, ind).x ) );
}
// https://iquilezles.org/articles/normalsSDF/
vec3 CalculateNormalGDF( in vec3 p  ){
    const vec2 k = vec2(1.0,-1.0);
    float t = 0.0; int ind = 0;
    return normalize(
            k.xyy*gdfScene( p + k.xyy * eps, t, ind).x +
            k.yyx* gdfScene( p + k.yyx * eps, t, ind).x +
            k.yxy* gdfScene( p + k.yxy * eps, t, ind).x +
            k.xxx* gdfScene( p + k.xxx * eps, t, ind).x );
}

hitresult raymarchGDFscene(vec3 ro, vec3 rd, float maxdist, float mindist, int steps){
    hitresult hr;
    hr.isHit = false;
    //hr.lowestDistance = INF;
    float t = 0.0; // total distance travelled
    // raymarching
    for (int i = 0; i < steps; i++){
        vec3 pos = ro + rd* t;// position along the ray

        float nThickness = 0.0; int intd = 0;
        float dist = gdfScene(pos, nThickness, intd).r;
        
        t += dist;
        //if (hr.lowestDistance > dist) hr.lowestDistance = dist;

        if (dist < (mindist + nThickness)){ // treat as if hit
             vec4 m = gdfScene(pos, nThickness, intd);
            hr.uvw = m.yzw;
            // before this completes do an alpha check
            //hr.colour = textureLod(sampler3D(GDFS[hr.cascade].handle2), hr.uvw, 0);
            
            //hr.colour = m.yzw;
            hr.cascade = intd;
            hr.isHit = true;
            hr.distance = dist;
            hr.totalDistance = t;
            hr.iterationsDBG = float(i) / float(steps);
            hr.normal = CalculateNormalGDF(pos); // really expensive
            hr.hitpos = pos;
            //if (hr.colour.a < 0.5) continue;
            break;// how small dist (radius around march)
        }
        if (dist > maxdist) break; // failed to hit
        //discard; // discard on far for transparency
        // how large dist (radius around march)
    }

    return hr;
}

hitresult raymarchGDFinverseScene(vec3 ro, vec3 rd, float maxdist, float mindist, int steps){
    hitresult hr;
    hr.isHit = false;
    //hr.lowestDistance = INF;
    float t = 0.0; // total distance travelled
    // raymarching
    for (int i = 0; i < steps; i++){
        vec3 pos = ro + rd* t;// position along the ray

        float nThickness = 0.0; int intd = 0;
        float dist = -gdfScene(pos, nThickness, intd).r;

        t += dist;
        //if (hr.lowestDistance > dist) hr.lowestDistance = dist;

        if (dist < (mindist + nThickness)){ // treat as if hit
            //vec4 m = -gdfScene(pos, nThickness, intd);
            hr.distance = -gdfScene(pos, nThickness, intd).r;
            //hr.uvw = m.yzw;
            // before this completes do an alpha check
            //hr.colour = textureLod(sampler3D(GDFS[hr.cascade].handle2), hr.uvw, 0);

            //hr.colour = m.yzw;
            //hr.cascade = intd;
            hr.isHit = true;
            //hr.distance = dist;
            //hr.iterationsDBG = float(i) / float(steps);
            //hr.normal = -CalculateNormalGDF(pos); // really expensive
            hr.hitpos = pos;
            //if (hr.colour.a < 0.5) continue;
            break;// how small dist (radius around march)
        }
        if (dist > maxdist) break; // failed to hit
        //discard; // discard on far for transparency
        // how large dist (radius around march)
    }

    return hr;
}

float CalcShadowFactorDIR(vec4 LightSpacePos, vec3 lightDirection, vec3 normal, vec3 iPosition){
    // perform perspective divide
    vec3 lightCoords = LightSpacePos.xyz / LightSpacePos.w;
    float shadow = 0.0f;
    
    // shadow calculation
    if (lightCoords.z <= 1.0f){
        sampler2D bluemap =sampler2D(BlueNoiseHandle) ;
        lightCoords = (lightCoords + 1.0f) / 2.0f;
        // get the current depth
        float currentDepth = lightCoords.z;
        // calculate shadow bias
        float bias = max(DirSMMaxBias * (1.0f - dot(normal, lightDirection)), 0.0005f);
        // PCF
        int sampleRadius = FilterRadius; // FilterRadius // NumberOfSamples
        vec2 texSize = vec2(textureSize(bluemap, 0));
        // uv
        vec2 offset = vec2(fract(frame * 0.618), fract(frame * 0.133));
        vec2 noiseUV = (gl_FragCoord.xy / texSize) + offset;

        vec2 pixelSize = 1.0 / textureSize(shadowMap, 0);
        float tsamples = 0.0;
        for(int y = -sampleRadius; y <= sampleRadius; y++){
            for(int x = -sampleRadius; x <= sampleRadius; x++){
                float angle = texture(bluemap, noiseUV).r * NumberOfSamples;
                vec2 foffset = vec2(cos(angle), sin(angle));
                float closestDepth = texture(shadowMap, vec3(lightCoords.xy + (vec2(x, y) * foffset) * pixelSize, currentDepth - bias )).r;
                //if (currentDepth > closestDepth + bias)
                shadow += (1.0f - closestDepth);
                tsamples += 1.0f;
            }
        }

        shadow /= tsamples;
    }

    return shadow;
}

vec4 direcLight(vec3 ARM, vec3 iNormal, vec3 iPosition){ // normals need to be recalculated based on rotation
    // shadow map 
    float shadow = 0.0f;

    vec3 normal = normalize(iNormal);

    vec3 lightDirection = normalize(directLightPos); //vec3(1.0f, 1.0f, 0.0f)
    float diffuse = max(dot(normal, lightDirection), 0.0f);

    vec4 fragPosLight = lightProjection * vec4(iPosition, 1.0f);

    float smShadow = 0.0;
    //float rmShadow = 0.0f;
    float ssShadow = 0.0f;
    if (doDirShadowMap) shadow = CalcShadowFactorDIR(fragPosLight, lightDirection, normal, iPosition);
    //float fAmbient = directAmbient * ARM.r;
    //float specularFactor = 1.0;
    float specular = 0.0f;
    if (doReflect && doDirSpecularLight && diffuse != 0.0f){
        //return (vec4( vec3(1.0 , 0.0, 0.0), 1.0));

        vec3 reflectionDirection = reflect(-lightDirection, normal);
        vec3 viewDirection = normalize(cameraPosition - iPosition);
        vec3 halfwayVec = normalize(viewDirection + lightDirection);

        float specAmount = pow(max(dot(normal, halfwayVec), 0.0f), 16);
        //float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
        specular = specAmount * dirSpecularLight;


        return ((diffuse * (1.0f - shadow) + directAmbient) +  (1.0 - ARM.g) * specular * (1.0f - shadow)) * vec4(directLightCol, 1.0f); }
    else{ return ((diffuse * (1.0f - shadow) + directAmbient)) * vec4(directLightCol, 1.0f); }
}

vec4 directLightRT(vec3 pos, vec3 normal, vec3 colour, float maxDist, vec3 direction){
    vec3 rayDir = normalize(direction);

    float dotNL = max(dot(normal, rayDir), 0.0f);
    if (dotNL <= 0.0f) return vec4(0.0f);


    // shoot ray towards light
    hitresult lightHit = raymarchGDFscene(pos, rayDir, maxDist, mindist, 512);
    // if we hit something, return 0
    if (lightHit.isHit) {
        return vec4(0.0f);
    }
    // else return light colour
    return vec4(colour * dotNL, 1.0f);
}

vec4 spotLight(int iteration, vec3 ARM, vec3 iNormal, vec3 iPosition){
    // controls how big the area that is lit up is
    float outerCone = 0.90f;
    float innerCone = 0.95f;

    // ambient lighting
    //float ambient = 0.0f;

    vec4 finalColour = vec4(0.0f);

    // diffuse lighting
    vec3 normal = normalize(iNormal);

    vec3 lightDirection = normalize(Lights[iteration].position - iPosition);
    float diffuse = max(dot(normal, lightDirection), 0.0f);

    // calculates the intensity of the crntPos based on its angle to the center of the light cone
    float angle = dot(Lights[iteration].rotation, -lightDirection); // direction
    float inten = clamp( (angle - outerCone) / (innerCone - outerCone), 0.0f, 1.0);

    float shadow = 0.0;

    float specular = 0.0f;
    if (doReflect && diffuse != 0.0f){

        // specular lighting
        //float specularLight = 0.50f;
        vec3 viewDirection = normalize(cameraPosition - iPosition);
        vec3 reflectionDirection = reflect(-lightDirection, normal);

        vec3 halfwayVec = normalize(lightDirection + viewDirection);

        float specAmount = pow(max(dot(normal, halfwayVec), 0.0f), 16);
        specular = specAmount * specularLight;

        finalColour = finalColour + ((diffuse * (1.0f - shadow) * inten + 0.0f) + (1.0 - ARM.g) * specular * (1.0f - shadow) * inten) * vec4(Lights[iteration].colour, 1.0) * inten;

    }
    else{

        finalColour = finalColour + ((diffuse * (1.0f - shadow) * inten + 0.0f) * vec4(Lights[iteration].colour, 1.0) * inten);
        //	finalColour = finalColour + (texture(diffuse0, texCoord) * (diffuse * inten + ambient) * vec4(Lights[iteration].colour, 1.0) * inten);
    }

    //finalColour = finalColour + (diffuse * inten + skyColor);

    return finalColour;
}

vec4 pointLight(int iteration, vec3 ARM, vec3 iNormal, vec3 iPosition){
    vec4 finalColour = vec4(0.0f);

    //vec3 lightVec = (Lights[iteration].position) - crntPos;
    vec3 lightVec = (Lights[iteration].position) - iPosition;

    // intensity of light with respect to distance
    float dist = length(lightVec);
    float a = 3.00f;
    float b = 0.70f;
    float inten = 1.0f / (a * dist * dist + b * dist + 1.0) * Lights[iteration].radius;

    // ambient lighting
    //float ambient = 0.0f;
    vec3 normal = normalize(iNormal);
    //vec3 normal = normalize(Normal); 

    vec3 lightDirection = normalize(lightVec);
    float diffuse = max(dot(normal, lightDirection), 0.0f);

    //float ssShadow = shadowTrace(lightDirection, normal, iPosition);

    float shadow = 0.0;

    float specular = 0.0f;
    if (doReflect && diffuse != 0.0f){
        // specular lighting
        //float specularLight = 0.50f;
        vec3 viewDirection = normalize( cameraPosition - iPosition);
        vec3 reflectionDirection = reflect(-lightDirection, normal);

        vec3 halfwayVec = normalize(lightDirection + viewDirection);

        float specAmount = pow(max(dot(normal, halfwayVec), 0.1f), 16);
        specular = specAmount * specularLight;

        finalColour = finalColour + ((diffuse * (1.0f - shadow)* inten + 0.0f) + (1.0 - ARM.g) * specular * (1.0f - shadow)* inten) * vec4(Lights[iteration].colour, 1.0 ) * inten;
    }
    else{
        finalColour = finalColour + ( (diffuse * (1.0f - shadow) * inten + 0.0f) * vec4(Lights[iteration].colour, 1.0) * inten);
    }

    return finalColour;
}

vec4 lights(vec3 ARM, vec3 iNormal, vec3 iPosition){
    vec4 colour = vec4(0.0);

    int maxLights = 64;
    for (int i = 0; i < min(lightCount, maxLights); i++){
        if (Lights[i].type == 0)
            colour += spotLight(i, ARM, iNormal, iPosition);
        if (Lights[i].type == 1)
            colour += pointLight(i, ARM, iNormal, iPosition);
    }
    
    if (doDirLight)
        colour+= direcLight(ARM, iNormal, iPosition);
    
    return colour;
}

vec3 sampleHemisphere(vec3 normal, float value){
    float u = rand(texCoord + rand(vec2(time, value) ) );
    float v = rand(vec2(u, rand( vec2(time, value) ) ) );

    float phi = 2.0 * 3.14159265 * u;
    float cosTheta = sqrt(1.0 - v);
    float sinTheta = sqrt(v);

    vec3 localDir = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 helper = abs(normal.y) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0);
    vec3 tangent = normalize(cross(helper, normal));
    vec3 bitangent = cross(normal, tangent);

    return tangent * localDir.x + bitangent * localDir.y + normal * localDir.z;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) { return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); }

void reflection(vec3 pArm, vec3 pNrm, vec3 ro, vec3 rd, float maxdist, float mindist, int steps, highp vec2 velocity, out vec3 spec, out vec3 specEmission){
    vec3 colour = vec3(0.0);
    vec3 emissionColour = vec3(0.0f);
    int hitcount = 0;
    int onHitCount = 0;
    
    vec3 lastorigin = ro;
    vec3 lastdir = rd;
        
    float rough = pArm.g;
    float met = pArm.b;
    //vec3 lColour = texture(gAlbedoSpec, texCoord.xy).rgb;
        //r_bounces
    vec3 dim = vec3(1.0f);
    
    for (int i = 0; i < r_bounces; i++){
        if (dot(dim, vec3(0.333)) <= 0.001f) break;
        hitcount++;
                    
        //vec3 indDir = sampleHemisphere(lastdir, float( i) + time);
        vec3 indDir = sampleHemisphere(lastdir, time + float(i));    
        vec3 incidentDir = normalize(mix(lastdir, indDir, rough * rough));
        //vec3 jitt = mix(vec3(0.0), indDir, rough);
        //vec3 incidentDir = normalize(lastdir + jitt);
    
        hitresult hr = raymarchGDFscene(lastorigin, incidentDir, maxdist, mindist, steps);

        if (hr.isHit){
            vec3 albedo = textureLod(sampler3D(GDFS[hr.cascade].handle2), hr.uvw, 0).rgb;
            vec2 rm = textureLod(sampler3D(GDFS[hr.cascade].handle4), hr.uvw, 0).rg;
            
            rough = rm.r;
            met = rm.g;
            
            #if forceMirror == 1 // the parent material will be the mirror so this does
            rough = 0.0;
                met = 1.0;
            #elif forceMirror == 2
                rough = 0.0;
            #endif
    
            vec3 shadow = lights(vec3(1.0, rough, met), hr.normal, hr.hitpos).rgb;
            shadow = clamp(shadow + directAmbient + r_shadow_ambient, 0.0, 1.0);
            
             vec3 F0 = mix(vec3(0.04), albedo, met);
            vec3 Fresnel = fresnelSchlick(max(dot(hr.normal, -incidentDir), 0.0), F0);

            vec3 kD = (vec3(1.0) - Fresnel) * (1.0 - met);
            #if doVXCache == 1
                vec3 IRC = textureLod(sampler3D(GDFS[hr.cascade].handle5), hr.uvw, 0).rgb;
                vec3 EMC = textureLod(sampler3D(GDFS[hr.cascade].handle6), hr.uvw, 0).rgb;
                vec3 secondary = (IRC * i_indirectBoost) + (EMC * e_emissionBoost);
            
                #if indirectMetallicMode == 1
                vec3 diffuseWeight = (1.0 - Fresnel) * (1.0 - met);
                secondary *= clamp(diffuseWeight, 0.0, 1.0);
                #endif
            
                vec3 lighting = (kD * albedo * (shadow + secondary));
            #elif doVXCache == 0
                vec3 lighting = (kD * albedo * shadow);
            #endif
            colour += lighting * dim;

            emissionColour += textureLod(sampler3D(GDFS[hr.cascade].handle3), hr.uvw, 0).rgb * dim;
                    
            //dim *= Fresnel * 0.5f;
            dim *= Fresnel;
            //dim *= 0.5;

            // this helps with self intersection because apparently we are getting that
            lastorigin = hr.hitpos + hr.normal * originEplison;
            lastdir = reflect(incidentDir, hr.normal); // probably looks very off cause the normal is smooth
            onHitCount++;
        }
        else {
            colour += texture(cmMainHandle, incidentDir).rgb * dim;
            break;  
        }
    }
    
    specEmission = emissionColour /  max(onHitCount, 1);

    spec = colour / max(hitcount, 1);
}

struct indirectChannels{
    vec3 specular; // this is here for accum
    vec4 indirect;
    vec4 emission;
    float ao;
};

void indirectAndEmissionMarch(vec3 pNrm, vec3 ro, float maxdist, float mindist, int steps, int samples, inout indirectChannels onic){
    vec3 indColour = vec3(0.0);
    vec3 emColour = vec3(0.0);
    float oclussion = 0.0f;

    int anyHits = 0;
    int surfaceHits = 0;
    
    for (int i = 0; i < samples; i++){
        anyHits++;
        
        vec3 newDir = sampleHemisphere(pNrm, float(i) + time);
        hitresult hr = raymarchGDFscene(ro, newDir, maxdist, mindist, steps);
        
        if (hr.isHit){
            vec3 albedo = textureLod(sampler3D(GDFS[hr.cascade].handle2), hr.uvw, 0).rgb;
            vec3 mEmission = textureLod(sampler3D(GDFS[hr.cascade].handle3), hr.uvw, 0).rgb;
            vec3 IRC = textureLod(sampler3D(GDFS[hr.cascade].handle5), hr.uvw, 0).rgb;
            vec3 EMC = textureLod(sampler3D(GDFS[hr.cascade].handle6), hr.uvw, 0).rgb;
            //vec3 mEmission = vec3(0.0f);
            
            vec3 direct = lights(vec3(1.0, 1.0, 0.0), hr.normal, hr.hitpos).rgb;
            direct += directAmbient;
            direct = clamp(direct, 0.0, 1.0);

            // cosine angle of the incoming ray
            float cosTheta = max(dot(pNrm, newDir), 0.0);

            //cachedIrradance
            //vec3 diffuseComponent = albedo.rgb * direct;
            vec3 contribution = vec3(0.0);
            #if doVXCache == 1
                contribution = albedo.rgb * (direct + IRC) + (mEmission + EMC); // sample emission as direct
            #elif doVXCache == 0
                contribution = albedo.rgb * direct + mEmission;
            #endif

            indColour += contribution * cosTheta * 2.0;
            emColour += mEmission;
            
            //oclussion += smoothstep(5.0, 0.0, hr.totalDistance);
            
            surfaceHits++;
        }
        else {
            vec3 sky = textureLod(cmMainHandle, newDir, 0).rgb;
            indColour += sky;
            //break;
        }
        //colour = colour / hitcount;
        //colour = (colour)/ hitcount;
        // this is where we write cache
    }
    indirectChannels nIC;
    nIC.indirect.rgb = indColour / max(anyHits, 1);
    nIC.emission.rgb = emColour /max(surfaceHits, 1);
    //nIC.ao = 1.0 - (oclussion / surfaceHits);
    //nIC.ao = 1.0 - oclussion / max(surfaceHits, 1);
    
    onic = nIC;
}

float hash1( float n ){
    return fract(sin(n)*43758.5453123);
}

float linearizeDepth(float depth, float NearPlane, float FarPlane){
    return (2.0 * NearPlane * FarPlane) / (FarPlane + NearPlane - (depth * 2.0 - 1.0) * (FarPlane - NearPlane));
}

vec3 WorldPosFromDepth(float depth, mat4 invProjMatrix, mat4 invViewMat) {
    float z = depth * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(texCoord * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = invProjMatrix * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = invViewMat * viewSpacePosition;

    return worldSpacePosition.xyz;
}

vec4 accumulate(vec3 Input, sampler2D previous, highp vec2 historyTexCoord, float factor){
    vec4 previousColour = texture(previous, historyTexCoord);
    
    vec3 accumulated = mix(Input, previousColour.rgb, factor);

    // calc variance
    float inputLuma = lumaFromRGB(Input);
    float accumLuma = lumaFromRGB(accumulated);
    float nvariance = (inputLuma - accumLuma) * (inputLuma - accumLuma);
    float accumVariance = mix(nvariance, previousColour.a, temporalAccumulationBlendFactor);

    return vec4(accumulated, accumVariance);
}

indirectChannels temporalAccumulate(indirectChannels Input, highp vec2 historyTexCoord, float currentDepth, vec3 currentPosition, float roughness){
    // rejection
    if (any(lessThan(historyTexCoord, vec2(0.0))) || any(greaterThan(historyTexCoord, vec2(1.0)))){
        return Input;
    }

    // disocclusion rejection here
    float previousDepth = texture(swrtHDepth, historyTexCoord).r;
    float previousDepthLinearized = linearizeDepth(previousDepth, NearPlane, FarPlane);
    float depthDifference = abs(previousDepthLinearized - currentDepth);
    float threshold = 0.5 * currentDepth;
    //float threshold = 0.5 * currentDepth;

    //vec3 previousPosition = WorldPosFromDepth(previousDepth, invHProjectionMatrix, invHViewMatrix);
    //vec3 previousPosition = WorldPosFromDepth(previousDepth, invProjectionMatrix, invViewMatrix);
    //vec3 posDifference = previousPosition - currentPosition;
    //float posDifFactor = dot(currentPosition - previousPosition, currentPosition - previousPosition);
    if (depthDifference > threshold) return Input; //return vec4(vec3(1.0,0.0,0.0), 1.0);
   // if (depthDifference > threshold || posDifFactor >  0.999) return Input; //return vec4(vec3(1.0,0.0,0.0), 1.0);/if (depthDifference > threshold) return Input;
   // if (posDifFactor > 0.999) return Input;
    
    // accumulate
    //float blendFactor = 0.9;
    //clamp(temporalAccumulationBlendFactor, 0.0, 0.9)
    float factor = clamp(temporalAccumulationBlendFactor, 0.0, 0.99);
    
    indirectChannels nIC; nIC = Input;
    nIC.indirect = accumulate(Input.indirect.rgb, hIndirect, historyTexCoord, factor);
    nIC.emission = accumulate(Input.emission.rgb, hEmission, historyTexCoord, factor);
    //nIC.ao = accumulate(vec3(Input.ao.r), hAO, historyTexCoord, factor).r;
    float rblendfactor = r_taBlendUnderTresh;
    
    if (roughness > r_taBlendTreshhold){
        rblendfactor = factor;
    }
    nIC.specular = accumulate(Input.specular.rgb, hSpecular, historyTexCoord, rblendfactor).rgb;
    
    return nIC;
}

vec3 rayDirfromCam(mat4 projection, mat4 view, vec2 uv){
    vec4 target = projection * vec4(uv.x, uv.y, -1.0, 1.0);
    vec3 rayDirView = normalize(target.xyz / target.w);
    vec3 rayDirWorld = normalize(mat3(view) * rayDirView);

    return normalize(rayDirWorld);
}

// thanks dustmite
vec3 getViewPos(vec2 uv){
    float depth = texture(depthMap, uv).r;
    vec4 ndc = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 vp = invProjectionMatrix * ndc;
    return vp.xyz / vp.w;
}

vec3 reconstructViewNormal(vec2 uv, vec3 vp){
    vec2 texelSize = 1.0 / textureSize(depthMap, 0);
    vec3 viewTop = getViewPos(uv + vec2(0.0, texelSize.y));
    vec3 viewRight = getViewPos(uv + vec2(texelSize.x, 0.0));
    
    vec3 dx = viewRight - vp;
    vec3 dy = viewTop - vp;
    
    return normalize(cross(dx, dy));
}

vec3 reconstructWorldNormal(vec2 uv, vec3 vp){
    return normalize(mat3(invViewMatrix) * reconstructViewNormal(uv, getViewPos(uv)));
}

vec3 raytocam(vec3 position){
    vec2 ndc = (texCoord / screenSize) * 2.0 - 1.0;
    vec4 clippos = vec4(ndc, -1.0, 1.0);
    vec4 wp = invCameraMatrix * clippos;
    vec3 pixelwp = wp.xyz / wp.w;
    
    return normalize(pixelwp - position);
}


vec3 offsetPosition(vec3 pos, float elipson){
    vec3 rnrm = reconstructWorldNormal(texCoord, getViewPos(texCoord) );
    vec3 dpos = pos + rnrm * elipson;
    return dpos;
}

vec3 pushoutPosition(vec3 pos, float minDist, float maxdist, int fromCamSteps, int toCamSteps, int pushSteps, float elipson){
    float nThickness = 0.0; int intd = 0;

    vec3 rnrm = reconstructWorldNormal(texCoord, getViewPos(texCoord) );
    // cheaper to try offsetting it first
    vec3 dpos = pos + rnrm * elipson;
    float fdist = gdfScene(dpos, nThickness, intd).r;

    if (fdist > minDist) return dpos;

    // try pushing out
    vec3 nPos = dpos;
    for (int x = 0; x < pushSteps; x++){
        float dist = gdfScene(nPos, nThickness, intd).r;
        nPos = nPos + rnrm * abs(min(dist, minDist));
        if (dist > minDist) return nPos;
    }

    vec2 uv = (texCoord - 0.5) * 2.0;
    vec3 cameraRayDir = rayDirfromCam(invProjectionMatrix, invViewMatrix, uv);
    
    hitresult hr = raymarchGDFscene(cameraPosition, cameraRayDir, maxdist, 0.3, fromCamSteps);

    if (!hr.isHit) {
        vec3 rd = raytocam(nPos);
        hitresult hr2 = raymarchGDFinverseScene(nPos, rd, maxdist, minDist, toCamSteps);   
        if (!hr2.isHit) return nPos;
        return hr2.hitpos + rnrm * 0.01;
    }

    return hr.hitpos;
}

// Copyright 2019 Google LLC.
// SPDX-License-Identifier: Apache-2.0

// Polynomial approximation in GLSL for the Turbo colormap
// Original LUT: https://gist.github.com/mikhailov-work/ee72ba4191942acecc03fe6da94fc73f

// Authors:
//   Colormap Design: Anton Mikhailov (mikhailov@google.com)
//   GLSL Approximation: Ruofei Du (ruofei@google.com)

vec3 TurboColormap(in float x) {
    const vec4 kRedVec4 = vec4(0.13572138, 4.61539260, -42.66032258, 132.13108234);
    const vec4 kGreenVec4 = vec4(0.09140261, 2.19418839, 4.84296658, -14.18503333);
    const vec4 kBlueVec4 = vec4(0.10667330, 12.64194608, -60.58204836, 110.36276771);
    const vec2 kRedVec2 = vec2(-152.94239396, 59.28637943);
    const vec2 kGreenVec2 = vec2(4.27729857, 2.82956604);
    const vec2 kBlueVec2 = vec2(-89.90310912, 27.34824973);

    x = clamp(x, 0.0, 1.0);
    vec4 v4 = vec4( 1.0, x, x * x, x * x * x);
    vec2 v2 = v4.zw * v4.z;
    return vec3(
            dot(v4, kRedVec4)   + dot(v2, kRedVec2),
            dot(v4, kGreenVec4) + dot(v2, kGreenVec2),
            dot(v4, kBlueVec4)  + dot(v2, kBlueVec2)
    );
}

void main(){

    #if drawSDFSCENE == 1 // SDF SCENE VIEW
        vec2 uv = (texCoord - 0.5) * 2.0;
        vec3 rayDir = rayDirfromCam(invProjectionMatrix, invViewMatrix, uv);
    
        //return;
        //hitresult hr = raytraceRootHitST(cameraPosition, rayDir, 128, mindist, 128, mdfLength);
        hitresult hr = raymarchGDFscene(cameraPosition, rayDir, 512, 0.001, 512);
        vec4 albedo = textureLod(sampler3D(GDFS[hr.cascade].handle2), hr.uvw, 0);
        vec3 emission = textureLod(sampler3D(GDFS[hr.cascade].handle3), hr.uvw, 0).rgb;
        //ospecular.rgb = emission;
        ospecular.rgb = albedo.rgb;

        //vec3 direct = lights(vec3(1.0, 1.0, 0.0), hr.normal, hr.hitpos).rgb;
        //direct += 0.035;
        //direct = clamp(direct, 0.0, 1.0);
        //ospecular.rgb = albedo.rgb * direct + emission;

        //ospecular.rgb = hr.hitpos * 0.01;
        //ospecular.rgb = TurboColormap(clamp(float(hr.iterationsDBG), 0.0, 1.0));
        //ospecular.rgb = hash31(float(hr.cascade)) * hr.iterationsDBG * 15.0;//   clamp(hr.iterationsDBG, 0.0, 1.0);
        //ospecular.rgb = textureLod(MDFS[hr.materialIndex].texture_diffuse_Handle, hr.uv, 0).rgb;
        //vec3(0.2 + 0.4 * mod(floor(p.x) + floor(p.z), 2.0));
        //ospecular.rgb = mix(vec3(0.0), hr.hitpos, triplanarTile(hr.hitpos, hr.normal, 2.0).r);
        //ospecular.rgb = hr.colour;
        //ospecular.rgb = textureLod(sampler3D(GDFS[hr.cascade].handle2), hr.uvw, 0).rgb;

    
        //ospecular.rgb = vec3(hr.iterationsDBG * 15.0);
        //ospecular.rgb = vec3(hr.uv, 0.0);
        //ospecular.rgb = mix(ospecular,  hash31(float(hr.materialIndex)), 0.5);
        //ospecular = hash31(hr.materialIndex);
        return;
    #endif

    float gdepth = texture2D(depthMap, texCoord).r;
    if (gdepth >= 0.99999) discard;
    
    vec3 gp = texture(gPosition, texCoord).xyz;
    //vec3 gp                  = WorldPosFromDepth(gdepth, invProjectionMatrix, invViewMatrix);
    vec3 gnrm              = normalize(texture(gNormal, texCoord).xyz);
    //vec3 rnrm = reconstructWorldNormal(texCoord, getViewPos(texCoord));
    vec3 galbedo          = texture(gAlbedoSpec, texCoord).xyz;
    vec3 gemission       = texture(gEmission, texCoord).xyz;
    vec3 garm              = texture(gSpecular, texCoord).xyz;
    highp vec2 velocity = texture(gVelocity, texCoord).rg;
    
    //float allignment = dot(gnrm, rnrm);
    //float allignmentMix = 1.0 - clamp(allignment, 0.0, 1.0);
    
    #if forceMirror == 1 
        garm.g = 0.0;
        garm.b = 1.0;
    #elif forceMirror == 2
        garm.g = 0.0;
    #endif
    
    #if r_roughnessFloorEnabled == 1
        garm.g = max(r_roughnessFloor, garm.g);
    #endif
    // final lighting
    //vec3 origin = gp + gnrm * originEplison;
    //origin = testAndPushoutPosition(origin, rnrm, minIntersectionDist, 32);
    //vec3 origin = pushoutPosition(gp, minIntersectionDist, 200.0, 4096, 4, 4, originEplison);
    vec3 origin = offsetPosition(gp, originEplison);
    //vec3 origin = gp + gnrm * originEplison; 
    //float ndist = distance(origin, gp);
    //vec3 vp = (invViewMatrix * vec4(getViewPos(texCoord), 1.0)).xyz;
    //ospecular.rgb = origin * ndist;;

    //ospecular.rgb = vec3(ndist);
    //ospecular.rgb = vec3(normalize(gp - origin));
    //return;
    
    indirectChannels nIC;
    vec3 tRef = vec3(0.0f);
    vec3 tRefEM = vec3(0.0f);

    //uint64_t t1 = clockARB();
    #if doReflection == 1
    reflection(garm, gnrm, origin, reflect(normalize(gp - cameraPosition),  gnrm), r_maxdist, mindist, r_steps, velocity, tRef, tRefEM);
    #endif
    indirectAndEmissionMarch(gnrm, origin, i_maxdist, mindist, i_steps, i_samples, nIC);
    //uint64_t t2 = clockARB();
    
    //nIC.ao = mix(1.0, nIC.ao, allignmentMix);
    //nIC.ao = mix(1.0, 0.0, allignmentMix);

    //float tCycle = float(t2 - t1);
    //float maxCycles = 1000000.0;
    //float timeMap = clamp(tCycle / maxCycles, 0.0, 1.0);
    
    // clamp is a bandaid fix for image overbrightening/energy explosion
    nIC.specular = tRef + tRefEM;

    //nIC.specular = vec3(TurboColormap(timeMap));
    
    // temporal accumulation here
    //swrtAO = directLightRT(origin, gnrm, directLightCol, 512, directLightPos);
    //swrtAO = vec4(calcAO(origin, gnrm, 3.0));
     //nIC.ao; // hAO
    //swrtAO.r = 1.0;
    
    if (doTemporalAccumulation){
        float d = linearizeDepth(texture2D(depthMap, texCoord).r, NearPlane, FarPlane);
        highp vec2 historyTexCoord = texCoord - velocity;

        indirectChannels nNIC = temporalAccumulate(nIC, historyTexCoord, d, gp, garm.g);
        
        oindirect = nNIC.indirect;
        oemission = nNIC.emission;
        ospecular = nNIC.specular;
        //swrtAO = nNIC.ao;
        return;
    }
    
    oindirect = nIC.indirect;
    oemission = nIC.emission;
    ospecular = nIC.specular;
    //swrtAO = nIC.ao;
}