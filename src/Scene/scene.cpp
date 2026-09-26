#include "scene.h"
#include <Render/window/WindowHandler.h>
#include <Gameplay/Player.h>
#include <Scene/LightingHandler.h>
#include <utils/logConsole.h>
#include <xhash>
#include <Scripting/ScriptObject.h>
#include <utils/FE_math.h>
#include "Systems/util/relationshipManager.h"
#include "Systems/util/UUID.h"
#include  "Render/pipeline/prebuilt_pipelines/dbgPass.h"
#include "Render/Handler/CubeVisualizer.h"
#include "Render/Handler/RenderHandler.h"
#include "Render/pipeline/prebuilt_pipelines/swrt.h"

std::string Scene::sceneName = ""; // Map loading
std::vector <SoundProgram> Scene::SoundObjects;
std::vector <std::unique_ptr<entity>> Scene::entityObjects;
Camera Scene::maincamera;

glm::vec3 Scene::initalCameraPos = glm::vec3(0, 0, 0);

Collision::AABB Scene::SceneBounds;
std::vector <Collision::AABB> Scene::rootnodes;

bool Scene::spawnNearCamera = true;
bool Scene::entityDeletionUnderGoing = false; // for qeueing wheter something should be deleted


BillBoard* PointLightIcon;
BillBoard* SpotLightIcon;
BillBoard* SoundIcon;

void Scene::calculateSceneBounds()
{
	std::vector<glm::vec3> points;
	std::vector<Collision::AABB> AABBs;

	for (size_t i = 0; i < entityObjects.size(); i++)
		for (size_t x = 0; x < entityObjects[i]->component.collider.rootnodes.size(); x++){
			// position, scale
			glm::vec3 p = entityObjects[i]->component.collider.rootnodes[x].position;
			glm::vec3 s = entityObjects[i]->component.collider.rootnodes[x].size;

			// rootnodes
			Collision::AABB newAABB;
			newAABB.position = p;
			newAABB.size = s;
			AABBs.push_back(newAABB);
			
			// points

			// retreive min max
			Collision::minmax newMinMax = Collision::returnMinMax(p, s);

			// push points into array
			points.push_back(newMinMax.max);
			points.push_back(newMinMax.min);
		}	

	rootnodes = AABBs;
	SceneBounds = Collision::createAABBfromPoints(points);
}

void Scene::init() {
	PointLightIcon = new BillBoard("Assets/Icons/point.png");
	SpotLightIcon = new BillBoard("Assets/Icons/spot.png"); // draw instanced option with array argument for transformations
	SoundIcon = new BillBoard("assets/Icons/soundIcon.png");

}

void Scene::loadSceneStateless(std::string path){
	// Attemp to delete previous scene
	Delete();

	if (FEImGuiWindow::imGuiEnabled) 
		FEImGuiWindow::loadContentObjects(path + "/ContentObject.scene");

	settingsLoad(path + "/Settings.scene");
	LightingHandler::loadScene(path + "/Lights.scene");
	enviromentLoad(path + "/Enviroment.scene"); // gives DefaultSkyboxPath
	Skybox::LoadSkyBoxTexture(Skybox::DefaultSkyboxPath); // cleanup this class, could add a load cubemap texture function to the texture class
	soundObjectLoad(path + "/Sound.scene");
	cameraSettingsLoad(path + "/Camera.scene");
	billBoardLoad(path + "/BillBoard.scene"); // here
	modelLoad(path + "/Model.scene");
}

void Scene::loadScene(std::string path) {
	Player::stopState();
	// Attemp to delete previous scene
	Delete();

	if (FEImGuiWindow::imGuiEnabled)
		FEImGuiWindow::loadContentObjects(path + "/ContentObject.scene");

	settingsLoad(path + "/Settings.scene");
	LightingHandler::loadScene(path + "/Lights.scene");
	enviromentLoad(path + "/Enviroment.scene"); // gives DefaultSkyboxPath
	Skybox::LoadSkyBoxTexture(Skybox::DefaultSkyboxPath); // cleanup this class, could add a load cubemap texture function to the texture class
	soundObjectLoad(path + "/Sound.scene");
	cameraSettingsLoad(path + "/Camera.scene");
	billBoardLoad(path + "/BillBoard.scene"); // here
	modelLoad(path + "/Model.scene");

	LogConsole::print("Loaded scene from: " + path);
}

void Scene::saveSceneStateless(std::string path){
	if (FEImGuiWindow::imGuiEnabled)
		FEImGuiWindow::saveContentObjects(path + "/ContentObject.scene");

	settingsSave(path + "/Settings.scene");
	LightingHandler::saveScene(path + "/Lights.scene");
	enviromentSave(path + "/Enviroment.scene");
	soundObjectSave(path + "/Sound.scene");
	cameraSettingsSave(path + "/Camera.scene");
	billBoardSave(path + "/BillBoard.scene");
	modelSave(path + "/Model.scene");
}


void Scene::saveScene(std::string path) {
	Player::stopState();
	if (FEImGuiWindow::imGuiEnabled)
		FEImGuiWindow::saveContentObjects(path + "/ContentObject.scene");

	settingsSave(path + "/Settings.scene");
	LightingHandler::saveScene(path + "/Lights.scene");
	enviromentSave(path + "/Enviroment.scene");
	soundObjectSave(path + "/Sound.scene");
	cameraSettingsSave(path + "/Camera.scene");
	billBoardSave(path + "/BillBoard.scene");
	modelSave(path + "/Model.scene");

	LogConsole::print("Saved scene to: " + path);
}

