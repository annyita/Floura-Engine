#version 460 core

#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_bindless_texture : require

// Outputs colors in RGBA
out vec4 FragColor;

// Imports the current position from the Vertex Shader
in vec3 crntPos;
// Imports the normal from the Vertex Shader
in vec3 Normal;
// Imports the color from the Vertex Shader
in vec3 color;
// Imports the texture coordinates from the Vertex Shader
in vec2 texCoord;

in vec4 fragPosLight;

uniform bool doReflect;
uniform uint64_t texture_diffuse_Handle;
uniform uint64_t texture_emission_Handle;

uniform sampler2D shadowMap;
uniform uint64_t BlueNoiseHandle;
uniform uint64_t bayerMatrixHandle;

// Gets the position of the light from the main function
//const vec3 lightPos = vec3(0.0, 5.0, 1.0);
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
uniform float deltatime;
uniform float time;

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

uniform float FogNearPlane;
uniform float FogFarPlane;
uniform float DepthDistance;
uniform vec3 fogColour;
uniform bool doFog;

uniform float NearPlane;
uniform float FarPlane;
uniform vec3 camPositon;

uniform bool doBinaryAlpha;
uniform int drawIndex;
uniform int meshIndex;
uniform int totalDrawCount;

float linearizeDepth(float depth, float NP, float FP){
	return (2.0 * NP * FP) / (FP + NP - (depth * 2.0 - 1.0) * (FP - NP));
}

float random(vec3 seed) {

	vec4 seed4 = vec4(seed, 1.0);
	float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
	return fract(sin(dot_product) * 43758.5453);
}

float CalcShadowFactorDIR(vec4 LightSpacePos, vec3 lightDirection, vec3 normal){
	// perform perspective divide
	vec3 lightCoords = LightSpacePos.xyz / LightSpacePos.w;
	float shadow = 0.0f;

	// shadow calculation
	if (lightCoords.z <= 1.0f){
		// transform to [0,1] range
		lightCoords = (lightCoords + 1.0f) / 2.0f;
		// get the current depth
		float currentDepth = lightCoords.z;
		// calculate shadow bias
		float bias = max(DirSMMaxBias * (1.0f - dot(normal, lightDirection)), 0.0005f);
		// PCF

		sampler2D bluemap =sampler2D(BlueNoiseHandle) ;
		
		int sampleRadius = FilterRadius; // FilterRadius // NumberOfSamples
		//vec2 pixelSize = (float(NumberOfSamples) * 0.1) / textureSize(shadowMap, 0);
		vec2 noiseUV = vec2(gl_FragCoord.xy) / vec2(textureSize(bluemap, 0));
		vec2 pixelSize = 1.0 / textureSize(shadowMap, 0);
		for(int y = -sampleRadius; y <= sampleRadius; y++){
		    for(int x = -sampleRadius; x <= sampleRadius; x++){
					float angle = texture(bluemap, noiseUV).r * NumberOfSamples;
					vec2 offset = vec2(cos(angle), sin(angle));

					float closestDepth = texture(shadowMap, lightCoords.xy + (vec2(x, y) * offset) * pixelSize).r;
					if (currentDepth > closestDepth + bias)
						shadow += 1.0f;
     
		    }    
		}

		shadow /= pow((sampleRadius * 2 + 1), 2);


	}

return shadow;
}

vec4 direcLight(){ // normals need to be recalculated based on rotation
	vec3 normal = normalize(Normal); 

	vec3 lightDirection = normalize(directLightPos); //vec3(1.0f, 1.0f, 0.0f)
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// shadow map 
	float shadow = 0.0f;

	if (doDirShadowMap)
	shadow = CalcShadowFactorDIR(fragPosLight, lightDirection, normal);
	// shadow map end
	

	return ((diffuse * (1.0f - shadow) + directAmbient)) * vec4(directLightCol, 1.0f);
}

vec4 pointLight(int iteration){	
	vec4 finalColour = vec4(0.0f);

	
	//vec3 lightVec = (Lights[iteration].position) - crntPos;
	vec3 lightVec = (Lights[iteration].position) - crntPos;

	// intensity of light with respect to distance
	float dist = length(lightVec);
	float a = 3.00f;
	float b = 0.70f;
	float inten = 1.0f / (a * dist * dist + b * dist + 1.0) * Lights[iteration].radius;

	// ambient lighting
	//float ambient = 0.0f;
	vec3 normal = normalize(Normal); 
	//vec3 normal = normalize(Normal); 

	vec3 lightDirection = normalize(lightVec);
	float diffuse = max(dot(normal, lightDirection), 0.0f);


	finalColour = finalColour + ( (diffuse * inten + 0.0f) * vec4(Lights[iteration].colour, 1.0) * inten);


	return finalColour;
}

