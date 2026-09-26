#include "LightingHandler.h"
#include <Render/Handler/RenderClass.h>
#include <glm/gtx/euler_angles.hpp>
#include <utils/logConsole.h>
#include "Scene/scene.h"
#include "utils/FE_math.h"

std::vector<LightingHandler::Light> LightingHandler::Lights;

void LightingHandler::clearSMFBO(){
	if (framePause) return;
	glBindFramebuffer(GL_FRAMEBUFFER, LightingHandler::shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, LightingHandler::shadowMapFBO2);
	glClear(GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, LightingHandler::shadowMapFBO3);
	glClear(GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

// DIR light
glm::vec3 LightingHandler::dirLightRot = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 LightingHandler::dirLightPosOut = glm::vec3(0.0f, 1.0f, 0.0f);;
glm::vec3 LightingHandler::directLightCol = glm::vec3(1.0f, 1.0f, 1.0f);
float LightingHandler::directAmbient = 0.20f;
float LightingHandler::dirSpecularLight = 0.20f;
float LightingHandler::DirSMMaxBias = 0.005f;
bool LightingHandler::doDirLight = false;
bool LightingHandler::doDirSpecularLight = true;

unsigned int LightingHandler::shadowMapFBO, LightingHandler::shadowMapHeight, LightingHandler::shadowMapWidth, LightingHandler::dirShadowMap;
unsigned int LightingHandler::shadowMapFBO2, LightingHandler::shadowMapHeight2, LightingHandler::shadowMapWidth2, LightingHandler::dirShadowMap2;
unsigned int LightingHandler::shadowMapFBO3, LightingHandler::shadowMapHeight3, LightingHandler::shadowMapWidth3, LightingHandler::dirShadowMap3;

float LightingHandler::distance = 35.0f;
glm::vec2 LightingHandler::dirNearFar = glm::vec2(0.1f, 75.0f); // 0.1f 75.0f
float LightingHandler::dirShadowheight = 20.0f;
bool LightingHandler::doDirShadowMap = true;
int LightingHandler::dirShadowMapHardness = 2;
int LightingHandler::dirShadowMapSamples = 1;

bool LightingHandler::framePause = false;
glm::mat4 LightingHandler::lightProjection;
Shader LightingHandler::dirShadowMapProgram;
Shader LightingHandler::dirShadowMapProgramBB;


void createSM(unsigned int & FBO, unsigned int & smTEX, unsigned int w, unsigned int h){
	glGenFramebuffers(1, &FBO);
	glGenTextures(1, &smTEX);
	glBindTexture(GL_TEXTURE_2D, smTEX);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	float clampColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, clampColor);

	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, smTEX, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void LightingHandler::setupShadowMapBuffer() {
	LightingHandler::dirShadowMapProgram.LoadShader("Assets/Shaders/Lighting/shadowMap.vert", "Assets/Shaders/Lighting/shadowMap.frag");

	// billboard
	LightingHandler::dirShadowMapProgramBB.LoadShader("Assets/Shaders/Db/BillBoardSM.vert", "Assets/Shaders/Db/BillBoardSM.frag");

//	shadowMapWidth = 2046;
//	shadowMapHeight = 2046;
	shadowMapHeight2 = 1024;
	shadowMapWidth2 =  1024;
	shadowMapHeight3 = 512;
	shadowMapWidth3 = 512;
	
	shadowMapWidth = 4096;
	shadowMapHeight = 4096;
	
	createSM(shadowMapFBO, dirShadowMap, shadowMapWidth, shadowMapHeight);
	//createSM(shadowMapFBO2, dirShadowMap2, shadowMapWidth2, shadowMapHeight2);
	//createSM(shadowMapFBO3, dirShadowMap3, shadowMapWidth3, shadowMapHeight3);
	
	return;

}

void drawSM(unsigned int & FBO, unsigned int w, unsigned int h)
{
	
}

void LightingHandler::drawShadowMap(Model*& model, int drawIndex) {
	if (!doDirShadowMap || framePause) {return;}

	glm::mat4 orthgonalProjection = glm::ortho(-distance, distance, -distance, distance, dirNearFar.x, dirNearFar.y); // last two are near and far 

	glm::vec3 CameraPos = Scene::maincamera.Position;
	glm::vec3 dirLightDirection = dirLightPosOut;
	glm::vec3 lightEyePosition = CameraPos + (dirShadowheight * dirLightDirection);
	glm::mat4 lightView = glm::lookAt(lightEyePosition, CameraPos, glm::vec3(0.0f, 1.0f, 0.0f));
	lightProjection = orthgonalProjection * lightView;


	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glViewport(0, 0, shadowMapWidth, shadowMapHeight); // dont touch for now
	
	LightingHandler::dirShadowMapProgram.Activate();
	glUniformMatrix4fv(glGetUniformLocation(LightingHandler::dirShadowMapProgram.ID, "lightProjection"), 1, GL_FALSE, glm::value_ptr(lightProjection));
	
	LightingHandler::dirShadowMapProgram.setBool("doBinaryAlpha", RenderClass::doBinaryAlpha);
	LightingHandler::dirShadowMapProgram.setBool("animateBinaryAlpha", RenderClass::animateBinaryAlpha);
	LightingHandler::dirShadowMapProgram.setHandleui64ARB("bayerMatrixHandle", RenderClass::bayermatrix->handle);
	LightingHandler::dirShadowMapProgram.setHandleui64ARB("BlueNoiseHandle", RenderClass::bluenoise->handle);
	LightingHandler::dirShadowMapProgram.setInt("drawIndex", drawIndex);
	
	
	dirShadowMapProgram.setTimeVariables();
	
	model->childrenRangeCull(CameraPos, dirNearFar.y);
	
	//bool tdld = model->doLodsDraw;
	//int tfll = model->forceLodLevel;
	
	//model->doLodsDraw = true;
	//model->forceLodLevel = 3;
	model->draw(LightingHandler::dirShadowMapProgram, Scene::maincamera);
	
	//model->doLodsDraw = tdld;
	//model->forceLodLevel = tfll;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, viewport[2], viewport[3]);
}

void LightingHandler::drawShadowMapBillboard(BillBoard*& bilboard, glm::vec3 translation, glm::vec3 scale) {
	if (!doDirShadowMap || framePause) {return;}

	
	glm::mat4 orthgonalProjection = glm::ortho(-distance, distance, -distance, distance, dirNearFar.x, dirNearFar.y); // last two are near and far 

	glm::vec3 CameraPos = Scene::maincamera.Position;
	glm::vec3 dirLightDirection = dirLightPosOut;
	glm::vec3 lightEyePosition = CameraPos + (dirShadowheight * dirLightDirection);
	glm::mat4 lightView = glm::lookAt(lightEyePosition, CameraPos, glm::vec3(0.0f, 1.0f, 0.0f));
	lightProjection = orthgonalProjection * lightView;

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glViewport(0, 0, shadowMapWidth, shadowMapHeight); // dont touch for now
	//glClear(GL_DEPTH_BUFFER_BIT);
	LightingHandler::dirShadowMapProgramBB.Activate();
	glUniformMatrix4fv(glGetUniformLocation(LightingHandler::dirShadowMapProgramBB.ID, "lightProjection"), 1, GL_FALSE, glm::value_ptr(lightProjection));

	//glCullFace(GL_FRONT);
	bilboard->drawShadowMap();
	//glCullFace(GL_BACK);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//Framebuffer::setViewportToViewPortResolution();
	glViewport(0, 0, viewport[2], viewport[3]);

}

void LightingHandler::sendToShader(Shader Shader){
	Shader.Activate();

	Shader.setFloat4("skyColor", glm::vec4(RenderClass::skyRGBA, 1.0f));
	Shader.setBool("doReflect", RenderClass::doReflections);


	int activeLightIndex = 0;

	for (size_t i = 0; i < Lights.size(); ++i){
		if (Lights[i].enabled){
			std::string uniformName = "Lights[" + std::to_string(activeLightIndex) + "].";
			Shader.setFloat3((uniformName + "position").c_str(), Lights[i].position);

			glm::vec3 baseDirection = glm::vec3(0, 0, -1);
			glm::vec3 eulerDegrees = Lights[i].rotation;
			glm::vec3 eulerRadians = glm::radians(eulerDegrees);
			glm::mat4 rotationMatrix = glm::yawPitchRoll(eulerRadians.y, eulerRadians.x, eulerRadians.z);
			glm::vec3 rotatedDirection = glm::vec3(rotationMatrix * glm::vec4(baseDirection, 0.0f));

			Shader.setFloat3((uniformName + "rotation").c_str(), rotatedDirection);

			Shader.setFloat3((uniformName + "colour").c_str(), Lights[i].colour);
			Shader.setFloat((uniformName + "radius").c_str(), Lights[i].radius);
			Shader.setInt((uniformName + "type").c_str(), Lights[i].type);

			activeLightIndex++;
		}

	}

	Shader.setFloat("DepthDistance", RenderClass::DepthDistance);
	Shader.setFloat("FogNearPlane", RenderClass::DepthPlane[0]);
	Shader.setFloat("FogFarPlane", RenderClass::DepthPlane[1]);
	Shader.setBool("doFog", RenderClass::doFog);
	Shader.setFloat3("fogColour", RenderClass::fogRGBA);
	Shader.setFloat("NearPlane", Scene::maincamera.nearFar.x);
	Shader.setFloat("FarPlane", Scene::maincamera.nearFar.y);
	Shader.setInt("lightCount", activeLightIndex);

	// directional light
	glm::vec3 baseDirectionDIR = glm::vec3(0, 0, -1);
	glm::vec3 eulerDegreesDIR = dirLightRot;
	glm::vec3 eulerRadiansDIR = glm::radians(eulerDegreesDIR);
	glm::mat4 rotationMatrixDIR = glm::yawPitchRoll(eulerRadiansDIR.y, eulerRadiansDIR.x, eulerRadiansDIR.z);
	glm::vec3 rotatedDirectionDIR = glm::vec3(rotationMatrixDIR * glm::vec4(baseDirectionDIR, 0.0f));

	dirLightPosOut = rotatedDirectionDIR;

	Shader.setBool("doDirLight", doDirLight);
	Shader.setBool("doDirSpecularLight", doDirSpecularLight);
	Shader.setFloat("directAmbient", directAmbient);
	Shader.setFloat("dirSpecularLight", dirSpecularLight);
	Shader.setFloat3("directLightPos", rotatedDirectionDIR); // 0.0f, 1.0f, 0.0f
	//
	Shader.setFloat3("directLightCol", directLightCol); // 1.0f, 1.0f, 1.0f

	//DirSMMaxBias
	Shader.setFloat("doDirShadowMap", doDirShadowMap);
	if (doDirShadowMap)
	{
		// shadow map
		Shader.setFloat("DirSMMaxBias", DirSMMaxBias);
		Shader.setInt("FilterRadius", dirShadowMapHardness);
		Shader.setInt("NumberOfSamples", dirShadowMapSamples);
		glUniformMatrix4fv(glGetUniformLocation(Shader.ID, "lightProjection"), 1, GL_FALSE, glm::value_ptr(lightProjection));

		glActiveTexture(GL_TEXTURE0 + 8);
		glBindTexture(GL_TEXTURE_2D, dirShadowMap);
		glUniform1i(glGetUniformLocation(Shader.ID, "shadowMap"), 8);
	}
	
}

float smAccum = 0.0;
float smAccumthresh = 1.0 / 30.0f;


// different thresh for different ranges, + only update on dirty 
void LightingHandler::update(){
	smAccum += TimeUtil::deltatime;
	if (smAccum > smAccumthresh){
		
		framePause = false;
		smAccum = 0.0;
	}
	else{ framePause = true; }

}

void LightingHandler::createLight(){
	Light tempLight;
	
	tempLight.position = glm::vec3(0);
	tempLight.rotation = glm::vec3(0);
	tempLight.colour = glm::vec3(1);
	tempLight.radius = 100.0f;
	tempLight.type = 0;
	tempLight.enabled = true;

	Lights.push_back(tempLight);

	raytracer::RTGlobalTransformFlag = true;

	LogConsole::print("Created LightObject");
}

void LightingHandler::deleteLight(int index){
	Lights.erase(Lights.begin() + index);
	raytracer::RTGlobalTransformFlag = true;
	LogConsole::print("Deleted LightObject at index: " + std::to_string(index));

	//IdManager::lowestLightIndexSync(); // sync up the index after deletion because the array has now changed
}

void LightingHandler::loadScene(std::string path){
	LightingHandler::deleteScene();

	std::ifstream file(path);
	if (!file.is_open()) {
		std::cout << "LightingHandler Failed to open file: " << path << std::endl;
		return;
	}
	json LightObjectFileData;
	try {
		file >> LightObjectFileData;
	}
	catch (const nlohmann::json::parse_error& e) {
		// This catch block specifically handles JSON parsing errors,
		// which gives more precise error information from the library.
		std::cout << "JSON Parse Error loading LightObject data: " << e.what() << std::endl;
		std::cout << "Error byte position: " << e.byte << std::endl; // Specific to nlohmann::json
	}
	catch (const std::ios_base::failure& e) {
		// This catch block handles file I/O errors (e.g., file not found, permission issues).
		std::cout << "File I/O Error loading LightObject data: " << e.what() << std::endl;
	}
	catch (const std::exception& e) {
		// A general catch-all for any other std::exception derived errors.
		std::cout << "An unexpected error occurred loading LightObject data: " << e.what() << std::endl;
	}
	file.close();

	for (const auto& item : LightObjectFileData) {

		glm::vec3 newPosition = glm::vec3(item.at("position")[0], item.at("position")[1], item.at("position")[2]);
		glm::vec3 newRotation = glm::vec3(item.at("rotation")[0], item.at("rotation")[1], item.at("rotation")[2]);
		glm::vec3 newColour = glm::vec3(item.at("colour")[0], item.at("colour")[1], item.at("colour")[2]);
		float radius = item.at("radius");
		std::string newType = item.at("type").get<std::string>();
		bool newEnabled = item.at("enabled").get<bool>();

		Light tempLight;

		tempLight.position = newPosition;
		tempLight.rotation = newRotation;
		tempLight.colour = newColour;
		tempLight.radius = radius;

		if (newType == "pointlight" || newType == "Pointlight") {
			tempLight.type = 1;
		}
		else if (newType == "spotlight" || newType == "Spotlight") {
			tempLight.type = 0;
		}

		tempLight.enabled = newEnabled;

		Lights.push_back(tempLight);


		// pushback here
	}
	std::cout << "Loaded Scene LightObject from: " << path << std::endl;
}

void LightingHandler::saveScene(std::string path)
{
	try {
		json lightData = json::array();  // New JSON array to hold model data

		int iteration = 0;
		// Serialize each modelObject into JSON
		for (const auto& Write : LightingHandler::Lights) {
			json LightJson;

			LightJson["position"] = { LightingHandler::Lights[iteration].position.x,  LightingHandler::Lights[iteration].position.y,  LightingHandler::Lights[iteration].position.z };
			LightJson["rotation"] = { LightingHandler::Lights[iteration].rotation.x,  LightingHandler::Lights[iteration].rotation.y,  LightingHandler::Lights[iteration].rotation.z };
			LightJson["colour"] = { LightingHandler::Lights[iteration].colour.x, LightingHandler::Lights[iteration].colour.y, LightingHandler::Lights[iteration].colour.z };

			if (LightingHandler::Lights[iteration].type == 1) {
				LightJson["type"] = "pointlight";
			}
			else if (LightingHandler::Lights[iteration].type == 0) {
				LightJson["type"] = "spotlight";
			}

			LightJson["radius"] = LightingHandler::Lights[iteration].radius;

			LightJson["enabled"] = LightingHandler::Lights[iteration].enabled;
			// ID
			//LightJson["IDuniqueIdentifier"] = LightingHandler::Lights[iteration].ID.UniqueNumber;

			lightData.push_back(LightJson);
			iteration++;
		}

		// Write to file
		std::ofstream outFile(path, std::ios::out);
		if (!outFile.is_open()) {
			std::cout << "Failed to write to " << path << std::endl;
			return;
		}

		outFile << lightData.dump(4);  // Pretty-print with indentation
		outFile.close();

		std::cout << "Successfully updated " << path << std::endl;

	}
	catch (const std::exception& e) {
		std::cout << "Exception: " << e.what() << std::endl;
	}
}

void LightingHandler::deleteScene()
{
	for (size_t i = 0; i < Lights.size(); ++i)
	{
		deleteLight(i);
		//IdManager::RemoveID(Lights[i].ID);
	}
	Lights.clear();
}

void LightingHandler::cleanup()
{
	// shaders
	dirShadowMapProgram.Delete();
	dirShadowMapProgramBB.Delete();
}