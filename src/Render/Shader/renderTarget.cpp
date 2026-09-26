#include "Render/Shader/renderTarget.h"
#include <Render/pipeline/prebuilt_pipelines/geometryPass.h>
//#include <Render/pipeline/prebuilt_pipelines/depreciated/raytracer.h>
#include <Render/window/WindowHandler.h>
#include "Scene/scene.h"
#include <glm/gtx/compatibility.hpp>
#include  "Render/pipeline/prebuilt_pipelines/historyPass.h"
#include  "Render/pipeline/prebuilt_pipelines/dbgPass.h"
#include <Render/pipeline/prebuilt_pipelines/swrt.h>

int renderTarget::tempWidth;
int renderTarget::tempHeight;

unsigned int renderTarget::ViewPortWidth = 800, renderTarget::ViewPortHeight = 600;
unsigned int renderTarget::windowWidth = 800, renderTarget::windowHeight = 600;

Shader renderTarget::frameBufferProgram;

unsigned int renderTarget::FBO;
unsigned int renderTarget::RBO;
unsigned int renderTarget::screentexture;

unsigned int renderTarget::FFBO;
unsigned int renderTarget::FRBO;
unsigned int renderTarget::Ftexture;

unsigned int renderTarget::cmFBO;
unsigned int renderTarget::cmRBO;
unsigned int renderTarget::cmtexture;

unsigned int renderTarget::SGFBO;
unsigned int renderTarget::SGRBO;
unsigned int renderTarget::skyGradientTexture;
//Framebuffer renderTarget::sgFrameBuffer;

float renderTarget::sharpness = 0.2f;

RenderQuad renderTarget::rq;

float s_ViewportVerticies[24] = {
	// Coords,   Texture cords
	 1.0f, -1.0f,  1.0f, 0.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	-1.0f,  1.0f,  0.0f, 1.0f,

	 1.0f,  1.0f,  1.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	-1.0f,  1.0f,  0.0f, 1.0f
};


void renderTarget::smInit(glm::vec2 res)
{
	// GEN FBO
	glGenFramebuffers(1, &renderTarget::cmFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::cmFBO);
	// GEN TEX and bind tex to fbo
	glGenTextures(1, &renderTarget::cmtexture);
	glBindTexture(GL_TEXTURE_2D, renderTarget::cmtexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, res.x, res.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTarget::cmtexture, 0);

	glGenRenderbuffers(1, &renderTarget::cmRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, renderTarget::cmRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, res.x, res.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderTarget::cmRBO);


	// Error checking
	auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer error: " << fboStatus << std::endl;
	}
}

void renderTarget::setupSGFBO(unsigned int width, unsigned int height){
	// GEN FBO
	glGenFramebuffers(1, &SGFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, SGFBO);
	// GEN TEX and bind tex to fbo
	glGenTextures(1, &skyGradientTexture); // GL_LINEAR
	glBindTexture(GL_TEXTURE_2D, skyGradientTexture); // GL_UNSIGNED_BYTE
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // GL_LINEAR_MIPMAP_LINEAR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, skyGradientTexture, 0);

	glGenRenderbuffers(1, &SGRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, SGRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, SGRBO);


	// Error checking
	auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer error: " << fboStatus << std::endl;
	}
	
}

void renderTarget::smUpdateResolution(glm::vec2 res){
	glBindTexture(GL_TEXTURE_2D, renderTarget::cmtexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, res.x, res.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
	// update renderbuffer texture
	glBindRenderbuffer(GL_RENDERBUFFER, renderTarget::cmRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, res.x, res.y);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}


void renderTarget::clearsmbuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::cmFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void renderTarget::setupFBO(unsigned int width, unsigned int height) {
	// Initialize viewport rectangle object drawn to viewport with framebuffer texture attached
	rq.init();
	// GEN FBO
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	// GEN TEX and bind tex to fbo
	glGenTextures(1, &screentexture); // GL_LINEAR
	glBindTexture(GL_TEXTURE_2D, screentexture); // GL_UNSIGNED_BYTE
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // GL_LINEAR_MIPMAP_LINEAR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screentexture, 0);

	glGenRenderbuffers(1, &RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);


	// Error checking
	auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer error: " << fboStatus << std::endl;
	}

	// GEN FBO
	glGenFramebuffers(1, &FFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FFBO);
	// GEN TEX and bind tex to fbo
	glGenTextures(1, &Ftexture);
	glBindTexture(GL_TEXTURE_2D, Ftexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Ftexture, 0);

	// Error checking
	auto fboStatus2 = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus2 != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Framebuffer error: " << fboStatus2 << std::endl;
	}

}

void renderTarget::setViewportToViewPortResolution(){glViewport(0, 0, ViewPortWidth, ViewPortHeight);}

void renderTarget::attemptFrameBufferResize(unsigned int width, unsigned int height)
{
	if (width == ViewPortWidth && height == ViewPortHeight ) return;
	renderTarget::ViewPortWidth = width;
	renderTarget::ViewPortHeight = height;
	
	updateFrameBufferResolution(width, height);
	//updateFrameBufferResolution(480, 480);
}

