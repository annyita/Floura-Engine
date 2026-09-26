#version 460 core

#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_bindless_texture : require

out vec4 FragColour;

in vec2 texCoord;
//in vec3 camPositon;
uniform vec3 camPos;

// geometry
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D depthMap;
uniform sampler2D gSpecular;
uniform sampler2D gVelocity;
uniform sampler2D gEmission;

//uniform sampler2D skyGradientTexture;

// history
uniform sampler2D hColour;
uniform sampler2D hDepthTexture;
uniform sampler2D hNormal;

uniform mat4 cameraMatrix;
uniform mat4 inverseViewMatrix;
uniform mat4 inverseProjection;
uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;

uniform vec3 lightColor;
uniform vec3 fogColor;
uniform vec3 skyColor;

uniform bool DEFtoggle;
uniform bool doFog;
uniform float time;
uniform int frame;

uniform float fogDepthDistance;
uniform float fogFarPlane;
uniform float fogNearPlane;

uniform float FarPlane;
uniform float NearPlane;

uniform vec3 directLightPos;
uniform vec3 directLightCol;
uniform float directAmbient; // 0.20f
uniform float dirSpecularLight;
uniform bool doDirLight;
uniform bool doDirSpecularLight;
uniform bool doDirShadowMap;
uniform int FilterRadius;
uniform int NumberOfSamples;
uniform float DirSMMaxBias;

uniform mat4 lightProjection;

// temp
uniform sampler3D indirectVolume;
uniform sampler3D emissionVolume;

// temporary
uniform vec3 ivPosition;
uniform float ivScale;

uniform uint64_t BlueNoiseHandle;
uniform uint64_t bayerMatrixHandle;

uniform sampler2DShadow  shadowMap;

uniform uint64_t cmMainHandle;

uniform int indirectSamples;

// temp fields
float specularLight = 0.50f;

uniform bool doReflect;

struct Light
{
    vec3 position;
    vec3 rotation;
    vec3 colour;
    float radius;
    int type;
};

uniform Light Lights[64];

uniform int lightCount;

uniform bool doSSR;
uniform bool doContactShadows;
uniform sampler2D caustic;

float linearizeDepth(float depth, float NP, float FP){
    return (2.0 * NP * FP) / (FP + NP - (depth * 2.0 - 1.0) * (FP - NP));
}

float random(vec3 seed) {
    vec4 seed4 = vec4(seed, 1.0);
    float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
    return fract(sin(dot_product) * 43758.5453);
}


vec4 RayCast(in vec3 dir, inout vec3 hitCoord, out float dDepth, int maxSteps, out float hit, in float step){
    hitCoord += dir * 0.1;
    //hitCoord += dir;

    dir *= step;
    float depth;
    vec4 projectedCoord = vec4(0.0);

    for (int i = 1; i < maxSteps; i++){
        hitCoord += dir;

        projectedCoord = projectionMatrix * vec4(hitCoord, 1.0);
        projectedCoord.xy /= projectedCoord.w;
        projectedCoord.xy = projectedCoord.xy * 0.5 + 0.5;

        if (projectedCoord.x < 0.0 || projectedCoord.x > 1.0 || projectedCoord.y < 0.0 || projectedCoord.y > 1.0) {
            break;
        }

        vec3 sampledWorldPos = textureLod(gPosition, projectedCoord.xy, 2).xyz;

        vec3 sampledViewPos = (viewMatrix * vec4(sampledWorldPos, 1.0)).xyz;

        if (length(sampledWorldPos) < 0.001) {
            continue;
        }
        dDepth = hitCoord.z - sampledViewPos.z;


        if((dir.z - dDepth) < 0.3){
            if(dDepth <= 0.0){
                vec4 Result;
                Result = vec4(projectedCoord.xy, dDepth, 1.0);
                hit = 1.0;

                return Result;
            }
        }
    }
    hit = 0.0;
    return vec4(projectedCoord.xy, dDepth, 1.0);
}


