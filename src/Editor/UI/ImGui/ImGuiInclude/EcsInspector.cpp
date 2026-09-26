#include "EcsInspector.h"
#include "Editor/UI/ImGui/ImGuiInclude.h"
#include "Editor/UI/ImGui/FE_ImGui.h"
#include "Scene/LightingHandler.h"
#include "Scene/scene.h"
#include "utils/logConsole.h"
#include "Gameplay/Player.h"
#include <Render/Handler/RenderHandler.h>
#include <Systems/util/UUID.h>
#include <Core/File/File.h>
#include <Systems/util/relationshipManager.h>

static const char* lightTypes[]{ "Spotlight","Pointlight" };
static int SelectedLight = 0;

char EcsInspector::NameBuffer[256] = "New Object";

void EcsInspector::InspectorWindow() {

	ImGui::Begin("Inspector"); // ImGUI window creation
	ImGui::Text("Inspector");
	ImGui::Text(("Selected Object Type : " + FEImGuiWindow::SelectedObjectType).c_str());
	ImGui::Text(("Index: " + std::to_string(FEImGuiWindow::SelectedObjectIndex)).c_str());

	// remove this trash, replace with enum switch
	
	if (FEImGuiWindow::SelectedObjectType == "Camera") CameraWindow();
	else if (FEImGuiWindow::SelectedObjectType == "Model") ModelWindow();
	else if (FEImGuiWindow::SelectedObjectType == "Billboard") BillBoardWindow();
	else if (FEImGuiWindow::SelectedObjectType == "Sound") SoundWindow();
	else if (FEImGuiWindow::SelectedObjectType == "Light") LightWindow();
	else if (FEImGuiWindow::SelectedObjectType == "DirectLight") {
		

		ImGui::Checkbox("Enabled", &LightingHandler::doDirLight);
		ImGui::Checkbox("Enabled Specular Light", &LightingHandler::doDirSpecularLight);
		ImGui::Spacing();
		FEImGui::DragVec3("Rotation", LightingHandler::dirLightRot, glm::vec3(0.0f), 100.0f);
		ImGui::Spacing();
		ImGui::DragFloat("Ambient Light", &LightingHandler::directAmbient);
		ImGui::DragFloat("Specular Light", &LightingHandler::dirSpecularLight);
		ImGui::ColorEdit3("Colour", &LightingHandler::directLightCol.r);	// sky and light
		ImGui::Spacing();
		ImGui::Text("Directional Shadow");
		ImGui::Checkbox("Do ShadowMap", &LightingHandler::doDirShadowMap);
		ImGui::DragFloat2("ShadowMap Near & Far", &LightingHandler::dirNearFar.x);
		ImGui::DragFloat("Distance", &LightingHandler::distance);
		ImGui::DragFloat("Height", &LightingHandler::dirShadowheight);
		ImGui::DragInt("Hardness", &LightingHandler::dirShadowMapHardness);
		ImGui::DragInt("Samples", &LightingHandler::dirShadowMapSamples);
		ImGui::DragFloat("Bias", &LightingHandler::DirSMMaxBias);

		ImGui::Image((ImTextureID)(uintptr_t)LightingHandler::dirShadowMap, ImVec2(200, 200), ImVec2(0, 1), ImVec2(1, 0));

	}
	else if (FEImGuiWindow::SelectedObjectType == "Skybox") SkyBoxWindow();
	else if (FEImGuiWindow::SelectedObjectType == "Environment")
	{
		ImGui::Checkbox("doReflections", &RenderClass::doReflections);
		ImGui::Checkbox("renderENV", &RenderHandler::renderENV);
		ImGui::Spacing();
		ImGui::Checkbox("doFog", &RenderClass::doFog); 		//Toggles
		ImGui::ColorEdit3("fog RGBA", &RenderClass::fogRGBA.r);	// sky and light
		ImGui::Spacing();
		ImGui::Text("DepthBuffer Settings (FOG)");
		ImGui::DragFloat("Depth Distance (FOG)", &RenderClass::DepthDistance);
		ImGui::DragFloat2("Near and Far Depth Plane", RenderClass::DepthPlane);
	}
	else if (FEImGuiWindow::SelectedObjectType == "Empty") EmptyWindow();
	else if (FEImGuiWindow::SelectedObjectType == "Material") MaterialWindow();
	// these all need to be replaced with enums
	
	ImGui::End();
}
std::string scriptName = "New Script";
std::string scriptPath = "Assets/Scripts/helloWorld.Lua";

