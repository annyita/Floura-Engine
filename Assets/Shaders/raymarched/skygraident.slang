#include "../slang/hashFunctions.slangh"
#include "../slang/basicRayFunctions.slangh"

struct Vertex
{
    float3 aPos;
    float2 aTex;
};

struct VOut
{
    float4 gl_position : SV_Position;
    float2 texCoord : TEXCOORD;
};

struct VertexToFragment
{
    float4 sv_position : SV_Position;
    float2 texCoord    : TEXCOORD;
};

[shader("vertex")]
VOut vertexMain(Vertex input){
    VOut output;
    
    output.texCoord = input.aTex;
    output.gl_position = float4(input.aPos, 1.0);
    
    return output;
}

// frag



// unforms wont work, need a dam ubo
//uniform float2 screenSize;
//uniform float4x4 	inverseViewMatrix;
//uniform float4x4  inverseProjectionMatrix;
//uniform float time;

// ubo or ssbo needed sadly

struct CameraUniforms{
    float2 screenSize;
    float4x4 u_ViewMatrix;
    float4x4 u_ProjectionMatrix;
    float4x4 inverseViewMatrix;
    float4x4 inverseProjectionMatrix;
}

ConstantBuffer<CameraUniforms> uCamera;

[shader("fragment")] 
float4 fragmentMain(VOut input) : SV_Target
{
    int2 size = int2(uCamera.screenSize);
    float2 uv = (input.texCoord - 0.5) * 2.0;
    
    // inverse on cpu and send it
    float3 rayDir = rayDirfromCam(uCamera.inverseProjectionMatrix, uCamera.inverseViewMatrix, uv);
    
    float coverage = 0.005f;
    // day
    //float3 zenithColour = float3(0.12, 0.45, 0.92);
    //float3 horizonColour = float3(0.60, 0.78, 0.98);
    //float3 groundColour  = float3(0.01, 0.01, 0.02);
    
    // sunset/rise
   float3 zenithColour  = float3(0.18, 0.52, 0.88);
   float3 horizonColour = float3(0.98, 0.82, 0.60);
   float3 groundColour  = float3(0.95, 0.45, 0.38);
    // night
    //float3 zenithColour = float3(0.01, 0.02, 0.05) * 0.8;
    //float3 horizonColour = float3(0.05, 0.08, 0.15);
    //float3 groundColour = float3(0.02, 0.03, 0.05);
        
    zenithColour += step(1.0 - coverage, random(rayDir.xy));
    float blend = clamp(rayDir.y, 0.0, 1.0);

    float3 colour = lerp(horizonColour, zenithColour, blend);
    if (rayDir.y < 0.0) colour = lerp(horizonColour, groundColour, clamp(-rayDir.y * 2.0, 0.0, 1.0));

    return float4(clamp(colour / 2.2, 0.0, 1.0), 1.0);
}