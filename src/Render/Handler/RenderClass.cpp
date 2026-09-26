#include "RenderClass.h"
#include "Render/Handler/CubeVisualizer.h"
#include "Render/Object/RenderQuad.h"
#include <Render/pipeline/prebuilt_pipelines/geometryPass.h>
//#include <Render/pipeline/prebuilt_pipelines/depreciated/raytracer.h>
#include <Render/window/WindowHandler.h>
#include <Scene/LightingHandler.h>
#include <Scene/scene.h>
#include  "Render/pipeline/prebuilt_pipelines/historyPass.h"
#include <Render/Handler/RenderHandler.h>
//#include  "Render/Handler/depreciated/sceneDescription.h"
#include  "Render/pipeline/prebuilt_pipelines/dbgPass.h"
#include <Render/render_util/flouraSlang.h>
#include <Render/pipeline/prebuilt_pipelines/flouraDeferred.h>
#include <Systems/Physics/SDF.h>
#include <Render/pipeline/prebuilt_pipelines/swrt.h>

Shader RenderClass::taaShader;
Shader RenderClass::skyGadientShader;
Shader RenderClass::billBoardShader;
Shader RenderClass::gPassShaderBillBoard;
Shader RenderClass::LineShader;

bool RenderClass::renderSkybox = true;
bool RenderClass::doReflections = true;
bool RenderClass::doSSR = true;
bool RenderClass::doContactShadows = false;
bool RenderClass::doFog = true;
GLfloat RenderClass::DepthDistance = 100.0f;
GLfloat RenderClass::DepthPlane[] = { 0.1f, 100.0f };
glm::vec3 RenderClass::skyRGBA = glm::vec3(1.0f);
glm::vec3 RenderClass::fogRGBA = glm::vec3( 1.0f);

Line3D* RenderClass::line;
Texture* RenderClass::bluenoise;
Texture* RenderClass::bayermatrix;
Texture* RenderClass::caustic;
Texture* RenderClass::ripples;
Texture* RenderClass::droplets;
Texture* RenderClass::puddles;
Texture3D* RenderClass::LUT;
bool RenderClass::doTAA = true;
float RenderClass::jitterFactor = 0.5f;
bool RenderClass::doBinaryAlpha = true;
bool RenderClass::animateBinaryAlpha = true;

Shader SolidColour;