std::string UUIDinput = "";

void EcsInspector::ModelWindow() {

	ImGui::Spacing();
	ImGui::Text((Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->name).c_str());
	//ID
	ImGui::Text(("UUID: " + (Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->UUIDstring)).c_str());
	ImGui::SameLine();
	if (ImGui::Button("copy")) ImGui::SetClipboardText(Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->UUIDstring.c_str());

	ImGui::Text(("render UUID: " + (Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.render.renderIDString)).c_str());
	ImGui::Text(("Instance UUID: " + (Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.render.instanceIDString)).c_str());


	ImGui::InputText("##Name", EcsInspector::NameBuffer, sizeof(EcsInspector::NameBuffer));
	ImGui::SameLine();
	if (ImGui::SmallButton("Apply Name"))Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->name = EcsInspector::NameBuffer;

	if (ImGui::TreeNode("Rendering Component")) {

		ImGui::Combo("Material", &FEImGuiWindow::MaterialSelectedIndex, FEImGuiWindow::MaterialObjecNames.data(), static_cast<int>(FEImGuiWindow::MaterialObjecNames.size()));
		ImGui::Text(("Current Material: " + Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.systems.material.Material.materialPath).c_str());
		ImGui::Spacing();
		if (ImGui::SmallButton("Apply Material")) {
			Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->LoadMaterial(FEImGuiWindow::MaterialObjectPaths[FEImGuiWindow::MaterialSelectedIndex]); // for now im just gonna load the wobbly shader
			LogConsole::print("Material Applied: " + FEImGuiWindow::MaterialObjectPaths[FEImGuiWindow::MaterialSelectedIndex]);
		}
		if (ImGui::SmallButton("Reload Current Material")) {
			Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->LoadMaterial(Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.systems.material.Material.materialPath);
			LogConsole::print("Reloaded Material: " + Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.systems.material.Material.materialPath);
		}
		ImGui::Checkbox("doRender", &Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.flags.render);
		ImGui::Checkbox("Draw Instanced", &Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.render.drawInstanced);
		ImGui::Checkbox("Cast Shadow", &Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.flags.castsShadow);
		ImGui::DragFloat2("UV Scale", &Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.systems.material.uvScale.x);

		
		if (ImGui::TreeNode("Meshes")) {
			
			// should do materials here, they need an overvamp anyways, a manager and to be per mesh
			
			ImGui::TreePop();// Ends The ImGui Window
		}
		
		//smoothnessValue
		ImGui::TreePop();// Ends The ImGui Window
	}
	ImGui::Spacing();
	entityTransformPane();
	ImGui::Spacing();
	entityPhysicsPane();
	ImGui::Spacing();
	entityScriptPane();
	ImGui::Spacing();
	if (ImGui::TreeNode("Collider Component")) {
		ImGui::TreePop();// Ends The ImGui Window
	}
	ImGui::Spacing();
	if (ImGui::TreeNode("Culling Component")) {

		ImGui::Checkbox("isBackFaceCulling", &Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.flags.doCulling);

		ImGui::TreePop();// Ends The ImGui Window
	}
	ImGui::Spacing();
	entityRelationsPane();
	ImGui::Spacing();
	if (ImGui::TreeNode("General Infomation:")) {
		int index = RenderHandler::fetchModelIndex(Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.render.renderID);
		if (index != -1)
		{
			ImGui::Text(("Material: " + Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.systems.material.Material.materialPath).c_str());
			std::string meshsize = "nummesh: " + std::to_string(RenderHandler::models[index].model->meshes.size());
			ImGui::Text(meshsize.c_str());
			
			ImGui::Text(("totalVertices: " + std::to_string(RenderHandler::models[index].model->totalVertices) ).c_str() );
			ImGui::Text(("totalIndices: " + std::to_string(RenderHandler::models[index].model->totalIndices) ).c_str() );
			ImGui::Text(("totalBones: " + std::to_string(RenderHandler::models[index].model->totalBones) ).c_str() );

			ImGui::Text("mesh names:");
			for (size_t i = 0; i < RenderHandler::models[index].model->meshes.size(); i++)
			{
				std::string meshNames = RenderHandler::models[index].model->meshes[i].name;
				ImGui::Text(meshNames.c_str());
			}
		}
		ImGui::TreePop();// Ends The ImGui Window
	}
	ImGui::Spacing();
	if (ImGui::SmallButton("Delete")) {
		Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->Delete();
		//ObjectManager::deleteObject('m', Scene::modelObjects[FEImGuiWindow::SelectedObjectIndex]->ID.UniqueNumber);
		//Scene::modelObjects[FEImGuiWindow::SelectedObjectIndex]->Delete();
		//Scene::modelObjects.erase(Scene::modelObjects.begin() + FEImGuiWindow::SelectedObjectIndex);
		FEImGuiWindow::SelectedObjectType = "";
		return;
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Duplicate")) {
		//ObjectManager::duplicateObject('o', Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->ID.UniqueNumber);
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Focus Camera")) {
		Scene::maincamera.Position = Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->fetchPosition();
	}
}
void EcsInspector::BillBoardWindow() {

	glm::vec3 bPos = Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->fetchPosition();
	glm::vec3 bScale = Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->fetchScale();

	ImGui::Text((Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->name).c_str());


	ImGui::Text(("UUID: " + (Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->UUIDstring)).c_str());
	ImGui::SameLine();
	if (ImGui::Button("copy")) ImGui::SetClipboardText(Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->UUIDstring.c_str());

	ImGui::InputText("##Name", EcsInspector::NameBuffer, sizeof(EcsInspector::NameBuffer));
	ImGui::SameLine();
	if (ImGui::SmallButton("Apply Name"))Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->name = EcsInspector::NameBuffer;
	
	ImGui::Spacing();
	entityTransformPane();
	ImGui::Spacing();
	entityPhysicsPane();
	ImGui::Spacing();
	entityScriptPane();
	ImGui::Spacing();

	ImGui::Checkbox("doPitch", &Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.render.BillBoard->doPitch);


	ImGui::Spacing();
	if (ImGui::SmallButton("Delete")) {
		Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->Delete();
		FEImGuiWindow::SelectedObjectType = "";
		// skip loop
		return;
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Duplicate")) {
		//ObjectManager::duplicateObject('o', Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->ID.UniqueNumber); // should be o
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Focus Camera")) {
		Scene::maincamera.Position = bPos;
	}

	Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->setPosition(bPos);
	Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->setScale(bScale);


}

