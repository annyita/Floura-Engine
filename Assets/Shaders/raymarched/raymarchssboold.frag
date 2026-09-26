#version 460 core

#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_bindless_texture : require

    struct boxRootNode{
        vec4 rootPos; // 16
        vec4 rootscale; // 16
    
        uint64_t instanceUUID;
        uint64_t padding;
    };

    struct voxel{
        boxRootNode aabb;
        vec4 albedo; // 16
        vec4 arm; // 16
        vec4 emission; // 16
        //vec4 padding; // 16
        vec4 localPosition; // 16
        vec4 localScale; // 16
        //mat4 localMatrix; // 64
    };

    struct voxelArray{
    // start and count
        int start;
        int count;
        int pad1;
        int pad2;
        
        uint64_t vxUUID;
        uint64_t padding;
    };

    struct voxelMeshQuickData{
        mat4 globalMatrix;
        uint64_t vxUUID;
        uint64_t padding;
        vec4 padding2;
    };

layout(std430, binding = 11) buffer voxelBuffer {
    voxel voxels[];
};
layout(std430, binding = 12) buffer voxelMeshBuffer {
    voxelArray voxelMeshes[];
};
layout(std430, binding = 13) buffer voxelQuickBuffer {
    voxelMeshQuickData quickData[];
};



out vec4 FragColor;

in highp vec2 texCoord;

uniform vec2 screenSize;
uniform sampler2D depthMap;

uniform mat4 u_ViewMatrix;
uniform mat4 u_ProjectionMatrix;
uniform vec3 cameraPosition;

uniform float time;

uniform uint64_t SusanneSDF64x64r8_handle;
uniform uint64_t Susanne64_handle;

uniform vec3 directLightPos;
uniform vec3 directLightCol;
uniform bool doDirLight;
uniform float directAmbient;

// temp


float maxdist = 1000.0f;
float mindist = 0.001f;
int steps = 128;

float r_maxdist = 30.0f;
int r_steps = 50;
int r_bounces = 3;
bool doReflections = false;

float i_maxdist = 128.0f;
int i_steps = 64;
int i_samples = 2;
bool doIndirect = false;

float maxshadowDistance = 0.0f;
//float ambientLight = 0.0;

float repeatDistance =15.0f;
bool doRepeat = false;

//vec3 skycolour = vec3(1.0, 1.0, 1.0);
vec3 skycolour = vec3(0.7, 0.7, 1.0);
//vec3 skycolour = vec3(0.0, 0.0, 0.0);

int SSAAsamples = 1;

float originEplison = 0.5f;

struct material{
    float ID; // dont even need rn, can just use index
    vec3 colour;
    float reflectionFactor;
    bool isVolumetric;
};

struct hitresult
{
    vec3 normal;
    vec3 hitpos;
    vec2 uv;
    float totalDistanceTravelled;
    float distance;
    float materialID;
    int materialIndex;
    bool isHit;
};

struct gbuffer
{
    vec3 colour;
    vec3 albedo;
    vec3 normal;
    float depth;
    bool hit;
    hitresult hr;
};

material materials[7];

void setupMaterials()
{
    materials[0].colour = vec3(1.0, 0.0, 0.0); // red
    materials[0].reflectionFactor = 0.2;
    materials[0].isVolumetric = false;
    materials[1].colour = vec3(0.0, 1.0, 0.0); // green
    materials[1].reflectionFactor = 0.2;
    materials[1].isVolumetric = false;
    materials[2].colour = vec3(1.0, 1.0, 1.0); // white
    materials[2].reflectionFactor = 0.0;
    materials[2].isVolumetric = true;
    
    materials[3].isVolumetric = false;
    
    materials[4].colour = vec3(1.0); // silver ref
    materials[4].reflectionFactor = 0.9;
    materials[4].isVolumetric = false;
    
    materials[5].isVolumetric = false;
    materials[5].reflectionFactor = 0.0;

    materials[6].isVolumetric = false;
    materials[6].reflectionFactor = 0.0;
}

vec3 getRainbowPalette(float t) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.0, 0.33, 0.67);

    return a + b * cos(6.28318 * (c * t + d));
}