RenderClass::renderersEnum RenderClass::currentRenderer = RenderClass::DEFERRED;
int RenderClass::currentRendererInd = 1;
void RenderClass::init(unsigned int width, unsigned int height) {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4), glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6); // Window Minimum and Maximum version
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //OpenGl Profile
	glfwWindowHint(GLFW_RESIZABLE, 1); // Start Resizable
	glfwWindowHint(GLFW_MAXIMIZED, 0); // Start Maximized
	glfwWindowHint(GLFW_DEPTH_BITS, 32); // DepthBuffer Bit
	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

	//
	//glfwWindowHint(GLFW_RED_BITS, 10);
	//glfwWindowHint(GLFW_GREEN_BITS, 10);
	//glfwWindowHint(GLFW_BLUE_BITS, 10);
	//glfwWindowHint(GLFW_ALPHA_BITS, 2);

	windowHandler::InitMainWindow();
	if (!gladLoadGL(glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return;
	}
	
	windowHandler::setVSync(windowHandler::doVsync); // Set Vsync to value of doVsync (bool)

	// glenables
	// depth pass. render things in correct order. eg sky behind wall, dirt under water, not random order
	glEnable(GL_DEPTH_TEST); // Depth buffer
	glDepthFunc(GL_LESS);
	//glEnable(GL_STENCIL_TEST); //stencil buffer
	//glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glEnable(GL_CULL_FACE); // Culling
	glCullFace(GL_BACK);

	//switch (frontFaceSide) { //currently set to false
	//case true: { glFrontFace(GL_CW); break; } // inside facing
	//case false: { glFrontFace(GL_CCW); break; } // outside facing
	//}

	glFrontFace(GL_CCW);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glEnable(GL_FRAMEBUFFER_SRGB);
	//glEnable(GL_MULTISAMPLE);
	
	HistoryPass::init(); // init the RQ of the HP
	RenderQuad::init();
	// put in one function
	renderTarget::setupFBO(width, height);
	renderTarget::setupSGFBO(width, height);
	dbgPass::setupDBGbuffers(width, height);
	GeometryPass::setupGbuffers(width, height); // here
	HistoryPass::setupHbuffers(width, height);
	SceneDescription::generateSceneBuffers();
	//SceneDescription::generateVoxelBuffers();
	FlouraSWRT::initSWRTssbo();
	FlouraDeferred::init();
	FlouraSWRT::setupSWRTbuffers(width, height);
	
	//bluenoise = new Texture(); bluenoise->createTexture("Assets/Dependants/LDR_LLL1_0.png", "misc", 6);
	bluenoise = new Texture(); bluenoise->linearFilter = false; bluenoise->createTexture("Assets/Dependants/LDR_RGBA_0.png", "misc", 6);
	bayermatrix = new Texture(); bayermatrix->createTexture("Assets/Dependants/bayer_matrix.png", "misc", 7);
	caustic = new Texture(); caustic->linearFilter = true; caustic->createTexture("Assets/Dependants/caustic-256.png", "misc", 9);
	ripples = new Texture(); ripples->linearFilter = true; ripples->createTexture("Assets/Dependants/atlas3x47_384x384.png", "misc", 10);
	droplets = new Texture(); droplets->linearFilter = true; droplets->createTexture("Assets/Dependants/wallDroplets.png", "misc", 11);
	puddles = new Texture(); puddles->linearFilter = true; puddles->createTexture("Assets/Dependants/puddles.png", "misc", 12);
	LUT = new Texture3D(); LUT->createTexture3D("Assets/Dependants/LUT_33.png", "lut", 15); 
	//raytracer::initcomputeShader(width, height); // Initialize compute shader for lighting pass
	//denoiser::initcomputeShader(width, height);

	initGlobalShaders();
	LightingHandler::setupShadowMapBuffer();
	// need to add debug buffers at some point
	//Framebuffer::setupNoiseMap();


	renderTarget::smInit(glm::vec2(128));
	RenderHandler::init();

	if (FEImGuiWindow::imGuiEnabled) {
		FEImGuiWindow::init();
		ImGuizmo::SetOrthographic(false);
		FEImGuiWindow::initImGui(windowHandler::window); // Initialize ImGUI
	}
	else{
		FEImGuiWindow::imGuiPanels[0] = false;
	}

	Skybox::init();
	CubeVisualizer::init();
	line = new Line3D(glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
}

void RenderClass::initGlobalShaders() {
	billBoardShader.LoadShader("Assets/Shaders/Db/BillBoard.vert", "Assets/Shaders/Db/BillBoard.frag");
	gPassShaderBillBoard.LoadShader("Assets/Shaders/gBuffer/geometryPassBillboard.vert", "Assets/Shaders/gBuffer/geometryPassBillboard.frag");
	SolidColour.LoadShader("Assets/Shaders/Lighting/Default.vert", "Assets/Shaders/Db/solidColour.frag");
	HistoryPass::hPassShader.LoadShader("Assets/Shaders/gBuffer/historybuffer.vert", "Assets/Shaders/gBuffer/historybuffer.frag");
	
	// keep an eye on these in a moment or later
	
	FlouraDeferred::createShaders();
	taaShader.LoadShader("Assets/Shaders/PostProcess/TAA.vert", "Assets/Shaders/PostProcess/TAA.frag");
	//raymarchShader.LoadShader("Assets/Shaders/raymarched/raymarch.vert", "Assets/Shaders/raymarched/raymarch.frag");
	FlouraSWRT::initShaders();
	flouraSDF::createShaders();
	//raymarchShaderT.LoadComputeShader("Assets/Shaders/raymarched/raymarch.comp");
	
	
	//std::string tvert; std::string tfrag;
	//if (FlouraSlang::compileSlangToGLSL("Assets/Shaders/raymarched/skygraident.slang", tvert, tfrag)){
	//	skyGadientShader.takePath = false; 
	//	std::cout << tfrag << std::endl;
	//	skyGadientShader.LoadShader(tvert.c_str(), tfrag.c_str());
	//}
	
	//skyGadientShader.isSpirv = true;
	//skyGadientShader.LoadShader("Assets/Shaders/raymarched/skygradient_vert.spv", "Assets/Shaders/raymarched/skygradient_frag.spv");
	//skyGadientShader.LoadShader("Assets/Shaders/raymarched/skygradient.vert", "Assets/Shaders/raymarched/skygradient.frag");
	

	LineShader.LoadShader("Assets/Shaders/Db/line.vert","Assets/Shaders/Db/line.frag");

	renderTarget::frameBufferProgram.LoadShader("Assets/Shaders/PostProcess/framebuffer.vert", "Assets/Shaders/PostProcess/framebuffer.frag");

}

void RenderClass::Cleanup() {
	billBoardShader.Delete();
	gPassShaderBillBoard.Delete();
	taaShader.Delete();
	//raymarchShader.Delete();
	//skyGadientShader.Delete();
	SolidColour.Delete();
	billBoardShader.Delete();
	LineShader.Delete();
	FlouraDeferred::Delete();
	FlouraSWRT::cleanupSWRTssbo();
	renderTarget::frameBufferProgram.Delete();
	GeometryPass::cleanupGbuffers();
	dbgPass::cleanupDBGbuffers();
	HistoryPass::cleanupHbuffers();
	FlouraSWRT::cleanupShaders();
	FlouraSWRT::cleanupSWRTbuffers();
	flouraSDF::cleanupShaders();
	
	CubeVisualizer::cleanup();
	line->~Line3D();
	
	bayermatrix->Delete();
	bluenoise->Delete();
	caustic->Delete();
	ripples->Delete();
	droplets->Delete();
	puddles->Delete();
	
	LUT->Delete();
}


void RenderClass::ClearFramebuffers() {
	// Clear first framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::FBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // Clear with colour
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Clear second framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::FFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // Clear with colour
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	// clear sk b
	//glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::SGFBO);
	//glClear(GL_COLOR_BUFFER_BIT); // Clear with colour
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Clear GBuffer
	glBindFramebuffer(GL_FRAMEBUFFER, GeometryPass::gBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear with colour
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	glBindFramebuffer(GL_FRAMEBUFFER,  dbgPass::dbgBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear with colour
	LightingHandler::clearSMFBO();
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

float Counter;
void RenderClass::Render(GLFWwindow* window, unsigned int width, unsigned int height) {
	LightingHandler::update();
	RenderClass::ClearFramebuffers(); // Clear Framebuffers
	
	// set clear colour
	glClearColor(skyRGBA.r, skyRGBA.g,skyRGBA.b, 1.0f);
	glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::FBO);
	glEnable(GL_DEPTH_TEST); // this line here caused me so much hell

	if (FEImGuiWindow::isWireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Enable wireframe mode
		glClearColor(pow(0.0f, Scene::maincamera.gamma), pow(0.0f, Scene::maincamera.gamma), pow(0.0f, Scene::maincamera.gamma), 1.0f);
	}

	if (!FEImGuiWindow::isWireframe && RenderClass::renderSkybox){
		Skybox::draw(Scene::maincamera, renderTarget::FBO, true);
		Skybox::setPreviousMats(Scene::maincamera);
		glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::FBO);
	}
	
	//RenderClass::skyGraidentPass();
	//glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::FBO);
	
	Scene::shadowmapDraw();
	
	Scene::draw();
	
	// should only do this when swrt is on
	RenderHandler::render();
	
	//physworld::debugDraw();
	
	// Framebuffer logic
	renderTarget::FBODraw(FEImGuiWindow::imGuiPanels[0], window);

	if (FEImGuiWindow::imGuiEnabled) {
		Counter += TimeUtil::deltatime;
		if (Counter >= 1 / 10.0f) { if (glfwGetKey(windowHandler::window, GLFW_KEY_F1) == GLFW_PRESS) { FEImGuiWindow::imGuiPanels[0] = !FEImGuiWindow::imGuiPanels[0]; } Counter = 0; }
		if (FEImGuiWindow::imGuiPanels[0]) FEImGuiWindow::Update();
	}

	glfwSwapBuffers(window); // Swap BackBuffer with FrontBuffer (DoubleBuffering)
}

void RenderClass::taaPass(){
	glDisable(GL_CULL_FACE);
	glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::FBO);
	taaShader.Activate();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderTarget::screentexture);
	glGenerateMipmap(GL_TEXTURE_2D);
	taaShader.setInt("screentexture", 0);
	taaShader.setTexture2D("gAlbedo", 1, GeometryPass::gAlbedo);
	taaShader.setTexture2D("gNormal", 2, GeometryPass::gNormal);
	taaShader.setTexture2D("depthMap", 3, GeometryPass::depthTexture);
	taaShader.setTexture2D("gVelocity", 4, GeometryPass::gVelocity);

	// skip 8 because of shadow map (i really need to use bindless on these)
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, HistoryPass::hColour);
	glGenerateMipmap(GL_TEXTURE_2D);
	taaShader.setInt("hColour", 5);
	
	taaShader.setTexture2D("hDepthTexture", 6, HistoryPass::hDepthTexture);
	
	taaShader.setFloat("NearPlane", Scene::maincamera.nearFar.x);
	taaShader.setFloat("FarPlane", Scene::maincamera.nearFar.y);
	
	taaShader.setFloat2("screenSize", glm::vec2(Scene::maincamera.width, Scene::maincamera.height));
	taaShader.setTimeVariables();
	
	
	//shader.
	RenderQuad::draw();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glActiveTexture(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderClass::skyGraidentPass(){
	glDisable(GL_CULL_FACE);
	glEnable(GL_DITHER); // idk if this did anything to help with the debanding, test later
	glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::SGFBO);
	skyGadientShader.Activate();
	
	//skyGadientShader.setFloat2("screenSize", glm::vec2(Scene::maincamera.width, Scene::maincamera.height));
	//skyGadientShader.setMat4("u_ViewMatrix", Scene::maincamera.view);
	//skyGadientShader.setMat4("u_ProjectionMatrix", Scene::maincamera.projectionAlwaysUnjittered);
	//skyGadientShader.setMat4("inverseViewMatrix", glm::inverse(Scene::maincamera.view));
	//skyGadientShader.setMat4("inverseProjectionMatrix", glm::inverse(Scene::maincamera.projectionAlwaysUnjittered));
	
	skyGadientShader.setFloat2(0, glm::vec2(Scene::maincamera.width, Scene::maincamera.height));
	skyGadientShader.setMat4(1, Scene::maincamera.view);
	skyGadientShader.setMat4(2, Scene::maincamera.projectionAlwaysUnjittered);
	skyGadientShader.setMat4(3, glm::inverse(Scene::maincamera.view));
	skyGadientShader.setMat4(4, glm::inverse(Scene::maincamera.projectionAlwaysUnjittered));

	skyGadientShader.setTimeVariables();
	
	LightingHandler::sendToShader(skyGadientShader);
	
	RenderQuad::draw();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glActiveTexture(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_DITHER);
}

void RenderClass::compileShaderUniforms(){
	for (int i = 0; i < Scene::entityObjects.size(); ++i) Scene::entityObjects[i]->component.systems.material.Material.compileUniforms();
}
