#version 460 core
#extension GL_ARB_gpu_shader_int64 : enable
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_fragment_shader_interlock : enable

layout(early_fragment_tests) in;
layout(pixel_interlock_ordered) in;

out vec4 FragColor;
in highp vec2 texCoord;

//r11f_g11f_b10f rgba16f
layout(rgba16f, binding = 0) uniform image3D outVXIRC0;
layout(rgba16f, binding = 1) uniform image3D outVXIRC1;
layout(rgba16f, binding = 2) uniform image3D outVXIRC2;
layout(rgba16f, binding = 3) uniform image3D outVXIRC3;

layout(rgba16f, binding = 4) uniform image3D outVXERC0;
layout(rgba16f, binding = 5) uniform image3D outVXERC1;
layout(rgba16f, binding = 6) uniform image3D outVXERC2;
layout(rgba16f, binding = 7) uniform image3D outVXERC3;

uniform sampler2D swrtIndirect;
uniform sampler2D swrtEmission;

uniform sampler2D depthMap;
uniform sampler2D gPosition;
uniform bool vxirc_DoScreenGather;

struct clipmapLevel{
    vec4 ps; // pos & scale
    uint64_t handle; // dist
    uint64_t handle2; // alb
    uint64_t handle3; // em
    uint64_t handle4; // metallic rough (not implemented)
    uint64_t handle5; // indirect cache
    uint64_t handle6; // emission cache?
    float thickness;
    uint dirty;
    uint pad;
    uint pad0;
};

// global distance fields
layout(std430, binding = 12) buffer GDF_Buffer {
    clipmapLevel GDFS[];
};


float sdBox(vec3 p, vec3 b){
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0f);
}

ivec3 getCascadeSize(int cascade){
    switch(cascade){
        case 0: return imageSize(outVXIRC0);
        case 1: return imageSize(outVXIRC1);
        case 2: return imageSize(outVXIRC2);
        case 3: return imageSize(outVXIRC3);
        default: return imageSize(outVXIRC0);
    }

    return ivec3(1);
}


vec4 loadIRCascade(int cascade, ivec3 uvw){
    switch(cascade){
        case 0: return imageLoad(outVXIRC0, uvw);
        case 1: return imageLoad(outVXIRC1, uvw);
        case 2: return imageLoad(outVXIRC2, uvw);
        case 3: return imageLoad(outVXIRC3, uvw);
        default: return vec4(0.0);
    }

    return vec4(0.0f);
}

vec4 loadERCascade(int cascade, ivec3 uvw){
    switch(cascade){
        case 0: return imageLoad(outVXERC0, uvw);
        case 1: return imageLoad(outVXERC1, uvw);
        case 2: return imageLoad(outVXERC2, uvw);
        case 3: return imageLoad(outVXERC3, uvw);
        default: return vec4(0.0);
    }

    return vec4(0.0f);
}

void storeCascade(int cascade, ivec3 uvw, vec4 indinp, vec4 emdinp){
    switch(cascade){
        case 0: imageStore(outVXIRC0, uvw, indinp); imageStore(outVXERC0, uvw, emdinp); break;
        case 1: imageStore(outVXIRC1, uvw, indinp); imageStore(outVXERC1, uvw, emdinp); break;
        case 2: imageStore(outVXIRC2, uvw, indinp); imageStore(outVXERC2, uvw, emdinp); break;
        case 3: imageStore(outVXIRC3, uvw, indinp); imageStore(outVXERC3, uvw, emdinp); break;
    }
}

ivec3 uvwFromCascade(vec3 gp, vec3 vxs, ivec3 resolution){
    vec3 uvw = (gp / (vxs * 2.0)) + 0.5;
    vec3 coords = uvw * vec3(resolution);
    ivec3 icoords = ivec3(floor(coords));
    return clamp(icoords, ivec3(1), resolution - ivec3(2));
}

void writeToCascade(vec3 gp, vec3 irr, vec3 em, int cascade){
    vec3 vxp = GDFS[cascade].ps.xyz;
    vec3 vxscale = vec3(GDFS[cascade].ps.w);
    vec3 tp = gp - vxp;
    if (sdBox(tp, vxscale) < 0.0){
        ivec3 UVW = uvwFromCascade(tp, vxscale, getCascadeSize(cascade));
        vec3 pIVX = loadIRCascade(cascade, UVW).rgb;
        vec3 pEVX = loadERCascade(cascade, UVW).rgb;

        vec3 nIVX = mix(irr, pIVX, 0.90);
        vec3 nEVX = mix(em, pEVX, 0.90);
        storeCascade(cascade, UVW, vec4(nIVX, 1.0), vec4(nEVX, 1.0));
    }
}

void main(){
    vec4 indirect               = texture(swrtIndirect,texCoord);
    vec4 emission             = texture(swrtEmission,texCoord);
    float gdepth                 = texture(depthMap, texCoord).r;
    vec3 gp                       = texture(gPosition, texCoord).rgb;

    // locked to prevent race conditions
    #if 1
    beginInvocationInterlockARB();

    if (vxirc_DoScreenGather){
        writeToCascade(gp, indirect.rgb, emission.rgb, 0);
        writeToCascade(gp, indirect.rgb, emission.rgb, 1);
        writeToCascade(gp, indirect.rgb, emission.rgb, 2);
        writeToCascade(gp, indirect.rgb, emission.rgb, 3);
    }
    endInvocationInterlockARB();
    #endif
}