void setupMaterialsAdvanced(vec3 p)
{
    materials[3].colour = vec3(0.2 + 0.4 * mod(floor(p.x) + floor(p.z), 2.0) ); // tiles
    materials[3].reflectionFactor = 0.0 + 0.4 * mod(floor(p.x) + floor(p.z), 2.0);
    

    materials[5].colour = vec3(vec3(0.96, 0.85, 0.53) + (vec3(0.96, 0.85, 0.53) * 0.5) * mod(floor(p.x) + floor(p.z), 2.0) ); // tiles


    materials[6].colour = vec3(1.0f);
}

vec3 rayDirfromCam(mat4 projection, mat4 view, vec2 uv){
    vec4 target = projection * vec4(uv.x, uv.y, -1.0, 1.0);
    vec3 rayDirView = normalize(target.xyz / target.w);
    vec3 rayDirWorld = normalize(mat3(view) * rayDirView);

    return normalize(rayDirWorld);
}


float signedDistanceSphere(vec3 p, float s){
    return length(p) - s;
}

float sdBox(vec3 p, vec3 b){
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0f);
}

float sdTorus( vec3 p, vec2 t ){
    vec2 q = vec2(length(p.xz)-t.x,p.y);
    return length(q)-t.y;
}

float smin(float a, float b, float k){
    float h = max( k-abs(a-b), 0.0) / k;
    return min(a,b) - h * h * h * k * (1.0/6.0);
}

vec3 rot3D(vec3 p, vec3 axis, float angle){
    return mix(dot(axis, p) * axis, p, cos(angle))
            + cross(axis, p * sin(angle));
}

mat2 rot2D(float angle){
    float s = sin(angle);
    float c = cos(angle);
    return mat2(c, -s, s, c);
}

float fOpUnionIDF(float res1, float res2){
    return (res1 < res2) ? res1 : res2;
}

vec2 fOpUnionID(vec2 res1, vec2 res2){
    return (res1.x < res2.x) ? res1 : res2;
}

vec3 fOpUnionID3(vec3 res1, vec3 res2){
    return (res1.x < res2.x) ? res1 : res2;
}

vec4 fOpUnionID4(vec4 res1, vec4 res2){
    return (res1.x < res2.x) ? res1 : res2;
}



vec2 fOpUnionRoundID(vec2 a, vec2 b, float r) {
    vec2 u = max(vec2(r - a.x, r - b.x), vec2(0.0));
    float blendedDistance = max(r, min(a.x, b.x)) - length(u);
    float finalID = (a.x < b.x) ? a.y : b.y;
    
    return vec2(blendedDistance, finalID);
}

vec4 fOpUnionRoundID4(vec4 a, vec4 b, float r) {
    vec4 u = max(vec4(r - a.x, r - b.x, 0.0, 0.0), vec4(0.0));
    float blendedDistance = max(r, min(a.x, b.x)) - length(u);
    vec3 final4 = (a.x < b.x) ? a.yzw : b.yzw;

    return vec4(blendedDistance, final4);
}

vec3 transformP(vec3 p, mat4 matrix){
    vec4 np = matrix * vec4(p, 1.0);
    return np.xyz;
}

#define INF 3.40282347e+38f

float texture3DSDF(vec3 p, vec3 s, uint64_t handle, int lodLevel){

    vec3 distance = abs(p) - s;
    
   float outsideDistance = length(max(distance, 0.0));
    
    // map coords to 3d uv space
    vec3 uvw = (p / (s* 2.0)) + 0.5;
    
    float SDF = textureLod(sampler3D(handle), uvw, lodLevel).r;
    
    if (outsideDistance > 0.0) return SDF + outsideDistance;
    
    return SDF;
}

vec3 texture3DSDFUV(vec3 p, vec3 s, uint64_t handle, int lodLevel){

    vec3 distance = abs(p) - s;

    float outsideDistance = length(max(distance, 0.0));

    // map coords to 3d uv space
    vec3 uvw = (p / (s* 2.0)) + 0.5;

    vec3 SDF = textureLod(sampler3D(handle), uvw, lodLevel).rgb;

    if (outsideDistance > 0.0) return vec3(SDF.r + outsideDistance, SDF.yz);

    return SDF;
}

