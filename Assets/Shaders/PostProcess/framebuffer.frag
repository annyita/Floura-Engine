#version 460 core

out vec4 FragColor;
in vec2 texCoords;

uniform sampler2D depthTexture;
uniform sampler2D albedo;
uniform sampler2D normal;
uniform sampler2D screenTexture;
uniform sampler2D BlueNoiseTex;
uniform mat4 cameraMatrix;

uniform sampler2D dbgColour;
uniform bool overlayDebug;

uniform float gamma;
uniform float sharpness;
uniform float time;
uniform float deltaTime;
uniform float accum24value;

uniform sampler3D LUT;

float calculateExposure(vec3 avgColor){
    float lum = dot(avgColor, vec3(0.2126, 0.7152, 0.0722));
    
    lum = max(lum, 0.0001f); 

    //float middleGray = 1.0f;
    float middleGray = 0.18f;
    float acesBoost = 1.6f;
    
    float minExp = 0.01f;
    //float maxExp = 5.0f;
    float maxExp = 2.0f;

    //float targetExposure = (middleGray / lum);
    float targetExposure = (middleGray / lum) * acesBoost;

    return clamp(targetExposure, minExp, maxExp);
}

// https://www.shadertoy.com/view/XdcXzn# credit here for this one
mat4 saturationMatrix( float saturation ){
    vec3 luminance = vec3( 0.3086, 0.6094, 0.0820 );

    float oneMinusSat = 1.0 - saturation;

    vec3 red = vec3( luminance.x * oneMinusSat );
    red+= vec3( saturation, 0, 0 );

    vec3 green = vec3( luminance.y * oneMinusSat );
    green += vec3( 0, saturation, 0 );

    vec3 blue = vec3( luminance.z * oneMinusSat );
    blue += vec3( 0, 0, saturation );

    return mat4( red,     0,
    green,   0,
    blue,    0,
    0, 0, 0, 1 );
}