void Scene::reloadScene(std::string path){
	if (FEImGuiWindow::imGuiEnabled) {
		if (FEImGuiWindow::imGuiEnabled) {
			FEImGuiWindow::ContentObjects.clear();
			FEImGuiWindow::ContentObjectNames.clear();
			FEImGuiWindow::ContentObjectPaths.clear();
			FEImGuiWindow::ContentObjectTypes.clear();
		}

		FEImGuiWindow::loadContentObjects(path + "/ContentObject.scene");
	}
	settingsLoad(path + "/Settings.scene");
	LightingHandler::deleteScene();
	LightingHandler::loadScene(path + "/Lights.scene");
	enviromentLoad(path + "/Enviroment.scene"); // gives DefaultSkyboxPath
	Skybox::LoadSkyBoxTexture(Skybox::DefaultSkyboxPath); // cleanup this class, could add a load cubemap texture function to the texture class

//	for (size_t i = 0; i < entityObjects.size(); i++)
//	{
//		if (entityObjects[i]->type == 'b') entityObjects[i]->Delete();
//	}
//	billBoardLoad(path + "/BillBoard.scene"); // here

	std::cout << "reminder to self: add billboard reload" << std::endl;
	// entity reloated stuff goes here
	modelReload(path + "/Model.scene");


	for (size_t i = 0; i < SoundObjects.size(); i++) {
		if (SoundObjects[i].isPlay) {
			SoundObjects[i].StopSound();
		}
	}

	SoundObjects.clear();


	soundObjectLoad(path + "/Sound.scene");
	cameraSettingsLoad(path + "/Camera.scene");



	LogConsole::print("Reloaded scene from: " + path);
}


void Scene::enviromentSave(std::string path){
	try {
		json EnviromentData = json::array();  // New JSON array to hold model data

		json JsonEnviroment;

		// sky
		JsonEnviroment["skyRGBA"][0] = RenderClass::skyRGBA[0];
		JsonEnviroment["skyRGBA"][1] = RenderClass::skyRGBA[1];
		JsonEnviroment["skyRGBA"][2] = RenderClass::skyRGBA[2];
		JsonEnviroment["DefaultSkyboxPath"] = Skybox::DefaultSkyboxPath;
		JsonEnviroment["DoSkyColour"] = Skybox::DoSbRGBA;
		JsonEnviroment["RenderSkybox"] = RenderClass::renderSkybox;
		JsonEnviroment["SkyRotation"][0] = Skybox::rotation.x;
		JsonEnviroment["SkyRotation"][1] = Skybox::rotation.y;
		JsonEnviroment["SkyRotation"][2] = Skybox::rotation.z;

		// directional light
		JsonEnviroment["DirEnabled"] = LightingHandler::doDirLight;
		JsonEnviroment["DirSpecEnabled"] = LightingHandler::doDirSpecularLight;
		JsonEnviroment["DirRotation"][0] = LightingHandler::dirLightRot[0];
		JsonEnviroment["DirRotation"][1] = LightingHandler::dirLightRot[1];
		JsonEnviroment["DirRotation"][2] = LightingHandler::dirLightRot[2];
		JsonEnviroment["DirAmbient"] = LightingHandler::directAmbient;
		JsonEnviroment["DirSpecular"] = LightingHandler::dirSpecularLight;
		JsonEnviroment["DirColour"][0] = LightingHandler::directLightCol[0];
		JsonEnviroment["DirColour"][1] = LightingHandler::directLightCol[1];
		JsonEnviroment["DirColour"][2] = LightingHandler::directLightCol[2];
		JsonEnviroment["DoShadowMap"] = LightingHandler::doDirShadowMap;
		JsonEnviroment["dirNearFar"][0] = LightingHandler::dirNearFar[0];
		JsonEnviroment["dirNearFar"][1] = LightingHandler::dirNearFar[1];
		JsonEnviroment["dirSmDistance"] = LightingHandler::distance;
		JsonEnviroment["dirSmHeight"] = LightingHandler::dirShadowheight;
		JsonEnviroment["dirShadowMapHardness"] = LightingHandler::dirShadowMapHardness;
		JsonEnviroment["dirShadowMapSamples"] = LightingHandler::dirShadowMapSamples;
		JsonEnviroment["DirSMMaxBias"] = LightingHandler::DirSMMaxBias;
		JsonEnviroment["indirectSamples"] = ProbeHandler::indirectSamples;



		EnviromentData.push_back(JsonEnviroment);

		// Write to file
		// Write back to file
		std::ofstream outFile(path, std::ios::out);
		if (!outFile.is_open()) {
			std::cout << ("Failed to write to ") << path << std::endl;
		}

		outFile << EnviromentData.dump(4);
		outFile.close();

		std::cout << "Successfully updated " << path << std::endl;

	}
	catch (const std::exception& e) {
		std::cout << "Exception: " << e.what() << std::endl;
	}
}

void Scene::enviromentLoad(std::string path)
{
	std::ifstream EnviromentDefaultFile(path);
	if (EnviromentDefaultFile.is_open()) {
		json EnviromentDefaultData;
		EnviromentDefaultFile >> EnviromentDefaultData;
		EnviromentDefaultFile.close();

		// sky
		RenderClass::skyRGBA[0] = EnviromentDefaultData[0]["skyRGBA"][0];
		RenderClass::skyRGBA[1] = EnviromentDefaultData[0]["skyRGBA"][1];
		RenderClass::skyRGBA[2] = EnviromentDefaultData[0]["skyRGBA"][2];
		Skybox::DefaultSkyboxPath = EnviromentDefaultData[0]["DefaultSkyboxPath"];
		Skybox::LoadSkyBoxTexture(Skybox::DefaultSkyboxPath);
		Skybox::DoSbRGBA = EnviromentDefaultData[0]["DoSkyColour"];;
		RenderClass::renderSkybox = EnviromentDefaultData[0]["RenderSkybox"];;
		Skybox::rotation.x = EnviromentDefaultData[0]["SkyRotation"][0];
		Skybox::rotation.y = EnviromentDefaultData[0]["SkyRotation"][1];
		Skybox::rotation.z = EnviromentDefaultData[0]["SkyRotation"][2];

		// directional light
		LightingHandler::doDirLight = EnviromentDefaultData[0]["DirEnabled"];
		LightingHandler::doDirSpecularLight = EnviromentDefaultData[0]["DirSpecEnabled"];
		LightingHandler::dirLightRot[0] = EnviromentDefaultData[0]["DirRotation"][0];
		LightingHandler::dirLightRot[1] = EnviromentDefaultData[0]["DirRotation"][1];
		LightingHandler::dirLightRot[2] = EnviromentDefaultData[0]["DirRotation"][2];
		LightingHandler::directAmbient = EnviromentDefaultData[0]["DirAmbient"];
		LightingHandler::dirSpecularLight = EnviromentDefaultData[0]["DirSpecular"];
		LightingHandler::directLightCol[0] = EnviromentDefaultData[0]["DirColour"][0];
		LightingHandler::directLightCol[1] = EnviromentDefaultData[0]["DirColour"][1];
		LightingHandler::directLightCol[2] = EnviromentDefaultData[0]["DirColour"][2];
		LightingHandler::doDirShadowMap = EnviromentDefaultData[0]["DoShadowMap"];
		LightingHandler::dirNearFar[0] = EnviromentDefaultData[0]["dirNearFar"][0];
		LightingHandler::dirNearFar[1] = EnviromentDefaultData[0]["dirNearFar"][1];
		LightingHandler::distance = EnviromentDefaultData[0]["dirSmDistance"];
		LightingHandler::dirShadowheight = EnviromentDefaultData[0]["dirSmHeight"];
		LightingHandler::dirShadowMapHardness = EnviromentDefaultData[0]["dirShadowMapHardness"];
		LightingHandler::dirShadowMapSamples = EnviromentDefaultData[0]["dirShadowMapSamples"];
		LightingHandler::DirSMMaxBias = EnviromentDefaultData[0]["DirSMMaxBias"];

		if (EnviromentDefaultData[0].contains("indirectSamples")) ProbeHandler::indirectSamples = EnviromentDefaultData[0]["indirectSamples"];
	}
	else {
		std::cerr << "Enviroment Failed to open " << path << std::endl;
	}
}

