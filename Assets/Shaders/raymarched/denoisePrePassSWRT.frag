#version 460 core

out vec4 FragColor;
in highp vec2 texCoord;

layout(location = 0) out vec4 hIndirect;
layout(location = 1) out vec4 hEmission;
layout(location = 2) out vec4 hIndirectSpecular;
layout(location = 3) out vec4 hEmissionSpecular;
layout(location = 4) out vec4 hSpecular;
layout(location = 5) out float hdepth;
layout(location = 6) out vec2 filteredVariance;

uniform sampler2D depthMap;
uniform sampler2D gNormal;
uniform sampler2D swrtIndirect;
uniform sampler2D swrtEmission;
uniform sampler2D swrtIndirectSpecular;
uniform sampler2D swrtEmissionSpecular;
uniform sampler2D swrtSpecular;

uniform bool doDenoise;
uniform bool doDenoiseSplitDBGView;
uniform float NearPlane;
uniform float FarPlane;

//bool forceMirror = true;

float lumaFromRGB(vec3 rgb){
    vec3 weights = vec3(0.2126, 0.7152, 0.0722);
    float luminance = dot(rgb, weights);
    return luminance;
}

float linearizeDepth(float depth, float NearPlane, float FarPlane){
    return (2.0 * NearPlane * FarPlane) / (FarPlane + NearPlane - (depth * 2.0 - 1.0) * (FarPlane - NearPlane));
}

float edgeAwareBoxBlur(sampler2D inputSampler, float colour, float linearizedDepth, vec3 normal, float farplane, int radius){
    vec2 lscreenSize = textureSize(inputSampler, 0);
    vec2 ltexelSize = 1.0 / lscreenSize;

    int h = 0; float avgC = 0.0f;
    for(int x = -radius; x <= radius; ++x){
        for(int y = -radius; y <= radius; ++y){
            vec2 offset = vec2(x, y) * ltexelSize;
            float neighbourDepth = texture(depthMap, texCoord + offset).r;
            float neighbourDepthLinearized = linearizeDepth(neighbourDepth, 0.1f, farplane);
            float depthDifference = abs(neighbourDepthLinearized - linearizedDepth);

            vec3 neighbourNormal = texture(gNormal, texCoord + offset).rgb;
            float normalSimilarity = dot(neighbourNormal, normal);
            if (depthDifference > 0.1 || normalSimilarity < 0.9) continue;

            float neighbourColour = texture(inputSampler, texCoord + offset).r;
            avgC += neighbourColour;

            h++;
        }
    }
    return avgC / h;
}


void main(){
    vec4 indirect               = texture(swrtIndirect,texCoord);
    vec4 emission             = texture(swrtEmission,texCoord);
    vec4 indirectSpecular  = texture(swrtIndirectSpecular,texCoord);
    vec4 emissionSpecular = texture(swrtEmissionSpecular,texCoord);
    vec4 specular               = texture(swrtSpecular,texCoord);
    float gdepth                 = texture(depthMap, texCoord).r;
    vec3 gnrm                   = normalize(texture(gNormal, texCoord).rgb);
    
    // set H

    hIndirect                = indirect;
    hEmission              = emission;
    hIndirectSpecular   = indirectSpecular;
    hEmissionSpecular = emissionSpecular;
    hSpecular               = specular;
    hdepth                  = gdepth;
    
    
    vec2 lscreenSize = textureSize(swrtIndirect, 0);
    bool split = true;
    
    vec2 finalVariance = vec2(indirect.a, emission.a);
    
    int denoiseRadius = 1;
    
    if (gl_FragCoord.x > lscreenSize.x / 2 && doDenoiseSplitDBGView) split = false;
    if (doDenoise && split){
        float d = linearizeDepth(gdepth, NearPlane, FarPlane);
        finalVariance.r = edgeAwareBoxBlur(swrtIndirect, indirect.a, d, normalize(gnrm), FarPlane, denoiseRadius);
        finalVariance.g = edgeAwareBoxBlur(swrtEmission,emission.a, d, normalize(gnrm), FarPlane, denoiseRadius);
    }

    filteredVariance = finalVariance;
}