void EcsInspector::SoundWindow()
{
	//Scene::SoundObjects
	ImGui::Text((Scene::SoundObjects[FEImGuiWindow::SelectedObjectIndex].name).c_str());
	ImGui::InputText("##Name", EcsInspector::NameBuffer, sizeof(EcsInspector::NameBuffer));
	ImGui::SameLine();
	if (ImGui::SmallButton("Apply Name")) Scene::SoundObjects[FEImGuiWindow::SelectedObjectIndex].name = EcsInspector::NameBuffer;

	ImGui::DragFloat3("Position", &Scene::SoundObjects[FEImGuiWindow::SelectedObjectIndex].position.x);
	ImGui::DragFloat("Volume", &Scene::SoundObjects[FEImGuiWindow::SelectedObjectIndex].currentvolume, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Pitch", &Scene::SoundObjects[FEImGuiWindow::SelectedObjectIndex].pitch, 0.01f, 0.1f, 2.0f);

	ImGui::Checkbox("Loop", &Scene::SoundObjects[FEImGuiWindow::SelectedObjectIndex].loop);
	ImGui::Checkbox("Play", &Scene::SoundObjects[FEImGuiWindow::SelectedObjectIndex].queuedPlay);
	ImGui::Checkbox("Is3D", &Scene::SoundObjects[FEImGuiWindow::SelectedObjectIndex].is3D);

	if (ImGui::SmallButton("Delete")) {
		Scene::SoundObjects[FEImGuiWindow::SelectedObjectIndex].DeleteSound();
		Scene::SoundObjects.erase(Scene::SoundObjects.begin() + FEImGuiWindow::SelectedObjectIndex);
		FEImGuiWindow::SelectedObjectType = "";
	}
}

void EcsInspector::LightWindow() {

	//ID
	//ImGui::Text(("ID: " + std::to_string(LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].ID.ObjType) + "*" + std::to_string(LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].ID.UniqueNumber)).c_str());
	// Index attached to ID
	//ImGui::Text(("ID Attached Index: " + std::to_string(LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].ID.index)).c_str());

	FEImGui::DragVec3("Position", LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].position, glm::vec3(0.0f), 100.0f);
	FEImGui::DragVec3("Rotation", LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].rotation, glm::vec3(0.0f), 100.0f);
	ImGui::ColorEdit3("Color", &LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].colour.x);
	ImGui::DragFloat("Radius", &LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].radius, 0.1f);
	ImGui::Combo("LightType", &LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].type, lightTypes, IM_ARRAYSIZE(lightTypes));
	ImGui::Spacing();

	//ImGui::Checkbox("Enabled", &Scene::LightObjectList[ImGuiWindow::SelectedObjectIndex].enabled);
	if (LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].enabled == 0) {
		if (ImGui::SmallButton("Enable")) {
			LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].enabled = !LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].enabled;
		}
	}
	else if (LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].enabled == 1) {
		if (ImGui::SmallButton("Disable")) {
			LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].enabled = !LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].enabled;
		}
	}
	//std::cout << Scene::LightObjectList[ImGuiWindow::SelectedObjectIndex].enabled << std::endl;

	//ImGui::Checkbox("Enabled", &Scene::enabled[ImGuiWindow::SelectedObjectIndex]);

	if (ImGui::SmallButton("Delete")) {
		//LightingHandler::Lights.erase(LightingHandler::Lights.begin() + FEImGuiWindow::SelectedObjectIndex);
		LightingHandler::deleteLight(FEImGuiWindow::SelectedObjectIndex);
		FEImGuiWindow::SelectedObjectIndex = 0; // reset index
		FEImGuiWindow::SelectedObjectType = "";
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Duplicate")) {
		//ObjectManager::duplicateObject('l', LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].ID.UniqueNumber);
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Focus Camera")) {
		Scene::maincamera.Position = LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].position;
	}
}
char SkyBoxPath[128] = "Assets/Skybox/";
void EcsInspector::SkyBoxWindow() {
	ImGui::ColorEdit3("Sky Colour", &RenderClass::skyRGBA.r);
	FEImGui::DragVec3("Skybox Rotation", Skybox::rotation, Scene::initalCameraPos, 100.0f);
	ImGui::Checkbox("Do Sky Colour", &Skybox::DoSbRGBA);
	ImGui::Checkbox("Render Skybox", &RenderClass::renderSkybox);
	ImGui::InputText("Skybox Path", SkyBoxPath, IM_ARRAYSIZE(SkyBoxPath));
	if (ImGui::Button("Load Skybox")) {
		Skybox::DefaultSkyboxPath = SkyBoxPath;
		Skybox::LoadSkyBoxTexture(SkyBoxPath);
	}
}