void Scene::modelReload(std::string path){
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cout << "Model Failed to open file: " << path << std::endl;
		return;
	}
	json modelFileData;
	try {
		file >> modelFileData;
	}
	catch (const nlohmann::json::parse_error& e) {
		// This catch block specifically handles JSON parsing errors,
		// which gives more precise error information from the library.
		std::cout << "JSON Parse Error loading model data: " << e.what() << std::endl;
		std::cout << "Error byte position: " << e.byte << std::endl; // Specific to nlohmann::json
	}
	catch (const std::ios_base::failure& e) {
		// This catch block handles file I/O errors (e.g., file not found, permission issues).
		std::cout << "File I/O Error loading model data: " << e.what() << std::endl;
	}
	catch (const std::exception& e) {
		// A general catch-all for any other std::exception derived errors.
		std::cout << "An unexpected error occurred loading model data: " << e.what() << std::endl;
	}
	file.close();
	int index = 1;
	for (const auto& item : modelFileData) {
		
		uint64_t tempUUID = UUID::StringToUUID(item.at("UUID").get<std::string>());
		
		//std::cout << "pre"<< entityObjects[index]->UUID << std::endl;
		//std::cout << "new"<< tempUUID << std::endl;
		if (entityObjects[index]->UUID != tempUUID) continue; // index++; // segfault here 

		//std::cout << "ee" << std::endl;

		entityObjects[index]->name = item.at("name").get<std::string>();
		entityObjects[index]->path = item.at("path").get<std::string>();
		entityObjects[index]->component.systems.material.Material.materialPath;
		std::string MaterialPath = item.at("MaterialPath").get<std::string>();

		// transform
		entityObjects[index]->setPosition(glm::vec3(item.at("Location")[0], item.at("Location")[1], item.at("Location")[2]));
		entityObjects[index]->setRotation(glm::vec3(item.at("Rotation")[0], item.at("Rotation")[1], item.at("Rotation")[2]));
		entityObjects[index]->setScale(glm::vec3(item.at("Scale")[0], item.at("Scale")[1], item.at("Scale")[2]));

		entityObjects[index]->component.flags.doCulling = item.at("isBackFaceCulling").get<bool>();
		entityObjects[index]->component.flags.castsShadow = item.at("CastShadow").get<bool>();
		entityObjects[index]->component.systems.material.uvScale = glm::vec2(item.at("uvScale")[0],
			item.at("uvScale")[1]);

		entityObjects[index]->component.render.drawInstanced = item.at("drawInstanced").get<bool>();

		//newObject->create('m', name, path, MaterialPath); // Load into this unique MaterialObject // this needs to run and somehow join up when complete?
		// what about the idea of creating them in a state without a actual model, then doing the create function on a thread and push back when joinable;

		// doRender
		entityObjects[index]->component.flags.render = item.at("doRender").get<bool>();

		for (auto* script : entityObjects[index]->ScriptObjects) {
			delete script;
		}
		entityObjects[index]->ScriptObjects.clear();

		if (item.contains("Scripts") && item["Scripts"].is_array()) {
			for (const auto& scriptItem : item["Scripts"]) {
				std::string name = scriptItem.at("name").get<std::string>();
				std::string path = scriptItem.at("path").get<std::string>();

				entityObjects[index]->addScript(path, name);

				std::string scriptUUIDStr = scriptItem.at("UUID").get<std::string>();
				uint64_t UUID = UUID::StringToUUID(scriptUUIDStr);

				entityObjects[index]->ScriptObjects.back()->UUID = UUID;
			}

		}

		if (item.contains("hasParent")) entityObjects[index]->component.relationship.hasParent = item.at("hasParent").get<bool>();

		uint64_t tempParentUUID = 0;
		if (item.contains("parentUUID")) tempParentUUID = UUID::StringToUUID(item.at("parentUUID").get<std::string>());
		

		int entityIndex = RelationshipManager::indexFromUUIDEntity(tempUUID);

		RelationshipManager::addParent(entityIndex, tempParentUUID);

		entityObjects[index]->component.relationship.childUUID.clear();

		if (item.contains("children") && item["children"].is_array()) {
			for (const auto& childItem : item["children"]) {

				std::string childUUIDStr = childItem.at("childUUID").get<std::string>();
				uint64_t childUUID = UUID::StringToUUID(childUUIDStr);
				//std::cout << childUUIDStr <<" E" << std::endl;
				
				entityObjects[index]->component.relationship.childUUID.push_back(childUUID);
			
				int cIndex = RelationshipManager::indexFromUUIDEntity(childUUID);

				if (cIndex != -1) {
					RelationshipManager::addParent(cIndex, entityObjects[index]->UUID);
				}
			}

		}
		
		index++;
	}
	std::cout << "Reloaded Scene Models from: " << path << std::endl;
}

