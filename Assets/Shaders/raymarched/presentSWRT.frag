#version 460 core
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_bindless_texture : require

#define clipmapcount 4

out vec4 FragColor;
in highp vec2 texCoord;

//presentImage

uniform sampler2D gEmission;
uniform sampler2D swrtSpecular;
uniform sampler2D depthMap;
uniform sampler2D gAlbedoSpec;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gSpecular;

uniform sampler2D dIndirect;
uniform sampler2D dEmission;

//uniform sampler2D swrtAO;

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
uniform sampler2DShadow  shadowMap;
uniform int FilterRadius;
uniform int NumberOfSamples;
uniform float DirSMMaxBias;
uniform bool doDirShadowMap;

uniform ivec2 screenSize;
uniform ivec2 scaledScreenSize;

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

// global distance fields
layout(std430, binding = 12) buffer GDF_Buffer {
    clipmapLevel GDFS[];
};

float CalcShadowFactorDIR(vec4 LightSpacePos, vec3 lightDirection, vec3 normal, vec3 iPosition){
    // perform perspective divide
    vec3 lightCoords = LightSpacePos.xyz / LightSpacePos.w;
    float shadow = 0.0f;

    // shadow calculation
    if (lightCoords.z <= 1.0f){
        lightCoords = (lightCoords + 1.0f) / 2.0f;
        // get the current depth
        float currentDepth = lightCoords.z;
        // calculate shadow bias
        float bias = max(DirSMMaxBias * (1.0f - dot(normal, lightDirection)), 0.0005f);
        // PCF
        int sampleRadius = FilterRadius; // FilterRadius // NumberOfSamples
        vec2 texSize = vec2(textureSize(BlueNoiseHandle, 0));
        // uv
        vec2 offset = vec2(fract(frame * 0.618), fract(frame * 0.133));
        vec2 noiseUV = (gl_FragCoord.xy / texSize) + offset;

        vec2 pixelSize = 1.0 / textureSize(shadowMap, 0);
        float tsamples = 0.0;
        for(int y = -sampleRadius; y <= sampleRadius; y++){
            for(int x = -sampleRadius; x <= sampleRadius; x++){
                float angle = texture(BlueNoiseHandle, noiseUV).r * NumberOfSamples;
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
    
    float specular = 0.0f;
    if (doReflect && doDirSpecularLight && diffuse != 0.0f){

        vec3 reflectionDirection = reflect(-lightDirection, normal);
        vec3 viewDirection = normalize(cameraPosition - iPosition);
        vec3 halfwayVec = normalize(viewDirection + lightDirection);

        float specAmount = pow(max(dot(normal, halfwayVec), 0.0f), 16);
        //float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
        specular = specAmount * dirSpecularLight;


        return ((diffuse * (1.0f - shadow) + directAmbient) +  (1.0 - ARM.g) * specular * (1.0f - shadow)) * vec4(directLightCol, 1.0f); }
    else{ return ((diffuse * (1.0f - shadow) + directAmbient)) * vec4(directLightCol, 1.0f); }
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

        finalColour = finalColour + ((diffuse * (1.0f - shadow) * inten + 0.0f) +  (1.0 - ARM.g) * specular * (1.0f - shadow) * inten) * vec4(Lights[iteration].colour, 1.0) * inten;
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

        finalColour = finalColour + ((diffuse * (1.0f - shadow)* inten + 0.0f) +  (1.0 - ARM.g) * specular * (1.0f - shadow)* inten) * vec4(Lights[iteration].colour, 1.0 ) * inten;
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

float linearizeDepth(float depth, float NearPlane, float FarPlane){
    return (2.0 * NearPlane * FarPlane) / (FarPlane + NearPlane - (depth * 2.0 - 1.0) * (FarPlane - NearPlane));
}

float logisticDepth(float depth, float steepness, float offset, float NearPlane, float FarPlane){
    float zVal = linearizeDepth(depth, NearPlane, FarPlane);
    float expVal = exp(clamp(-steepness * (zVal - offset), -10.0, 10.0));
    return 1.0 / (1.0 + expVal);
}

vec3 calculateFog(float near, float far, float steepness, float depthDistance, float depth, vec3 fogColour, vec3 colour){
    float linearizedFogDepth = linearizeDepth(depth, near, far);
    float logisticizedDepth = logisticDepth(depth, steepness, depthDistance, near, far);
    return colour * (1.0f - logisticizedDepth) + vec3(logisticizedDepth * vec3(fogColour)); // fog
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) { return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); }

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898, 78.233))) * 43758.5453);
}

float sdBox(vec3 p, vec3 b){
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0f);
}

#define INF 3.40282347e+38f