void EcsInspector::CameraWindow() {

	if (ImGui::TreeNode("Transform Component")) {
		ImGui::Text("Transformations: ");


		FEImGui::DragVec3("Camera Position", Scene::maincamera.Position, Scene::initalCameraPos, 100.0f);

		FEImGui::DragVec3("Initial Camera Position", Scene::initalCameraPos, glm::vec3(0.0f), 100.0f);

		//ImGui::DragFloat3("inital Camera Position", &Scene::initalCameraPos.x); // set inital cam pos
		if (ImGui::SmallButton("Reset Camera Position")) {
			Scene::maincamera.Position = Scene::initalCameraPos;
		} // reset cam pos
		ImGui::DragFloat("Camera Speed", &Scene::maincamera.s_scrollSpeed); //Camera

		FEImGui::DragVec3("Camera Collider Scale", Player::cameraColliderScale, glm::vec3(0.0f), 100.0f);

		ImGui::TreePop();// Ends The ImGui Window
	}
	ImGui::Spacing();
	if (ImGui::TreeNode("Settings Component")) {
		ImGui::Text("Settings: ");
		//sensitivity
		ImGui::DragFloat2("Camera Sensitivity", &Scene::maincamera.sensitivity.x);
		ImGui::Spacing();
		ImGui::DragFloat("FOV", &Scene::maincamera.fov, 0.1f, 160.0f); //FOV
		ImGui::DragFloat("Gamma", &Scene::maincamera.gamma);
		ImGui::DragFloat2("Near and Far Plane", &Scene::maincamera.nearFar.x); // Near and FarPlane

		ImGui::TreePop();// Ends The ImGui Window
	}
	ImGui::Spacing();
	if (ImGui::TreeNode("Collision Component")) {
		ImGui::Text("Collisions: ");
		ImGui::Text(("Foot Collision: " + std::to_string(Player::isColliding)).c_str());
		ImGui::Checkbox("CollideWithCamera: ", &Player::CollideWithCamera);
		ImGui::Checkbox("DoGravity: ", &Player::s_DoGravity);

		ImGui::TreePop();// Ends The ImGui Window
	}
}