void Scene::modelLoad(std::string path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cout << "Model Failed to open file: " << path << std::endl;
		return;
	}
	json modelFileData;
	try {
		file >> modelFileData;
	}
	catch (const nlohmann::json::parse_error& e) {
		// This catch block specifically handles JSON parsing errors,
		// which gives more precise error information from the library.
		std::cout << "JSON Parse Error loading model data: " << e.what() << std::endl;
		std::cout << "Error byte position: " << e.byte << std::endl; // Specific to nlohmann::json
	}
	catch (const std::ios_base::failure& e) {
		// This catch block handles file I/O errors (e.g., file not found, permission issues).
		std::cout << "File I/O Error loading model data: " << e.what() << std::endl;
	}
	catch (const std::exception& e) {
		// A general catch-all for any other std::exception derived errors.
		std::cout << "An unexpected error occurred loading model data: " << e.what() << std::endl;
	}
	file.close();

	for (const auto& item : modelFileData) {
		std::unique_ptr<entity> newObject = std::make_unique<entity>(); // Use std::make_unique

		std::string name = item.at("name").get<std::string>();
		std::string path = item.at("path").get<std::string>();
		std::string MaterialPath = item.at("MaterialPath").get<std::string>();

		// transform
		newObject->setPosition(glm::vec3(item.at("Location")[0], item.at("Location")[1], item.at("Location")[2]));
		newObject->setRotation(glm::vec3(item.at("Rotation")[0], item.at("Rotation")[1], item.at("Rotation")[2]));
		newObject->setScale(glm::vec3(item.at("Scale")[0], item.at("Scale")[1], item.at("Scale")[2]));

		if (item.contains("isBackFaceCulling"))newObject->component.flags.doCulling = item.at("isBackFaceCulling").get<bool>();
		if (item.contains("CastShadow"))newObject->component.flags.castsShadow = item.at("CastShadow").get<bool>();
		if (item.contains("uvScale"))
			newObject->component.systems.material.uvScale = glm::vec2(item.at("uvScale")[0],
				item.at("uvScale")[1]);
		if (item.contains("drawInstanced"))newObject->component.render.drawInstanced = item.at("drawInstanced").get<bool>();
		if (item.contains("doRender"))newObject->component.flags.render = item.at("doRender").get<bool>();


		uint64_t tempUUID = 0;
		if (item.contains("UUID"))tempUUID = UUID::StringToUUID(item.at("UUID").get<std::string>());

		if (item.contains("Scripts") && item["Scripts"].is_array()) {
			for (const auto& scriptItem : item["Scripts"]) {
				std::string name = scriptItem.at("name").get<std::string>();
				std::string path = scriptItem.at("path").get<std::string>();

				newObject->addScript(path, name);

				std::string scriptUUIDStr = scriptItem.at("UUID").get<std::string>();
				uint64_t UUID = UUID::StringToUUID(scriptUUIDStr);

				newObject->ScriptObjects.back()->UUID = UUID;
			}

		}

		if (item.contains("children") && item["children"].is_array()) {
			for (const auto& childItem : item["children"]) {

				std::string childUUIDStr = childItem.at("childUUID").get<std::string>();
				uint64_t childUUID = UUID::StringToUUID(childUUIDStr);

				newObject->component.relationship.childUUID.push_back(childUUID);
			}
		}

		if (item.contains("hasParent")) newObject->component.relationship.hasParent = item.at("hasParent").get<bool>();
		
		uint64_t tempParentUUID = 0;
		if (item.contains("parentUUID")) tempParentUUID = UUID::StringToUUID(item.at("parentUUID").get<std::string>());
		newObject->component.relationship.parentUUID = tempParentUUID;

		entityObjects.push_back(std::move(newObject));
		
		entityObjects.back()->createwUUID(tempUUID, entity::ENT_MODEL_TYPE, name, path, MaterialPath);
	}
	std::cout << "Loaded Scene Models from: " << path << std::endl;
}

void Scene::modelSave(std::string path) {
	try {
		json settingsData = json::array();  // New JSON array to hold model data
		for (size_t i = 0; i < entityObjects.size(); i++)
		{
			if (entityObjects[i]->type == entity::ENT_MODEL_TYPE) // model
			{
				json modelJson;
				modelJson["name"] = entityObjects[i]->name;

				modelJson["path"] = entityObjects[i]->path;
				glm::vec3 objPos = entityObjects[i]->fetchPosition();
				glm::vec3 objScale = entityObjects[i]->fetchScale();
				glm::vec3 objRot = entityObjects[i]->fetchRotation();

				modelJson["Location"] = { objPos.x, objPos.y, objPos.z };
				modelJson["Rotation"] = { objRot.x, objRot.y, objRot.z };
				modelJson["Scale"] = { objScale.x, objScale.y, objScale.z };

				modelJson["isBackFaceCulling"] = entityObjects[i]->component.flags.doCulling;
				modelJson["MaterialPath"] = entityObjects[i]->component.systems.material.Material.materialPath;
				modelJson["CastShadow"] = entityObjects[i]->component.flags.castsShadow;
				glm::vec2 uvScale = Scene::entityObjects[i]->component.systems.material.uvScale;
				modelJson["uvScale"] = { uvScale.x, uvScale.y };

				modelJson["drawInstanced"] = Scene::entityObjects[i]->component.render.drawInstanced;

				modelJson["doRender"] = entityObjects[i]->component.flags.render;
				modelJson["UUID"] = entityObjects[i]->UUIDstring;


				json scriptsArray = json::array();
				for (size_t x = 0; x < entityObjects[i]->ScriptObjects.size(); x++)
				{
					json scriptEntry;
					scriptEntry["name"] = entityObjects[i]->ScriptObjects[x]->name;
					scriptEntry["path"] = entityObjects[i]->ScriptObjects[x]->path;
					scriptEntry["UUID"] = UUID::UUIDToString(entityObjects[i]->ScriptObjects[x]->UUID);
					scriptsArray.push_back(scriptEntry);
				}
				modelJson["Scripts"] = scriptsArray;

				json relationshipArray = json::array();
				for (size_t x = 0; x < entityObjects[i]->component.relationship.childUUID.size(); x++)
				{
					json retaltionEntry;
					retaltionEntry["childUUID"] = UUID::UUIDToString(entityObjects[i]->component.relationship.childUUID[x]);
					relationshipArray.push_back(retaltionEntry);
				}
				modelJson["children"] = relationshipArray;
				modelJson["hasParent"] = entityObjects[i]->component.relationship.hasParent;
				modelJson["parentUUID"] = UUID::UUIDToString(entityObjects[i]->component.relationship.parentUUID);

				settingsData.push_back(modelJson);
			}

		}
		// Serialize each modelObject into JSON
		

		// Write to file
		std::ofstream outFile(path, std::ios::out);
		if (!outFile.is_open()) {
			std::cout << "Failed to write to " << path << std::endl;
			return;
		}

		outFile << settingsData.dump(4);  // Pretty-print with indentation
		outFile.close();

		std::cout << "Successfully updated " << path << std::endl;

	}
	catch (const std::exception& e) {
		std::cout << "Exception: " << e.what() << std::endl;
	}
}

