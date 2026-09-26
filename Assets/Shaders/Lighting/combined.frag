#version 460 core

#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_bindless_texture : require

out vec4 FragColor;
in vec3 crntPos;
in vec3 Normal;
in vec3 color;
in vec2 texCoord;

in vec4 fragPosLight;

in vec3 camPositon;

//TBN
in vec3 Normal0;
in vec3 Tangent0;
in vec3 Bitangent0;

uniform bool doReflect;
float specularLight = 0.50f; // 0.50f

// Gets the Texture Units from the main function
//uniform sampler2D texture_diffuse; // _Handle
//uniform sampler2D texture_roughness;
//uniform sampler2D texture_normal;

uniform uint64_t texture_diffuse_Handle;
uniform uint64_t texture_roughness_Handle;
uniform uint64_t texture_normal_Handle;
uniform uint64_t texture_emission_Handle;
//uniform uint64_t texture_displacement_Handle;

uniform sampler2D shadowMap;
//uniform sampler2D BlueNoiseTex;

uniform uint64_t bayerMatrixHandle;
uniform uint64_t BlueNoiseHandle;

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
uniform int frame;
uniform int indirectSamples;

struct Light{
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
//uniform vec3 camPositon;

// reflections
in vec3 reflectedVector;
in vec3 NviewVector;
//uniform samplerCube skybox; 
uniform uint64_t cmMainHandle;
uniform float smoothnessValue;

uniform bool doBinaryAlpha;
uniform bool animateBinaryAlpha;

uniform int drawIndex;
uniform int meshIndex;
uniform int totalDrawCount;

float linearizeDepth(float depth, float NP, float FP) { return (2.0 * NP * FP) / (FP + NP - (depth * 2.0 - 1.0) * (FP - NP)); }

vec3 CalcNewNormal(vec2 UV){
	//	return normalize(Normal); 
	// texture
	//vec3 normalTex = texture(texture_normal0, texCoord).xyz;

	sampler2D normsamp = sampler2D(texture_normal_Handle);
	vec3 normalTex = normalize(texture(normsamp, UV).xyz * 2.0f - 1.0f);

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

float CalcShadowFactorDIR(vec4 LightSpacePos, vec3 lightDirection, vec3 normal){
	// perform perspective divide
	vec3 lightCoords = LightSpacePos.xyz / LightSpacePos.w;
	float shadow = 0.0f;

	// shadow calculation
	if (lightCoords.z <= 1.0f)
	{
		sampler2D bluemap =sampler2D(BlueNoiseHandle) ;

		// transform to [0,1] range
		lightCoords = (lightCoords + 1.0f) / 2.0f;
		// get the current depth
		float currentDepth = lightCoords.z;
		// calculate shadow bias
		float bias = max(DirSMMaxBias * (1.0f - dot(normal, lightDirection)), 0.0005f);
		// PCF
		int sampleRadius = FilterRadius; // FilterRadius // NumberOfSamples
		//vec2 pixelSize = (float(NumberOfSamples) * 0.1) / textureSize(shadowMap, 0);
		vec2 texSize = vec2(textureSize(bluemap, 0));

		vec2 offset = vec2(fract(frame * 0.618), fract(frame * 0.133));
		vec2 noiseUV = (gl_FragCoord.xy / texSize) + offset;
		vec2 pixelSize = 1.0 / textureSize(shadowMap, 0);
		for(int y = -sampleRadius; y <= sampleRadius; y++)
		{
		    for(int x = -sampleRadius; x <= sampleRadius; x++)
		    {
					float angle = texture(bluemap, noiseUV).r * NumberOfSamples;
					vec2 foffset = vec2(cos(angle), sin(angle));

					float closestDepth = texture(shadowMap, lightCoords.xy + (vec2(x, y) * foffset) * pixelSize).r;
					if (currentDepth > closestDepth + bias)
						shadow += 1.0f;
     
		    }    
		}

		shadow /= pow((sampleRadius * 2 + 1), 2);


	}

return shadow;
}

vec4 direcLight(sampler2D specSamp){ // normals need to be recalculated based on rotation



	// shadow map 
	float shadow = 0.0f;


	vec3 normal = CalcNewNormal(texCoord);

	vec3 lightDirection = normalize(directLightPos); //vec3(1.0f, 1.0f, 0.0f)
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	if (doDirShadowMap)
	shadow = CalcShadowFactorDIR(fragPosLight, lightDirection, normal);
	// shadow map end

		float specular = 0.0f;
	if (doReflect && doDirSpecularLight && diffuse != 0.0f){

	vec3 reflectionDirection = reflect(-lightDirection, normal);
	vec3 viewDirection = normalize(camPositon - crntPos);
	vec3 halfwayVec = normalize(viewDirection + lightDirection);

	float specAmount = pow(max(dot(normal, halfwayVec), 0.0f), 16);
	//float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 16);
	specular = specAmount * dirSpecularLight;

	return ((diffuse * (1.0f - shadow) + directAmbient) + texture(specSamp, texCoord).g * specular * (1.0f - shadow)) * vec4(directLightCol, 1.0f); }
	else{ return ((diffuse * (1.0f - shadow) + directAmbient)) * vec4(directLightCol, 1.0f); }
}

vec4 pointLight(int iteration, sampler2D specSamp){	
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
	vec3 normal = CalcNewNormal(texCoord);
	//vec3 normal = normalize(Normal); 

	vec3 lightDirection = normalize(lightVec);
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	float specular = 0.0f;
	if (doReflect && diffuse != 0.0f){
	// specular lighting
		//float specularLight = 0.50f;
		vec3 viewDirection = normalize( (camPositon) - crntPos);
		vec3 reflectionDirection = reflect(-lightDirection, normal);

		vec3 halfwayVec = normalize(lightDirection + viewDirection);
		
		float specAmount = pow(max(dot(normal, halfwayVec), 0.1f), 16);
		specular = specAmount * specularLight;

		finalColour = finalColour + ((diffuse * inten + 0.0f) + texture(specSamp, texCoord).g * specular * inten) * vec4(Lights[iteration].colour, 1.0 ) * inten;
	}
	else{
		finalColour = finalColour + ( (diffuse * inten + 0.0f) * vec4(Lights[iteration].colour, 1.0) * inten);
		//finalColour = finalColour + (texture(diffuse0, texCoord) * (diffuse * inten + ambient) * vec4(Lights[iteration].colour, 1.0) * inten);
	}
	
	return finalColour;
}

vec4 spotLight(int iteration, sampler2D specSamp){
	// controls how big the area that is lit up is
	float outerCone = 0.90f;
	float innerCone = 0.95f;

	// ambient lighting
	//float ambient = 0.0f;

	vec4 finalColour = vec4(0.0f);

		// diffuse lighting
	vec3 normal = CalcNewNormal(texCoord);

	vec3 lightDirection = normalize(Lights[iteration].position - crntPos);
	float diffuse = max(dot(normal, lightDirection), 0.0f);

	// calculates the intensity of the crntPos based on its angle to the center of the light cone
	float angle = dot(Lights[iteration].rotation, -lightDirection); // direction
	float inten = clamp( (angle - outerCone) / (innerCone - outerCone), 0.0f, 1.0); 

	float specular = 0.0f;
	if (doReflect && diffuse != 0.0f){

		// specular lighting
	//float specularLight = 0.50f;
	vec3 viewDirection = normalize(camPositon - crntPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);

	vec3 halfwayVec = normalize(lightDirection + viewDirection);

	float specAmount = pow(max(dot(normal, halfwayVec), 0.0f), 16);
	specular = specAmount * specularLight;

	finalColour = finalColour + ((diffuse * inten + 0.0f) + texture(specSamp, texCoord).g * specular * inten) * vec4(Lights[iteration].colour, 1.0) * inten;

	}
	else{
	
	finalColour = finalColour + ((diffuse * inten + 0.0f) * vec4(Lights[iteration].colour, 1.0) * inten);
	//	finalColour = finalColour + (texture(diffuse0, texCoord) * (diffuse * inten + ambient) * vec4(Lights[iteration].colour, 1.0) * inten);
	}

	//finalColour = finalColour + (diffuse * inten + skyColor);

	return finalColour; 
}

vec4 lights(sampler2D specSamp){
	//vec4 diffuseTex = texture(texture_diffuse0, texCoord);
	vec4 finalColour = vec4(0.0);
    //return (diffuseTex * skyColor);
	int maxLights = 64;
	for (int i = 0; i < min(lightCount, maxLights); i++){
			if (Lights[i].type == 0){
			finalColour += spotLight(i, specSamp);
			}

			if (Lights[i].type == 1){
			finalColour += pointLight(i, specSamp);
		}
	}
	//return finalColour;
		//FragColor = direcLight(); doDirLight

	if (doDirLight){// if direct light is enabled, add it to the final color
		finalColour += direcLight(specSamp);
	}

	return finalColour; // was xyz
} 

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898, 78.233))) * 43758.5453);
}