vec4 sceneSDF(vec3 p ) // scene
{

    float materialID0 = 0.0;
    float materialID1 = 1.0;
    float materialID2 = 2.0;
    float materialID3 = 3.0;
    float materialID4 = 4.0;
    float materialID5 = 5.0;
    float materialID6 = 6.0;
    
    vec2 priorSDF = vec2(INF, -1.0);
    
    for (int i = 0; i < voxelMeshes.length(); i++ ){ //  meshes
        
        int count = voxelMeshes[i].count;
        int start = voxelMeshes[i].start;
        int end = start + count;
        //count+
        
        mat4 gm = mat4(1.0f);

        // search for global matrix in quick data
        for (int x = 0; x < quickData.length(); x++ ){
            if (quickData[x].vxUUID == voxelMeshes[i].vxUUID)
            gm = quickData[x].globalMatrix;
        }
        
        vec3 gp = transformP(p, gm);;
        
        // voxel walk
        for (int x = start; x < min(end, 9999); x++ ){ /// 9999 set as max for now so driver dont crash
            //mat4 lp = voxels[x].localMatrix;
            //mat4 ngm = lp * gm;
            vec3 lp = voxels[x].localPosition.xyz;
            vec3 ls = voxels[x].localScale.xyz;

            vec3 fp = gp - lp;
            float box = sdBox(fp, ls);
            //vec3 fp = (gp - lp) / ls;
            //float box = sdBox(fp, vec3(1.0));
            
            //float boxFixed = box * min(ls.x, min(ls.y, ls.z));
            //fOpUnionRoundID
            priorSDF = fOpUnionRoundID(priorSDF, vec2(box, materialID6), 2.0);
            //priorSDF = fOpUnionID(priorSDF, vec2(box, materialID6));
        }
    
    }
    return vec4(priorSDF, 0.0f, 0.0f);
    
    
    vec3 p2 = p;
    //vec3 p3 = transformP(p, mat4(1.0f));
    //if (doRepeat) p = mod(vec3(p.x, 0.0, p.z), repeatDistance) -repeatDistance * 0.5;
    if (doRepeat) p.xz = mod(p.xz, repeatDistance) -repeatDistance * 0.5;
    //rp.zy *= rot2D(time);

    //vec3 sdfv = texture3DSDFUV(p, vec3(5.0), Susanne64_handle, 0);
    //vec4 res2 = vec4(sdfv.r, materialID0, sdfv.gb);

    //for (int i = 0; i < 10; i++ ){
      //  vec3 np2 = vec3(-i * 20, 3, 15);
         //   vec3 sdfv2 = texture3DSDFUV(p -np2, vec3(5.0), Susanne64_handle);

      //  res2 = fOpUnionID4(res2, vec4(sdfv2.r, materialID0, sdfv2.gb));
    //}
    //float sdfv = texture3DSDF(rp, vec3(5.0), Susanne64_handle);
    
    //return res2;
    //return vec4(sdfv.x, materialID0, sdfv.gb);
    
    //vec3 spherePos = vec3(sin(time) * 3, 0, 0);
    vec3 spherePos = vec3(0.0, 0.0, 0.0);
    float sphere = signedDistanceSphere(p - spherePos, 1.0);

    vec3 q = p;
    
    q.xy *= rot2D(time);// rotate around Z

    float bScale = 1.0;
    float box = sdBox(q - vec3(-3, 0, 0) * bScale, vec3(0.75)) / bScale;
    
    float b2scale = clamp(sin(time) * 0.55, 0.2, 5.0);
    float box2 = sdBox((p - vec3(-15.0, 2.5, 0.0)) * b2scale, vec3(0.75)) / b2scale;

    float torus = sdTorus(p - vec3(0.0, 2.0, 2.0), vec2(1.0, 0.5));
    
    float ground = p2.y - -2.0;
    ///float ground = p2.y;
    float bScale2 = 0.55;

    vec2 smoothUnion = fOpUnionRoundID(vec2(sphere, materialID0), vec2(box, materialID1), 2.0);

    vec2 res = vec2(0.0f);
    res = fOpUnionID(smoothUnion, vec2(torus, materialID5));
    res = fOpUnionID(res, vec2(box2, materialID2));
    res = fOpUnionID(res, vec2(sdfv.r, materialID0));
    
    res = fOpUnionID(res, vec2(ground, materialID3));
    //res = fOpUnionID(res, priorSDF);

    //res = fOpUnionID(res, vec2(ground, materialID3));
    //res = fOpUnionID(res, vec2(wall, materialID3));
    //ground2

    
        for (int i = 0; i < 8; i++ ){
            vec3 spherePos2 = vec3(-i * 3, 0, 15);
            float sphere2 = signedDistanceSphere(p - spherePos2, 1.0);
            res = fOpUnionID(res, vec2(sphere2, materialID4)); 
        }
    
    return vec4(res,0.0f,0.0f);
    
    //return vec2(min(smin(sphere, box, 2.0), min(box2, torus)), materialID);
    //return min(sphere, box);
    //return sphere;
}