void Scene::billBoardSave(std::string path) {
	try {
		json settingsData = json::array();  // New JSON array to hold model data

		// Serialize each modelObject into JSON
		for (size_t i = 0; i < entityObjects.size(); i++)
		{
			if (entityObjects[i]->type == entity::ENT_BILLBOARD_TYPE)
			{
				json BillBoardJson;
				BillBoardJson["name"] = entityObjects[i]->name;
				BillBoardJson["path"] = entityObjects[i]->path;

				BillBoardJson["doPitch"] = entityObjects[i]->component.render.BillBoard->doPitch;

				glm::vec3 objPos = entityObjects[i]->fetchPosition();
				glm::vec3 objScale = entityObjects[i]->fetchScale();

				BillBoardJson["position"] = { objPos.x, objPos.y, objPos.z };
				BillBoardJson["scale"] = { objScale.x, objScale.y, objScale.z };
				BillBoardJson["UUID"] = entityObjects[i]->UUIDstring;

				json relationshipArray = json::array();
				for (size_t x = 0; x < entityObjects[i]->component.relationship.childUUID.size(); x++)
				{
					json retaltionEntry;
					retaltionEntry["childUUID"] = UUID::UUIDToString(entityObjects[i]->component.relationship.childUUID[x]);
					relationshipArray.push_back(retaltionEntry);
				}
				BillBoardJson["children"] = relationshipArray;
				BillBoardJson["hasParent"] = entityObjects[i]->component.relationship.hasParent;
				BillBoardJson["parentUUID"] = UUID::UUIDToString(entityObjects[i]->component.relationship.parentUUID);

				settingsData.push_back(BillBoardJson);
			}
		}
		// Write to file
		std::ofstream outFile(path, std::ios::out);
		if (!outFile.is_open()) {
			std::cout << "Failed to write to " << path << std::endl;
			return;
		}

		outFile << settingsData.dump(4);  // Pretty-print with indentation
		outFile.close();

		std::cout << "Successfully updated " << path << std::endl;

	}
	catch (const std::exception& e) {
		std::cout << "Exception: " << e.what() << std::endl;
	}

}

void Scene::settingsSave(std::string path) {
	try {
		json SettingsData = json::array();  // New JSON array to hold model data

		json JsonSettings;

		//JsonSettings["skyRGBA"][0] = RenderClass::skyRGBA[0];
		//JsonSettings["skyRGBA"][1] = RenderClass::skyRGBA[1];
		//JsonSettings["skyRGBA"][2] = RenderClass::skyRGBA[2];

		JsonSettings["fogRGBA"][0] = RenderClass::fogRGBA[0];
		JsonSettings["fogRGBA"][1] = RenderClass::fogRGBA[1];
		JsonSettings["fogRGBA"][2] = RenderClass::fogRGBA[2];

		JsonSettings["doReflections"] = RenderClass::doReflections;
		JsonSettings["doFog"] = RenderClass::doFog;

		JsonSettings["DepthDistance"] = RenderClass::DepthDistance;
		JsonSettings["DepthPlane"][0] = RenderClass::DepthPlane[0];
		JsonSettings["DepthPlane"][1] = RenderClass::DepthPlane[1];

		JsonSettings["Window"] = windowHandler::s_WindowTitle;
		//JsonSettings["DefaultSkyboxPath"] = Skybox::DefaultSkyboxPath;

		SettingsData.push_back(JsonSettings);

		// Write to file
		// Write back to file
		std::ofstream outFile(path, std::ios::out);
		if (!outFile.is_open()) {
			std::cout << ("Failed to write to ") << path << std::endl;
		}

		outFile << SettingsData.dump(4);
		outFile.close();

		std::cout << "Successfully updated " << path << std::endl;

	}
	catch (const std::exception& e) {
		std::cout << "Exception: " << e.what() << std::endl;
	}
}

void Scene::cameraSettingsSave(std::string path) {
	try {
		json CameraData = json::array();  // New JSON array to hold model data

		json JsonCamera;
		JsonCamera["initialCameraPos"] = { Scene::initalCameraPos.x, Scene::initalCameraPos.y, Scene::initalCameraPos.z };
		JsonCamera["FOV"] = Scene::maincamera.fov;
		JsonCamera["nearPlane"] = Scene::maincamera.nearFar.x;
		JsonCamera["farPlane"] = Scene::maincamera.nearFar.y;
		JsonCamera["cameraColliderScale"][0] = Player::cameraColliderScale.x;
		JsonCamera["cameraColliderScale"][1] = Player::cameraColliderScale.y;
		JsonCamera["cameraColliderScale"][2] = Player::cameraColliderScale.z;
		JsonCamera["gamma"] = Scene::maincamera.gamma;

		CameraData.push_back(JsonCamera);

		// Write to file
		// Write back to file
		std::ofstream outFile(path, std::ios::out);
		if (!outFile.is_open()) {
			std::cout << ("Failed to write to ")<< path << std::endl;
		}

		outFile << CameraData.dump(4);
		outFile.close();

		std::cout << "Successfully updated " << path << std::endl;

	}
	catch (const std::exception& e) {
		std::cout << "Exception: " << e.what() << std::endl;
	}
}

void Scene::AddSceneSoundObject(std::string name, std::string path) {
	LogConsole::print("not implemented");
	SoundProgram nSoundProjram;
	nSoundProjram.ChangeSound(path);
	nSoundProjram.name = name;
	nSoundProjram.Set3D(true);
	SoundObjects.push_back(nSoundProjram);
}

