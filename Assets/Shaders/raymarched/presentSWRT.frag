#version 460 core

out vec4 FragColor;
in highp vec2 texCoord;

//presentImage

uniform sampler2D presentImage;
uniform sampler2D depthMap;
uniform sampler2D gAlbedoSpec;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gSpecular;

uniform sampler2D dIndirect;
uniform sampler2D dEmission;

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
        float specularFactor = ARM.g;

        vec3 reflectionDirection = reflect(-lightDirection, normal);
        vec3 viewDirection = normalize(cameraPosition - iPosition);
        vec3 halfwayVec = normalize(viewDirection + lightDirection);

        float specAmount = pow(max(dot(normal, halfwayVec), 0.0f), 16);
        //float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
        specular = specAmount * dirSpecularLight;


        return ((diffuse * (1.0f - shadow) + directAmbient) + specularFactor* specular * (1.0f - shadow)) * vec4(directLightCol, 1.0f); }
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
        float specularFactor = ARM.g;
        
        // specular lighting
        //float specularLight = 0.50f;
        vec3 viewDirection = normalize(cameraPosition - iPosition);
        vec3 reflectionDirection = reflect(-lightDirection, normal);

        vec3 halfwayVec = normalize(lightDirection + viewDirection);

        float specAmount = pow(max(dot(normal, halfwayVec), 0.0f), 16);
        specular = specAmount * specularLight;

        finalColour = finalColour + ((diffuse * (1.0f - shadow) * inten + 0.0f) + specularFactor * specular * (1.0f - shadow) * inten) * vec4(Lights[iteration].colour, 1.0) * inten;
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
        float specularFactor = ARM.g;
        
        // specular lighting
        //float specularLight = 0.50f;
        vec3 viewDirection = normalize( cameraPosition - iPosition);
        vec3 reflectionDirection = reflect(-lightDirection, normal);

        vec3 halfwayVec = normalize(lightDirection + viewDirection);

        float specAmount = pow(max(dot(normal, halfwayVec), 0.1f), 16);
        specular = specAmount * specularLight;

        finalColour = finalColour + ((diffuse * (1.0f - shadow)* inten + 0.0f) + specularFactor * specular * (1.0f - shadow)* inten) * vec4(Lights[iteration].colour, 1.0 ) * inten;
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

void main(){
    float gdepth = texture2D(depthMap, texCoord).r;
    vec3 galbedo = texture(gAlbedoSpec, texCoord).rgb;
    vec3 garm = texture(gSpecular, texCoord).rgb;
    vec3 gnrm = texture(gNormal, texCoord).rgb;
    vec3 gp = texture(gPosition, texCoord).rgb;
    vec3 reflect = texture(presentImage, texCoord).rgb;
    vec3 indirect = texture(dIndirect, texCoord).rgb;
    vec3 emission = texture(dEmission, texCoord).rgb;
    
    //float far = 500.0f;
    //vec3 blurredIndirect = edgeAwareBoxBlur(dIndirect, indirect, linearizeDepth(gdepth, 0.1f, far), gnrm, far, 3);
    vec3 direct = lights(garm, gnrm, gp).rgb;
    vec3 gi = (direct + indirect);
    vec3 final = gi + reflect + emission;
    
    if (gdepth >= 0.99999) { final = vec3(1.0f);}
    vec3 combined = galbedo * final;
    //vec3 combined = final;

    
    /// dbg
    /* */
    #if 0
        //if (gdepth >= 0.99999) return;
        FragColor = vec4(reflect, 1.0);
        return;
    #endif
    
    /*
    float near = 0.1f;
    float far = 120.0f;
    float depthDist = 30.0f;
    vec3 fogColour = vec3(0.3f);
    float steepness = 0.1f;
    
    vec3 colour = calculateFog(near, far, steepness, depthDist, gdepth, fogColour, combined);
    */
        
    FragColor = vec4(combined, 1.0f); //texture(presentImage, texCoord)
}