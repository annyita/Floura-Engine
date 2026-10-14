#include "swrt.h"
#include "Render/pipeline/prebuilt_pipelines/geometryPass.h"
#include "Scene/scene.h"
#include "Scene/LightingHandler.h"
#include <chrono>    
#include "utils/FE_math.h"
#include "Render/Handler/RenderHandler.h"

Framebuffer FlouraSWRT::swrtBuffer; 
Framebuffer FlouraSWRT::historyBuffer; 
Framebuffer FlouraSWRT::denoiseBuffer; 

Shader FlouraSWRT::raymarchRQShader;
Shader FlouraSWRT::indirectProbeShader;
Shader FlouraSWRT::raymarchCompShader;
Shader FlouraSWRT::gatherCombinePassShader;
Shader FlouraSWRT::denoisePrePassShader;
Shader FlouraSWRT::denoiseShader;
Shader FlouraSWRT::pingPongShader;
//Shader FlouraSWRT::pingPongCompShader;
Shader FlouraSWRT::presentShader;
Shader FlouraSWRT::GDFdrawShader;

int FlouraSWRT::localPerModelMeshCountCap = 4000; // 10
std::vector<FlouraSWRT::MDF> FlouraSWRT::MDFS;

Texture3D* FlouraSWRT::ring0; 
Texture3D* FlouraSWRT::ring1; 
Texture3D* FlouraSWRT::ring2; 
Texture3D* FlouraSWRT::ring3;
Texture3D* FlouraSWRT::ring0A; 
Texture3D* FlouraSWRT::ring1A; 
Texture3D* FlouraSWRT::ring2A; 
Texture3D* FlouraSWRT::ring3A;
Texture3D* FlouraSWRT::ring0E; 
Texture3D* FlouraSWRT::ring1E; 
Texture3D* FlouraSWRT::ring2E; 
Texture3D* FlouraSWRT::ring3E;
Texture3D* FlouraSWRT::ring0RoughMet; 
Texture3D* FlouraSWRT::ring1RoughMet; 
Texture3D* FlouraSWRT::ring2RoughMet; 
Texture3D* FlouraSWRT::ring3RoughMet; 
Texture3D* FlouraSWRT::ring0IRC; 
Texture3D* FlouraSWRT::ring1IRC; 
Texture3D* FlouraSWRT::ring2IRC; 
Texture3D* FlouraSWRT::ring3IRC;
Texture3D* FlouraSWRT::ring0ERC; 
Texture3D* FlouraSWRT::ring1ERC; 
Texture3D* FlouraSWRT::ring2ERC; 
Texture3D* FlouraSWRT::ring3ERC;

std::vector<accelerate::clipmapLevel> FlouraSWRT::clips;
std::vector<FlouraSWRT::gdfDrawbatch> FlouraSWRT::gdfINFO;

bool FlouraSWRT::doTemporalAccumulation = true;
bool FlouraSWRT::doSVGF = true;
float FlouraSWRT::temporalAccumulationBlendFactor = 0.90f;
float FlouraSWRT::resScaleFactor = RenderClass::resQuality;
int FlouraSWRT::passNum = 5;
int FlouraSWRT::autoMDFres = 64;
double FlouraSWRT::rmTime = 0.0f;

// vxirc
bool FlouraSWRT::vxirc_DoWorldGather = true;
bool FlouraSWRT::vxirc_DoScreenGather = true;
int FlouraSWRT::vxirc_fibonacciSphereSamples = 12;

void FlouraSWRT::initShaders(){
	GDFdrawShader.LoadComputeShader("Assets/Shaders/compute/gdfDraw.comp");
    raymarchRQShader.LoadShader("Assets/Shaders/common/triangle.vert", "Assets/Shaders/raymarched/traceSWRT.frag");
	indirectProbeShader.LoadComputeShader("Assets/Shaders/raymarched/indirectProbeSWRT.comp");
	raymarchCompShader.LoadComputeShader("Assets/Shaders/raymarched/traceSWRT.comp");
	gatherCombinePassShader.LoadShader("Assets/Shaders/common/triangle.vert", "Assets/Shaders/raymarched/combineGatherCaptureSWRT.frag");
	denoisePrePassShader.LoadShader("Assets/Shaders/common/triangle.vert", "Assets/Shaders/raymarched/denoisePrePassSWRT.frag");
	denoiseShader.LoadShader("Assets/Shaders/common/triangle.vert", "Assets/Shaders/raymarched/denoiseSWRT.frag");
	pingPongShader.LoadShader("Assets/Shaders/common/triangle.vert", "Assets/Shaders/raymarched/pingpongSWRT.frag");
	//pingPongCompShader.LoadComputeShader("Assets/Shaders/raymarched/pingpongSWRT.comp");
	presentShader.LoadShader("Assets/Shaders/common/triangle.vert", "Assets/Shaders/raymarched/presentSWRT.frag");
}

void FlouraSWRT::cleanupShaders(){
	GDFdrawShader.Delete();
	raymarchRQShader.Delete();
	indirectProbeShader.Delete();
	raymarchCompShader.Delete();
	gatherCombinePassShader.Delete();
	denoisePrePassShader.Delete();
	denoiseShader.Delete();
	pingPongShader.Delete();
	//pingPongCompShader.Delete();
	presentShader.Delete();
}