uint64_t Scene::AddEntityObject(entity::ENT_TYPE_ENUM type, std::string name, std::string path, glm::vec3 spawnPosition, glm::vec3 spawnScale, glm::vec3 spawnRotation)
{
	std::unique_ptr<entity> newEntity = std::make_unique<entity>(); // Use std::make_unique
	
	//if (spawnNearCamera) newEntity->setPosition(maincamera.Position - ( FE_Math::getForwardFromViewMatrix(maincamera.cameraMatrix) * 5.0f ));
	newEntity->setPosition(spawnPosition);
	newEntity->setScale(spawnScale);
	newEntity->setRotation(spawnRotation);
	
	newEntity->create(type, name, path, "Assets/Material/Default.Material");
	
	uint64_t UUID = newEntity->UUID;
	
	entityObjects.emplace_back(std::move(newEntity));
	LogConsole::print("Created Entity: " + name);
	
	return UUID;
}

void Scene::billBoardLoad(std::string path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cout << "billboard Failed to open file: " << path << std::endl;
		return;
	}
	json BillBoardFileData;
	try {
		file >> BillBoardFileData;
	}
	catch (const nlohmann::json::parse_error& e) {
		// This catch block specifically handles JSON parsing errors,
		// which gives more precise error information from the library.
		std::cout << "JSON Parse Error loading BillBoard data: " << e.what() << std::endl;
		std::cout << "Error byte position: " << e.byte << std::endl; // Specific to nlohmann::json
	}
	catch (const std::ios_base::failure& e) {
		// This catch block handles file I/O errors (e.g., file not found, permission issues).
		std::cout << "File I/O Error loading BillBoard data: " << e.what() << std::endl;
	}
	catch (const std::exception& e) {
		// A general catch-all for any other std::exception derived errors.
		std::cout << "An unexpected error occurred loading BillBoard data: " << e.what() << std::endl;
	}
	file.close();

	for (const auto& item : BillBoardFileData) {
		std::unique_ptr<entity> newEntity = std::make_unique<entity>(); // Use std::make_unique

		//std::cout << "BillBoard doPitch: " << item.at("doPitch") <<std::endl;
		newEntity->setPosition(glm::vec3(item.at("position")[0], item.at("position")[1], item.at("position")[2]));
		newEntity->setScale(glm::vec3(item.at("scale")[0], item.at("scale")[1], item.at("scale")[2]));
		// IDs
		//newEntity->ID.UniqueNumber = item.at("IDuniqueIdentifier").get<unsigned int>();

		std::string name = item.at("name").get<std::string>();
		std::string nPath = item.at("path").get<std::string>();

		if (item.contains("children") && item["children"].is_array()) {
			for (const auto& childItem : item["children"]) {

				std::string childUUIDStr = childItem.at("childUUID").get<std::string>();
				uint64_t childUUID = UUID::StringToUUID(childUUIDStr);

				newEntity->component.relationship.childUUID.push_back(childUUID);
			}

		}

		if (item.contains("hasParent")) newEntity->component.relationship.hasParent = item.at("hasParent").get<bool>();

		uint64_t tempParentUUID = 0;
		if (item.contains("parentUUID")) tempParentUUID = UUID::StringToUUID(item.at("parentUUID").get<std::string>());
		newEntity->component.relationship.parentUUID = tempParentUUID;

		uint64_t tempUUID = UUID::StringToUUID(item.at("UUID").get<std::string>());

		newEntity->createwUUID(tempUUID, entity::ENT_BILLBOARD_TYPE, name, nPath, ""); // type, name, path, materialpath // add material path for bb later
		
		newEntity->component.render.BillBoard->doPitch = item.at("doPitch");
		entityObjects.push_back(std::move(newEntity)); // Add the configured object to the vector
	}
	std::cout << "Loaded Scene BillBoards from: " << path << std::endl;
}

void Scene::soundObjectLoad(std::string path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cout << "Failed to open file: " << path << std::endl;
		return;
	}
	json CubeColliderFileData;
	try {
		file >> CubeColliderFileData;
	}
	catch (const nlohmann::json::parse_error& e) {
		// This catch block specifically handles JSON parsing errors,
		// which gives more precise error information from the library.
		std::cout << "JSON Parse Error loading SoundObject data: " << e.what() << std::endl;
		std::cout << "Error byte position: " << e.byte << std::endl; // Specific to nlohmann::json
	}
	catch (const std::ios_base::failure& e) {
		// This catch block handles file I/O errors (e.g., file not found, permission issues).
		std::cout << "File I/O Error loading SoundObject data: " << e.what() << std::endl;
	}
	catch (const std::exception& e) {
		// A general catch-all for any other std::exception derived errors.
		std::cout << "An unexpected error occurred loading SoundObject data: " << e.what() << std::endl;
	}
	file.close();
	for (const auto& item : CubeColliderFileData) {

		SoundProgram newSoundObject;
		std::string path = item.at("path").get<std::string>();
		std::string name = item.at("name").get<std::string>();
		float pitch = item.at("pitch").get<float>();
		float volume = item.at("volume").get<float>();
		bool isLoop = item.at("isLoop").get<bool>();
		glm::vec3 SoundPosition = glm::vec3(item.at("SoundPosition")[0],
			item.at("SoundPosition")[1], item.at("SoundPosition")[2]);
		bool is3DSound = item.at("is3Dsound").get<bool>();

		newSoundObject.CreateSound(path, name);
		newSoundObject.SetPitch(pitch);
		newSoundObject.SetVolume(volume);
		newSoundObject.SetSoundPosition(SoundPosition);
		newSoundObject.Set3D(is3DSound); // seems the actual soundclass doesnt like this, so ill move on and fix this later down the line
		newSoundObject.loop = isLoop;
		SoundObjects.push_back(newSoundObject); // Add the configured object to the vector
	}
	std::cout << "Loaded Scene SoundObject from: " << path << std::endl;
}

