#version 460 core
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_bindless_texture : require

in vec2 texCoord;
uniform uint64_t texture_diffuse_Handle;
uniform uint64_t bayerMatrixHandle;
uniform uint64_t BlueNoiseHandle;
uniform bool animateBinaryAlpha;
uniform int frame;
uniform float time;

uniform int drawIndex;
uniform int meshIndex;
uniform int totalDrawCount;

void blueNoiseOpacity(float Threshold){
	sampler2D bluemap =sampler2D(BlueNoiseHandle) ;
	vec2 texSize = vec2(textureSize(bluemap, 0));

	int nFrame = totalDrawCount;
	vec2 offset = vec2(fract(nFrame * 0.618), fract(nFrame * 0.133));

	vec2 noiseUV = (gl_FragCoord.xy / texSize) + offset;
	float noise = texture(bluemap, noiseUV).r;

	// normal ranges should be 0.0f-1.0f;
	if (noise > Threshold) discard;
}

void BayerNoiseOpacity(float Threshold){// for fade out or opacity (cheap) (could fade out near farplane or nearplane)
	sampler2D baySamp = sampler2D(bayerMatrixHandle);
	vec2 bayUV = vec2(gl_FragCoord.xy) / vec2(textureSize(baySamp, 0)); // new uvec2

	vec2 offset = vec2(0.0,0.0);
	//int nFrame = drawIndex + meshIndex;
	int nFrame = frame + totalDrawCount;
	if (animateBinaryAlpha) offset = vec2(fract(nFrame * 0.618), fract(nFrame * 0.133));
	float bayer = texture(baySamp, bayUV + offset).r;

	float clampedThreshold = clamp(Threshold, 0.2, 1.0);

	// normal ranges should be 0.0f-1.0f;
	if (bayer > clampedThreshold || Threshold <= 0) discard;
}

void animatedBlueNoiseOpacity(float Threshold){// for fade out or opacity (cheap) (could fade out near farplane or nearplane)
	sampler2D bluemap =sampler2D(BlueNoiseHandle) ;
	vec2 texSize = vec2(textureSize(bluemap, 0));
	
	//int nFrame = frame + drawIndex + meshIndex;
	int nFrame = totalDrawCount;
	if (animateBinaryAlpha)  nFrame += frame;
	vec2 offset = vec2(fract(nFrame * 0.618), fract(nFrame * 0.133));

	vec2 noiseUV = (gl_FragCoord.xy / texSize) + offset;


	float noise = texture(bluemap, noiseUV).r;


	// normal ranges should be 0.0f-1.0f;
	if (noise > Threshold) discard;
}

void scrollingBayerNoiseOpacity(float Threshold) // for fade out or opacity (cheap) (could fade out near farplane or nearplane)
{
	sampler2D baySamp = sampler2D(bayerMatrixHandle);
	vec2 bayUV = vec2(gl_FragCoord.xy) / vec2(textureSize(baySamp, 0)); // new uvec2

	float scrollSpeed = 0.5;

	bayUV = fract(bayUV + (scrollSpeed * time));

	float bayer = texture(baySamp, bayUV).r;


	float clampedThreshold = clamp(Threshold, 0.2, 1.0);

	// normal ranges should be 0.0f-1.0f;
	if (bayer > Threshold) discard;
}

void main(){
	sampler2D Sampler = sampler2D(texture_diffuse_Handle);
	float diffuseTex = texture(Sampler, texCoord).a;

	if (diffuseTex <= 0.0)
	discard;

	//scrollingBayerNoiseOpacity(diffuseTex);
	//blueNoiseOpacity(diffuseTex);
	animatedBlueNoiseOpacity(diffuseTex);
}