float CalcShadowFactorDIR(vec4 LightSpacePos, vec3 lightDirection, vec3 normal, vec3 iPosition)
{
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

#define Scale vec3(.8, .8, .8)
#define K 19.19

vec3 hash(vec3 a){
    a = fract(a * Scale);
    a += dot(a, a.yxz + K);
    return fract((a.xxy + a.yxx)*a.zyx);
}

float hash2(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float shadowTrace(vec3 lightDirection, vec3 normal, vec3 iPosition){
    if (!doContactShadows) return 0.0f;
    vec3 viewPos    = (viewMatrix * vec4(iPosition, 1.0)).xyz;
    vec3 viewNormal = normalize(mat3(viewMatrix) * normal);

    vec3 viewLightDir = normalize(mat3(viewMatrix) * lightDirection);

    vec3 rayOrigin = viewPos + viewNormal * 0.1;


    vec2 uv = gl_FragCoord.xy;
    float noiseX = hash2(uv + vec2(time));
    float noiseY = hash2(uv + vec2(time + 1.5));
    vec3 jitter = vec3(noiseX - 0.5, noiseY - 0.5, 0.0) * 0.03;

    vec3 rayDir = normalize(viewLightDir + jitter);

    float dDepth;
    float hit = 0.0;
    
    float shadow = 0.0f;
    
    //vec4 csCoords = RayCast((vec3(jitt)) + rayDir, rayOrigin, dDepth, 12, hit, 0.05);
    vec4 csCoords = RayCast(rayDir, rayOrigin, dDepth, 64, hit, 0.05);
    if (hit == 1.0f) shadow += 1.0f;
    
    return shadow;
}

vec4 direcLight(vec3 ARM, vec3 iNormal, vec3 iPosition)
{ // normals need to be recalculated based on rotation
    
  // shadow map 
  float shadow = 0.0f;

  vec3 normal = normalize(iNormal);
    
  vec3 lightDirection = normalize(directLightPos); //vec3(1.0f, 1.0f, 0.0f)
  float diffuse = max(dot(normal, lightDirection), 0.0f);
  
    vec4 fragPosLight = lightProjection * vec4(iPosition, 1.0f);
    float smShadow = 0.0;
    float ssShadow = 0.0f;
  if (doDirShadowMap)
  {
      smShadow = CalcShadowFactorDIR(fragPosLight, lightDirection, normal, iPosition);
      
  }
    ssShadow = shadowTrace(lightDirection, normal, iPosition);
    //return vec4(ssShadow  ,0.0 ,0.0, 1.0);

    shadow = max(smShadow, ssShadow);
    //shadow = smShadow;
    //shadow = ssShadow;
  // shadow map end
    
    //float fAmbient = directAmbient * ARM.r;
  float specular = 0.0f;
  if (doReflect && doDirSpecularLight && diffuse != 0.0f){

      //return (vec4( vec3(1.0 , 0.0, 0.0), 1.0));
      
      vec3 reflectionDirection = reflect(-lightDirection, normal);
      vec3 viewDirection = normalize(camPos - iPosition);
      vec3 halfwayVec = normalize(viewDirection + lightDirection);

      float specAmount = pow(max(dot(normal, halfwayVec), 0.0f), 16);
      //float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
      specular = specAmount * dirSpecularLight;
      
      
   //   return ((diffuse * (1.0f - shadow)) + ARM.g* specular * (1.0f - shadow)) * vec4(directLightCol, 1.0f); }
  //else{ return ((diffuse * (1.0f - shadow))) * vec4(directLightCol, 1.0f); }
    
      return ((diffuse * (1.0f - shadow) + directAmbient) + ARM.g* specular * (1.0f - shadow)) * vec4(directLightCol, 1.0f); }
  else{ return ((diffuse * (1.0f - shadow) + directAmbient)) * vec4(directLightCol, 1.0f); }
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

    float ssShadow = shadowTrace(lightDirection, normal, iPosition);
    
    float shadow = ssShadow;

    float specular = 0.0f;
    if (doReflect && diffuse != 0.0f){
        // specular lighting
        //float specularLight = 0.50f;
        vec3 viewDirection = normalize( (camPos) - iPosition);
        vec3 reflectionDirection = reflect(-lightDirection, normal);

        vec3 halfwayVec = normalize(lightDirection + viewDirection);

        float specAmount = pow(max(dot(normal, halfwayVec), 0.1f), 16);
        specular = specAmount * specularLight;

        finalColour = finalColour + ((diffuse * (1.0f - shadow)* inten + 0.0f) +ARM.b * specular * (1.0f - shadow)* inten) * vec4(Lights[iteration].colour, 1.0 ) * inten;
    }
    else{
        finalColour = finalColour + ( (diffuse * (1.0f - shadow) * inten + 0.0f) * vec4(Lights[iteration].colour, 1.0) * inten);
        //finalColour = finalColour + (texture(diffuse0, texCoord) * (diffuse * inten + ambient) * vec4(Lights[iteration].colour, 1.0) * inten);
    }

    return finalColour;
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

    float ssShadow = shadowTrace(lightDirection, normal, iPosition);

    float shadow = ssShadow;

    float specular = 0.0f;
    if (doReflect && diffuse != 0.0f){

        // specular lighting
        //float specularLight = 0.50f;
        vec3 viewDirection = normalize(camPos - iPosition);
        vec3 reflectionDirection = reflect(-lightDirection, normal);

        vec3 halfwayVec = normalize(lightDirection + viewDirection);

        float specAmount = pow(max(dot(normal, halfwayVec), 0.0f), 16);
        specular = specAmount * specularLight;

        finalColour = finalColour + ((diffuse * (1.0f - shadow) * inten + 0.0f) + ARM.b * specular * (1.0f - shadow) * inten) * vec4(Lights[iteration].colour, 1.0) * inten;

    }
    else{

        finalColour = finalColour + ((diffuse * (1.0f - shadow) * inten + 0.0f) * vec4(Lights[iteration].colour, 1.0) * inten);
        //	finalColour = finalColour + (texture(diffuse0, texCoord) * (diffuse * inten + ambient) * vec4(Lights[iteration].colour, 1.0) * inten);
    }

    //finalColour = finalColour + (diffuse * inten + skyColor);

    return finalColour;
}

vec4 lights(vec3 ARM, vec3 iNormal, vec3 iPosition){
    //vec4 diffuseTex = texture(texture_diffuse0, texCoord);
    vec4 finalColour = vec4(0.0);
    //return (diffuseTex * skyColor);
    int maxLights = 64;
    for (int i = 0; i < min(lightCount, maxLights); i++)
    {
        if (Lights[i].type == 0){
            finalColour += spotLight(i, ARM, iNormal, iPosition);
        }

        if (Lights[i].type == 1){
            finalColour += pointLight(i, ARM, iNormal, iPosition);
        }

    }
    //return finalColour;
    //FragColor = direcLight(); doDirLight

    if (doDirLight) // if direct light is enabled, add it to the final color
    {
        finalColour += direcLight(ARM, iNormal, iPosition);
    }

    ///return vec4(finalColour.xyz, diffuseTex.a);
    return vec4(finalColour); // was xyz
    //return vec4((diffuseTex.xyz * skyColor.xyz) + finalColour.xyz, diffuseTex.a);
    //return (diffuseTex.xyz * skyColor.xyz) + finalColour.xyz;
}

// looks best on glass and solids
bool BayerNoiseOpacity(float Threshold) // for fade out or opacity (cheap) (could fade out near farplane or nearplane)
{
    sampler2D baySamp = sampler2D(bayerMatrixHandle);
    vec2 bayUV = vec2(gl_FragCoord.xy) / vec2(textureSize(baySamp, 0)); // new uvec2
    float bayer = texture(baySamp, bayUV).r;

    float clampedThreshold = clamp(Threshold, 0.2, 1.0);

    // normal ranges should be 0.0f-1.0f;
    if (bayer > Threshold || Threshold <= 0) return true;

    return false;
}

// looks best on decals and foliage
bool blueNoiseOpacity(float Threshold){// for fade out or opacity (cheap) (could fade out near farplane or nearplane)
    sampler2D bluemap =sampler2D(BlueNoiseHandle) ;
    vec2 noiseUV = vec2(gl_FragCoord.xy) / vec2(textureSize(bluemap, 0)); // new uvec2
    
    vec2 scrollingUV = noiseUV + fract(time * vec2(12.9898, 78.233));
    float noise = texture(bluemap, scrollingUV).r;
    //float noise = texture(bluemap, noiseUV).r;

    // normal ranges should be 0.0f-1.0f;
    if (noise > Threshold) return true;

    return false;
}

vec3 rough(vec3 ARM, vec3 iNormal, vec3 viewVector){

    //float rough = ARM.g;
    //return vec3(rough);
    if (doReflect){
        float rough = ARM.g;
        float met = ARM.b; // just gonna use this as a multiplier thingy instead of a materiallic model
        //rough =0.0;
        
        // vectors
        vec3 normal = normalize(iNormal);
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
        return reflectionColour * met;
        //return reflectionColour;
    }
    return vec3(0.0f);
}

// (thanks learnopengl)
float GeometrySchlickGGX(float NdotV, float roughness){
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / max(denom, 0.0001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness){
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

void Reflect(vec3 albedo,  vec3 iNormal, vec3 viewVector, out vec3 diffuse, out vec3 specular, vec3 ARM){

    //if (doReflect && depth < maxDist)
    if (doReflect){
        
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
}

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 sampleHemisphere2(vec3 normal, float u, float v){

    float phi = 2.0 * 3.14159265 * u;
    float cosTheta = sqrt(1.0 - v);
    float sinTheta = sqrt(v);

    vec3 localDir = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 helper = abs(normal.y) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0);
    vec3 tangent = normalize(cross(helper, normal));
    vec3 bitangent = cross(normal, tangent);

    return tangent * localDir.x + bitangent * localDir.y + normal * localDir.z;
}

vec3 sampleHemisphere(vec3 normal, float random){
    float u = rand(vec2(gl_FragCoord.xy) + random);
    float v = rand(vec2(u, random));

    float phi = 2.0 * 3.14159265 * u;
    float cosTheta = sqrt(1.0 - v);
    float sinTheta = sqrt(v);

    vec3 localDir = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 helper = abs(normal.y) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0);
    vec3 tangent = normalize(cross(helper, normal));
    vec3 bitangent = cross(normal, tangent);

    return tangent * localDir.x + bitangent * localDir.y + normal * localDir.z;
}



vec4 ssgi(int samples, vec3 gp, vec3 nrm, out vec3 emission){
    vec3 viewPos    = (viewMatrix * vec4(gp, 1.0)).xyz;
    vec3 viewNormal = mat3(viewMatrix) * nrm;

    vec3 V = normalize(viewPos);
    vec3 R = reflect(V, viewNormal);

    vec3 hitPos = viewPos;
    float dDepth;
    float hit = 0.0;

    vec3 indirectColour = vec3(0.0f);
    vec3 emissionColour = vec3(0.0f);
    
    float giboost = 1.0;
    float emboost = 1.0;
    
    if (samples <= 0) return vec4(indirectColour, 0.0);
    
    int hitcount = 0;
    
    for (int i = 0; i < samples; i++){
        vec3 randomDir = sampleHemisphere(normalize(R), (time * gl_FragCoord.z + random(gp) ));
        vec3 dir =reflect(V, randomDir);

        float hit = 0.0;
        vec4 csCoords = RayCast(randomDir, hitPos, dDepth, 32, hit, 0.4);

        if (hit == 1.0){
            hitcount++;
            vec3 hitAlbedo = texture(gAlbedoSpec, csCoords.xy).rgb;
            vec3 hitARM = texture(gSpecular, csCoords.xy).rgb;
            vec3 hitNRM = texture(gNormal, csCoords.xy).rgb;
            vec3 hitP = texture(gPosition, csCoords.xy).rgb;
            vec3 shadow = lights(hitARM, hitNRM, hitP).rgb;
            vec3 SSGI = hitAlbedo * shadow;
            vec3 EM = texture(gEmission, csCoords.xy).rgb;
            
            indirectColour += SSGI* giboost;
            emissionColour += EM * emboost;
        }
    }

    emission = clamp((emissionColour / hitcount), 0.0, 1.0), 1.0;
    return vec4( clamp((indirectColour / hitcount), 0.0, 1.0), 1.0);
}

vec3 indirectIBL(int samples, vec3 ARM, vec3 iNormal, vec3 viewVector){
    float rough = ARM.g;

    vec3 NreflectedVector = reflect(viewVector, iNormal);

    samplerCube cmSamp = samplerCube(cmMainHandle);

    int lastLOD = textureQueryLevels(cmSamp) - 1;

    float maxLod = lastLOD;

    float lod =  rough * maxLod;

    lod = min(lod, 10.0);

    //float specularIntensity = mix(0.04, 1.0, met);

    vec3 indirectColour = vec3(0.0f);

    if (samples <= 0) return indirectColour;

    sampler2D bluemap =sampler2D(BlueNoiseHandle) ;
    vec2 noiseUV = vec2(gl_FragCoord.xy) / vec2(textureSize(bluemap, 0)); // new uvec2

    vec2 scrollingUV = noiseUV + fract(time * vec2(12.9898, 78.233));
    vec2 blueNoise = texture(bluemap, scrollingUV).rg;
    
    for (int i = 0; i < samples; i++){
        //gl_FragCoord
        //vec3 randomDir = sampleHemisphere(normalize(NreflectedVector), rand(vec2(i + (gl_FragCoord.z * time) ) * vec2(1, 2)) );

        //float u = fract(blueNoise.r + float(i) * 0.61803398875);
        //float v = fract(blueNoise.g + float(i) * 0.61803398875);

        float u = fract(blueNoise.r + float(i) * 0.61803398875);
        float v = fract(blueNoise.g + float(i) * 0.61803398875);

        //vec3 randomDir = sampleHemisphere2(normalize(NreflectedVector), u , v);
        //vec3 randomDir = sampleHemisphere2(normalize(NreflectedVector), u + (gl_FragCoord.z ), v + (gl_FragCoord.z ));
        vec3 randomDir = sampleHemisphere(normalize(NreflectedVector), u + (time * gl_FragCoord.z ) );
        //vec3 randomDir = sampleHemisphere(normalize(NreflectedVector), i + time); // i thought it would be better to add time for a film grain look, it would also solve with taa

        //int skyLOD = textureQueryLevels(skybox) - 4; // use mipmap for more preformance
        vec3 skyboxColour = textureLod(cmSamp, randomDir, lod).rgb;
        //vec3 skyboxColour = texture(skybox, randomDir).rgb; 

        indirectColour += skyboxColour;
    }

    return (indirectColour / samples);
}

float sdBox(vec3 p, vec3 b){
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0f);
}

vec4 sampleTexture3D(vec3 p, vec3 vp, vec3 s, sampler3D handle, out bool emptySpace){
    emptySpace = false;
    // map coords to 3d uv space
    vec3 lp = p -vp;
    //vec3 uvw = (lp / (s* 2.0)) + 0.5;
    vec3 uvw = (lp / (s* 2.0)) + 0.5;
    //vec3 uvw = (p / (s* 2.0)) + 0.5;

    if (any(lessThan(uvw, vec3(0.0))) || any(greaterThan(uvw, vec3(1.0)))){
        emptySpace = true;
        return vec4(0.0, 0.0, 0.0, 1.0);
    }

    //return vec4(uvw, 1.0);
    return texture(handle, uvw);
}

vec4 sampleProbeGrid(vec3 p, vec3 vp, vec3 s, vec3 surfaceNrm, sampler3D handle, out bool emptySpace){
    emptySpace = true;
    vec3 HE = s *0.5f;
    vec3 DE = s *2.0f;
    if (sdBox(p - vp, HE) >= 0.0f) return vec4(vec3(0.0), -1.0);
    
    float startingOffset = 0.5;
    float bias = 0.1f;
    float pushOutBias = 0.1f;
    float minDist = 0.05f;
    int pushSamples = 8;
    bool doPush = true;

    vec3 offset = p+ surfaceNrm * startingOffset;
    
    vec4 tp = sampleTexture3D(offset, vp, HE, handle, emptySpace);
    
    if (!doPush) return tp;

    if (tp.a <= minDist) {
        for (int i = 0; i < pushSamples; i++){

            if (tp.a <= minDist) {
                
               // if (sdBox(offset - vp, s) >= 0.0f) return vec4(HE);
                
                bias += pushOutBias;
                offset = p + surfaceNrm * bias;
                tp = sampleTexture3D(offset, vp, HE, handle, emptySpace);
            }
        }
    }
    return tp;
}

vec4 triplanarMap(sampler2D iTexture, vec3 position, vec3 normal, vec2 offset){
    vec2 uvX = position.zy / vec2(2.0f);
    vec2 uvY = position.xz / vec2(2.0f);
    vec2 uvZ = position.xy / vec2(2.0f);

    vec4 tX =texture(iTexture, uvX + offset);
    vec4 tY =texture(iTexture, uvY + offset);
    vec4 tZ =texture(iTexture, uvZ + offset);

    vec3 blendedWeights = abs(normal);
    blendedWeights = pow(blendedWeights, vec3(4.0));
    blendedWeights /= (blendedWeights.x + blendedWeights.y + blendedWeights.z);
    return tX * blendedWeights.x + tY * blendedWeights.y + tZ * blendedWeights.z;
}

void main(){
    vec2 velocity = texture(gVelocity, texCoord).rg;
    //vec2 scaledVelocity = velocity * 1.0;
    //FragColor = vec4(scaledVelocity, 0.0f, 1.0f);

    //return;

    vec4 albedo = texture(gAlbedoSpec, texCoord);
    //vec3 skyGradient = texture(skyGradientTexture, texCoord).rgb;

    //FragColor = vec4(skyGradient.rgb, 1.0f);
    //return;

    if (albedo.a <= 0.0){
        FragColour = vec4(albedo.rgb, 1.0f); return; // sg
    }

    float depth = texture(depthMap, texCoord).r;

    //early z cutoff
    if (depth >= 0.99999){
        FragColour = vec4(albedo.rgb, 1.0f); // sg
        return;
    }

    vec3 normal = texture(gNormal, texCoord).rgb;


   // vec3 viewNormal = vec3(normal * vec3(vec4(inverseViewMatrix).xyz) );
   // vec3 viewPos = textureLod(gPosition, texCoord, 2).xyz;

 //   vec3 reflected = normalize(reflect(normalize(viewPos), normalize(viewNormal)));

   // vec3 e = vec3(vec4(inverseViewMatrix).xyz);

    //FragColor = vec4( e, 1.0);
    
    //return;


    //                      AO  Rough  MAT
    //vec3 ARM = vec3(0.0, 1.0, 1.0); // placeholder
    vec3 ARM = texture(gSpecular, texCoord).rgb;// placeholder
    //gSpecular  

    //float displacement = texture(gNormal, texCoord).a;

    vec3 position = texture(gPosition, texCoord).rgb;
    //float depth2 = linearizeDepth(texture(gPosition, texCoord).a, 0.1, 100.0);
    //gl_fragdepth
    //FragColor = vec4(position, 1.0); return;
    
    vec3 viewVector = position - camPos;
    
    vec3 emission = texture(gEmission, texCoord).rgb;
    
    //vec3 direct = ARM.r * lights(ARM, normal, position).rgb;
    vec3 shadow =  lights(ARM, normal, position).rgb;
    //vec3 direct = shadow * ARM.r;
    //float nCaulstic = triplanarMap(caustic, position, normal, vec2(0.0)).r;


    //caustic
    vec3 direct = shadow;
    //direct += nCaulstic;
    
    //vec3 CMGI = indirectIBL(indirectSamples, ARM, normal, viewVector);// [placeholder
    vec3 ssem = vec3(0.0f);
    vec3 ssgi = ssgi(indirectSamples, position, normal, ssem).rgb;

    //vec3 indirect = ssgi * CMGI.rgb;
    vec3 indirect = ssgi;
    //vec3 indirect = CMGI.rgb;


    //vec3 volumePos = vec3(-30.000, 17.000, 42.000);
    //vec3 volumeScale = vec3(45.0f, 20.0f, 25.0f);

    /*
    // should do these in one
    bool emptySpace = false;
    vec4 indirectProbe = sampleProbeGrid(position, ivPosition, vec3(ivScale), normal, indirectVolume, emptySpace);
    vec4 emissionProbe = sampleProbeGrid(position, ivPosition, vec3(ivScale), normal, emissionVolume, emptySpace);
    //fragColour = vec4(indirectProbe.rgb, 1.0f);
    if (emptySpace){
        indirectProbe.rgb = textureLod(samplerCube(cmMainHandle), vec3(0.0, 1.0, 0.0) * 5.0f, 10).rgb;
    }
    */
    
    //vec3 gi = (direct + indirectProbe.rgb);
    vec3 gi = (direct + indirect);

    vec3 specular = vec3(0.0f);
    vec3 diffuse  = vec3(0.0f);

    //Reflect(albedo.rgb, normal, viewVector, diffuse, specular, ARM);
    //bool bClamp = false;
    //vec4 nssr = vec4(0.0); //bClamp
    
    vec3 reflections = vec3(0.0);
    
   // if (doReflect)
    //{
        //Reflect(albedo.rgb, normal, viewVector, diffuse, specular, ARM);
        //reflections = diffuse + specular;
    //}
    

    //if (blueNoiseOpacity(0.0)) nssr = ssr(ARM);
    
    //FragColor = nssr; return;
    //vec3 reflections = diffuse + specular;
    
    
    //vec3 cubeMapPlusSSR = reflections;
    //vec3 cubeMapPlusSSR = mix(reflections, nssr.rgb, nssr.a);
    
    vec3 final = albedo.rgb * gi + reflections + ssem;
    //vec3 final = albedo.rgb * gi + reflections + emissionProbe.rgb;
    


    //vec3 final = direct;
    //vec3 final = albedo.rgb * gi + nssr.rgb + emission;
    //vec3 final = 
    //vec3 final = albedo.rgb *  gi + roughness;
    
    //final = vec4(totalDiffuse, 1.0);

    FragColour = vec4(final, 1.0f);
    //FragColour = vec4(indirect, 1.0f);
    //FragColour.rgb = normal;
    //FragColour = vec4(vec3(depth2), 1.0);
    //FragColour = vec4(vec3(linearizeDepth(depth , 0.1, 100.0)), 1.0);
    //FragColour = vec4(position, 1.0);
    /*
    vec3 volumePos = vec3(0.0f);
    vec3 volumeScale = vec3(50.0f);

    vec4 indirectProbe = sampleProbeGrid(position, volumePos, volumeScale, normal, BN3D);
    //tp = vec4(clamp(vec3(tp.a), 0.0, 1.0), tp.a);
    FragColour = vec4(indirectProbe.rgb, 1.0f);
*/
    //FragColour = vec4(indirectProbe.rgb, 1.0f);
    
    //FragColour = vec4( vec3(linearizeDepth(depth, 0.1, 5.0)), 1.0);
    
    //float ld = linearizeDepth(depth, 0.1, 10.0);

    //FragColor.rgb = vec3(ld);
    
    //FragColor.rgb = direct;
    
    //FragColor.rgb = ssgi(indirectSamples, ARM).rgb;
    
    //FragColor = vec4(vec3(ARM.g, 0.0, 0.0), 1.0);

    //FragColor = vec4(vec3(displacement),1.0);
    //FragColor = vec4(nEmissionblur, 1.0f);
    //FragColor  = vec4(emission, 1.0f);
    //FragColor = final;

    //FragColor = vec4(vec3(hDepthLinearized) , 1.0);
    
    //FragColor = vec4(texture(hColour, texCoord));
    
//    FragColor = vec4(final , 1.0f);

    //vec2 velocity = texture(gVelocity, texCoord).rg;
    
    //FragColor = vec4(vec3(toon) , 1.0f);
    
    //FragColor = vec4(rough(ARM, normal, viewVector), 1.0f);
    //FragColor = vec4(vec3(ARM.g), 1.0f);
    
    //FragColor = vec4(vec3(displacement), 1.0);
    //FragColor = vec4(normal, 1.0);
    //FragColor = vec4(normalize(normal), 1.0);
    //FragColor = vec4(albedo.rgb, 1.0);
}