void Scene::soundObjectSave(std::string path)
{
	try {
		json SoundData = json::array();  // New JSON array to hold model data

		// Serialize each modelObject into JSON
		for (const auto& obj : SoundObjects) {
			json SoundJson;
			SoundJson["name"] = obj.name;
			SoundJson["path"] = obj.path;
			SoundJson["pitch"] = obj.pitch;
			SoundJson["volume"] = obj.currentvolume;
			SoundJson["isLoop"] = obj.loop;
			SoundJson["SoundPosition"] = { obj.position.x, obj.position.y, obj.position.z };
			SoundJson["is3Dsound"] = obj.is3D;

			SoundData.push_back(SoundJson);
		}

		// Write to file
		std::ofstream outFile(path, std::ios::out);
		if (!outFile.is_open()) {
			std::cout << "Failed to write to " << path << std::endl;
			return;
		}

		outFile << SoundData.dump(4);  // Pretty-print with indentation
		outFile.close();

		std::cout << "Successfully updated " << path << std::endl;

	}
	catch (const std::exception& e) {
		std::cout << "Exception: " << e.what() << std::endl;
	}
}

void Scene::settingsLoad(std::string path) {
	std::ifstream engineDefaultFile(path);
	if (engineDefaultFile.is_open()) {
		json engineDefaultData;
		engineDefaultFile >> engineDefaultData;
		engineDefaultFile.close();

		//RenderClass::skyRGBA[0] = engineDefaultData[0]["skyRGBA"][0];
		//RenderClass::skyRGBA[1] = engineDefaultData[0]["skyRGBA"][1];
		//RenderClass::skyRGBA[2] = engineDefaultData[0]["skyRGBA"][2];

		RenderClass::fogRGBA[0] = engineDefaultData[0]["fogRGBA"][0];
		RenderClass::fogRGBA[1] = engineDefaultData[0]["fogRGBA"][1];
		RenderClass::fogRGBA[2] = engineDefaultData[0]["fogRGBA"][2];

		RenderClass::doReflections = engineDefaultData[0]["doReflections"];
		RenderClass::doFog = engineDefaultData[0]["doFog"];

		RenderClass::DepthDistance = engineDefaultData[0]["DepthDistance"];
		RenderClass::DepthPlane[0] = engineDefaultData[0]["DepthPlane"][0];
		RenderClass::DepthPlane[1] = engineDefaultData[0]["DepthPlane"][1];

		windowHandler::s_WindowTitle = engineDefaultData[0]["Window"];
		glfwSetWindowTitle(windowHandler::window, (windowHandler::s_WindowTitle).c_str());
		
		// window name needs to be set here

		//Skybox::DefaultSkyboxPath = engineDefaultData[0]["DefaultSkyboxPath"];
		//Skybox::LoadSkyBoxTexture(Skybox::DefaultSkyboxPath);
	}
	else {
		std::cerr << "Settings Failed to open " << path << std::endl;
	}
}

void Scene::cameraSettingsLoad(std::string path) {
	std::ifstream CameraFile(path);
	if (CameraFile.is_open()) {
		json CameraData;
		CameraFile >> CameraData;
		CameraFile.close();

		initalCameraPos = glm::vec3((CameraData[0]["initialCameraPos"][0]),
			(CameraData[0]["initialCameraPos"][1]),
			(CameraData[0]["initialCameraPos"][2])
		);
		Scene::maincamera.Position = Scene::initalCameraPos;
		float fov = CameraData[0]["FOV"].get<float>();
		//std::cout << fov << std::endl;
		float nearPlane = CameraData[0]["nearPlane"].get<float>();
		float farPlane = CameraData[0]["farPlane"].get<float>();
		Scene::maincamera.fov = fov;
		Scene::maincamera.nearFar.x = nearPlane;
		Scene::maincamera.nearFar.y = farPlane;
		Player::cameraColliderScale.x = CameraData[0]["cameraColliderScale"][0];
		Player::cameraColliderScale.y = CameraData[0]["cameraColliderScale"][1];
		Player::cameraColliderScale.z = CameraData[0]["cameraColliderScale"][2];
		Scene::maincamera.gamma = CameraData[0]["gamma"].get<float>();

	}
	else {
		std::cerr << "Camera Failed to open " << path << std::endl;
	}
}


void Scene::shadowmapDraw(){
	for (size_t i = 0; i < entityObjects.size(); i++){
		entityObjects[i]->drawShadowMap();
	}
}

void Scene::callAllScriptInit(){
	for (size_t i = 0; i < entityObjects.size(); i++){
		for (size_t x = 0; x < entityObjects[i]->ScriptObjects.size(); x++){
			entityObjects[i]->initScript(x);
		}
	}
}

void Scene::resetAllScripts(){
	for (size_t i = 0; i < entityObjects.size(); i++){
		for (size_t x = 0; x < entityObjects[i]->ScriptObjects.size(); x++){
			entityObjects[i]->reloadScript(x);
		}
	}
}

void Scene::onBeginningOfFrame(){
	// update prior transform
	for (size_t i = 0; i < entityObjects.size(); i++){
		entityObjects[i]->component.systems.previousTransformation = entityObjects[i]->component.systems.transformation;
	}
}

void Scene::draw() {
	if (dbgPass::overlayDebug){
		if (Collision::showBoxCollider){
			CubeVisualizer::draw(SceneBounds.position,
				SceneBounds.size, glm::vec3(1.0f, 0.0f, 0.0f),5.0, true, false);
		}
	}
	
	// entities shadow map should go above here
	for (size_t i = 0; i < entityObjects.size(); i++){
		entityObjects[i]->draw();
	}

	if (FEImGuiWindow::showViewportIcons) {
		for (size_t i = 0; i < SoundObjects.size(); i++) {
			if (SoundObjects[i].is3D)
			{
				SoundIcon->doPitch = true;
				SoundIcon->updatePosition(SoundObjects[i].position);
				SoundIcon->updateScale(glm::vec3(1));
				SoundIcon->draw();
			}
		}
		for (size_t i = 0; i < LightingHandler::Lights.size(); i++)
		{

			if (LightingHandler::Lights[i].type == 0) {
				SpotLightIcon->doPitch = true;
				SpotLightIcon->updatePosition(LightingHandler::Lights[i].position);
				SpotLightIcon->updateScale(glm::vec3(0.3f));
				SpotLightIcon->draw();
			}
			else if (LightingHandler::Lights[i].type) {
				PointLightIcon->doPitch = true;
				PointLightIcon->updatePosition(LightingHandler::Lights[i].position);
				PointLightIcon->updateScale(glm::vec3(0.3f));
				PointLightIcon->draw();
			}
		}
	}
}