vec3 ACESFilm(vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

float random (vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

float kernel[9] = float[](
0.0, -1.0,  0.0,
-1.0,  5.0, -1.0,
0.0, -1.0,  0.0
);


vec3 vignette(vec3 colour, float u, float v){
  vec2 nvuv = texCoords - 0.5;
  float nvdist = length(nvuv);
  float vignette = smoothstep(u, v, nvdist);
  colour.rgb *= vignette;

  return colour;
}

vec3 filmgrain(vec3 colour, float amount, float randomValue)
{
  float noise = random(vec2(texCoords.x * randomValue, texCoords.y + randomValue + 2) );

    //vec2 noiseUV = vec2(texCoords.xy) / vec2(textureSize(BlueNoiseTex, 0)); // new uvec2

    //vec2 scrollingUV = noiseUV + fract(randomValue * vec2(12.9898, 78.233));
    //vec2 blueNoise = texture(BlueNoiseTex, scrollingUV).rg;
    
  return colour + noise.r * amount;
}

vec3 filmgrainColoured(vec3 colour, float amount, float randomValue){
  float r = random(vec2(texCoords.x + randomValue, texCoords.y + randomValue) );
  float g = random(vec2(texCoords.x + randomValue + 0.1, texCoords.y + randomValue + 0.3) );
  float b = random(vec2(texCoords.x + randomValue + 0.2, texCoords.y + randomValue) );

    vec2 noiseUV = vec2(texCoords.xy) / vec2(textureSize(BlueNoiseTex, 0)); // new uvec2

    vec2 scrollingUV = noiseUV + fract(randomValue * vec2(12.9898, 78.233));
    vec2 blueNoise = texture(BlueNoiseTex, scrollingUV).rg;

  return colour + ( vec3(r,g,b) * amount );
}


float lumaFromRGB(vec3 rgb){
    vec3 weights = vec3(0.2126, 0.7152, 0.0722);
    float luminance = dot(rgb, weights);
    return luminance;
}

float sobelKernel[9] = float[](
1.0, 1.0,  1.0,
1.0,  -8.0, 1.0,
1.0, 1.0,  1.0
);

vec3 sobel(sampler2D tex){
    vec2 texelSize = 1.0 / textureSize(tex, 0);
    
    vec2 offsets[9] = vec2[](
    vec2(-texelSize.x, texelSize.y),
    vec2(0.0, texelSize.y),
    vec2(texelSize.x, texelSize.y),
    vec2(-texelSize.x, 0.0),
    vec2(0.0),
    vec2(texelSize.x, 0.0),
    vec2(-texelSize.x, -texelSize.y),
    vec2(0.0, -texelSize.y),
    vec2(texelSize.x, -texelSize.y)
    );
    
    vec3 colour = vec3(0.0);
    for (int i = 0; i < 9; i++)
    {
        vec3 t = texture(tex, texCoords + offsets[i]).rgb;
        
        colour += t * sobelKernel[i];
    }
    
    return colour;
}

vec3 spawnSharpenedImage(sampler2D image, vec2 texCoord2, float weight) {
    
    float nSharpness = weight * sharpness;
    if (nSharpness <= 0.0) return texture(image, texCoords).rgb;

    vec2 texelSize = 1.0 / textureSize(image, 0);
    
    float neighbor = nSharpness * -1.0;
    float center = nSharpness * 4.0 + 1.0;
    
    return texture(image, texCoords + vec2(0, texelSize.y)).rgb * neighbor +
    texture(image, texCoords + vec2(-texelSize.x, 0)).rgb * neighbor +
    texture(image, texCoords).rgb * center +
    texture(image, texCoords + vec2(texelSize.x, 0)).rgb * neighbor +
    texture(image, texCoords + vec2(0, -texelSize.y)).rgb * neighbor;
}

float linearizeDepth(float depth, float NearPlane, float FarPlane){
    return (2.0 * NearPlane * FarPlane) / (FarPlane + NearPlane - (depth * 2.0 - 1.0) * (FarPlane - NearPlane));
}

vec3 getCA(sampler2D Input, vec2 tc, float iOffset){
    return vec3(
            texture(Input, tc - iOffset).r,
            texture(Input, tc).g,
            texture(Input, tc + iOffset).b
    );
}

void main() {
    //FragColor.rgb =texture(screenTexture, texCoords).rgb; return;
    
    //float lumaSobel = lumaFromRGB(sobel(depthTexture));
    //vec3 edges = vec3(clamp(lumaSobel * 500.0, 0.0, 1.0));
    //if (lumaSobel > 0.00001f) lumaSobel = 1.0;
    //lumaSobel = clamp(lumaSobel, 0.0, 1.0);
   // lumaSobel = 1.0 - lumaSobel;
    
    //float luma = lumaFromRGB(texture(screenTexture, texCoords).rgb);
    
    vec3 colour = spawnSharpenedImage(screenTexture, texCoords, 1.0);
    //vec3 colour = texture(screenTexture, texCoords).rgb;
    vec3 nrm = texture(normal, texCoords).rgb;
    float depth = linearizeDepth(texture(depthTexture, texCoords).r, 0.1f, 100.0f);
   
    //colour = mix(colour, getCA(screenTexture, texCoords, 0.004f), 0.3f);
    //colour = vignette(colour, 0.9, 0.5);
    //colour = filmgrainColoured(colour, 0.03, random(texCoords * accum24value));
    
    int lastLOD = textureQueryLevels(screenTexture) - 1;
	vec3 avgColour = textureLod(screenTexture, vec2(0.5, 0.5), lastLOD).rgb;
    //vec3 avgColour = textureLod(screenTexture, texCoords, lastLOD).rgb;

    float autoExposure = calculateExposure(avgColour);
    //float autoExposure = 3.0;
    
    vec3 tonemappedColour = vec3(1.0f) - exp(-colour  * autoExposure);
    //vec3 tonemappedColour = vec3(1.0f) - exp(-colour  * autoExposure);

    //vec3 tonemappedColour = vec3(1.0f) - exp(-colour  * 5.0);

    vec3 aces = ACESFilm(colour * autoExposure * 1.8);
    //vec3 aces = ACESFilm(colour * 1.0 * 1.8);
    

    // hint of saturation
    //aces = vec3(saturationMatrix(1.5) * vec4(aces,1.0));

    //aces = filmgrain(aces, 0.1, accum24value);
    FragColor.rgb = pow(aces, vec3(gamma)); // aces
    //FragColor.rgb = pow(colour, vec3(gamma)); // off

    /*
    vec3 lutSize = vec3(textureSize(LUT, 0));
    vec3 uvw = (colour.rgb * float(lutSize - 1.0) + 0.5) / lutSize;
    FragColor.rgb = texture(LUT, uvw).rgb;
   */
    
    if (overlayDebug){
        vec4 dbg = texture(dbgColour, texCoords);
        //FragColor.rgb *= dbg.rgb;
        FragColor.rgb = mix(FragColor.rgb, dbg.rgb, 0.5);
    }
    

    //float luma = lumaFromRGB(texture(screenTexture, texCoords).rgb);

    //FragColor.rgb = vec3(luma);
    //FragColor.rgb = vec3(lumaSobel * luma);
}