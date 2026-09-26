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
Shader FlouraSWRT::denoisePrePassShader;
Shader FlouraSWRT::denoiseShader;
Shader FlouraSWRT::pingPongShader;
//Shader FlouraSWRT::pingPongCompShader;
Shader FlouraSWRT::presentShader;
Shader FlouraSWRT::GDFdrawShader;

FlouraSWRT::probeChunk FlouraSWRT::testChunk;
int FlouraSWRT::localPerModelMeshCountCap = 4000; // 10
std::vector<FlouraSWRT::localSDF> FlouraSWRT::localSDFS;

Texture3D* FlouraSWRT::ring0; 
Texture3D* FlouraSWRT::ring1; 
Texture3D* FlouraSWRT::ring2; 
Texture3D* FlouraSWRT::ring3;
Texture3D* FlouraSWRT::ring0A; 
Texture3D* FlouraSWRT::ring1A; 
Texture3D* FlouraSWRT::ring2A; 
Texture3D* FlouraSWRT::ring3A;
std::vector<accelerate::clipmapLevel> FlouraSWRT::clips;
std::vector<FlouraSWRT::gdfDrawbatch> FlouraSWRT::gdfINFO;

bool FlouraSWRT::doTemporalAccumulation = true;
bool FlouraSWRT::doSVGF = true;
bool FlouraSWRT::doDenoiseSplitDBGView;
float FlouraSWRT::temporalAccumulationBlendFactor = 0.90f;
float FlouraSWRT::resScaleFactor = 0.75;
int FlouraSWRT::passNum = 5;
int FlouraSWRT::autoMDFres = 64;
double FlouraSWRT::rmTime = 0.0f;

void FlouraSWRT::initShaders(){
	GDFdrawShader.LoadComputeShader("Assets/Shaders/compute/gdfDraw.comp");
    raymarchRQShader.LoadShader("Assets/Shaders/raymarched/raymarch.vert", "Assets/Shaders/raymarched/traceSWRT.frag");
	indirectProbeShader.LoadComputeShader("Assets/Shaders/raymarched/indirectProbeSWRT.comp");
	raymarchCompShader.LoadComputeShader("Assets/Shaders/raymarched/traceSWRT.comp");
	denoisePrePassShader.LoadShader("Assets/Shaders/raymarched/raymarch.vert", "Assets/Shaders/raymarched/denoisePrePassSWRT.frag");
	denoiseShader.LoadShader("Assets/Shaders/raymarched/raymarch.vert", "Assets/Shaders/raymarched/denoiseSWRT.frag");
	pingPongShader.LoadShader("Assets/Shaders/raymarched/raymarch.vert", "Assets/Shaders/raymarched/pingpongSWRT.frag");
	//pingPongCompShader.LoadComputeShader("Assets/Shaders/raymarched/pingpongSWRT.comp");
	presentShader.LoadShader("Assets/Shaders/raymarched/raymarch.vert", "Assets/Shaders/raymarched/presentSWRT.frag");
}

