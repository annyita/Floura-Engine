#ifndef SWRT_CLASS_H
#define SWRT_CLASS_H

#include <Render/Shader/shaderClass.h>
#include <Render/Object/texture3D.h>
#include <Render/Buffer/Framebuffer.h>
#include <Systems/Physics/accelerate.h>

class FlouraSWRT {
public:
    struct gdfDrawbatch{
        int resolution = 1;
        int vxResolution = 1;
       GLuint ID = 0;
        GLuint ID2 = 0;
        GLuint ID3 = 0;
        GLuint ID4 = 0;
        GLuint ID5 = 0;
        GLuint ID6 = 0;
    };
    
    struct MDF{
        glm::vec4 position; // uv.x
        glm::vec4 extents; // uv.y
        glm::vec4 rootPosition;
        glm::vec4 rootExtents;
        glm::vec4 gPosition;
        glm::vec4 gExtents;
        //glm::vec4 gRotation;
        glm::mat4 globalTransform;
        
        uint64_t instanceUUID;
        uint64_t SDF_Handle;
        uint64_t UV_Handle;
        uint64_t ALB_Handle;
        uint64_t EM_Handle;
        uint64_t ARM_Handle;
        glm::vec4 extraFactors;
        glm::vec4 extraFactors2;
        //uint64_t texture_diffuse_Handle;
        //uint64_t texture_roughness_Handle;
        //uint64_t texture_normal_Handle;
        //uint64_t texture_emission_Handle;
    };

    static  int localPerModelMeshCountCap;
    
    static std::vector<MDF> MDFS;
    
    static Texture3D* ring0; 
    static Texture3D* ring1; 
    static Texture3D* ring2; 
    static Texture3D* ring3;
    
    static Texture3D* ring0A; 
    static Texture3D* ring1A; 
    static Texture3D* ring2A; 
    static Texture3D* ring3A;
    
    static Texture3D* ring0E; 
    static Texture3D* ring1E; 
    static Texture3D* ring2E; 
    static Texture3D* ring3E;
    
    static Texture3D* ring0RoughMet; 
    static Texture3D* ring1RoughMet; 
    static Texture3D* ring2RoughMet; 
    static Texture3D* ring3RoughMet; 
    
    static Texture3D* ring0IRC; 
    static Texture3D* ring1IRC; 
    static Texture3D* ring2IRC; 
    static Texture3D* ring3IRC;
    
    static Texture3D* ring0ERC; 
    static Texture3D* ring1ERC; 
    static Texture3D* ring2ERC; 
    static Texture3D* ring3ERC;
    
    static std::vector<accelerate::clipmapLevel> clips;
    static std::vector<gdfDrawbatch> gdfINFO;
    
    static bool doTemporalAccumulation;
    static bool doSVGF;
    static float temporalAccumulationBlendFactor;
    static float resScaleFactor;
    static int passNum;
    static int autoMDFres;
    static double rmTime;
    
    // vxirc
    static bool vxirc_DoWorldGather;
    static bool vxirc_DoScreenGather;
    static int vxirc_fibonacciSphereSamples;
    
    static void initShaders();
    
    static void cleanupShaders();
    
    static void setupSWRTbuffers(unsigned int width, unsigned int height);
    
    static void updateSWRTbuffersResolution(unsigned int width, unsigned int height);
    
    static void cleanupSWRTbuffers();
    
    static void draw();
    
private:
    static Framebuffer swrtBuffer; 
    static Framebuffer historyBuffer; 
    static Framebuffer denoiseBuffer; 

    
    static Shader raymarchRQShader;
    static Shader indirectProbeShader;
    static Shader raymarchCompShader;
    static Shader gatherCombinePassShader;
    static Shader denoisePrePassShader;
    static Shader denoiseShader;
    static Shader pingPongShader;
    //static Shader pingPongCompShader;
    static Shader presentShader;
    static Shader GDFdrawShader;
public:
    
    static void rmPassRenderQuad(unsigned int w, unsigned int h);

    static void indirectProbePass(GLuint IRID, GLuint EMID, int w, int h, int d, glm::vec4 probePos);
private:
    static void rmPassCompute(unsigned int w, unsigned int h);
    static void combineGatherPass();
    static void denoisePrePass();
    static void denoisePass();
    static void pingPongPass();    
    //static void pingPongPassCompute(unsigned int w, unsigned int h);
    static void presentPass(unsigned int w, unsigned int h);
public:
    
    // lets do buffers down here
    static GLuint localSDF_SSBOID;
    static GLuint GDF_SSBOID;
    
    static void initSWRTssbo();
    static void cleanupSWRTssbo();
    
    static void updateSDFBuffer();
    static void updateGDFBuffer();
    
    static void wipeScene();
    
    static void uploadToLSDFScene(uint64_t instanceUUID);
    static void removeFromLSDFScene(uint64_t instanceUUID);
    
    // unused, reimplement
    static void updateUVscale(uint64_t instanceUUID, glm::vec2& scale);
    static void updateGlobalTransformation(uint64_t instanceUUID, glm::mat4& gt, glm::vec3 gRotation);
    
    // mostly GDF
    static float gdfAccumthresh;
    static float clipAccum;
    
    static void update();
    static void MDFdebugDraw();
    static void GDFdebugDraw();
    
    static bool dirtyGDF;
    static float gdfAccum;
    static float clipAccumthresh;
    
    static void GDFdraw();
    
    static float cacheAccum;
    static float cacheAccumthresh;
    static bool isCacheFreshlyWiped;
    
    static void worldSpaceCacheGather();
    
};

#endif