vec3 sampleHemisphere2(vec3 normal, float u, float v){
	float phi = 2.0 * 3.14159265 * u;
	float cosTheta = sqrt(1.0 - v);
	float sinTheta = sqrt(v);

	vec3 localDir = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

	vec3 helper = abs(normal.y) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0);
	vec3 tangent = normalize(cross(helper, normal));
	vec3 bitangent = cross(normal, tangent);

	return tangent * localDir.x + bitangent * localDir.y + normal * localDir.z;
}

vec3 sampleHemisphere(vec3 normal, float random){
    float u = rand(vec2(gl_FragCoord.xy) + random); 
    float v = rand(vec2(u, random));

    float phi = 2.0 * 3.14159265 * u;
    float cosTheta = sqrt(1.0 - v);
    float sinTheta = sqrt(v);

    vec3 localDir = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 helper = abs(normal.y) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0);
    vec3 tangent = normalize(cross(helper, normal));
    vec3 bitangent = cross(normal, tangent);

    return tangent * localDir.x + bitangent * localDir.y + normal * localDir.z;
}

	// (thanks learnopengl)
float GeometrySchlickGGX(float NdotV, float roughness){
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

	return nom / max(denom, 0.0001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness){
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}  

vec3 metRough(vec3 albedo, out vec3 nFer, out float nMet, out vec3 irradiance, sampler2D specSamp){
	// textures
	vec3 metallicRoughness = texture(specSamp, texCoord).rgb; // metalic
	float rough = metallicRoughness.g;
	float met =  metallicRoughness.b;
	nMet = met;

	// vectors
	vec3 normal = normalize(CalcNewNormal(texCoord));
	//vec3 normal = normalize(Normal);
	vec3 v = normalize(NviewVector);
	vec3 r = reflect(v, normal);
	float nDotV = max(dot(normal, -v), 0.0001);

	samplerCube cmSamp = samplerCube(cmMainHandle);

	// LOD based Roughness (irradiance probes);
	float maxLod = float(textureQueryLevels(cmSamp) - 1);
	float lod = rough * maxLod; 
	lod = min(lod, 10.0); 

	vec3 reflectionColour = textureLod(cmSamp, r, lod).rgb;
	irradiance = textureLod(cmSamp, normal, maxLod).rgb;

	// fresnel
	float reflectivity = 0.04f;
	vec3 nF = mix(vec3(reflectivity), albedo, met);
	nFer = nF;
	vec3 F = nF + (max(vec3(1.0 - rough), nF) - nF) * pow(clamp(1.0 - nDotV, 0.0, 1.0), 5.0);

	// visibility factor 
	float G = GeometrySmith(normal, -v, r, rough);

	//return reflectionColour * F;
	return reflectionColour * F * G;
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

// looks best on decals and foliage
bool blueNoiseOpacity(float Threshold){ // for fade out or opacity (cheap) (could fade out near farplane or nearplane)
	sampler2D bluemap =sampler2D(BlueNoiseHandle) ;
	vec2 texSize = vec2(textureSize(bluemap, 0));
	
	//int nFrame = frame + drawIndex + meshIndex;
	int nFrame = totalDrawCount;
	if (animateBinaryAlpha) nFrame += frame;
	vec2 offset = vec2(fract(nFrame * 0.618), fract(nFrame * 0.133));
	
	vec2 noiseUV = (gl_FragCoord.xy / texSize) + offset;

	float noise = texture(bluemap, noiseUV).r;

	// normal ranges should be 0.0f-1.0f;
	if (noise > Threshold) return true;

	return false;
}

// maxDist = 50.0;

//void Reflect(vec3 albedo, out vec3 diffuse, out vec3 specular, sampler2D specSamp, float depth)
void Reflect(vec3 albedo, out vec3 diffuse, out vec3 specular, sampler2D specSamp){
	/*
	float fadeDistance = 10.0;
	float distToFar = maxDist - depth;
	float farOpacity = distToFar / fadeDistance;
	farOpacity = clamp(farOpacity, 0.0, 1.0);

	if (blueNoiseOpacity(farOpacity)) return;
	*/
	//	if (doReflect && depth < maxDist)
	if (doReflect){
		float met = 0;
		vec3 nF = vec3(0.0f);
		vec3 irradiance = vec3(0.0f);
		specular = metRough(albedo, nF, met, irradiance, specSamp);

		vec3 nDiffuse = vec3(0.0f);
		nDiffuse = albedo * (1.0f - nF);
		nDiffuse *= (1.0f - met);

		diffuse = irradiance * nDiffuse;
	}
}


vec3 rough(sampler2D specSamp){
	if (doReflect){
	// textures
	vec3 metallicRoughness = texture(specSamp, texCoord).rgb; // metalic
	float rough = metallicRoughness.g;

	// vectors
	vec3 normal = normalize(CalcNewNormal(texCoord));
	//vec3 normal = normalize(Normal);
	vec3 v = normalize(NviewVector);
	vec3 r = reflect(v, normal);
	float nDotV = max(dot(normal, -v), 0.0001);

	samplerCube cmSamp = samplerCube(cmMainHandle);

	// LOD based Roughness (irradiance probes);
	float maxLod = float(textureQueryLevels(cmSamp) - 1);
	float lod = rough * maxLod; 
	lod = min(lod, 10.0); 

	vec3 reflectionColour = textureLod(cmSamp, r, lod).rgb;
	return reflectionColour;
	}
	return vec3(0.0f);
}


vec3 indirectIBL(int samples, sampler2D specSamp){

	vec3 metallicRoughness = texture(specSamp, texCoord).rgb; // metalic

	float rough = metallicRoughness.g;
	float met =  metallicRoughness.b;

	vec3 normal = CalcNewNormal(texCoord);

	vec3 NreflectedVector = reflect(NviewVector, normal);

	samplerCube cmSamp = samplerCube(cmMainHandle);

	int lastLOD = textureQueryLevels(cmSamp) - 1;

	float maxLod = lastLOD;

	float lod =  rough * maxLod; 

	lod = min(lod, 10.0); 

	//float specularIntensity = mix(0.04, 1.0, met);

	vec3 indirectColour = vec3(0.0f);

	if (samples <= 0) return indirectColour;

	sampler2D bluemap =sampler2D(BlueNoiseHandle) ;
	vec2 noiseUV = vec2(gl_FragCoord.xy) / vec2(textureSize(bluemap, 0));
	vec2 scrollingUV = noiseUV + fract(time * vec2(12.9898, 78.233));
	vec2 blueNoise = texture(bluemap, scrollingUV).rg;
	
	for (int i = 0; i < samples; i++){
	//gl_FragCoord
		//vec3 randomDir = sampleHemisphere(normalize(NreflectedVector), i + (gl_FragCoord.z * time));


		float u = fract(blueNoise.r + float(i) * 0.61803398875);
		float v = fract(blueNoise.g + float(i) * 0.61803398875);

		//float u = texture(bluemap, scrollingUV).r;
		//float v = texture(bluemap, scrollingUV + vec2(0.5)).g;

		//float u = texture(bluemap, scrollingUV).r;
		//float v = texture(bluemap, scrollingUV + vec2(0.5)).r;
		//float v = fract(u + 0.61803398875);
		
		//vec3 randomDir = sampleHemisphere2(normalize(NreflectedVector), u , v);
		
		//vec3 randomDir = sampleHemisphere(normalize(NreflectedVector), i + time); // i thought it would be better to add time for a film grain look, it would also solve with taa
		vec3 randomDir = sampleHemisphere(normalize(NreflectedVector), u + (time * gl_FragCoord.z ) );
		//vec3 randomDir = sampleHemisphere2(normalize(NreflectedVector), u + (gl_FragCoord.z ), v + (gl_FragCoord.z ));
		
			//int skyLOD = textureQueryLevels(skybox) - 4; // use mipmap for more preformance
			vec3 skyboxColour = textureLod(cmSamp, randomDir, lod).rgb; 
			//vec3 skyboxColour = texture(skybox, randomDir).rgb; 

		indirectColour += skyboxColour;
	}

	return (indirectColour / samples);
	//return (indirectColour / samples) * met;
}

float logisticDepth(float depth, float steepness, float offset, float NearPlane, float FarPlane){
    float zVal = linearizeDepth(depth, NearPlane, FarPlane);
    float expVal = exp(clamp(-steepness * (zVal - offset), -10.0, 10.0));
    return 1.0 / (1.0 + expVal);
}

vec4 calculateFog(float near, float far, float depthDistance, vec3 Colour, vec4 originalImage){
	float linearizedFogDepth = linearizeDepth(gl_FragCoord.z, near, far);
	float logisticizedDepth = logisticDepth(gl_FragCoord.z, 0.1f, depthDistance, near, far);
	return originalImage * (1.0f - logisticizedDepth) + vec4(logisticizedDepth * vec4(Colour, 1.0f)); // fog
}

void main(){
	float linearizedDepth = linearizeDepth(gl_FragCoord.z, NearPlane, FarPlane);
	//early z cutoff
	if (linearizedDepth > FarPlane)
	discard;

	float fadeDistance = 10.0;
	float distToFar = FarPlane - linearizedDepth;
	float farOpacity = distToFar / fadeDistance;
	farOpacity = clamp(farOpacity, 0.0, 1.0);

	if (blueNoiseOpacity(farOpacity)) discard;
		
	sampler2D difusesamp = sampler2D(texture_diffuse_Handle);
	vec4 albedo = texture(difusesamp, texCoord);
	if (albedo.a <= 0.0)
	discard;

	//if (blueNoiseOpacity(albedo.a) && doBinaryAlpha) discard;
	if (blueNoiseOpacity(albedo.a) && doBinaryAlpha)discard;
	//if (BayerNoiseOpacity(albedo.a)) discard;
	vec3 specular = vec3(0.0f);
	vec3 diffuse  = vec3(0.0f);

	sampler2D specSamp = sampler2D(texture_roughness_Handle);
	vec3 ARM = texture(specSamp, texCoord).rgb;
	
	Reflect(albedo.rgb, diffuse, specular, specSamp);
	
	//texture_emission_Handle
	sampler2D emissionSamp = sampler2D(texture_emission_Handle);
	vec3 emission = texture(emissionSamp, texCoord).rgb;

	//vec3 direct =  ARM.r * lights(specSamp).rgb;
	vec3 direct = lights(specSamp).rgb;
	//direct *= vec3(ARM.r);
	vec3 indirect = indirectIBL(indirectSamples, specSamp);

	vec3 gi = (direct + indirect);

	float met = texture(specSamp, texCoord).b;

	vec3 reflections = diffuse + specular;

	//vec4 final = vec4(finalRGB, 1.0f);

	// albedo * gi + spec + em
	vec4 final = albedo * vec4(gi, 1.0f) + vec4(reflections, 1.0f) + vec4(emission,1.0f);

	//vec4 final = vec4(totalDiffuse, 1.0) * vec4(gi, 1.0f) + vec4(specular, 1.0f);

	if (doFog) final = calculateFog(FogNearPlane, FogFarPlane, DepthDistance, fogColour, final); // fog

	//final = vec4(reflections,1.0f);

	FragColor = final;
	
	//FragColor = vec4(emission,1.0f);
	//FragColor = vec4(rough(specSamp), 1.0f);
	//FragColor = vec4(finalRGB , 1.0f);
	//FragColor = diffuseTex * vec4(1.0f) + vec4(specular , 1.0f);

	//sampler2D dispSamp = sampler2D(texture_displacement_Handle);
	//FragColor = vec4(vec3(texture(dispSamp, texCoord).rgb ),1.0f );


	//sampler2D norSamp = sampler2D(texture_normal_Handle);
	//FragColor = vec4(vec3(texture(norSamp, texCoord).a ),1.0f );
	//FragColor = vec4(vec3(texture(norSamp, texCoord).rgb ),1.0f );

	//FragColor = vec4(vec3(texture(specSamp, texCoord).a ),1.0f );
	//FragColor = vec4( vec3(texture(texture_roughness0, texCoord).r ),1.0f );
	//FragColor = vec4( vec3(texture(texture_roughness0, texCoord).g ),1.0f ); // roughness
	//FragColor = vec4( vec3(texture(texture_roughness0, texCoord).b ),1.0f ); // metal
	//FragColor = vec4(gi, 1.0f);
	//FragColor = vec4(indirect, 1.0f);
	//FragColor = vec4(direct, 1.0f);
	//FragColor = vec4(gi, 1.0f) + vec4(specular, 1.0f);
}