void FlouraSWRT::cleanupShaders(){
	GDFdrawShader.Delete();
	raymarchRQShader.Delete();
	indirectProbeShader.Delete();
	raymarchCompShader.Delete();
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
	swrtBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	//emission 2
	swrtBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	//indirectSpecular 3
	swrtBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	//emissionSpecular 4
	swrtBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	//direct 5
	//swrtBuffer.uploadAttachment(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	
	swrtBuffer.createBuffers(width, height);
	
	// hIndirect 0
	historyBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	// hEmission 1
	historyBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	// hIndirectSpecular 2
	historyBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	// hEmissionSpecular 3
	historyBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	// hSpecular 4
	historyBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	// swrtHDepth 5
	historyBuffer.uploadAttachment(GL_R32F, GL_RED, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	// filteredVariance 6
	historyBuffer.uploadAttachment(GL_RG16F, GL_RGBA, GL_FLOAT, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	
	historyBuffer.createBuffers(width, height);
	
	// present image 0
	denoiseBuffer.uploadAttachment(GL_RGB16F, GL_RGB, GL_FLOAT, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
	// dIndirect 1
	denoiseBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
	// dEmissions 2
	denoiseBuffer.uploadAttachment(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
	
	denoiseBuffer.createBuffers(width, height);
	
	const int r0r = 128;
	const int r1r = 128;
	const int r2r = 128;
	const int r3r = 128;
	
	ring0 = new Texture3D(); ring0->createImage3D(r0r, r0r, r0r, "clipmap", 0, GL_RGBA16F); // GL_RGBA32F
	ring1 = new Texture3D(); ring1->createImage3D(r1r, r1r, r1r, "clipmap", 0, GL_RGBA16F); // GL_R8_SNORM
	ring2 = new Texture3D(); ring2->createImage3D(r2r, r2r, r2r, "clipmap", 0, GL_RGBA16F); // GL_RGBA16F
	ring3 = new Texture3D(); ring3->createImage3D(r3r, r3r, r3r, "clipmap", 0, GL_RGBA16F);
	ring0A = new Texture3D(); ring0A->createImage3D(r0r, r0r, r0r, "clipmap", 0, GL_RGBA8);
	ring1A = new Texture3D(); ring1A->createImage3D(r1r, r1r, r1r, "clipmap", 0, GL_RGBA8);
	ring2A = new Texture3D(); ring2A->createImage3D(r2r, r2r, r2r, "clipmap", 0, GL_RGBA8);
	ring3A = new Texture3D(); ring3A->createImage3D(r3r, r3r, r3r, "clipmap", 0, GL_RGBA8);
	
	gdfDrawbatch nGDB;
	nGDB.resolution = ring0->width; nGDB.ID = ring0->ID; nGDB.ID2 = ring0A->ID;
	gdfINFO.push_back(nGDB);
	nGDB.resolution = ring1->width; nGDB.ID = ring1->ID; nGDB.ID2 = ring1A->ID;
	gdfINFO.push_back(nGDB);
	nGDB.resolution = ring2->width; nGDB.ID = ring2->ID; nGDB.ID2 = ring2A->ID;
	gdfINFO.push_back(nGDB);
	nGDB.resolution = ring3->width; nGDB.ID = ring3->ID; nGDB.ID2 = ring3A->ID;
	gdfINFO.push_back(nGDB);
	
	accelerate::clipmapLevel clip;
	clip.dirty = 1;
	
	clip.handle = ring0->handle;
	clip.handle2 = ring0A->handle;
	clip.ps = glm::vec4(0.0,0.0,0.0,20.0); // 256
	clip.thickness = 0.0;
	clips.push_back(clip);
	
	clip.handle = ring1->handle;
	clip.handle2 = ring1A->handle;
	clip.ps = glm::vec4(0.0,0.0,0.0,30.0); // 128
	clip.thickness = 0.05;
	clips.push_back(clip);
	
	/**/
	clip.handle = ring2->handle;
	clip.handle2 = ring2A->handle;
	clip.ps = glm::vec4(0.0,0.0,0.0,50.0); // 64
	clip.thickness = 0.1;
	clips.push_back(clip);
	
	clip.handle = ring3->handle;
	clip.handle2 = ring3A->handle;
	clip.ps = glm::vec4(0.0,0.0,0.0,100.0); // 32
	clip.thickness = 0.15;
	clips.push_back(clip);
	
	/*
	testChunk.indirectVolume = new Texture3D();
	testChunk.emissionVolume = new Texture3D();
	testChunk.indirectVolume->createImage3D(128,128,128, "probe", 1, GL_RGBA16F);
	testChunk.emissionVolume->createImage3D(128,128,128, "probe", 1, GL_RGBA16F);
	testChunk.scale = 70.0f;
	*/
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
	
	//testChunk.indirectVolume->Delete();
	//testChunk.emissionVolume->Delete();
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
	
	raymarchRQShader.setTexture2D("swrtHDepth",          11,  historyBuffer.fetchID(5));
	raymarchRQShader.setTexture2D("hIndirect",               12,  historyBuffer.fetchID(0));
	raymarchRQShader.setTexture2D("hEmission",             14,  historyBuffer.fetchID(1));
	raymarchRQShader.setTexture2D("hIndirectSpecular",   15, historyBuffer.fetchID(2));
	raymarchRQShader.setTexture2D("hEmissionSpecular", 16, historyBuffer.fetchID(3));
	raymarchRQShader.setTexture2D("hSpecular",               17,historyBuffer.fetchID(4));
	raymarchRQShader.setTexture2D("presentImage",         18,denoiseBuffer.fetchID(0));
	
	raymarchRQShader.setFloat("NearPlane", Scene::maincamera.nearFar.x);
	raymarchRQShader.setFloat("FarPlane", Scene::maincamera.nearFar.y);
	raymarchRQShader.setBool("doTemporalAccumulation", doTemporalAccumulation);
	raymarchRQShader.setFloat("temporalAccumulationBlendFactor", temporalAccumulationBlendFactor);
	raymarchRQShader.setBool("doDenoiseSplitDBGView", doDenoiseSplitDBGView);
	
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
	
    RenderQuad::draw();
}

// max allocation size there should be
void FlouraSWRT::prepChunks(){
	testChunk.lposition = testChunk.position; // set the history
	glm::vec3 gridPos = FE_Math::snapToGrid(Scene::maincamera.Position, testChunk.scale);
	testChunk.position = gridPos;
	
	if (testChunk.lposition != testChunk.position) testChunk.dirty = true;
	else testChunk.dirty = false;
}

float FlouraSWRT::pAccum = 0.0;
float FlouraSWRT::pAccumthresh = 1.0 / 1.0f;
//float FlouraSWRT::pAccumthresh = 1.0 / 24.0f;

//#include "utils/imageWrite.h"

void FlouraSWRT::indirectProbePass(){
	// shadows should probably update first
	pAccum += TimeUtil::deltatime;
	
	// reflection draw
	if (pAccum > pAccumthresh){
		prepChunks();
		
		indirectProbeShader.Activate();
		//glBindImageTexture(1, testVolume->ID, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
		glBindImageTexture(0, testChunk.indirectVolume->ID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
		glBindImageTexture(1, testChunk.emissionVolume->ID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
		
		LightingHandler::sendToShader(indirectProbeShader);
		indirectProbeShader.setTimeVariables();
		indirectProbeShader.setInt3("volumeSize", testChunk.indirectVolume->width,testChunk.indirectVolume->height,testChunk.indirectVolume->depth);
		
		indirectProbeShader.setFloat("NearPlane", Scene::maincamera.nearFar.x);
		indirectProbeShader.setFloat("FarPlane", Scene::maincamera.nearFar.y);
		indirectProbeShader.setFloat3("cameraPosition", Scene::maincamera.Position);
		Skybox::SkyboxCubemap->cubemapToUUIDShader("cmMainHandle", indirectProbeShader);
		indirectProbeShader.setFloat3("sceneBoundPos", Scene::SceneBounds.position);
		indirectProbeShader.setFloat3("sceneBoundScale", Scene::SceneBounds.size);
		
		// temp
		indirectProbeShader.setFloat3("ivPosition", testChunk.position);
		indirectProbeShader.setFloat("ivScale", testChunk.scale);
		indirectProbeShader.setBool("ivDirty", testChunk.dirty);
		
		glDispatchCompute((testChunk.indirectVolume->width + 7) / 8, (testChunk.indirectVolume->height + 7) / 8, ((testChunk.indirectVolume->depth + 7) / 8));
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	
		pAccum = 0.0;
		
		// temp
		/*
		FlouraImageWrite::writeImage3DToDiskPNG(testChunk.indirectVolume->ID,
		 128,128,128,
		"Cache/lightmap_128.png", GL_RGB, GL_UNSIGNED_BYTE, 3); // GB
		*/
	}
	
}

void FlouraSWRT::rmPassCompute(unsigned int w, unsigned int h){
	raymarchCompShader.Activate();
	
	glBindImageTexture(0, swrtBuffer.fetchID(0), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glBindImageTexture(1, swrtBuffer.fetchID(1), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
	glBindImageTexture(2, swrtBuffer.fetchID(2), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glBindImageTexture(3, swrtBuffer.fetchID(3), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	glBindImageTexture(4, swrtBuffer.fetchID(4), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
	//glBindImageTexture(5, swrtBuffer.fetchID(5), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
	
	raymarchCompShader.setTexture2D("gVelocity", 6, GeometryPass::gVelocity);
    raymarchCompShader.setTexture2D("gPosition", 7, GeometryPass::gPosition);
	// shadow map at 8
    raymarchCompShader.setTexture2D("gNormal",             9,                GeometryPass::gNormal);
    raymarchCompShader.setTexture2D("gAlbedoSpec",      10,              GeometryPass::gAlbedo);
    raymarchCompShader.setTexture2D("depthMap",          11,              GeometryPass::depthTexture);
    raymarchCompShader.setTexture2D("gSpecular",           12,              GeometryPass::gSpecular);
    raymarchCompShader.setTexture2D("gEmission",           13,              GeometryPass::gEmission);
	raymarchCompShader.setTexture2D("swrtHDepth",        14,    historyBuffer.fetchID(5));
	raymarchCompShader.setTexture2D("hIndirect",              15,   historyBuffer.fetchID(0));
	raymarchCompShader.setTexture2D("hEmission",             16,  historyBuffer.fetchID(1));
	raymarchCompShader.setTexture2D("hIndirectSpecular",  17,  historyBuffer.fetchID(2));
	raymarchCompShader.setTexture2D("hEmissionSpecular", 18, historyBuffer.fetchID(3));
	raymarchCompShader.setTexture2D("presentImage",        19, denoiseBuffer.fetchID(0));
	
	
	raymarchCompShader.setFloat("NearPlane", Scene::maincamera.nearFar.x);
	raymarchCompShader.setFloat("FarPlane", Scene::maincamera.nearFar.y);
	
	raymarchCompShader.setBool("doTemporalAccumulation", doTemporalAccumulation);
	raymarchCompShader.setFloat("temporalAccumulationBlendFactor", temporalAccumulationBlendFactor);
	raymarchCompShader.setBool("doDenoiseSplitDBGView", doDenoiseSplitDBGView);
	
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

void FlouraSWRT::denoisePrePass(){
	glBindFramebuffer(GL_FRAMEBUFFER, historyBuffer.FBO);
	
	denoisePrePassShader.Activate();
	denoisePrePassShader.setTexture2D("depthMap",                   0,    GeometryPass::depthTexture);
	denoisePrePassShader.setTexture2D("gNormal",                      1,    GeometryPass::gNormal);
	denoisePrePassShader.setTexture2D("swrtIndirect",                 2,    swrtBuffer.fetchID(0));
	denoisePrePassShader.setTexture2D("swrtEmission",               3,    swrtBuffer.fetchID(2));
	denoisePrePassShader.setTexture2D("swrtIndirectSpecular",    4,    swrtBuffer.fetchID(3));
	denoisePrePassShader.setTexture2D("swrtEmissionSpecular",  5,    swrtBuffer.fetchID(4));
	denoisePrePassShader.setTexture2D("swrtSpecular",               6,    swrtBuffer.fetchID(1));
	
	denoisePrePassShader.setBool("doDenoise", doSVGF);
	denoisePrePassShader.setBool("doDenoiseSplitDBGView", doDenoiseSplitDBGView);
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
		denoiseShader.setBool("doDenoiseSplitDBGView", doDenoiseSplitDBGView);
		denoiseShader.setFloat("NearPlane", Scene::maincamera.nearFar.x);
		denoiseShader.setFloat("FarPlane", Scene::maincamera.nearFar.y);
		
		denoiseShader.setTexture2D("swrtSpecular",              1,     swrtBuffer.fetchID(1));
		//denoiseShader.setTexture2D("swrtDbgColour",          2,     swrtBuffer.fetchID(5));
		denoiseShader.setTexture2D("gEmission",                  3,     GeometryPass::gEmission);
		denoiseShader.setTexture2D("gNormal",                    4,     GeometryPass::gNormal);
		denoiseShader.setTexture2D("depthMap",                  5,    GeometryPass::depthTexture);
		denoiseShader.setTexture2D("swrtIndirect",                6,    swrtBuffer.fetchID(0));
		denoiseShader.setTexture2D("swrtEmission",               7,   swrtBuffer.fetchID(2));
		denoiseShader.setTexture2D("swrtIndirectSpecular",    8,   swrtBuffer.fetchID(3));
		denoiseShader.setTexture2D("swrtEmissionSpecular",  9,   swrtBuffer.fetchID(4));
		denoiseShader.setTexture2D("filteredVariance",           10, historyBuffer.fetchID(6));
		denoiseShader.setTexture2D("gSpecular",                    11, GeometryPass::gSpecular);
		denoiseShader.setTexture2D("gPosition",                     12, GeometryPass::gPosition);
	
		RenderQuad::draw();
		passIndex++;
	}
}

void FlouraSWRT::pingPongPass(){
	glBindFramebuffer(GL_FRAMEBUFFER, swrtBuffer.FBO);
	
	pingPongShader.Activate();
	pingPongShader.setTexture2D("dIndirect",   0, denoiseBuffer.fetchID(1));
	pingPongShader.setTexture2D("dEmission", 1, denoiseBuffer.fetchID(2));

	RenderQuad::draw();
}

void FlouraSWRT::presentPass(unsigned int w, unsigned int h){
	glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::FBO);
	
	presentShader.Activate();
	presentShader.setTimeVariables();
	presentShader.setTexture2D("presentImage",1,  denoiseBuffer.fetchID(0));
	presentShader.setTexture2D("depthMap",2,  GeometryPass::depthTexture);
	presentShader.setTexture2D("gAlbedoSpec",3,    GeometryPass::gAlbedo);
	presentShader.setTexture2D("gNormal", 4,     GeometryPass::gNormal);
	presentShader.setTexture2D("gSpecular",5, GeometryPass::gSpecular);
	presentShader.setTexture2D("gPosition",6, GeometryPass::gPosition);
	presentShader.setTexture2D("dIndirect",   7, denoiseBuffer.fetchID(1));
	// 8 is shadow map ^
	presentShader.setTexture2D("dEmission", 9, denoiseBuffer.fetchID(2));
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
	glBufferData(GL_SHADER_STORAGE_BUFFER, localSDFS.size() * sizeof(localSDF), localSDFS.data(), GL_DYNAMIC_DRAW);
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
	for (size_t x = 0; x <localSDFS.size();)
		localSDFS.erase(localSDFS.begin() + x);
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
		localSDF nLSDF;
		nLSDF.instanceUUID = instanceUUID;
		nLSDF.SDF_Handle = RenderHandler::models[ind].model->meshSDFs[i]->handle;
		nLSDF.ALB_Handle = RenderHandler::models[ind].model->meshSDFs2[i]->handle;
        
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
		localSDFS.push_back(nLSDF);
	}
	updateSDFBuffer();
}

void FlouraSWRT::removeFromLSDFScene(uint64_t instanceUUID){
	for (size_t x = 0; x <localSDFS.size();)
		if (localSDFS[x].instanceUUID == instanceUUID){
			localSDFS.erase(localSDFS.begin() + x);
			//break;
		}
		else{ x++; }
	updateSDFBuffer();
}

void FlouraSWRT::updateGlobalTransformation(uint64_t instanceUUID, glm::mat4& gt, glm::vec3 gRotation){
	for (size_t x = 0; x <localSDFS.size(); x++)
		if (localSDFS[x].instanceUUID == instanceUUID){
			
			glm::mat4 localMatrix = FE_Math::composeMatrixWDegrees(localSDFS[x].position, localSDFS[x].extents, glm::vec3(0.0f));
			
			glm::mat4 totalMatrix = gt * localMatrix;
			
			// trs extraction (just ts here)
			localSDFS[x].gPosition = glm::vec4(glm::vec3(totalMatrix[3]),1.0f);
			glm::vec3 ns = glm::vec3(0.0f);
			ns.x = glm::length(glm::vec3(totalMatrix[0]));
			ns.y = glm::length(glm::vec3(totalMatrix[1]));
			ns.z = glm::length(glm::vec3(totalMatrix[2]));
			localSDFS[x].gExtents = glm::vec4(glm::vec3(ns), 1.0f);
			glm::mat4 gRotationMatrix = FE_Math::composeMatrixWDegrees(glm::vec3(0.0), glm::vec3(1.0f), gRotation);
			localSDFS[x].globalTransform = glm::inverse(gRotationMatrix);
			
			Collision::AABB nRoot = Collision::createAABBfromRubiksCubePoints(Collision::transformRubiks(Collision::aabbToRubixCubePoints(localSDFS[x].position, localSDFS[x].extents), gt));
			localSDFS[x].rootPosition = glm::vec4(nRoot.position,  1.0);
			localSDFS[x].rootExtents = glm::vec4(nRoot.size, 1.0);
		}
	
	updateSDFBuffer();
}

float FlouraSWRT::clipAccum = 0.0;
float FlouraSWRT::clipAccumthresh =  1.0 / 1.0f;;

void FlouraSWRT::update(){
	clipAccum += TimeUtil::deltatime;
	if (clipAccum < clipAccumthresh) return;
	accelerate::transformClips(Scene::maincamera.Position, clips);
	clipAccum = 0.0;
}

#include <Render/Handler/CubeVisualizer.h>
void FlouraSWRT::MDFdebugDraw(){
	for (int i = 0; i < localSDFS.size(); ++i){
		CubeVisualizer::draw(localSDFS[i].rootPosition, localSDFS[i].rootExtents, glm::vec3(1.0, 0.0, 0.0),
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
float FlouraSWRT::gdfAccumthresh =  1.0 / 1.0f;;

void FlouraSWRT::GDFdraw(){
	gdfAccum += TimeUtil::deltatime;
	
	if (gdfAccum < gdfAccumthresh) return;
	
	int successfulDraws = 0;
	for (int i = 0; i < clips.size(); ++i){
		//std::cout << raytracer::RTGlobalTransformFlag << std::endl;
		if (clips[i].dirty <1 && !dirtyGDF) continue;
		glBindImageTexture(0, gdfINFO[i].ID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F); // GL_RGBA16F GL_R8_SNORM
		glBindImageTexture(1, gdfINFO[i].ID2, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
		
		GDFdrawShader.Activate();
		GDFdrawShader.setInt3("volumeSize", gdfINFO[i].resolution, gdfINFO[i].resolution, gdfINFO[i].resolution);
		GDFdrawShader.setFloat4("positionScale", clips[i].ps);
		
		glDispatchCompute((gdfINFO[i].resolution + 7) / 8, (gdfINFO[i].resolution + 7) / 8, ((gdfINFO[i].resolution + 7) / 8));
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		
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


