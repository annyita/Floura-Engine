#version 460 core

#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_bindless_texture : require

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedo;
layout(location = 3) out vec4 gSpecular;
layout(location = 4) out vec4 gVelocity;
layout(location = 5) out vec3 gEmission;

in highp vec3 crntPos;
in vec3 Normal;
in vec3 color;
in vec2 texCoord;

in vec3 Normal0;
in vec3 Tangent0;
in vec3 Bitangent0;

in vec4 currentPos;
in vec4 previousPos;

in vec2 uvscale;
in vec2 texcoord2;

in mat4 ioCamMatrix;

//uint64_t
// these need to move to bindless
uniform uint64_t texture_diffuse_Handle;
uniform uint64_t texture_roughness_Handle;
uniform uint64_t texture_normal_Handle;
uniform uint64_t texture_emission_Handle;
//uniform sampler2D noiseMapTexture;

uniform uint64_t BlueNoiseHandle;
uniform uint64_t bayerMatrixHandle;
uniform int frame;

uniform vec2 currentJitter;
uniform vec2 previousJitter;
uniform vec2 scaledCurrentJitter;
uniform vec2 scaledPreviousJitter;

uniform bool doBinaryAlpha;
uniform bool animateBinaryAlpha;

uniform float time;
uniform float NearPlane;
uniform float FarPlane;

uniform vec3 camPos;
uniform sampler2D ripplesHandle;
uniform int drawIndex;
uniform int meshIndex;
uniform int totalDrawCount;


vec3 CalcNewNormal(vec2 texcoords) //pomCoords
{
	//	return normalize(Normal); 
	// texture
	//vec3 normalTex = texture(texture_normal0, texCoord).xyz;

	sampler2D nSamp = sampler2D(texture_normal_Handle);

	vec3 normalTex = normalize(texture(nSamp, texcoords).xyz * 2.0f - 1.0f);

	// transform from 0,1 to -1, 1
	//normalTex = 2.0 * normalTex - vec3(1.0);

	// normalize tangent space vector
	vec3 nNormal = normalize(Normal0);
	vec3 nTangent = normalize(Tangent0);
	vec3 nBitangent = normalize(Bitangent0);

	// make the tbn 
	mat3 nTBN = mat3(nTangent, nBitangent, nNormal);

	vec3 newNormal = normalize(nTBN * normalTex);

	return newNormal;
}

float random(vec3 seed) {

	vec4 seed4 = vec4(seed, 1.0);
	float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
	return fract(sin(dot_product) * 43758.5453);
}

// looks best on decals and foliage
void blueNoiseOpacity(float Threshold){// for fade out or opacity (cheap) (could fade out near farplane or nearplane)
	sampler2D bluemap =sampler2D(BlueNoiseHandle) ;
	vec2 texSize = vec2(textureSize(bluemap, 0));

	int nFrame = totalDrawCount;
	if (animateBinaryAlpha)  nFrame += frame;
	vec2 offset = vec2(fract(nFrame * 0.618), fract(nFrame * 0.133));

	vec2 noiseUV = (gl_FragCoord.xy / texSize) + offset;

	
	float noise = texture(bluemap, noiseUV).r;
	
	
	// normal ranges should be 0.0f-1.0f;
	if (noise > Threshold) discard;
}

void BayerNoiseOpacity(float Threshold){
	sampler2D baySamp = sampler2D(bayerMatrixHandle);
	vec2 texSize = vec2(textureSize(baySamp, 0));

	int nFrame = totalDrawCount;
	vec2 offset = vec2(fract(nFrame * 0.618), fract(nFrame * 0.133));
	vec2 bayUV = (gl_FragCoord.xy / texSize) + offset;
	float bayer = texture(baySamp, bayUV).r;


	float clampedThreshold = clamp(Threshold, 0.2, 1.0);

	// normal ranges should be 0.0f-1.0f;
	if (bayer > Threshold) discard;
}

bool boolBayerNoiseOpacity(float Threshold){
	sampler2D baySamp = sampler2D(bayerMatrixHandle);
	vec2 bayUV = vec2(gl_FragCoord.xy) / vec2(textureSize(baySamp, 0)); // new uvec2

	//float scrollSpeed = 0.5;

	//bayUV = fract(bayUV + (scrollSpeed * time));

	float bayer = texture(baySamp, bayUV).r;


	float clampedThreshold = clamp(Threshold, 0.2, 1.0);
    bool discardb = false;
	// normal ranges should be 0.0f-1.0f;
	if (bayer > Threshold) discardb = true;
	
	return discardb;
}

float linearizeDepth(float depth, float NP, float FP) { return (2.0 * NP * FP) / (FP + NP - (depth * 2.0 - 1.0) * (FP - NP)); }