void renderTarget::updateFrameBufferResolution(unsigned int width, unsigned int height) {
	//Framebuffer::ViewPortWidth = width;
	//Framebuffer::ViewPortHeight = height;
	
	glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
	Scene::maincamera.SetViewportSize(static_cast<int>(width), static_cast<int>(height));
	
	
	// seperate res scaling from final buffer
	glBindTexture(GL_TEXTURE_2D, screentexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	// update renderbuffer texture
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	
	// sky stuff
	
	//renderTarget::sgFrameBuffer.resize(width, height);
	
	glBindTexture(GL_TEXTURE_2D, skyGradientTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	glBindRenderbuffer(GL_RENDERBUFFER, SGRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);


	
	glBindTexture(GL_TEXTURE_2D, Ftexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	dbgPass::updateDBGResolution(width, height);
	GeometryPass::updateGbufferResolution(width, height);
	HistoryPass::updateHbufferResolution(width, height);
	//raytracer::resizeTexture(width, height);
	FlouraSWRT::updateSWRTbuffersResolution(width, height);
}

float fps24accumulator = 0;
float accum24value = 0;
void rtFinalUnifroms(){
	renderTarget::frameBufferProgram.Activate();
	renderTarget::frameBufferProgram.setFloat("time", glfwGetTime());
	renderTarget::frameBufferProgram.setFloat("deltaTime", TimeUtil::deltatime);
	renderTarget::frameBufferProgram.setFloat("sharpness", renderTarget::sharpness);
	renderTarget::frameBufferProgram.setFloat("accum24value", accum24value);
	renderTarget::frameBufferProgram.setBool("overlayDebug", dbgPass::overlayDebug);

	// draw the framebuffer
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderTarget::screentexture);
	glGenerateMipmap(GL_TEXTURE_2D);
	renderTarget::frameBufferProgram.setInt("screenTexture", 0);
	

	renderTarget::frameBufferProgram.setTexture2D("depthTexture", 1 ,GeometryPass::depthTexture);
	renderTarget::frameBufferProgram.setTexture2D("albedo", 2, GeometryPass::gAlbedo);
	renderTarget::frameBufferProgram.setTexture2D("normal", 3, GeometryPass::gNormal);
	renderTarget::frameBufferProgram.setTexture2D("dbgColour", 10, dbgPass::dbgColour);
	//dbgPass
	
	
	renderTarget::frameBufferProgram.setFloat("gamma", Scene::maincamera.gamma);
	RenderClass::bluenoise->Bind();
	renderTarget::frameBufferProgram.setInt("BlueNoiseTex", 11);
	
	renderTarget::frameBufferProgram.setHandleui64ARB("LUT", RenderClass::LUT->handle);
	
	glDisable(GL_DEPTH_TEST);
}

void renderTarget::FBO2Draw() {
	//glDisable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, FFBO);
	//glViewport(0, 0, windowWidth, windowHeight);
	glClear(GL_COLOR_BUFFER_BIT);
	rtFinalUnifroms();
	rq.draw();
	
	//glBindFramebuffer(GL_FRAMEBUFFER, 0); glEnable(GL_DEPTH_TEST); return;
	//glViewport(0, 0, windowWidth, windowHeight);
	// unbind fbo and present to screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//	glActiveTexture(GL_TEXTURE0);
//	glBindTexture(GL_TEXTURE_2D, Ftexture);
//	rq.draw();

	//glEnable(GL_DEPTH_TEST);
}



void renderTarget::FBODraw(bool imGuiPanels, GLFWwindow* window) {

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Restore normal rendering < wireframe

	glActiveTexture(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	fps24accumulator += TimeUtil::deltatime;
	if (fps24accumulator >= 0.041f){accum24value++;fps24accumulator = 0.0f;}

	
	int newWidth, newHeight;
	glfwGetWindowSize(window, &newWidth, &newHeight);
	renderTarget::windowWidth = newWidth;
	renderTarget::windowHeight = newHeight;
	
	rtFinalUnifroms();
	
	if (!imGuiPanels) {
		//updateDisplayResolution(newWidth, newHeight);
		renderTarget::attemptFrameBufferResize(newWidth, newHeight); // Update frame buffer resolution 
		
		//glViewport(0, 0, ViewPortWidth, ViewPortHeight); // set viewport
		
		glClearColor(RenderClass::skyRGBA.r,RenderClass::skyRGBA.g,RenderClass::skyRGBA.b, 1.0f);
		if (FEImGuiWindow::isWireframe) glClearColor(pow(0.0f, Scene::maincamera.gamma), pow(0.0f, Scene::maincamera.gamma), pow(0.0f, Scene::maincamera.gamma), 1.0f);
		
		glClear(GL_COLOR_BUFFER_BIT);
		rq.draw();	
		
	}
	else{
		renderTarget::FBO2Draw();
	}
	glEnable(GL_DEPTH_TEST);
}

void renderTarget::Delete(){
	glDeleteFramebuffers(1, &FBO);
	glDeleteRenderbuffers(1, &RBO);
	glDeleteTextures(1, &screentexture);
	glDeleteFramebuffers(1, &FFBO);
	glDeleteRenderbuffers(1, &FRBO);
	glDeleteTextures(1, &Ftexture);
}