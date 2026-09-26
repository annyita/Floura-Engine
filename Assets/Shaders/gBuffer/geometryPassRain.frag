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
in vec3 colour;
in vec2 texCoord;

in vec3 Normal0;
in vec3 Tangent0;
in vec3 Bitangent0;

in vec4 currentPos;
in vec4 previousPos;

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
uniform sampler2D ripplesHandle;
uniform sampler2D dropletsHandle;
uniform sampler2D puddlesHandle;
uniform int drawIndex;
uniform int meshIndex;
uniform int totalDrawCount;

vec3 CalcNewNormal(vec3 normal){
	//	return normalize(Normal); 
	// texture
	//vec3 normalTex = texture(texture_normal0, texCoord).xyz;

	vec3 normalTex = normalize(normal * 2.0f - 1.0f);
	

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
void blueNoiseOpacity(float Threshold){ // for fade out or opacity (cheap) (could fade out near farplane or nearplane)
	sampler2D bluemap =sampler2D(BlueNoiseHandle) ;
	vec2 texSize = vec2(textureSize(bluemap, 0));

	//vec2 offset = vec2(0.0,0.0);
	//int nFrame = frame + drawIndex + meshIndex;
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

vec4 triplanarMap(sampler2D fTexture, sampler2D wTexture, vec3 position, vec3 normal, vec2 offset, vec2 zoom){
	vec2 uvX = position.zy / zoom;
	vec2 uvY = position.xz / zoom;
	vec2 uvZ = position.xy / zoom;

	vec4 tX =texture(wTexture, uvX + offset);
	vec4 tY =texture(fTexture, uvY + offset);
	vec4 tZ =texture(wTexture, uvZ + offset);

	vec3 blendedWeights = abs(normal);
	blendedWeights = pow(blendedWeights, vec3(4.0));
	blendedWeights /= (blendedWeights.x + blendedWeights.y + blendedWeights.z);
	return tX * blendedWeights.x + tY * blendedWeights.y + tZ * blendedWeights.z;
}

float triplanarPuddles(vec3 position, vec3 normal, vec2 offset, vec2 zoom){
	vec2 uvY = position.xz / zoom;

	float tX = -1.0f;
	float tY =texture(puddlesHandle, uvY + offset).r;
	float tZ = -1.0f;

	vec3 blendedWeights = abs(normal);
	blendedWeights = pow(blendedWeights, vec3(4.0));
	blendedWeights /= (blendedWeights.x + blendedWeights.y + blendedWeights.z);
	return tX * blendedWeights.x + tY * blendedWeights.y + tZ * blendedWeights.z;
}

void main(){
	ivec2 texelCoord = ivec2(gl_FragCoord.xy);
	
	sampler2D aSamp = sampler2D(texture_diffuse_Handle);
	sampler2D nSamp = sampler2D(texture_normal_Handle);
	sampler2D sSamp = sampler2D(texture_roughness_Handle);
	
	//vec2 tc2 = crntPos.xz / vec2(1.0);

	vec4 albedoTex = texture(aSamp, texCoord);
	
	highp vec2 currentNDC = currentPos.xy / (currentPos.w);
	highp vec2 previousNDC = previousPos.xy / (previousPos.w);
	highp vec2 velocity = (currentNDC + scaledCurrentJitter) - (previousNDC + scaledPreviousJitter);

	//if (length(velocity) < 0.001) { velocity = vec2(0.0); }
	
	gVelocity = vec4(velocity * 0.5, 1.0, 1.0); // velocity + alpha
	
    // Discard fragment if alpha is too low
    if (albedoTex.a <= 0.0) // Adjust threshold if needed
    discard;

	if (doBinaryAlpha) blueNoiseOpacity(albedoTex.a);
	//BayerNoiseOpacity(albedoTex.a);

	gPosition.rgb = crntPos; // Output position as-is
	vec3 normal = CalcNewNormal(texture(nSamp, texCoord).xyz);
	//gNormal.rgb = normal;
	gNormal.a = texture(nSamp, texCoord).a; // Fetch normal from texture
	gNormal.rgb = normal;

    // Assign Albedo RGB from texture
    //gAlbedoSpec.rgb = texture(diffuse0, texCoord).rgb * (texture(noiseMapTexture, texCoord) * 5).rgb;
	gAlbedo = albedoTex;

	//gSpecular.rgb = vec3(1.0f, 0.0f, 0.0f);
	gSpecular = texture(sSamp, texCoord);
	//gSpecular.rgb = vec3(1.0f);
	//gVelocity = vec4(vec3(1.0, 0.0, 0.0), 1.0);

	float puddle = max(triplanarPuddles(crntPos, normal, vec2(0.0), 
			vec2(30.0)), triplanarPuddles(crntPos, normal, vec2(2.0), vec2(10.0)));
	if (puddle < 0.9){
		vec3 rippleNormal = CalcNewNormal(triplanarMap(ripplesHandle, dropletsHandle, crntPos, normal, vec2(0.0), vec2(1.0, -1.0) * 1).rgb);
		
		if (puddle == -1.0){
			gNormal.rgb = rippleNormal;
			gSpecular.rgb = vec3(1.0, 0.0, 1.0);
		}
		else{
			gNormal.rgb = mix(normal, rippleNormal, puddle);
			gSpecular.rgb = mix(gSpecular.rgb, vec3(1.0, 0.0, 1.0), puddle);
		}
	}
	
	
	vec3 emission = texture(sampler2D(texture_emission_Handle), texCoord).rgb;
	gEmission = emission;
	//gEmission
}