vec4 spotLight(int iteration){
	// controls how big the area that is lit up is
	float outerCone = 0.90f;
	float innerCone = 0.95f;

	// ambient lighting
	//float ambient = 0.0f;

	vec4 finalColour = vec4(0.0f);

		// diffuse lighting
	vec3 normal = normalize(Normal); 

	vec3 lightDirection = normalize(Lights[iteration].position - crntPos);
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// calculates the intensity of the crntPos based on its angle to the center of the light cone
	float angle = dot(Lights[iteration].rotation, -lightDirection); // direction
	float inten = clamp( (angle - outerCone) / (innerCone - outerCone), 0.0f, 1.0); 

	finalColour = finalColour + ((diffuse * inten + 0.0f) * vec4(Lights[iteration].colour, 1.0) * inten);

	return finalColour; 
}

vec4 lights(int lodcount, float linearizedDepth){
	//vec4 diffuseTex = texture(texture_diffuse0, texCoord);
	vec4 finalColour = vec4(0.0);
    //return (diffuseTex * skyColor);
	int maxLights = 64;
	
	if (lodcount != 2){
		if (lodcount == 1 && linearizedDepth < 50.0f){
			for (int i = 0; i < min(lightCount, maxLights); i++)
				{
					if (Lights[i].type == 0){
					finalColour += spotLight(i);
					}
				
					if (Lights[i].type == 1){
					finalColour += pointLight(i);
				}

			}
		}
	}
	//return finalColour;
		//FragColor = direcLight(); doDirLight

	if (doDirLight){// if direct light is enabled, add it to the final color
		finalColour += direcLight();
	}

		///return vec4(finalColour.xyz, diffuseTex.a);
		return vec4(finalColour); // was xyz
		//return vec4((diffuseTex.xyz * skyColor.xyz) + finalColour.xyz, diffuseTex.a);
		//return (diffuseTex.xyz * skyColor.xyz) + finalColour.xyz;
}


// looks best on glass and solids
void BayerNoiseOpacity(float Threshold){// for fade out or opacity (cheap) (could fade out near farplane or nearplane)
	sampler2D baySamp = sampler2D(bayerMatrixHandle);
	
	vec2 bayUV = vec2(gl_FragCoord.xy) / vec2(textureSize(baySamp, 0)); // new uvec2

	float scrollSpeed = 0.5;

	//int nFrame = drawIndex + meshIndex;
	int nFrame = totalDrawCount;
	vec2 offset = vec2(fract(nFrame * 0.618), fract(nFrame * 0.133));
	//bayUV = fract(bayUV + (scrollSpeed * time));
	float bayer = texture(baySamp, bayUV + offset).r;


	float clampedThreshold = clamp(Threshold, 0.2, 1.0);

	// normal ranges should be 0.0f-1.0f;
	if (bayer > Threshold || Threshold <= 0) discard;
}
int lodcount = 0;

void main(){
	float linearizedDepth = linearizeDepth(gl_FragCoord.z, NearPlane, FarPlane);
	//early z cutoff
	if (linearizedDepth > FarPlane)
	discard;
	if (lodcount == 3) {FragColor = vec4(0.0f); return;};
	
	sampler2D difusesamp = sampler2D(texture_diffuse_Handle);

	//int lastLOD = textureQueryLevels(difusesamp) - 1;

	//float maxLod = lastLOD;

	//lod = min(lod, maxLod); 

	vec4 albedo = texture(difusesamp, texCoord);
	if (albedo.a <= 0.0)
	discard;

	if (doBinaryAlpha) BayerNoiseOpacity(albedo.a);

	vec3 direct = lights(lodcount, linearizedDepth).rgb;

	//texture_emission_Handle
	sampler2D emissionSamp = sampler2D(texture_emission_Handle);
	vec3 emission = texture(emissionSamp, texCoord).rgb;
	
	// albedo * gi + spec + em
	vec4 final = albedo * vec4(direct, 1.0f) + vec4(emission, 1.0f);

	FragColor = final;
}