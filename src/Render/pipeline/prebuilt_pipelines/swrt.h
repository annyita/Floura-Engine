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
       GLuint ID = 0;
        GLuint ID2 = 0;
    };
    
    struct localSDF{
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
        uint64_t ALB_Handle;
        uint64_t padding;
        
        //uint64_t texture_diffuse_Handle;
        //uint64_t texture_roughness_Handle;
        //uint64_t texture_normal_Handle;
        //uint64_t texture_emission_Handle;
    };

    static  int localPerModelMeshCountCap;
    
    static std::vector<localSDF> localSDFS;
    
    static Texture3D* ring0; 
    static Texture3D* ring1; 
    static Texture3D* ring2; 
    static Texture3D* ring3;
    static Texture3D* ring0A; 
    static Texture3D* ring1A; 
    static Texture3D* ring2A; 
    static Texture3D* ring3A;
    static std::vector<accelerate::clipmapLevel> clips;
    static std::vector<gdfDrawbatch> gdfINFO;
    
    struct probeChunk{
        Texture3D* indirectVolume;
        Texture3D* emissionVolume;
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 lposition = glm::vec3(0.0f);
        float scale = 10.0f;
        bool dirty = false;
    };
    
    static bool doTemporalAccumulation;
    static bool doSVGF;
    static bool doDenoiseSplitDBGView;
    static float temporalAccumulationBlendFactor;
    static float resScaleFactor;
    static int passNum;
    static int autoMDFres;
    static double rmTime;
    
    
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
    static Shader denoisePrePassShader;
    static Shader denoiseShader;
    static Shader pingPongShader;
    //static Shader pingPongCompShader;
    static Shader presentShader;
    static Shader GDFdrawShader;
public:
    
    static probeChunk testChunk;
    static void rmPassRenderQuad(unsigned int w, unsigned int h);
    
    static void prepChunks();
    static void indirectProbePass();
private:
    static void rmPassCompute(unsigned int w, unsigned int h);
    static void denoisePrePass();
    static void denoisePass();
    static void pingPongPass();    
    //static void pingPongPassCompute(unsigned int w, unsigned int h);
    static void presentPass(unsigned int w, unsigned int h);
    
    static float pAccum;
    static float pAccumthresh;
    
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
    
    //static void updateUVscale(uint64_t instanceUUID, glm::vec2& scale);
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
    
};

#endif