#ifndef SCENE_CLASS_H	
#define SCENE_CLASS_H

#include <iostream>
#include <Sound/SoundProgram.h>
#include <Sound/SoundRunner.h>
#include <Scene/Object/Entity.h>
#include <thread>
#include "Camera/Camera.h"
#include <Systems/Physics/Collision.h>
#include "ProbeHandler.h"

using json = nlohmann::json;

//testclass
class Scene{
public:

	static std::string sceneName; // Map loading
	static std::vector <SoundProgram> SoundObjects;
	static std::vector <std::unique_ptr<entity>> entityObjects;
	static Camera maincamera;
	static glm::vec3 initalCameraPos;
	
	static Collision::AABB SceneBounds;
	static std::vector <Collision::AABB> rootnodes;
	static bool entityDeletionUnderGoing;

	static bool spawnNearCamera;
	
	static void calculateSceneBounds();

	static void init();

	static void loadSceneStateless(std::string path);
	static void loadScene(std::string path);

	static void saveSceneStateless(std::string path);
	static void saveScene(std::string path);
	
	static void reloadScene(std::string path);

	static void enviromentSave(std::string path);

	static void enviromentLoad(std::string path);

	static void modelReload(std::string path);

	static void modelLoad(std::string path);

	static void modelSave(std::string path);
	
	static void billBoardSave(std::string path);

	static void settingsSave(std::string path);

	static void cameraSettingsSave(std::string path);

	static void AddSceneSoundObject(std::string name, std::string path);

	static uint64_t AddEntityObject(entity::ENT_TYPE_ENUM type, std::string name, std::string path, glm::vec3 spawnPosition,  glm::vec3 spawnScale, glm::vec3 spawnRotation);

	static void billBoardLoad(std::string path);

	static void soundObjectLoad(std::string path); 

	static void soundObjectSave(std::string path);

	static void settingsLoad(std::string path);

	static void cameraSettingsLoad(std::string path);

	static void shadowmapDraw();
	
	static void callAllScriptInit();

	static void resetAllScripts();

	static void onBeginningOfFrame();
	
	static void draw();// put void calculateSceneBounds();

	static void Update(); 
	
	static void sceneUpdateWork();

	static void Delete();

	static void saveEntityState();

	static void restoreEntitiesToState();
	
	static void queuedDeletionLoop();
	
	static Collision::HitResult traceIntoScene(glm::vec3 ro, glm::vec3 rd);

	static void voxelizeArea(glm::vec3 p, glm::vec3 s, Texture3D& texture, int sliceSize, GLuint slot);
	
	static void tagMeshes(glm::vec3 p, glm::vec3 s, std::vector<Mesh*> &meshes, std::vector<glm::mat4>& matrices);

	
private:
};
#endif