float heightScale = 0.05;
bool invertPOM = true;
float minLayers = 8.0;
float maxLayers = 64.0;
float POMfadeDistance = 30.0f;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir, sampler2D heightsamp, float scale) {
	float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));

	float layerDepth = 1.0 / numLayers;
	float currentLayerDepth = 0.0;

	vec2 P = viewDir.xy * scale;
	vec2 deltaTexCoords = P / numLayers;

	vec2  currentTexCoords     = texCoords;
	float currentDepthMapValue = 0.0f;
	if (invertPOM) currentDepthMapValue = 1.0 - texture(heightsamp, currentTexCoords).a;
	else currentDepthMapValue = texture(heightsamp, currentTexCoords).a;
	
	int i = 0; int maxIterations = 4096;
	
	while(currentLayerDepth < currentDepthMapValue && i < maxIterations) {
		currentTexCoords -= deltaTexCoords;
		if (invertPOM) currentDepthMapValue = 1.0 - texture(heightsamp, currentTexCoords).a;
		else currentDepthMapValue = texture(heightsamp, currentTexCoords).a;
		currentLayerDepth += layerDepth;
		i++;
	}

	vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
	float afterDepth  = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = 0.0;
	if (invertPOM) beforeDepth = 1.0 - texture(heightsamp, prevTexCoords).a - currentLayerDepth + layerDepth;
	else beforeDepth = texture(heightsamp, prevTexCoords).a - currentLayerDepth + layerDepth;

	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

	return finalTexCoords;
}

float calcPOMdepth(vec3 gp){
	vec4 clipPos = ioCamMatrix * vec4(gp, 1.0);
	float ndcDepth = clipPos.z / clipPos.w;
	
	return ndcDepth * 0.5 + 0.5;
}

void main(){
	sampler2D nSamp = sampler2D(texture_normal_Handle);

	mat3 worldToTangent = transpose(mat3(normalize(Tangent0), normalize(Bitangent0), normalize(Normal0)));
	vec3 viewDirWorld = normalize(camPos - crntPos);
	vec3 viewDirTangent = worldToTangent * viewDirWorld;
	//vec3 viewDirTangent =  viewDirWorld;
	viewDirTangent.y = -viewDirTangent.y;
	

	vec2 tiledTexCoord = texCoord * uvscale;
	float adjustedHeightScale = heightScale / max(uvscale.x, uvscale.y);
	
	// depth
	float linearizedDepth = linearizeDepth(gl_FragCoord.z, NearPlane, FarPlane);
//POMfadeDistance

	
	vec2 pomCoords = tiledTexCoord;
	if (linearizedDepth < POMfadeDistance) pomCoords = ParallaxMapping(tiledTexCoord, viewDirTangent, nSamp, adjustedHeightScale); // the problem with shadows on this are the depth is flat
	
	//if(pomCoords.x > 1.0 || pomCoords.y > 1.0 || pomCoords.x < 0.0 || pomCoords.y < 0.0) discard;


	//if(pomCoords.x > texCoord.x * uvscale.x || pomCoords.y > texCoord.y * uvscale.y || pomCoords.x < 0.0 || pomCoords.y < 0.0) discard;
	//if(pomCoords.x > uvscale.x || pomCoords.y >  uvscale.y || pomCoords.x < 0.0 || pomCoords.y < 0.0) 	discard; 
    // temp out
	
	sampler2D aSamp = sampler2D(texture_diffuse_Handle);
	sampler2D sSamp = sampler2D(texture_roughness_Handle);

	vec4 albedoTex = texture(aSamp, pomCoords);
	
	highp vec2 currentNDC = currentPos.xy / (currentPos.w);
	highp vec2 previousNDC = previousPos.xy / (previousPos.w);
	highp vec2 velocity = (currentNDC + scaledCurrentJitter) - (previousNDC + scaledPreviousJitter);

	if (length(velocity) < 0.001) { velocity = vec2(0.0); }
	
	gVelocity = vec4(velocity * 0.5, 1.0, 1.0); // velocity + alpha
	
    // Discard fragment if alpha is too low
    if (albedoTex.a <= 0.0) // Adjust threshold if needed
    discard;

	//float linearizedDepth = linearizeDepth(gl_FragCoord.z, NearPlane, FarPlane);
	float fadeDistance = 10.0;
	float distToFar = FarPlane - linearizedDepth;
	float farOpacity = distToFar / fadeDistance;
	farOpacity = clamp(farOpacity, 0.0, 1.0);

	blueNoiseOpacity(farOpacity);

	if (doBinaryAlpha) blueNoiseOpacity(albedoTex.a);
	//BayerNoiseOpacity(albedoTex.a);

	//gPosition = crntPos; // Output position as-is
	float displacement = 0.0f;
	if (invertPOM) displacement = 1.0 - texture(nSamp, pomCoords).a; // Fetch normal from texture
	else displacement = texture(nSamp, pomCoords).a; // Fetch normal from texture
	
	vec3 wpos = crntPos - (normalize(Normal0) * (1.0 - displacement) * heightScale);
	gPosition.rgb = wpos;
	
	//albedoTex.r = displacement;
	
	gl_FragDepth = calcPOMdepth(wpos);
	
	gNormal.rgb = CalcNewNormal(pomCoords);
    
	gNormal.a = displacement;

    // Assign Albedo RGB from texture
    //gAlbedoSpec.rgb = texture(diffuse0, texCoord).rgb * (texture(noiseMapTexture, texCoord) * 5).rgb;
	gAlbedo = albedoTex;

	//gSpecular.rgb = vec3(1.0f, 0.0f, 0.0f);
	gSpecular = texture(sSamp, pomCoords);
	//gVelocity = vec4(vec3(1.0, 0.0, 0.0), 1.0);
	
	vec3 emission = texture(sampler2D(texture_emission_Handle), pomCoords).rgb;
	gEmission = emission;
	//gEmission
}