vec3 CalculateNormal( in vec3 p ){
    const float eps = 0.0001;
    const vec2 h = vec2(eps,0);
    return normalize( vec3(sceneSDF(p+h.xyy).x - sceneSDF(p-h.xyy).x,
    sceneSDF(p+h.yxy).x - sceneSDF(p-h.yxy).x,
    sceneSDF(p+h.yyx).x - sceneSDF(p-h.yyx).x ) );
}


float softshadow(vec3 ro, vec3 rd, float start, float end, float k){
    float result = 1.0;
    float previousDistToScene = 1e20;
    for (float distTravelled = start; distTravelled < end; ){
        float distToScene = sceneSDF(ro + rd * distTravelled).x;
        
        if (distToScene < 0.001){
            return 0.0;
        }
        
        float y = distToScene * distToScene / (2.0 * previousDistToScene);
        float d = sqrt(distToScene * distToScene - y * y);
        result = min(result, k * d / max(0.0, distTravelled - y));
        previousDistToScene = distToScene;
        distTravelled += distToScene;
        
    };
    return result;
}

hitresult internalRayMarchScene(vec3 ro, vec3 rd, float maxdist, float mindist, int steps){
    hitresult hr;
    hr.isHit = false;

    float t = 0.0; // total distance travelled

    // raymarching
    for (int i = 0; i < steps; i++){
        vec3 pos = ro + rd* t;// position along the ray

        vec4 m = -sceneSDF(pos);
        float dist = m.x;

        setupMaterialsAdvanced(pos);

        t += dist;

        if (dist < mindist){ // treat as if hit
            hr.isHit = true;
            hr.distance = dist;
            hr.normal = CalculateNormal(pos);
            hr.materialID = m.y;
            hr.materialIndex = int(m.y);
            hr.totalDistanceTravelled = t;
            hr.hitpos = pos;
            hr.uv = m.zw;
            
            break;// how small dist (radius around march)
        }
        if (dist > maxdist) break; // failed to hit
        //discard; // discard on far for transparency
        // how large dist (radius around march)
    }

    return hr;
}

hitresult raymarchScene(vec3 ro, vec3 rd, float maxdist, float mindist, int steps)
{
    hitresult hr;
    hr.isHit = false;

    float t = 0.0; // total distance travelled

    // raymarching
    for (int i = 0; i < steps; i++)
    {
        vec3 pos = ro + rd* t;// position along the ray
        
        vec4 m = sceneSDF(pos);
        float dist = m.x;
        
        setupMaterialsAdvanced(pos);

        t += dist;

        if (dist < mindist){ // treat as if hit
            hr.isHit = true;
            hr.distance = dist;
            hr.normal = CalculateNormal(pos);
            hr.materialID = m.y;
            hr.materialIndex = int(m.y);
            hr.totalDistanceTravelled = t;
            hr.hitpos = pos;
            hr.uv = m.zw;
            
            break;// how small dist (radius around march)
        }
        if (dist > maxdist) break; // failed to hit
        //discard; // discard on far for transparency
        // how large dist (radius around march)
    }
    
    return hr;
}