vec4 gdfScene(vec3 p, out int cascade){
    float nBD = INF;
    cascade = -1;
    //return vec4(INF, 0.0, 0.0, 0.0);
    //for (int i = 0; i < 1; i++ ){
    for (int i = 0; i < clipmapcount; i++ ){
        vec3 pos = GDFS[i].ps.xyz;
        vec3 scale = vec3(GDFS[i].ps.w);
        vec3 gp = p - pos;
        
        float bd = sdBox(gp, scale);
        if (bd > 0.0 || bd > -1.0 && bd < 0.0 && rand(texCoord + vec2(i) + time) < (bd + 1.0)){
            //if (bd > 0.0){
            nBD = min(nBD, bd);
            continue;
        }

        cascade = i;

        vec3 distance = abs(gp) - scale;

        float outsideDistance = length(max(distance, 0.0));

        // map coords to 3d uv space
        vec3 uvw = (gp / (scale* 2.0)) + 0.5;

        float SDF = textureLod(sampler3D(GDFS[i].handle), uvw, 0).r;
        //float spheredist = sdCapsule(p - cameraPosition, vec3(0.0, 0.5, 0.0), vec3(0.0, -1.3, 0.0), 0.5);
        //SDF = fOpUnionID4(SDF, vec4(spheredist, vec3(1.0)));
        //SDF.y = texelFetch(sampler3D(GDFS[i].handle), ivec3(uvw), 0).y;
        //
        //float ID = texelFetch(sampler3D(GDFS[i].handle), ivec3(uvw), 0).g;
        //vec2 SDF = vec2(dist, ID);
        if (outsideDistance > 0.0) return vec4(SDF + outsideDistance, uvw);

        return vec4(SDF, uvw);
    }

    return vec4(INF, 0.0, 0.0, 0.0);
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

#define indirectMetallicMode 1 // 0 = false, 1 = true 
#define forceMirror 0// 0 = false, 1 = true, 2 = true + unaltered metallic
float i_indirectBoost = 1.0f; // default at 1.0
float e_emissionBoost = 1.0f; // default at 1.0

void main(){
    float gdepth = texture2D(depthMap, texCoord).r;
    vec3 galbedo = texture(gAlbedoSpec, texCoord).rgb;
    vec3 garm = texture(gSpecular, texCoord).rgb;
    vec3 gnrm = texture(gNormal, texCoord).rgb;
    vec3 gp = texture(gPosition, texCoord).rgb;
    vec3 reflect = texture(swrtSpecular, texCoord).rgb;
    vec3 indirect = texture(dIndirect, texCoord).rgb;
    vec3 emission = texture(dEmission, texCoord).rgb;
    vec3 gemission  = texture(gEmission, texCoord).rgb;
    
    indirect *= i_indirectBoost;
    emission *= e_emissionBoost;

    #if forceMirror == 1
        garm.g = 0.0;
        garm.b = 1.0;
    #elif forceMirror == 2
        garm.g = 0.0;
    #endif
    
    #if indirectMetallicMode == 1
    float met = garm.b;

    vec3 F0 = mix(vec3(0.04), galbedo, met);
    vec3 Fresnel = fresnelSchlick(max(dot(gnrm, normalize(cameraPosition - gp)), 0.0), F0);

    float specularWeight = mix(0.04, 1.0, met);
    vec3 diffuseWeight = (1.0 - Fresnel) * (1.0 - met);

    indirect *= clamp(diffuseWeight, 0.0, 1.0);
    #endif
    
    //float far = 500.0f;
    //vec3 blurredIndirect = edgeAwareBoxBlur(dIndirect, indirect, linearizeDepth(gdepth, 0.1f, far), gnrm, far, 3);
    
    //float dfao = mix(0.3, 1.0, texture(swrtAO, texCoord).r);
    //float dfao = mix(0.0, 1.0, texture(swrtAO, texCoord).r);
    vec3 direct = lights(garm, gnrm, gp).rgb;
    //vec3 gi = (direct + indirect) * dfao;
    vec3 gi = (direct + indirect);
    //vec3 final = gi + reflect + (emission + gemission);
    vec3 final = gi + reflect + gemission;
    
    if (gdepth >= 0.99999) { final = vec3(1.0f);}
    vec3 combined = galbedo * final;
    //vec3 combined = final;
    
    /// dbg
    /* */
    #if 0
        //FragColor.rgb = galbedo * (indirect + texture(swrtDirect, texCoord).rgb) + reflect + gemission;
    //FragColor.rgb = vec3(dfao);
    //return;
    /*
        int cascade = 0;  //  + gnrm * 0.9
        vec3 uvw = gdfScene(gp + gnrm * 2.0, cascade).yzw;
        vec4 vxalb = textureLod(sampler3D(GDFS[cascade].handle2), uvw, 0);
        vec4 vxirc = textureLod(sampler3D(GDFS[cascade].handle5), uvw, 0);
        vec4 vxerc = textureLod(sampler3D(GDFS[cascade].handle6), uvw, 0);
        vec3 fIND = vec3(0.0);
        vec3 fEM = vec3(0.0);
        if (vxirc.w > 0.0){
            fIND = vxirc.rgb;
            fEM = vxerc.rgb;
        }
*/
        //FragColor.rgb = (direct + (mix(vxirc.rgb, indirect, 0.5)));
        //FragColor.rgb = (direct + fIND.rgb) + fEM;
        //FragColor.rgb = fIND.rgb + fEM;    
        //FragColor.rgb = ( (direct + fIND.rgb) * dfao) + fEM;
   //     FragColor.rgb = (direct + fIND.rgb) + fEM;
        //FragColor = vec4(gi, 1.0);
        //FragColor.rgb = galbedo * vec3(direct + vxirc.rgb);
        //if (gdepth >= 0.99999) return;
        //FragColor = vec4(reflect, 1.0);
        //FragColor = vec4(gi, 1.0);
        //FragColor = vec4(final, 1.0);

    float variance = texture(dIndirect, texCoord).w;
    FragColor = vec4(TurboColormap(variance), 1.0);
    //FragColor = vec4(vec3(variance), 1.0);
    return;
    #endif
    
    /*
    float near = 0.1f;
    float far = 120.0f;
    float depthDist = 70.0f;
    vec3 fogColour = vec3(0.7, 0.7f, 0.7f);
    float steepness = 0.1f;
    if (gdepth >= 0.99999) {
        FragColor = vec4(combined, 1.0f); //texture(presentImage, texCoord)
        return;
    }
    combined = calculateFog(near, far, steepness, depthDist, gdepth, fogColour, combined);
    */
        
    FragColor = vec4(combined, 1.0f); //texture(presentImage, texCoord)
}