void FlouraSWRT::setupSWRTbuffers(unsigned int width, unsigned int height){
	// type and format can be made from internal format so please add in the future
	
	// indirect 0
 	swrtBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	//specular 1
	swrtBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
	//emission 2
	swrtBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	//ao 3
	//swrtBuffer.uploadAttachment(GL_R8, GL_RED, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
	
	swrtBuffer.createBuffers(width, height);
	
	// hIndirect 0
	historyBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	// hEmission 1
	historyBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	// hSpecular 2
	historyBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	// swrtHAO 3
	//historyBuffer.uploadAttachment(GL_R8, GL_RED, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	// swrtHDepth 3
	historyBuffer.uploadAttachment(GL_R32F, GL_RED, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	// filteredVariance 4
	historyBuffer.uploadAttachment(GL_RG16F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	
	historyBuffer.createBuffers(width, height);
	
	// dIndirect 0
	denoiseBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
	// dEmissions 1
	denoiseBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
	
	denoiseBuffer.createBuffers(width, height);
	
	
	// add denoise buffer and clipmaps cleanup
	const int r0r = 128; // 128
	const int r1r = 128; // 256
	const int r2r = 128;
	const int r3r = 128;
	
	const int r0vx = 16; // 128
	const int r1vx = 32; // 256
	const int r2vx = 32; // 64
	const int r3vx = 32;	
	// SD
	ring0 = new Texture3D(); ring0->createImage3D(r0r, r0r, r0r, "GDF", 0, GL_R16F); // GL_RGBA32F
	ring1 = new Texture3D(); ring1->createImage3D(r1r, r1r, r1r, "GDF", 0, GL_R16F); // GL_R8_SNORM // GL_RGBA8_SNORM
	ring2 = new Texture3D(); ring2->createImage3D(r2r, r2r, r2r, "GDF", 0, GL_R16F); // GL_RGBA16F
	ring3 = new Texture3D(); ring3->createImage3D(r3r, r3r, r3r, "GDF", 0, GL_R16F); // GL_R16F
	// ALB
	ring0A = new Texture3D(); ring0A->filtering = GL_LINEAR; ring0A->createImage3D(r0r, r0r, r0r, "vxALB", 0, GL_RGBA8);
	ring1A = new Texture3D(); ring1A->filtering = GL_LINEAR; ring1A->createImage3D(r1r, r1r, r1r, "vxALB", 0, GL_RGBA8);
	ring2A = new Texture3D(); ring2A->filtering = GL_LINEAR; ring2A->createImage3D(r2r, r2r, r2r, "vxALB", 0, GL_RGBA8);
	ring3A = new Texture3D(); ring3A->filtering = GL_LINEAR; ring3A->createImage3D(r3r, r3r, r3r, "vxALB", 0, GL_RGBA8);
	// EM
	ring0E = new Texture3D(); ring0E->filtering = GL_LINEAR; ring0E->createImage3D(r0r, r0r, r0r, "vxEM", 0, GL_R11F_G11F_B10F); // GL_R11F_G11F_B10F
	ring1E = new Texture3D(); ring1E->filtering = GL_LINEAR; ring1E->createImage3D(r1r, r1r, r1r, "vxEM", 0, GL_R11F_G11F_B10F);
	ring2E = new Texture3D(); ring2E->filtering = GL_LINEAR; ring2E->createImage3D(r2r, r2r, r2r, "vxEM", 0, GL_R11F_G11F_B10F);
	ring3E = new Texture3D(); ring3E->filtering = GL_LINEAR; ring3E->createImage3D(r3r, r3r, r3r, "vxEM", 0, GL_R11F_G11F_B10F);
	// METROUGH
	ring0RoughMet = new Texture3D(); ring0RoughMet->filtering = GL_LINEAR; ring0RoughMet->createImage3D(r0r, r0r, r0r, "vxARM", 0, GL_RG8); // GL_R11F_G11F_B10F
	ring1RoughMet = new Texture3D(); ring1RoughMet->filtering = GL_LINEAR; ring1RoughMet->createImage3D(r1r, r1r, r1r, "vxARM", 0, GL_RG8);
	ring2RoughMet = new Texture3D(); ring2RoughMet->filtering = GL_LINEAR; ring2RoughMet->createImage3D(r2r, r2r, r2r, "vxARM", 0, GL_RG8);
	ring3RoughMet = new Texture3D(); ring3RoughMet->filtering = GL_LINEAR; ring3RoughMet->createImage3D(r3r, r3r, r3r, "vxARM", 0, GL_RG8);
	// vx irradance cache ring0IRC->filtering = ircfilter; 
	GLenum ircfilter = GL_LINEAR; //GL_NEAREST
	ring0IRC = new Texture3D(); ring0IRC->filtering = ircfilter; ring0IRC->createImage3D(r0vx, r0vx, r0vx, "vxIrrCache", 0, GL_RGBA16F); // GL_RGBA16F
	ring1IRC = new Texture3D(); ring1IRC->filtering = ircfilter; ring1IRC->createImage3D(r1vx, r1vx, r1vx, "vxIrrCache", 0, GL_RGBA16F); // GL_R11F_G11F_B10F
	ring2IRC = new Texture3D(); ring2IRC->filtering = ircfilter; ring2IRC->createImage3D(r2vx, r2vx, r2vx, "vxIrrCache", 0, GL_RGBA16F);
	ring3IRC = new Texture3D(); ring3IRC->filtering = ircfilter; ring3IRC->createImage3D(r3vx, r3vx, r3vx, "vxIrrCache", 0, GL_RGBA16F);
	// vx emission cache
	ring0ERC = new Texture3D(); ring0ERC->filtering = ircfilter; ring0ERC->createImage3D(r0vx, r0vx, r0vx, "vxEmCache", 0, GL_RGBA16F); // GL_RGBA16F
	ring1ERC = new Texture3D(); ring1ERC->filtering = ircfilter; ring1ERC->createImage3D(r1vx, r1vx, r1vx, "vxEmCache", 0, GL_RGBA16F); // GL_R11F_G11F_B10F
	ring2ERC = new Texture3D(); ring2ERC->filtering = ircfilter; ring2ERC->createImage3D(r2vx, r2vx, r2vx, "vxEmCache", 0, GL_RGBA16F);
	ring3ERC = new Texture3D(); ring3ERC->filtering = ircfilter; ring3ERC->createImage3D(r3vx, r3vx, r3vx, "vxEmCache", 0, GL_RGBA16F);

	gdfDrawbatch nGDB;
	nGDB.resolution = ring0->width; nGDB.vxResolution = ring0IRC->width;
	nGDB.ID = ring0->ID; nGDB.ID2 = ring0A->ID; nGDB.ID3 = ring0E->ID; nGDB.ID4 = ring0RoughMet->ID; nGDB.ID5 = ring0IRC->ID; nGDB.ID6 = ring0ERC->ID;
	gdfINFO.push_back(nGDB);
	nGDB.resolution = ring1->width; nGDB.vxResolution = ring1IRC->width;
	nGDB.ID = ring1->ID; nGDB.ID2 = ring1A->ID; nGDB.ID3 = ring1E->ID; nGDB.ID4 = ring1RoughMet->ID; nGDB.ID5 = ring1IRC->ID; nGDB.ID6 = ring1ERC->ID;
	gdfINFO.push_back(nGDB);
	nGDB.resolution = ring2->width; nGDB.vxResolution = ring2IRC->width;
	nGDB.ID = ring2->ID; nGDB.ID2 = ring2A->ID; nGDB.ID3 = ring2E->ID; nGDB.ID4 = ring2RoughMet->ID;  nGDB.ID5 = ring2IRC->ID; nGDB.ID6 = ring2ERC->ID;
	gdfINFO.push_back(nGDB);
	nGDB.resolution = ring3->width; nGDB.vxResolution = ring3IRC->width;
	nGDB.ID = ring3->ID; nGDB.ID2 = ring3A->ID; nGDB.ID3 = ring3E->ID; nGDB.ID4 = ring3RoughMet->ID; nGDB.ID5 = ring3IRC->ID; nGDB.ID6 = ring3ERC->ID;
	gdfINFO.push_back(nGDB);
	
	accelerate::clipmapLevel clip;
	clip.dirty = 1;
	
	clip.handle = ring0->handle;
	clip.handle2 = ring0A->handle;
	clip.handle3 = ring0E->handle;
	clip.handle4 = ring0RoughMet->handle;
	clip.handle5 = ring0IRC->handle;
	clip.handle6 = ring0ERC->handle;
	clip.ps = glm::vec4(0.0,0.0,0.0,10.0); // 256
	clip.thickness = 0.0;
	clips.push_back(clip);
	
	clip.handle = ring1->handle;
	clip.handle2 = ring1A->handle;
	clip.handle3 = ring1E->handle;
	clip.handle4 = ring1RoughMet->handle;
	clip.handle5 = ring1IRC->handle;
	clip.handle6 = ring1ERC->handle;
	clip.ps = glm::vec4(0.0,0.0,0.0,30.0); // 128
	clip.thickness = 0.0;
	clips.push_back(clip);
	
	/**/
	clip.handle = ring2->handle;
	clip.handle2 = ring2A->handle;
	clip.handle3 = ring2E->handle;
	clip.handle4 = ring2RoughMet->handle;
	clip.handle5 = ring2IRC->handle;
	clip.handle6 = ring2ERC->handle;
	clip.ps = glm::vec4(0.0,0.0,0.0,50.0); // 64
	clip.thickness = 0.05;
	clips.push_back(clip);
	
	clip.handle = ring3->handle;
	clip.handle2 = ring3A->handle;
	clip.handle3 = ring3E->handle;
	clip.handle4 = ring3RoughMet->handle;
	clip.handle5 = ring3IRC->handle;
	clip.handle6 = ring3ERC->handle;
	clip.ps = glm::vec4(0.0,0.0,0.0,100.0); // 32
	clip.thickness = 0.1;
	clips.push_back(clip);

}

void FlouraSWRT::updateSWRTbuffersResolution(unsigned int width, unsigned int height){
	if (resScaleFactor != 1.0){
		width = static_cast<unsigned int>(static_cast<float>(width) * resScaleFactor);
		height = static_cast<unsigned int>(static_cast<float>(height) * resScaleFactor);
	}
	swrtBuffer.resizeBuffers(width, height);
	historyBuffer.resizeBuffers(width, height);
	denoiseBuffer.resizeBuffers(width, height);
	
	glBindTexture(GL_TEXTURE_2D, 0);
}

void FlouraSWRT::cleanupSWRTbuffers(){
	swrtBuffer.deleteBuffers();
	historyBuffer.deleteBuffers();
	denoiseBuffer.deleteBuffers();
	
	ring0->Delete();
	ring1->Delete();
	ring2->Delete();
	ring3->Delete();
	
	ring0A->Delete();
	ring1A->Delete();
	ring2A->Delete();
	ring3A->Delete();
	
	ring0E->Delete();
	ring1E->Delete();
	ring2E->Delete();
	ring3E->Delete();
	
	ring0RoughMet->Delete();
	ring1RoughMet->Delete();
	ring2RoughMet->Delete();
	ring3RoughMet->Delete();
	
	ring0IRC->Delete();
	ring1IRC->Delete();
	ring2IRC->Delete();
	ring3IRC->Delete();
	
	ring0ERC->Delete();
	ring1ERC->Delete();
	ring2ERC->Delete();
	ring3ERC->Delete();

}

void FlouraSWRT::draw(){
	glDisable(GL_CULL_FACE);
	
	rmTime = 0.0;
	if (resScaleFactor != 1.0){
		unsigned int w = static_cast<unsigned int>(static_cast<float>(renderTarget::ViewPortWidth) * resScaleFactor);	
		unsigned int h = static_cast<unsigned int>(static_cast<float>(renderTarget::ViewPortHeight) * resScaleFactor);
		glViewport(0, 0, w, h );
		//std::cout << w << " " << h << std::endl;
		rmPassRenderQuad(w, h);
		//rmPassCompute(w,h);
		combineGatherPass();
		denoisePrePass();
		denoisePass();
		w = renderTarget::ViewPortWidth;	
		h = renderTarget::ViewPortHeight;
		glViewport(0, 0, w, h );
		presentPass(w, h);
	}
	else{
		unsigned int w = renderTarget::ViewPortWidth;	
		unsigned int h = renderTarget::ViewPortHeight;
		rmPassRenderQuad(w, h);
		//rmPassCompute(w,h);
		combineGatherPass();
		denoisePrePass();
		denoisePass();
		presentPass(w, h);
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glActiveTexture(0);
}

void FlouraSWRT::rmPassRenderQuad(unsigned int w, unsigned int h){
	glBindFramebuffer(GL_FRAMEBUFFER, swrtBuffer.FBO);
	
    raymarchRQShader.Activate();
	raymarchRQShader.setTexture2D("gVelocity",      0,  GeometryPass::gVelocity);
    raymarchRQShader.setTexture2D("gPosition",      1,  GeometryPass::gPosition);
    raymarchRQShader.setTexture2D("gNormal",       2,  GeometryPass::gNormal);
    raymarchRQShader.setTexture2D("gAlbedoSpec", 3,  GeometryPass::gAlbedo);
    raymarchRQShader.setTexture2D("depthMap",     5,  GeometryPass::depthTexture);
    raymarchRQShader.setTexture2D("gSpecular",      6,  GeometryPass::gSpecular);
    raymarchRQShader.setTexture2D("gEmission",    10,  GeometryPass::gEmission);
	
	raymarchRQShader.setTexture2D("hIndirect",               11,  historyBuffer.fetchID(0));
	raymarchRQShader.setTexture2D("hEmission",             12,  historyBuffer.fetchID(1));
	raymarchRQShader.setTexture2D("hSpecular",               13,historyBuffer.fetchID(2));
	//raymarchRQShader.setTexture2D("hAO",               14,historyBuffer.fetchID(3));
	raymarchRQShader.setTexture2D("swrtHDepth",          15,  historyBuffer.fetchID(3));
	//raymarchRQShader.setTexture2D("presentImage",         18,denoiseBuffer.fetchID(0));
	
	raymarchRQShader.setFloat("NearPlane", Scene::maincamera.nearFar.x);
	raymarchRQShader.setFloat("FarPlane", Scene::maincamera.nearFar.y);
	raymarchRQShader.setBool("doTemporalAccumulation", doTemporalAccumulation);
	raymarchRQShader.setFloat("temporalAccumulationBlendFactor", temporalAccumulationBlendFactor);
	
    raymarchRQShader.setFloat2("screenSize", glm::vec2(w, h));
    raymarchRQShader.setMat4("viewMatrix", Scene::maincamera.view);
	raymarchRQShader.setMat4("invViewMatrix", glm::inverse(Scene::maincamera.view));
    raymarchRQShader.setMat4("projectionMatrix", Scene::maincamera.projection);
	raymarchRQShader.setMat4("invProjectionMatrix", glm::inverse(Scene::maincamera.projection));
	raymarchRQShader.setMat4("invHViewMatrix", glm::inverse(Scene::maincamera.hView));
	raymarchRQShader.setMat4("invHProjectionMatrix",glm::inverse(Scene::maincamera.hProjection));
	raymarchRQShader.setMat4("invCameraMatrix", glm::inverse(Scene::maincamera.cameraMatrix));
	
    raymarchRQShader.setFloat3("cameraPosition", Scene::maincamera.Position);
    //raymarchShader.setFloat3("skycolour", skyRGBA);
    raymarchRQShader.setHandleui64ARB("BlueNoiseHandle", RenderClass::bluenoise->handle);
    Skybox::SkyboxCubemap->cubemapToUUIDShader("cmMainHandle", raymarchRQShader);
	
	raymarchRQShader.setFloat3("sceneBoundPos", Scene::SceneBounds.position);
	raymarchRQShader.setFloat3("sceneBoundScale", Scene::SceneBounds.size);
	
	
    raymarchRQShader.setTimeVariables();
	
    LightingHandler::sendToShader(raymarchRQShader);
	//Shader.setFloat4("skyColor", glm::vec4(RenderClass::skyRGBA, 1.0f));
	
    RenderQuad::draw();
}

//#include "utils/imageWrite.h"

void FlouraSWRT::indirectProbePass(GLuint IRID, GLuint EMID, int w, int h, int d, glm::vec4 probePos){
	// shadows should probably update first
		//prepChunks();

		indirectProbeShader.Activate();
		//glBindImageTexture(1, testVolume->ID, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
		glBindImageTexture(0, IRID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
		glBindImageTexture(1, EMID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
		
		LightingHandler::sendToShader(indirectProbeShader);
		indirectProbeShader.setTimeVariables();
		indirectProbeShader.setInt3("volumeSize", w,h,d);
	
		Skybox::SkyboxCubemap->cubemapToUUIDShader("cmMainHandle", indirectProbeShader);
		
		// temp
		indirectProbeShader.setFloat4("positionScale", probePos);
		indirectProbeShader.setInt("fibonacciSphereSamples", vxirc_fibonacciSphereSamples);

		glDispatchCompute((w + 7) / 8, (h + 7) / 8, ((d + 7) / 8));
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// temp
		/*
		FlouraImageWrite::writeImage3DToDiskPNG(testChunk.indirectVolume->ID,
		 128,128,128,
		"Cache/lightmap_128.png", GL_RGB, GL_UNSIGNED_BYTE, 3); // GB
		*/
}

void FlouraSWRT::rmPassCompute(unsigned int w, unsigned int h){
	raymarchCompShader.Activate();
	
	glBindImageTexture(0, swrtBuffer.fetchID(0), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glBindImageTexture(1, swrtBuffer.fetchID(1), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
	glBindImageTexture(2, swrtBuffer.fetchID(2), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	//glBindImageTexture(5, swrtBuffer.fetchID(5), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
	
	raymarchCompShader.setTexture2D("gVelocity", 6, GeometryPass::gVelocity);
    raymarchCompShader.setTexture2D("gPosition", 7, GeometryPass::gPosition);
	// shadow map at 8
    raymarchCompShader.setTexture2D("gNormal",             9,                GeometryPass::gNormal);
    raymarchCompShader.setTexture2D("gAlbedoSpec",      10,              GeometryPass::gAlbedo);
    raymarchCompShader.setTexture2D("depthMap",          11,              GeometryPass::depthTexture);
    raymarchCompShader.setTexture2D("gSpecular",           12,              GeometryPass::gSpecular);
    raymarchCompShader.setTexture2D("gEmission",           13,              GeometryPass::gEmission);
	raymarchCompShader.setTexture2D("hIndirect",              15,   historyBuffer.fetchID(0));
	raymarchCompShader.setTexture2D("hEmission",             16,  historyBuffer.fetchID(1));
	raymarchCompShader.setTexture2D("swrtHDepth",        14,    historyBuffer.fetchID(3));
	//raymarchCompShader.setTexture2D("presentImage",        19, denoiseBuffer.fetchID(0));
	
	
	raymarchCompShader.setFloat("NearPlane", Scene::maincamera.nearFar.x);
	raymarchCompShader.setFloat("FarPlane", Scene::maincamera.nearFar.y);
	
	raymarchCompShader.setBool("doTemporalAccumulation", doTemporalAccumulation);
	raymarchCompShader.setFloat("temporalAccumulationBlendFactor", temporalAccumulationBlendFactor);
	
    raymarchCompShader.setFloat2("screenSize", glm::vec2(w, h));
    raymarchCompShader.setMat4("u_ViewMatrix", Scene::maincamera.view);
    //raymarchShader.setMat4("u_ProjectionMatrix", Scene::maincamera.projectionAlwaysUnjittered);
    raymarchCompShader.setMat4("u_ProjectionMatrix", Scene::maincamera.projection);
    raymarchCompShader.setFloat3("cameraPosition", Scene::maincamera.Position);
    //raymarchShader.setFloat3("skycolour", skyRGBA);
    raymarchCompShader.setHandleui64ARB("BlueNoiseHandle", RenderClass::bluenoise->handle);
    Skybox::SkyboxCubemap->cubemapToUUIDShader("cmMainHandle", raymarchCompShader);
	
	raymarchCompShader.setFloat3("sceneBoundPos", Scene::SceneBounds.position);
	raymarchCompShader.setFloat3("sceneBoundScale", Scene::SceneBounds.size);
	
    raymarchCompShader.setTimeVariables();
	
    LightingHandler::sendToShader(raymarchCompShader);
	
	glDispatchCompute((w + 7) / 8, (h + 3) / 4, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void FlouraSWRT::combineGatherPass(){
	gatherCombinePassShader.Activate();
	gatherCombinePassShader.setTexture2D("swrtIndirect",                 0,    swrtBuffer.fetchID(0));
	gatherCombinePassShader.setTexture2D("swrtEmission",               2,    swrtBuffer.fetchID(2));
	gatherCombinePassShader.setTexture2D("depthMap",5,    GeometryPass::depthTexture);
	gatherCombinePassShader.setTexture2D("gPosition",7,    GeometryPass::gPosition);
	gatherCombinePassShader.setBool("vxirc_DoScreenGather", vxirc_DoScreenGather);
	
	// vx irc
	glBindImageTexture(0, ring0IRC->ID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F); // GL_RGBA16F
	glBindImageTexture(1, ring1IRC->ID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F); // GL_R11F_G11F_B10F
	glBindImageTexture(2, ring2IRC->ID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
	glBindImageTexture(3, ring3IRC->ID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
	
	glBindImageTexture(4, ring0ERC->ID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F); // GL_RGBA16F
	glBindImageTexture(5, ring1ERC->ID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F); // GL_R11F_G11F_B10F
	glBindImageTexture(6, ring2ERC->ID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
	glBindImageTexture(7, ring3ERC->ID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
	
	RenderQuad::draw();
	
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void FlouraSWRT::denoisePrePass(){
	glBindFramebuffer(GL_FRAMEBUFFER, historyBuffer.FBO);
	
	denoisePrePassShader.Activate();
	denoisePrePassShader.setTexture2D("swrtIndirect",                 0,    swrtBuffer.fetchID(0));
	denoisePrePassShader.setTexture2D("swrtSpecular",               1,    swrtBuffer.fetchID(1));
	denoisePrePassShader.setTexture2D("swrtEmission",               2,    swrtBuffer.fetchID(2));
	denoisePrePassShader.setTexture2D("swrtAO", 4, swrtBuffer.fetchID(3));
	denoisePrePassShader.setTexture2D("depthMap",5,    GeometryPass::depthTexture);
	denoisePrePassShader.setTexture2D("gNormal",6,    GeometryPass::gNormal);
	denoisePrePassShader.setTexture2D("gPosition",7,    GeometryPass::gPosition);

	denoisePrePassShader.setBool("doDenoise", doSVGF);
	denoisePrePassShader.setFloat("NearPlane", Scene::maincamera.nearFar.x);
	denoisePrePassShader.setFloat("FarPlane", Scene::maincamera.nearFar.y);
	 
	RenderQuad::draw();
}

void FlouraSWRT::denoisePass(){
	int passIndex = 0;// usually is 5
	//const int passNum = 5;
	//const int passRadius[5] = {4, 8, 16, 32, 64};
	const int passRadius[5] = {2, 4, 8, 16, 32};
	//const int passRadius[5] = {1, 2, 4, 8, 16};
	for (int i = 0; i < passNum; ++i){
		if (passIndex != 0) pingPongPass();

		glBindFramebuffer(GL_FRAMEBUFFER, denoiseBuffer.FBO);
		denoiseShader.Activate();
		
		denoiseShader.setInt("passIndex",passIndex);
		denoiseShader.setInt("passNum",passNum);
		denoiseShader.setInt("passRadius", passRadius[i]);
		
		denoiseShader.setBool("doDenoise", doSVGF);
		denoiseShader.setFloat("NearPlane", Scene::maincamera.nearFar.x);
		denoiseShader.setFloat("FarPlane", Scene::maincamera.nearFar.y);
		
		denoiseShader.setTexture2D("swrtSpecular",              1,     swrtBuffer.fetchID(1));
		//denoiseShader.setTexture2D("swrtDbgColour",          2,     swrtBuffer.fetchID(5));
		denoiseShader.setTexture2D("gNormal",                    4,     GeometryPass::gNormal);
		denoiseShader.setTexture2D("depthMap",                  5,    GeometryPass::depthTexture);
		denoiseShader.setTexture2D("swrtIndirect",                6,    swrtBuffer.fetchID(0));
		denoiseShader.setTexture2D("swrtEmission",               7,   swrtBuffer.fetchID(2));
		denoiseShader.setTexture2D("filteredVariance",           10, historyBuffer.fetchID(4));
		denoiseShader.setTexture2D("gPosition",                     12, GeometryPass::gPosition);
	
		RenderQuad::draw();
		passIndex++;
	}
}

void FlouraSWRT::pingPongPass(){
	glBindFramebuffer(GL_FRAMEBUFFER, swrtBuffer.FBO);
	
	pingPongShader.Activate();
	pingPongShader.setTexture2D("dIndirect",   0, denoiseBuffer.fetchID(0));
	pingPongShader.setTexture2D("dEmission", 1, denoiseBuffer.fetchID(1));
	
	RenderQuad::draw();
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void FlouraSWRT::presentPass(unsigned int w, unsigned int h){
	glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::FBO);
	
	presentShader.Activate();
	presentShader.setTimeVariables();
	presentShader.setTexture2D("gEmission",0, GeometryPass::gEmission);
	presentShader.setTexture2D("swrtSpecular",               1,    swrtBuffer.fetchID(1));
	presentShader.setTexture2D("depthMap",2,  GeometryPass::depthTexture);
	presentShader.setTexture2D("gAlbedoSpec",3,    GeometryPass::gAlbedo);
	presentShader.setTexture2D("gNormal", 4,     GeometryPass::gNormal);
	presentShader.setTexture2D("gSpecular",5, GeometryPass::gSpecular);
	presentShader.setTexture2D("gPosition",6, GeometryPass::gPosition);
	presentShader.setTexture2D("dIndirect",   7, denoiseBuffer.fetchID(0));
	// 8 is shadow map ^
	presentShader.setTexture2D("dEmission", 9, denoiseBuffer.fetchID(1));
	//presentShader.setTexture2D("swrtAO", 10, swrtBuffer.fetchID(3));
	//
	presentShader.setFloat2("screenSize", glm::vec2(renderTarget::ViewPortWidth, renderTarget::ViewPortHeight));
	presentShader.setFloat2("scaledScreenSize", glm::vec2(w, h));
	presentShader.setFloat3("cameraPosition", Scene::maincamera.Position);
	presentShader.setHandleui64ARB("BlueNoiseHandle", RenderClass::bluenoise->handle);
	LightingHandler::sendToShader(presentShader);
	
	RenderQuad::draw();
}


GLuint FlouraSWRT::localSDF_SSBOID;
GLuint FlouraSWRT::GDF_SSBOID;

void FlouraSWRT::initSWRTssbo(){
	glGenBuffers(1, &localSDF_SSBOID);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, localSDF_SSBOID);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 1024, NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, localSDF_SSBOID); // 6
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind
	
	glGenBuffers(1, &GDF_SSBOID);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, GDF_SSBOID);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 1024, NULL, GDF_SSBOID);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, GDF_SSBOID); // 6
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind
}

void FlouraSWRT::cleanupSWRTssbo(){
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glDeleteBuffers(1, &localSDF_SSBOID);
	
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glDeleteBuffers(1, &GDF_SSBOID);
}

void FlouraSWRT::updateSDFBuffer(){
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, localSDF_SSBOID);
	glBufferData(GL_SHADER_STORAGE_BUFFER, MDFS.size() * sizeof(MDF), MDFS.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, localSDF_SSBOID);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void FlouraSWRT::updateGDFBuffer(){
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, GDF_SSBOID);
	glBufferData(GL_SHADER_STORAGE_BUFFER, clips.size() * sizeof(accelerate::clipmapLevel), clips.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, GDF_SSBOID);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void FlouraSWRT::wipeScene(){
	for (size_t x = 0; x <MDFS.size();)
		MDFS.erase(MDFS.begin() + x);
	updateSDFBuffer();
}

void FlouraSWRT::uploadToLSDFScene(uint64_t instanceUUID){
	//return;
	//for (size_t x = 0; x <localSDFS.size(); x++)
	//    if (localSDFS[x].instanceUUID == instanceUUID)
	//        break;
    
	uint64_t RenderUUID = RenderHandler::findRenderUUIDwIstanceUUID(instanceUUID);
	int ind = RenderHandler::fetchModelIndex(RenderUUID);
    
	if (!RenderHandler::models[ind].model->sdfCompatible) return;
    //		RenderHandler::models[ind].model->meshSDFs.size() != RenderHandler::models[ind].model->meshes.size()
	// SDF meshes
	//for (int i = 0; i <  RenderHandler::models[ind].model->meshes.size(); ++i){
	
	if (RenderHandler::models[ind].model->mdfFormation == 1){
		std::cout << "MDF size: " << RenderHandler::models[ind].model->meshSDFs.size() << std::endl; 
	}
	
	for (int i = 0; i <  RenderHandler::models[ind].model->meshSDFs.size(); ++i){
		MDF nLSDF;
		nLSDF.instanceUUID = instanceUUID;
		nLSDF.SDF_Handle = RenderHandler::models[ind].model->meshSDFs[i]->handle;
		nLSDF.UV_Handle = RenderHandler::models[ind].model->meshSDFs2[i]->handle;
		//nLSDF.EM_Handle = RenderHandler::models[ind].model->meshSDFs3[i]->handle;
        
		// could this be the issue
		Collision::AABB ab;
		//std::cout << "formation: " << RenderHandler::models[ind].model->mdfFormation << std::endl;
		
		switch (RenderHandler::models[ind].model->mdfFormation){
		case 0:
			ab = Collision::rootNodeFromRubixPoints( RenderHandler::models[ind].model->meshAabbPoints[i], RenderHandler::models[ind].model->lModelMatrix[i]); // local
			break;
		case 1:
			ab = RenderHandler::models[ind].model->mdfNodes[i];
			break;
		default:
			ab = Collision::rootNodeFromRubixPoints( RenderHandler::models[ind].model->meshAabbPoints[i], RenderHandler::models[ind].model->lModelMatrix[i]); // local
			break;
		}
        
		nLSDF.position = glm::vec4(ab.position, 1.0f);
		nLSDF.extents = glm::vec4(ab.size, 1.0f);
		nLSDF.globalTransform = glm::mat4(1.0f);
		
		/**/
		for (int z = 0; z < RenderHandler::models[ind].model->meshes[i].textures.size(); ++z){
			std::string type = RenderHandler::models[ind].model->meshes[i].textures[z].type;
			if (type == "texture_diffuse")
				nLSDF.ALB_Handle = RenderHandler::models[ind].model->meshes[i].textures[z].handle;
			else if (type == "texture_roughness")
				nLSDF.ARM_Handle = RenderHandler::models[ind].model->meshes[i].textures[z].handle;
			else if (type == "texture_emission"){
				nLSDF.EM_Handle = RenderHandler::models[ind].model->meshes[i].textures[z].handle;
				nLSDF.extraFactors.r = RenderHandler::models[ind].model->meshes[i].textures[z].powerFactor;
			}
		}
		
		
		MDFS.push_back(nLSDF);
	}
	updateSDFBuffer();
}

void FlouraSWRT::removeFromLSDFScene(uint64_t instanceUUID){
	for (size_t x = 0; x <MDFS.size();)
		if (MDFS[x].instanceUUID == instanceUUID){
			MDFS.erase(MDFS.begin() + x);
			//break;
		}
		else{ x++; }
	updateSDFBuffer();
}
/**/
void FlouraSWRT::updateUVscale(uint64_t instanceUUID, glm::vec2& scale){
	for (size_t x = 0; x <MDFS.size(); x++)
		if (MDFS[x].instanceUUID == instanceUUID){
			// putting in w component for padding and to save space
			MDFS[x].position.w = scale.x;
			MDFS[x].extents.w = scale.y;
			//break;
		}

	updateSDFBuffer();
}

void FlouraSWRT::updateGlobalTransformation(uint64_t instanceUUID, glm::mat4& gt, glm::vec3 gRotation){
	for (size_t x = 0; x <MDFS.size(); x++)
		if (MDFS[x].instanceUUID == instanceUUID){
			
			glm::mat4 localMatrix = FE_Math::composeMatrixWDegrees(MDFS[x].position, MDFS[x].extents, glm::vec3(0.0f));
			
			glm::mat4 totalMatrix = gt * localMatrix;
			
			// trs extraction (just ts here)
			MDFS[x].gPosition = glm::vec4(glm::vec3(totalMatrix[3]),1.0f);
			glm::vec3 ns = glm::vec3(0.0f);
			ns.x = glm::length(glm::vec3(totalMatrix[0]));
			ns.y = glm::length(glm::vec3(totalMatrix[1]));
			ns.z = glm::length(glm::vec3(totalMatrix[2]));
			MDFS[x].gExtents = glm::vec4(glm::vec3(ns), 1.0f);
			glm::mat4 gRotationMatrix = FE_Math::composeMatrixWDegrees(glm::vec3(0.0), glm::vec3(1.0f), gRotation);
			MDFS[x].globalTransform = glm::inverse(gRotationMatrix);
			
			Collision::AABB nRoot = Collision::createAABBfromRubiksCubePoints(Collision::transformRubiks(Collision::aabbToRubixCubePoints(MDFS[x].position, MDFS[x].extents), gt));
			MDFS[x].rootPosition = glm::vec4(nRoot.position,  1.0);
			MDFS[x].rootExtents = glm::vec4(nRoot.size, 1.0);
		}
	
	updateSDFBuffer();
}

float FlouraSWRT::clipAccum = 0.0;
float FlouraSWRT::clipAccumthresh =  1.0 / 8.0f;

void FlouraSWRT::update(){
	clipAccum += TimeUtil::deltatime;
	if (clipAccum < clipAccumthresh) return;
	accelerate::transformClips(Scene::maincamera.Position, clips);
	clipAccum = 0.0;
}

#include <Render/Handler/CubeVisualizer.h>
void FlouraSWRT::MDFdebugDraw(){
	for (int i = 0; i < MDFS.size(); ++i){
		CubeVisualizer::draw(MDFS[i].rootPosition, MDFS[i].rootExtents, glm::vec3(1.0, 0.0, 0.0),
			3.0, true, false);
	}
}

void FlouraSWRT::GDFdebugDraw(){
	for (int i = 0; i < clips.size(); ++i){
		CubeVisualizer::draw(clips[i].ps, glm::vec3(clips[i].ps.w), glm::vec3(clips[i].ps.w / 200.0, 0.5, 0.5),
			5.5, true, false);
	}
}

bool FlouraSWRT::dirtyGDF = true;
float FlouraSWRT::gdfAccum = 0.0;
float FlouraSWRT::gdfAccumthresh =  1.0f / 12.0f;

void FlouraSWRT::GDFdraw(){
	gdfAccum += TimeUtil::deltatime;
	
	if (gdfAccum < gdfAccumthresh) return;
	
	int successfulDraws = 0;
	for (int i = 0; i < clips.size(); ++i){
		//std::cout << raytracer::RTGlobalTransformFlag << std::endl;
		if (clips[i].dirty <1 && !dirtyGDF) continue;
		
		if (clips[i].dirty > 0){
			GLfloat clearColour[4] = {0.0f, 0.0f ,0.0f, 0.0f};
			glClearTexImage(gdfINFO[i].ID5, 0, GL_RGBA, GL_FLOAT, clearColour);
			glClearTexImage(gdfINFO[i].ID6, 0, GL_RGBA, GL_FLOAT, clearColour);
			isCacheFreshlyWiped = true;
		}
		
		glBindImageTexture(0, gdfINFO[i].ID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F); // GL_RGBA16F GL_R8_SNORM
		glBindImageTexture(1, gdfINFO[i].ID2, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
		glBindImageTexture(2, gdfINFO[i].ID3, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R11F_G11F_B10F);
		glBindImageTexture(3, gdfINFO[i].ID4, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG8);
		//
		
		GDFdrawShader.Activate();
		GDFdrawShader.setInt("GDFIndex", i);
		GDFdrawShader.setInt3("volumeSize", gdfINFO[i].resolution, gdfINFO[i].resolution, gdfINFO[i].resolution);
		GDFdrawShader.setFloat4("positionScale", clips[i].ps);
		
		glDispatchCompute((gdfINFO[i].resolution + 7) / 8, (gdfINFO[i].resolution + 7) / 8, ((gdfINFO[i].resolution + 7) / 8));
		//glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		
		// could call a probe light here??? maybe even store some dirty flag
		
		//std::cout << "gdf draw: " << i << std::endl;	
		// set clean
		clips[i].dirty = 0;
		successfulDraws++;
	}
	// some logic here to update the buffers
	if (successfulDraws > 0) updateGDFBuffer();
	
	successfulDraws = 0;
	dirtyGDF = false;
	
	gdfAccum = 0.0;
}

float FlouraSWRT::cacheAccum = 0.0;
float FlouraSWRT::cacheAccumthresh = 30.0f;
bool FlouraSWRT::isCacheFreshlyWiped = false;

void FlouraSWRT::worldSpaceCacheGather(){
	cacheAccum += TimeUtil::deltatime;
	if (cacheAccum < 1.0 / cacheAccumthresh && !isCacheFreshlyWiped || !vxirc_DoWorldGather) return;
	
	for (int i = 0; i < clips.size(); ++i){
		indirectProbePass(gdfINFO[i].ID5, gdfINFO[i].ID6, 
			gdfINFO[i].vxResolution, gdfINFO[i].vxResolution, gdfINFO[i].vxResolution,
			clips[i].ps);
	}
	cacheAccum = 0.0f;
	isCacheFreshlyWiped = false;
}


