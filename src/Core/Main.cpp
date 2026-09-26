#include "Main.h"
#include "utils/timeUtil.h" 
#include <thread>
#include <chrono>
#include "File/File.h"
#include <Editor/UI/ImGui/ImGuiWindow.h>
#include <Render/Handler/RenderClass.h>
#include "scene/FE_LAYER.h"
#include "scene/scene.h"
#include <Gameplay/Player.h>
#include <Render/window/WindowHandler.h>
#include <Scene/LightingHandler.h>
#include "Editor/UI/ImGui/OpenSceneWindow.h"
#include "Sound/SoundRunner.h"
#include <thread>
#include "Systems/Physics/physworld.h"
#include "Render/Handler/LoadHandler.h"
#include "Systems/general/jobClass.h"
#include "argumentHandler.h"
#include <Render/pipeline/prebuilt_pipelines/swrt.h>

bool Main::sleepState = true;
bool Main::lockGameThread = false; // big no rn holds renderer
bool Main::lockPhysicsThread = false;
bool Main::lockWorkerThread = false;


int main(int argc, char *argv[]){
	ArgumentHandler::processArguments(argc, argv);

	//CloseConsoleWindow();
	Main::sleepState = true;
	FileClass::loadSettings();
	SoundRunner::init();
	RenderClass::init(windowHandler::width, windowHandler::height);
	Scene::maincamera.InitCamera(windowHandler::width, windowHandler::height, Scene::initalCameraPos); 	// camera ratio pos

	Scene::init();
	Scene::loadScene(Scene::sceneName);

	//two classes to test stuff
	FE_LAYER::init();
	Player::init();
	
	//std::thread pollThread(pollEventsLoop, windowHandler::window);
	
	glfwMakeContextCurrent(nullptr);
	std::thread gameThread(Main::gameLoop, windowHandler::window);
	std::thread physicsThread(Main::physicsLoop, windowHandler::window);
	std::thread workerThread(Main::workerLoop, windowHandler::window);
	
	while (!glfwWindowShouldClose(windowHandler::window)){ // main thread
		// no lock here, handles inputs
		TimeUtil::mainThreadUpdate(); // update time
		getAjob::onBeginningOfMainThread();
		windowHandler::PollMousePositionsMainWindow();
		Scene::maincamera.inputsMouse(windowHandler::window);
		glfwPollEvents(); // poll
	}
	gameThread.join();
	physicsThread.join();
	workerThread.join();
	
	// Cleanup: Delete all objects on close
	Main::sleepState = false;
	
	if (FEImGuiWindow::imGuiEnabled)
		ImGui_ImplOpenGL3_Shutdown(), ImGui_ImplGlfw_Shutdown(), ImGui::DestroyContext(); // Kill ImGui

	RenderClass::Cleanup();
	Skybox::Delete();
	Skybox::cleanup();
	FE_LAYER::Delete(); // Delete scene
	SoundRunner::Delete();
	Scene::Delete();
	LightingHandler::cleanup();
	glfwDestroyWindow(windowHandler::window), glfwTerminate(); // Kill opengl
	return 0;
}

void Main::gameLoop(GLFWwindow* window){
	glfwMakeContextCurrent(window); // hand over context to thread
	while (!glfwWindowShouldClose(window)){
		if (lockGameThread) continue;
		
		TimeUtil::update();
		getAjob::onBeginningOfGameThread();
		// cam
		Scene::maincamera.saveLastMaticies();
		Scene::maincamera.updateHaltonJitter(RenderClass::jitterFactor); // jitter
		Scene::maincamera.Inputs(windowHandler::window, TimeUtil::deltatime);
		Scene::maincamera.updateMatrix(); // Update: fov, near and far plane
		Scene::maincamera.applyJitter = RenderClass::doTAA; // apply to projection matrix
		
		// scene
		Scene::onBeginningOfFrame(); // for velocity atm
		FE_LAYER::onBeginningOfFrame();
		
		Scene::sceneUpdateWork(); // belongs to work thread storing here for now
		
		Scene::Update();
		// misc
		FE_LAYER::Update();
		
		Player::update();
		
		// should only do this when swrt is on
		FlouraSWRT::update();
		// render
		//LoadHandler::updateFromOpenGLThread(); // for the opengl specific stuff that cant run on the other threads to-do with loading
		
		RenderClass::Render(windowHandler::window, windowHandler::width, windowHandler::height);
		
		ProbeHandler::dirtyScene = false;
	}
	
	glfwMakeContextCurrent(nullptr);
}

void Main::physicsAttemptThreadUnlock(){
	if (!lockPhysicsThread) return; // thread not locked
	
	// now for the giant condition (small for now, and yeah I know there's better ways to do this)
	if (!Scene::entityDeletionUnderGoing){
		lockPhysicsThread = false;
	}
}

void Main::physicsLoop(GLFWwindow* window){
	while (!glfwWindowShouldClose(window)){
		if (lockPhysicsThread){
			physicsAttemptThreadUnlock(); // attempt to unlock and return
			continue;
		}
		
		// could limit tick rate
		TimeUtil::physicsThreadUpdate();
		getAjob::onBeginningOfPhysicsThread();
		physworld::physicsArrayDynamicUpdateLoop(physworld::worldGravity, TimeUtil::ptTimer.deltatime); //  main update
		physworld::update(TimeUtil::ptTimer.deltatime);
		
		if (Player::s_DoGravity) {
			Player::force += Player::mass * Player::gravity; // applying foce

			Player::velocity += Player::force / Player::mass * TimeUtil::ptTimer.deltatime;
			Scene::maincamera.Position += Player::velocity * TimeUtil::ptTimer.deltatime;

			Player::force = glm::vec3(0.0f); // reset force at end
		}
		
		physworld::collisionResolve(); // resolve the collisions
		for (int i = 0; i < 32; ++i){ // solve 32 times
			physworld::collisionResolveCamera();
		}
		
	}
}

// this thread is for worker tasks
void Main::workerLoop(GLFWwindow* window){
	while (!glfwWindowShouldClose(window)){
		if (lockWorkerThread) continue;
		getAjob::onBeginningOfPhysicsThread();
		TimeUtil::workerThreadUpdate();
		//RenderHandler::updateLoadUnloadedModels(); // load unloaded models, so they are threaded
	}
}


