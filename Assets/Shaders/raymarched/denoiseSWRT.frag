#version 460 core

//out vec4 FragColor;
in highp vec2 texCoord;

layout(location = 0) out vec3 presentImage;
// denoise buffer
layout(location = 1) out vec4 dindirect;
layout(location = 2) out vec4 dEmission;
//layout(location = 3) out vec4 dIndirectSpecular;
//layout(location = 4) out vec4 dEmissionSpecular;

uniform sampler2D swrtSpecular;

uniform sampler2D gEmission;
uniform sampler2D gNormal;
uniform sampler2D depthMap;
uniform sampler2D gSpecular;
uniform sampler2D gPosition;

uniform sampler2D swrtIndirect;
uniform sampler2D swrtEmission;
uniform sampler2D swrtIndirectSpecular;
uniform sampler2D swrtEmissionSpecular;
uniform sampler2D filteredVariance;

uniform int denoiseRadius;
uniform bool doDenoise;
uniform bool doDenoiseSplitDBGView;

uniform float NearPlane;
uniform float FarPlane;

uniform int passIndex;
uniform int passNum;
uniform int passRadius;

float lumaFromRGB(vec3 rgb){
    vec3 weights = vec3(0.2126, 0.7152, 0.0722);
    float luminance = dot(rgb, weights);
    return luminance;
}

float kernel[5] = float[5](1.0/16.0, 1.0 / 4.0, 3.0 / 8.0, 1.0 / 4.0, 1.0 / 16.0);

//vec4 indirectWaveletDenoise(sampler2D inputSampler, sampler2D variance, int channel, vec3 colour, float linearizedDepth, vec3 normal, float farplane, int radius){
vec4 indirectWaveletDenoise(sampler2D inputSampler, sampler2D variance, int channel, vec3 colour, vec3 cGP, vec3 normal, float farplane){
    vec2 inputSize = textureSize(inputSampler, 0);
    vec2 texelSize = 1.0 / inputSize;

    float cVar = texture(variance, texCoord)[channel];
    float cLuma = lumaFromRGB(colour);

    //float sigmaDepth = 0.05f;
    float sigmaPos = 0.05; //  0.05
    float sigmaNormal = 0.2; // 0.2;
    //float sigmaPos = 1.0; //  0.05
    //float sigmaNormal = 1.0; // 0.2;
    
    vec3 sumColour = vec3(0.0f);
    float sumWeight = 0.0f;
    float sumVar = 0.0f;
    float sumWeightsq = 0.0f;

    int r = 2; // force to 2 rn for 5 radius
    for(int x = -r; x <= r; ++x)
    for(int y = -r; y <= r; ++y){
        vec2 offset = vec2(x, y) * float(passRadius) * texelSize;

        float spatialWeight = kernel[x + 2] * kernel[y + 2];

        // gonna sample up here
        vec3 neighbourColour = texture(inputSampler, texCoord + offset).rgb;

        vec3 neighbourGposition = texture(gPosition, texCoord + offset).rgb;
        vec3 gPositionDifference = neighbourGposition - cGP;
        float dist2 = dot(gPositionDifference,gPositionDifference);
        float positionWeight = min(exp(-(dist2)/sigmaPos),1.0);

        vec3 neighbourNormal = texture(gNormal, texCoord + offset).rgb;
        float normalCos = max(dot(neighbourNormal, normal), 0.0);
        float normaDifference = 1.0 - normalCos;
        float normalWeight = exp( -(normaDifference * normaDifference) / (2.0 * sigmaNormal * sigmaNormal));

        float neighbourVar = texture(variance, texCoord + offset)[channel];
        float neighbourLuma = lumaFromRGB(neighbourColour);
        float lumaDifference  = abs(cLuma - neighbourLuma);
        
        float noiseLvl = sqrt(max(0.0, cVar));
        float lumaWeight = exp(-lumaDifference / (4.0 * noiseLvl + 1e-4) );
        
        //float tWeight = spatialWeight * depthWeight * normalWeight * lumaWeight;
        float tWeight = spatialWeight * positionWeight * normalWeight * lumaWeight;
        //float tWeight = spatialWeight * positionWeight * normalWeight;
        
        sumColour += neighbourColour * tWeight;
        sumVar += neighbourVar * (tWeight * tWeight);
        sumWeight += tWeight;
        sumWeightsq += (tWeight * tWeight);
    }
    
    vec4 outColour = vec4(vec3(
            (sumWeight > 0.0) ? (sumColour / sumWeight) : colour),  
            (sumWeightsq > 0.0) ? (sumVar / sumWeightsq) : cVar);
    return outColour;
}

void main(){
    float gdepth                 = texture2D(depthMap, texCoord).r;
    if (gdepth >= 0.99999) discard;

    vec4 indirect                = texture(swrtIndirect, texCoord);
    vec4 emission              = texture(swrtEmission, texCoord);
    vec3 indirectSpecular   = texture(swrtIndirectSpecular, texCoord).rgb;
    vec3 emissionSpecular = texture(swrtEmissionSpecular, texCoord).rgb;
    vec3 specular               = texture(swrtSpecular, texCoord).rgb;
    
    vec3 gemission  = texture(gEmission, texCoord).rgb;
    vec3 gnrm         = normalize(texture(gNormal, texCoord).rgb); // move into do denoise
    vec3 gSpecular   = texture(gSpecular, texCoord).rgb;
    
    vec4 nIndirect                = indirect;
    vec4 nEmission              = emission;

    vec2 lscreenSize = textureSize(swrtIndirect, 0);
    bool split = true;
    if (texCoord.x > lscreenSize.x / 2 && doDenoiseSplitDBGView) split = false;
    
    if (doDenoise && split){
        vec3 gp = texture(gPosition, texCoord).rgb;
        if (passIndex == 0){ // first pass
            nIndirect = indirectWaveletDenoise(swrtIndirect, filteredVariance, 0, indirect.rgb, gp, gnrm, FarPlane);
            nEmission = indirectWaveletDenoise(swrtEmission, filteredVariance, 1, emission.rgb, gp, gnrm, FarPlane);
        }
        else{ // secondary (reads off pingpong)
            nIndirect = indirectWaveletDenoise(swrtIndirect, swrtIndirect, 3, indirect.rgb, gp, gnrm, FarPlane);
            nEmission = indirectWaveletDenoise(swrtEmission, swrtEmission, 3, emission.rgb, gp, gnrm, FarPlane);
        }
    }

    // write into d buffer
    dindirect = nIndirect;
    dEmission = nEmission;
    
    if (passNum != passIndex + 1) return;
    //vec3 gi = direct.rgb + nIndirect.rgb;
    // nSpecular - indirectSpecular < i currently am passing specualr thru here temporarily 
    // needs to be seperated into two
    presentImage = clamp(specular, 0.0, 1.0);
    dEmission.rgb = (nEmission.rgb + emissionSpecular + gemission);

    //vec3 combine = nIndirect.rgb + (nEmission.rgb + emissionSpecular + gemission);
    
    //FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    //FragColor = vec4(gi, 1.0f);
    //presentImage = direct.rgb;
    //presentImage = gi;
    //presentImage = nIndirectSpecular.rgb;
    //presentImage = combine.rgb;
    //presentImage = nIndirect.rgb;
    //presentImage = nIndirectSpecular.rgb;
}