void EcsInspector::EmptyWindow()
{

	
	ImGui::Spacing();
	ImGui::Text((Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->name).c_str());
	//ID
	ImGui::Text(("UUID: " + (Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->UUIDstring)).c_str());
	ImGui::SameLine();
	if (ImGui::Button("copy")) ImGui::SetClipboardText(Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->UUIDstring.c_str());
	
	ImGui::InputText("##Name", EcsInspector::NameBuffer, sizeof(EcsInspector::NameBuffer));
	ImGui::SameLine();
	if (ImGui::SmallButton("Apply Name"))Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->name = EcsInspector::NameBuffer;
	
	ImGui::Spacing();
	entityTransformPane();
	ImGui::Spacing();
	entityPhysicsPane();
	ImGui::Spacing();
	entityScriptPane();
	ImGui::Spacing();
}

static std::string materialType = "Standard";
static std::string materialName = "placeholder";

void EcsInspector::MaterialOnHit()
{
	
}

void EcsInspector::MaterialWindow()
{
	ImGui::Text(("Name: "  + FEImGuiWindow::ContentObjectNames[FEImGuiWindow::SelectedObjectIndex] ).c_str());
	
	std::vector<char> materialTypeBuffer(materialType.size() + 256);
	strncpy(materialTypeBuffer.data(), materialType.c_str(), materialTypeBuffer.size());
	materialTypeBuffer[materialTypeBuffer.size() - 1] = '\0';
	if (ImGui::InputText("Type", materialTypeBuffer.data(), materialTypeBuffer.size()))
	{
		materialType = std::string(materialTypeBuffer.data());
	}
	
	std::vector<char> materialNameBuffer(materialName.size() + 256);
	strncpy(materialNameBuffer.data(), materialName.c_str(), materialNameBuffer.size());
	materialNameBuffer[materialNameBuffer.size() - 1] = '\0';
	if (ImGui::InputText("Name", materialNameBuffer.data(), materialNameBuffer.size()))
	{
		materialName = std::string(materialNameBuffer.data());
	}
	
	ImGui::Spacing();
	ImGui::Text("Paths:");
	
	
	
	ImGui::Spacing();
	ImGui::Text("Variables:");
	
}