vec3 simpleReflection(material parentMaterial, vec3 ro, vec3 rd, float maxdist, float mindist, int steps, int bounces){
    vec3 colour = vec3(0.0);

    int hitcount = 0;

    vec3 lastorigin = ro;
    vec3 lastdir = rd;

    float dim = 1.0f;
    
    for (int i = 0; i < bounces; i++){
        if (dim <= 0.0f) break;
        hitcount++;
        
        hitresult hr = raymarchScene(lastorigin, lastdir, maxdist, mindist, steps);
        material nMaterial = materials[hr.materialIndex];
        
        if (hr.isHit){

            vec3 origin = hr.hitpos + hr.normal * originEplison;
            lastorigin = origin;

            lastdir = reflect(lastdir, hr.normal);
            
            float shadow = softshadow(origin, normalize(directLightPos), 0.01f, maxshadowDistance, 16.0);
            shadow += directAmbient; // global lighting
            shadow = clamp(shadow, 0.0, 1.0);
            
            material nMaterial = materials[hr.materialIndex];

            vec3 albedo = nMaterial.colour;
            float reflectionFactor = nMaterial.reflectionFactor;

            vec3 diffuseComponent = albedo * shadow * (1.0 - reflectionFactor);

            colour += diffuseComponent * dim;
            dim *= reflectionFactor;
        }
        else {
            colour += skycolour * dim; 
            break;
        }
    }
    
    return clamp(colour, 0.0, 1.0);
}

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 sampleHemisphere(vec3 normal, int index)
{
    float u = rand(texCoord + rand(vec2(time, float(index) ) ) );
    float v = rand(vec2(u, rand( vec2(time, float(index) ) ) ) );

    float phi = 2.0 * 3.14159265 * u;
    float cosTheta = sqrt(1.0 - v);
    float sinTheta = sqrt(v);

    vec3 localDir = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 helper = abs(normal.y) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0);
    vec3 tangent = normalize(cross(helper, normal));
    vec3 bitangent = cross(normal, tangent);

    return tangent * localDir.x + bitangent * localDir.y + normal * localDir.z;
}

vec3 indirectMarch(vec3 ro, vec3 rd, float maxdist, float mindist, int steps, int samples){
    vec3 colour = vec3(0.0);

    int hitcount = 0;

    for (int i = 0; i < samples; i++)
    {
        hitcount++;

        vec3 newDir = sampleHemisphere(rd, i);
        
        hitresult hr = raymarchScene(ro, newDir, maxdist, mindist, steps);
        material nMaterial = materials[hr.materialIndex];

        if (hr.isHit){

            vec3 origin = hr.hitpos + hr.normal * originEplison;

            float shadow = softshadow(origin, normalize(directLightPos), 0.01f, maxshadowDistance, 16.0);
            shadow += directAmbient; // global lighting
            shadow = clamp(shadow, 0.0, 1.0);

            vec3 albedo = nMaterial.colour;

            vec3 diffuseComponent = albedo * shadow;

            colour += diffuseComponent;
        }
        else {
            colour += skycolour;
            break;
        }
    }

    return clamp(colour / hitcount, 0.0, 1.0);
}



gbuffer lightAThit(hitresult hr, vec3 rayDir){
    gbuffer ngbuffer;

    if (hr.isHit){

        //uniform vec3 directLightPos;
        //uniform vec3 directLightCol;
        //uniform bool doDirLight;
        
        vec3 origin = hr.hitpos + hr.normal * originEplison;
        float shadow = softshadow(origin, normalize(directLightPos), 0.01f, maxshadowDistance, 16.0);
        shadow += directAmbient; // global lighting
        shadow = clamp(shadow, 0.0, 1.0);

        material nMaterial = materials[hr.materialIndex];

        vec3 albedo = nMaterial.colour;
        float reflectionFactor = nMaterial.reflectionFactor;

        vec3 rv = reflect(rayDir,  hr.normal); // reflection vector (outgoing and ingoing vectors)

        // reflection pass here
        vec3 r = vec3(0.0);
        if (doReflections) r = simpleReflection(nMaterial, origin, rv, r_maxdist, 0.001f, r_steps, r_bounces) * reflectionFactor;

        vec3 indirect = vec3(0.0);
        if (doIndirect) indirect = indirectMarch(origin, hr.normal, i_maxdist, 0.001f, i_steps, i_samples);

        vec3 gi = (shadow + indirect);
        //vec3 gi = ((shadow) + indirect);

        vec3 baseColour = albedo * gi * (1.0 - reflectionFactor);
        vec3 combine = baseColour + r;

        vec3 fog = mix(combine, skycolour, 1.0 - exp(-0.0008 * hr.distance * hr.distance) ); //firsthit.totalDistanceTravelled

        ngbuffer.colour = fog;
        ngbuffer.albedo = albedo;
        ngbuffer.normal = hr.normal;
        ngbuffer.depth = hr.totalDistanceTravelled;
        ngbuffer.hit = true;
        ngbuffer.hr = hr;
        //colour = r;
    }
    else {
        ngbuffer.colour = skycolour - max(0.95 * rayDir.y, 0.0);
        ngbuffer.hit = false;
    }
    
    return ngbuffer;
}