void Scene::Update() {
	for (size_t i = 0; i < entityObjects.size(); i++){
		//entityObjects[i]->updateCollision();

		entityObjects[i]->update();
	}

	for (size_t i = 0; i < SoundObjects.size(); i++) {

		if (SoundObjects[i].is3D){
			SoundObjects[i].updateCameraPosition();
			SoundObjects[i].SetSoundPosition(SoundObjects[i].position);
		}
		
		if (!SoundObjects[i].isPlay && SoundObjects[i].loop || !SoundObjects[i].isPlay && SoundObjects[i].queuedPlay) {
			SoundObjects[i].PlaySound();
		}
	}
}

// update but on the work thread (expensive tasks)
void Scene::sceneUpdateWork(){
	if (ProbeHandler::dirtyScene){
		calculateSceneBounds();
	}
}

void Scene::Delete() {
	// deselect object probably
	FEImGuiWindow::SelectedObjectIndex = -1;
	FEImGuiWindow::SelectedObjectType = ' ';
	
	FlouraSWRT::wipeScene();
	
	for (size_t i = 0; i < entityObjects.size(); i++){
		entityObjects[i]->Delete(); }
	entityObjects.clear();

	for (size_t i = 0; i < SoundObjects.size(); i++) {
		if (SoundObjects[i].isPlay) {
			SoundObjects[i].StopSound();
		}
	}

	SoundObjects.clear();

	for (size_t i = 0; i < SoundObjects.size(); i++)
	{SoundObjects[i].DeleteSound();}
	SoundObjects.clear();

	LightingHandler::deleteScene();
	
	// no skybox delete for now

	if (FEImGuiWindow::imGuiEnabled) {
		FEImGuiWindow::ContentObjects.clear();
		FEImGuiWindow::ContentObjectNames.clear();
		FEImGuiWindow::ContentObjectPaths.clear();
		FEImGuiWindow::ContentObjectTypes.clear();

	}
}

void Scene::saveEntityState()
{
}

void Scene::restoreEntitiesToState()
{
}

void Scene::queuedDeletionLoop(){
	for (int i = 0; i < Scene::entityObjects.size(); ++i){
		if (Scene::entityObjects[i]->queuedForDeletion){
			Scene::entityObjects[i]->queuedDeletion();
			Scene::entityObjects.erase(Scene::entityObjects.begin() + i);
		}
	}
	Scene::entityDeletionUnderGoing = false;
}

Collision::HitResult Scene::traceIntoScene(glm::vec3 ro, glm::vec3 rd){
	Collision::HitResult hr;

	for (int i = 0; i < Scene::entityObjects.size(); ++i){
		
	}
	
	return hr;
}

void Scene::voxelizeArea(glm::vec3 p, glm::vec3 s, Texture3D& texture, int sliceSize, GLuint slot){
	std::vector<Mesh*> nMeshes;
	std::vector<glm::mat4> nMatrices;
	
	tagMeshes(p, s, nMeshes, nMatrices);
	
	/*
	std::vector<glm::vec3> positions;
	
	for (int i = 0; i < nMeshes.size(); ++i){
		for (int x = 0; x < nMeshes[i]->vertices.size(); ++x){
			positions.push_back(nMeshes[i]->vertices[x].position);
		}
	}
	std::cout << nMeshes.size() << std::endl;
	
	//Collision::AABB root = Collision::createAABBfromRubiksCubePoints(Collision::fetchFurthestPoints(positions));
	
	//std::cout<<root.position.x << " " << root.position.y << " " << root.position.z << " " << std::endl;
	//std::cout<<root.size.x << " " << root.size.y << " " << root.size.z << " " << std::endl;
	*/
	Collision::AABB root;
	root.position = p;
	root.size =  s;
	
	voxelizer::bakeMeshVXGAccelMeshPrims(nMeshes, nMatrices, root, sliceSize, texture, slot);
}

void Scene::tagMeshes(glm::vec3 p, glm::vec3 s, std::vector<Mesh*>& meshes, std::vector<glm::mat4>& matrices){
	for (int i = 0; i < entityObjects.size(); ++i){
		Collision::AABB& modelNode = entityObjects[i]->component.collider.modelNode;
		Collision::HitResult modelHR = Collision::AABBvsAABB(p, s, modelNode.position, modelNode.size);
		
		int mIndex = RenderHandler::fetchModelIndex(entityObjects[i]->component.render.renderID);
		
		// early meshnode and index edge case
		if (!modelHR.isColliding || mIndex < 0) continue;
		
		// compose global matrix
		glm::mat4 gMatix = FE_Math::composeMatrixWDegrees(
			entityObjects[i]->component.systems.transformation.position,
			entityObjects[i]->component.systems.transformation.scale, 
			entityObjects[i]->component.systems.transformation.rotation
			);
		
		// each mesh
		for (int x = 0; x < RenderHandler::models[mIndex].model->meshes.size(); ++x){
			// compose local matrix
			glm::mat4 lMatrix  = FE_Math::composeMatrixWDegrees(
				RenderHandler::models[mIndex].model->localTransformation[x].position,
				RenderHandler::models[mIndex].model->localTransformation[x].scale,
				RenderHandler::models[mIndex].model->localTransformation[x].rotation
				);
			// compose the final matrix
			glm::mat4 fMatrix = gMatix * lMatrix;
			
			// should do mesh node collison before building prims
			
			// can compose prims for a better tagging to speed up the voxelization step (fk it imma do it)
			std::vector<Vertex> nVertices = RenderHandler::models[mIndex].model->meshes[x].vertices;
			
			// transform vertices
			for (int y = 0; y< nVertices.size(); ++y)
				FE_Math::transformPoint(nVertices[y].position, fMatrix);
			
			// generate prims
			std::vector<accelerate::BVH_primitive> nPrims = accelerate::buildIndicesIntoPrims(nVertices,
				 RenderHandler::models[mIndex].model->meshes[x].indices);


			for (int y = 0; y < nPrims.size(); ++y){
				Collision::HitResult pHR = Collision::AABBvsAABB(p, s, nPrims[y].extents.position, nPrims[y].extents.size);
				if (pHR.isColliding){
					meshes.push_back(&RenderHandler::models[mIndex].model->meshes[x]);
					matrices.push_back(fMatrix);
					break;
				}
			}
		}
	}
	
}