void EcsInspector::entityScriptPane()
{
		if (ImGui::TreeNode("Scripting Components")) {


		std::vector<char> nameBuffer(scriptName.size() + 256);
		strncpy(nameBuffer.data(), scriptName.c_str(), nameBuffer.size());
		nameBuffer[nameBuffer.size() - 1] = '\0';
		if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size()))
		{
			scriptName = std::string(nameBuffer.data());
		}

		std::vector<char> pathBuffer(scriptPath.size() + 256);
		strncpy(pathBuffer.data(), scriptPath.c_str(), pathBuffer.size());
		pathBuffer[pathBuffer.size() - 1] = '\0';
		if (ImGui::InputText("Path", pathBuffer.data(), pathBuffer.size()))
		{
			scriptPath = std::string(pathBuffer.data());
		}

		if (ImGui::ImageButton("##plusIcon", (ImTextureID)FEImGuiWindow::plusIcon.ID, ImVec2(10, 10))) 
		{
			Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->addScript(scriptPath, scriptName);
		}

		for (size_t i = 0; i < Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->ScriptObjects.size(); i++)
		{

			if (ImGui::ImageButton(("##crossIcon" + std::to_string(i)).c_str(), (ImTextureID)FEImGuiWindow::crossIcon.ID, ImVec2(10, 10)))
			{
				Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->removeScript(i);
				break;
			}
			ImGui::SameLine();
			// entity::reloadScript
			
			if (ImGui::TreeNode((Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->ScriptObjects[i]->name + " #" + std::to_string(i)).c_str()))
			{
				std::string id = UUID::UUIDToString(Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->ScriptObjects[i]->UUID);
				ImGui::Text(("UUID: " + id).c_str());
				ImGui::Text(("path: " + Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->ScriptObjects[i]->path).c_str());

				if (ImGui::Button("reload script"))
				{
					Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->reloadScript(i);
				}
				if (ImGui::Button("Open in text editor"))
				{
					FileClass::currentPath = Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->ScriptObjects[i]->path;
					FileClass::loadContents();
				}

				ImGui::TreePop();
			}


		}

		ImGui::TreePop();
	}
}

void EcsInspector::entityPhysicsPane()
{
	if (ImGui::TreeNode("Physics Component")) {

		ImGui::Checkbox("has Dynamics ", &Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.physobject.hasRigidbody);

		ImGui::Checkbox("affected By Gravity", &Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.physobject.affectedByGravity);

		if (ImGui::SmallButton("Clear Motion"))
		{
			Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.physobject.velocity = glm::vec3(0.0f);
		}

		ImGui::TreePop();// Ends The ImGui Window
	}
}

void EcsInspector::entityTransformPane()
{
	if (ImGui::TreeNode("Transform Component")) {
		ImGui::Text("Transformations: ");
		glm::vec3 modelPos = Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->fetchPosition();
		glm::vec3 modelScale = Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->fetchScale();
		glm::vec3 modelRot = Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->fetchRotation();
		
		// position
		FEImGui::DragVec3("Position", modelPos, glm::vec3(0.0f), 100.0f);
		FEImGui::DragVec3("Scale", modelScale, glm::vec3(1.0f), 100.0f); 
		FEImGui::DragVec3("Rotation", modelRot, glm::vec3(0.0f), 100.0f);
		
		Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->setPosition(modelPos);
		Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->setScale(modelScale);
		Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->setRotation(modelRot);
		ImGui::TreePop();// Ends The ImGui Window
	}
}

void EcsInspector::entityRelationsPane()
{
	if (ImGui::TreeNode("Relationship Component")) {

		
		FEImGui::stringInputTextBox("UUIDinput", UUIDinput);

		if (ImGui::Button("Add Parent"))
		{
			if (!UUIDinput.empty()) RelationshipManager::addParent(FEImGuiWindow::SelectedObjectIndex, UUID::StringToUUID(UUIDinput));
		}

		if (ImGui::Button("Remove Parent"))
		{
			RelationshipManager::removeParent(FEImGuiWindow::SelectedObjectIndex);
		}
		ImGui::Spacing();
		ImGui::Text(("HasParent: " + std::to_string(Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.relationship.hasParent)).c_str());
		ImGui::Text(("Parent UUID: " + std::to_string(Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.relationship.parentUUID)).c_str());
		ImGui::Spacing();
		ImGui::Text(("NumChildren: " + std::to_string(Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.relationship.childUUID.size())).c_str());
		ImGui::Text("Child UUIDs:");
		for (size_t i = 0; i < Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.relationship.childUUID.size(); i++)
		{
			ImGui::Text((std::to_string(Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->component.relationship.childUUID[i]) + " num: " + std::to_string(i)).c_str());
		}

		ImGui::TreePop();
	}	
}