vec4 volumetric(vec3 albedo, vec3 ro, vec3 rd, float maxdist, float mindist, int steps){
    vec4 colour = vec4(0.0);
    vec3 elipson = rd * 0.1;
    hitresult internalHit = internalRayMarchScene(ro + elipson, rd, maxdist, mindist, steps);

    float beers = 0.0;
    //
    if (internalHit.isHit){
        //vec3 n3 = noise3(1.0);
        beers = exp(-internalHit.totalDistanceTravelled  * 0.5);
        vec3 elipson2 = rd * 0.1;
        hitresult behindhit = raymarchScene(internalHit.hitpos + elipson2, rd, maxdist, mindist, steps);
        //if (behindhit.isHit && false)
        if (behindhit.isHit){
            gbuffer nngbuffer = lightAThit(behindhit, rd);
            colour = vec4(mix(albedo,nngbuffer.colour, beers), 1.0);
        }
        else {
            colour = vec4(mix(albedo, skycolour - max(0.95 * rd.y, 0.0), beers), 1.0);
        }
    }

    return colour;
}

const vec2 HaltonJitters[16] = {
vec2( 0.0000f, -0.1667f),
vec2(-0.2500f,  0.1667f),
vec2( 0.2500f, -0.3889f),
vec2(-0.3750f, -0.0556f),
vec2( 0.1250f,  0.2778f),
vec2(-0.1250f, -0.2778f),
vec2( 0.3750f,  0.0556f),
vec2(-0.4375f,  0.3889f),
vec2( 0.0625f, -0.4630f),
vec2(-0.1875f, -0.1296f),
vec2( 0.3125f,  0.2037f),
vec2(-0.3125f, -0.3519f),
vec2( 0.1875f, -0.0185f),
vec2(-0.0625f,  0.3148f),
vec2( 0.4375f, -0.2407f),
vec2(-0.4688f,  0.0926f)
};

float linearizeDepth(float depth, float NP, float FP)
{
    return (2.0 * NP * FP) / (FP + NP - (depth * 2.0 - 1.0) * (FP - NP));
}

void main(){
    setupMaterials();
    
    ivec2 size = ivec2(screenSize);
    
    vec2 uv = (texCoord - 0.5) * 2.0;
    
    float depth = linearizeDepth(texture2D(depthMap, texCoord).r, 0.1f, maxdist);
    //FragColor = vec4(vec3(depth), 1.0);
    //return;
    //vec3 ro = vec3(0.0, 0.0, -3.0);// origin
    vec3 ro = cameraPosition;
    vec3 rd = normalize(vec3(uv, 1));// dir
    
    vec3 aNomal = vec3(0.0);
    float aDepth = 0.0;

    gbuffer ngbuffer;
    
    bool anyhit = false;
    int hitcount = 0;
    //depth
    
    for (int i = 0; i < SSAAsamples; i++) // 4 samples
    {
        vec2 jitter = HaltonJitters[i] / size;
        vec2 nuv = uv;
        
        if (SSAAsamples > 1) nuv = nuv + jitter * 3.5;
        vec3 rayDir = rayDirfromCam(inverse(u_ProjectionMatrix), inverse(u_ViewMatrix), nuv);
        
        float t = 0.0;// total distance travelled
        hitresult firsthit = raymarchScene(ro, rayDir, maxdist, mindist, steps);

        hitcount++;
        
        // final lighting
        gbuffer nngbuffer = lightAThit(firsthit, rayDir); // 0.001f
        if (materials[nngbuffer.hr.materialIndex].isVolumetric)
        { 
            vec4 nvolumetric = volumetric(nngbuffer.colour, firsthit.hitpos, rayDir, maxdist, mindist, steps);
            ngbuffer.colour += nvolumetric.rgb;   
        }
        else         ngbuffer.colour += nngbuffer.colour;
        
        ngbuffer.albedo += nngbuffer.albedo;
        ngbuffer.normal += nngbuffer.normal;
        ngbuffer.depth += nngbuffer.depth;
    }

    //if (hitcount == 0 || !anyhit) discard;

    vec3 colour = vec3(0.0);
    
    if (hitcount == 0 || !anyhit) colour = ngbuffer.colour / hitcount;
    else  colour = ngbuffer.colour / float(hitcount);
    //colour = aColour / float(hitcount);
    
    //vec3 colour = aColour / float(hitcount);
    //float ndepth = aDepth / float(hitcount);
    //vec3 normal = aNomal / float(hitcount);

    //if (ndepth > depth) discard;

    FragColor = vec4(clamp(colour, 0.0, 1.0), 1.0);
}