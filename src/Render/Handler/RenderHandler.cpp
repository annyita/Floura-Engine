#include "RenderHandler.h"
#include "Systems/util/UUID.h"
#include <Scene/LightingHandler.h>
#include <Render/Handler/ShaderHandler.h>
#include <Render/Handler/RenderClass.h>
#include <Render/Object/Skybox.h>
#include <Render/pipeline/prebuilt_pipelines/geometryPass.h>
#include <Scene/scene.h>
#include <Editor/UI/ImGui/ImGuiWindow.h>
#include <Render/Shader/renderTarget.h>
#include <Render/pipeline/prebuilt_pipelines/historyPass.h>
#include "Scene/FE_LAYER.h"
#include "utils/FE_math.h"
#include "LoadHandler.h"
#include <Render/pipeline/prebuilt_pipelines/flouraDeferred.h>
#include <Render/pipeline/prebuilt_pipelines/swrt.h>

#include "Systems/Physics/SDF.h"

std::unordered_map<std::string, uint64_t> RenderHandler::pKeyHandleMapRender;



std::vector<RenderHandler::modelObject> RenderHandler::models;
std::vector<RenderHandler::renderQueueData> RenderHandler::renderQueueDataVector;

bool RenderHandler::renderENV = false;

int RenderHandler::drawCount = 0;

uint64_t RenderHandler::fetchHandle(std::string path)
{
	auto it = pKeyHandleMapRender.find(path);
	if (it != pKeyHandleMapRender.end()) {
		return it->second;
	}
	return 0;
}

int RenderHandler::fetchModelIndex(uint64_t RenderID) // use a map here << this slows things down
{
	for (size_t i = 0; i < models.size(); i++){
		if (models[i].RenderID == RenderID) return (int)i;
	}
	return -1;
}

void RenderHandler::updateLoadUnloadedModels(){
	for (int i = 0; i < models.size(); ++i) {// should have a int count for how many have gotten loaded, and - on it to skip the for loop
		if (models[i].model->disableConstructorLoadingModelFlag && !models[i].model->loaded) // if the flag is active, and the model is not classed as loaded
			models[i].model->loadModelPathless(); // load
	}
}

RenderHandler::batchOfUUID RenderHandler::addModel(std::string path){
	uint64_t nUUID = fetchHandle(path);
	uint64_t nIUUID = UUID::returnHandle();
	if (nUUID == 0){ // if equal to zero handle does not exist in array, we can create away
		// assign new handle
		nUUID = UUID::returnHandle();
		pKeyHandleMapRender[path] = nUUID;
		modelObject newModelObject;
		// should have a model json thing here
		
		std::string nPath = path;
		bool hasMDF = true;
		float thickness = 1.0f;
		int sliceSize = FlouraSWRT::autoMDFres;
		int mdfFormation = 0; // 0 per mesh, 1 grid
		int f2division = 2;
		
		if (path.ends_with(".model")){
			std::cout << "model template:" << std::endl;
			
			std::ifstream file(path);
			if (file.is_open()) {
				json data;
				file >> data;
				file.close();
				
				nPath = data[0]["path"].get<std::string>();
				hasMDF = data[0]["hasMDF"].get<bool>();;
				thickness = data[0]["MDF_thickness"].get<float>();
				sliceSize = data[0]["sliceSize"].get<int>();
				mdfFormation = data[0]["MDF_Formation"].get<int>();
				if (mdfFormation == 1 && data[0].contains("MDF_Formation2Div"))
					f2division = data[0]["MDF_Formation2Div"].get<int>();
			}
			else std::cerr << "failed to open: " << path << std::endl;
		}
		
		//mdfFormation = 1; 
		//f2division = 2;
		
		newModelObject.path = nPath;
		newModelObject.RenderID = nUUID;
		newModelObject.instances = 1;
		//newModelObject.model = new Model(path.c_str(), true, true, true); // to attempt the threaded worker load do here <<
		newModelObject.model = new Model(nPath.c_str(), false, false, false); // to attempt the threaded worker load do here <<
		//newModelObject.model = new Model(path.c_str(), false, true, true); // to attempt the threaded worker load do here <<
		//LoadHandler::addToModelMeshCreateW_RenderIDQueue(nUUID); // << to run on opengl thread
		//LoadHandler::addToModelTextureCreateW_RenderIDQueue(nUUID);
		newModelObject.model->createMeshAABBs();
		newModelObject.model->generateMeshBlases(8, 16);
		
		if (hasMDF){
			switch (mdfFormation){
			case 0: // per mesh
				newModelObject.model->mdfFormation = 0;
				//newModelObject.model->MDFgeneratePrim(32, 15); 
				newModelObject.model->MDFgeneratePrimGPU(sliceSize, 15, thickness); 
				//newModelObject.model->MDFgenerateBlas(64, 15);	
				break;
			case 1: // grid todo fix the culling on this and the degen bits so this goes fast, already is promising in vram <3
				newModelObject.model->mdfFormation = 1;
				newModelObject.model->MDFgenerateGridPrimGPU(sliceSize, 15, thickness, f2division); 
				break;
			}
		
			//flouraSDF::cacheSDF("Cache/SDF/temp/", newModelObject.model->hash, newModelObject.model->meshSDFs);
		}
		//newModelObject.model->VXGgeneratePrim(32, 15); 
		//newModelObject.model->VXGgenerateBlas(33, 15); 
		//voxelizer::cacheVXG("Cache/VXG/temp/", newModelObject.model->hash, newModelObject.model->meshVXGs);
		
		newModelObject.model->renderID = nUUID;
		Model::instaceData IsD;  IsD.ID =nIUUID;
		newModelObject.model->instacesData.push_back(IsD);  // set
		models.push_back(newModelObject);
	}
	else{
		int index = fetchModelIndex(nUUID);
		if (index != -1){
			Model::instaceData IsD; IsD.ID =nIUUID;
			models[index].model->instacesData.push_back(IsD); // add
			models[index].instances += 1;
		}
	}
	batchOfUUID nBatchOfUUIDS;
	nBatchOfUUIDS.instanceUUID = nIUUID;
	nBatchOfUUIDS.RenderID = nUUID;
	return nBatchOfUUIDS;
}

void RenderHandler::addToRenderQueue(renderQueueData data){
	renderQueueDataVector.push_back(data);
}

void RenderHandler::clearRenderQueue(){
	drawCount = 0;
	renderQueueDataVector.clear();
}

float dAccum = 0.0;
float dAccumthresh = 1.0 / 1.0f;

void RenderHandler::render(){
	if (RenderClass::currentRendererInd == RenderClass::NONE){
		clearRenderQueue();
		return;
	}
	
	shadowDraw();
	
	// shadows should probably update first
	dAccum += TimeUtil::deltatime;
	
	// reflection draw
	if (RenderClass::doReflections && renderENV  &&  dAccum > dAccumthresh || ProbeHandler::indirectSamples > 0 && renderENV  &&  dAccum > dAccumthresh){
		float range = 100.0f;
		
		//glm::vec3(Scene::maincamera.Position.x, Scene::maincamera.Position.y, Scene::maincamera.Position.z)
		cmDraw(renderQueueDataVector, tempCM, cmShader, glm::vec2(512), glm::vec3(Scene::maincamera.Position.x, Scene::maincamera.Position.y, Scene::maincamera.Position.z), range);
		// cmDraw(renderQueueDataVector, tempCM, cmShader, glm::vec2(256), glm::vec3(0.0f, 5.0f, 0.0f));
		Skybox::unbind();
		dAccum = 0.0;
	}

	
	regularDraw();
	instancedDraw();
	
	if (FEImGuiWindow::isWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Enable wireframe mode
	
	FE_LAYER::draw();
	//FlouraSWRT::GDFdebugDraw();
	FlouraSWRT::MDFdebugDraw();
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Restore normal rendering < wireframe


	switch (RenderClass::currentRendererInd){
	case RenderClass::DEFERRED:
		glDisable(GL_CULL_FACE);
		if (renderENV)  tempCM->cubemapToUUIDShader("cmMainHandle", FlouraDeferred::DFL_Shader);
		else Skybox::SkyboxCubemap->cubemapToUUIDShader("cmMainHandle", FlouraDeferred::DFL_Shader);
		FlouraDeferred::DeferredLightingPass(); // Forward Lighting Pass
		FlouraDeferred::ssrPass(); // << overhead
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glActiveTexture(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		if (RenderClass::doTAA) RenderClass::taaPass();
		HistoryPass::hPassDraw();
		break;
	case RenderClass::SWRT:
		//if (raytracer::RTGlobalTransformFlag) SceneDescription::updateQuickVoxelData();
		//SceneDescription::updateQuickVoxelData();
		FlouraSWRT::GDFdraw();
		FlouraSWRT::draw();
		//RenderClass::raymarchingPass(); // comment out when not using
		raytracer::RTGlobalTransformFlag = false; // this is dumb i know
		
		if (RenderClass::doTAA) RenderClass::taaPass();
		HistoryPass::hPassDraw();
		break;
	default:
		break;
	}
	
	// after render clear render queue
	clearRenderQueue();
}

void RenderHandler::removeInstancewRenderID(uint64_t RenderID, uint64_t instanceID){
	int index = fetchModelIndex(RenderID);
	for (int i = 0; i < models[index].model->instacesData.size(); ++i){
		if (instanceID != models[index].model->instacesData[i].ID) continue;
		models[index].model->instacesData.erase(models[index].model->instacesData.begin() + i);
	}
	removeInstance(index);
}

void RenderHandler::removeInstance(int index)
{
	// bounds check
	if (index < 0 || index >= (int)models.size()) return;

	models[index].instances -= 1;

	if (models[index].instances <= 0)
	{
		// erase the item in map
		auto handleIt = pKeyHandleMapRender.find(models[index].path);
		if (handleIt != pKeyHandleMapRender.end()) {
			pKeyHandleMapRender.erase(handleIt);
		}
		// erase model
		delete models[index].model;
		models[index].model = nullptr;
		models.erase(models.begin() + index);
	}
}

uint64_t RenderHandler::findRenderUUIDwIstanceUUID(uint64_t InstanceUUID){
	for (size_t i = 0; i < RenderHandler::models.size(); i++){
		for (size_t x = 0; x < RenderHandler::models[i].model->instacesData.size(); x++){
			if (RenderHandler::models[i].model->instacesData[x].ID == InstanceUUID)
				return RenderHandler::models[i].RenderID;
		}
		
	}
	return uint64_t(0);
}

uint64_t RenderHandler::findModelUUIDwRenderUUID(uint64_t RenderID){
	int index = fetchModelIndex(RenderID);
	if (index != -1){
		return RenderHandler::models[index].model->UUID;
	}

	return uint64_t(0);
}

uint64_t RenderHandler::findModelUUIDwInstanceUUID(uint64_t InstanceUUID){
	uint64_t renderUUID = findRenderUUIDwIstanceUUID(InstanceUUID);
	return findModelUUIDwRenderUUID(renderUUID);
}

void RenderHandler::init(){
	tempCM = new Cubemap();
	//tempCM->loadCubeMap("Assets/Skybox/clearsky/Skybox.json"); // temp issue stems from this itself??
	cmShader.LoadShader("Assets/Shaders/Lighting/Default.vert", "Assets/Shaders/Lighting/reflection.frag");
}

glm::vec3 rqtargets[] = {
glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(-1.0f,  0.0f,  0.0f),
glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f),
glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f,  0.0f, -1.0f)
};

glm::vec3 rqups[] = {
	glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f),
	glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f,  0.0f, -1.0f),
	glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)
};


Shader RenderHandler::cmShader;
Cubemap* RenderHandler::tempCM;

void RenderHandler::cmDraw(std::vector<renderQueueData> rqdVector, Cubemap*& cm, Shader& shader, glm::vec2 resolution, glm::vec3 pos, float range){
	tempCM->resizeCubeMap(resolution); // seems to remove the texture, keep an eye on this later
	
	// creation
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (GLAD_GL_ARB_bindless_texture && cm->handle != 0) {
		glMakeTextureHandleNonResidentARB(cm->handle);
	}

	if (cm->ID == 0) {
		glDeleteTextures(1, &cm->ID);
		// Creates the cubemap texture object
		glGenTextures(1, &cm->ID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cm->ID);

		glTexStorage2D(GL_TEXTURE_CUBE_MAP, 5, GL_RGBA8, (int)resolution.x, (int)resolution.y);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		// These are very important to prevent seams
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_SEAMLESS, GL_TRUE);

	}



	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::cmFBO);


	int width = (int)resolution.x; int height = (int)resolution.y;
	glViewport(0, 0, width, height); glPixelStorei(GL_PACK_ALIGNMENT, 1);
	renderTarget::smUpdateResolution(resolution); glBindFramebuffer(GL_FRAMEBUFFER, 0);

	Camera nCamera;
	nCamera.InitCamera(int(resolution.x), int(resolution.y), pos); // Matching your Position
	nCamera.fov = 90.0f;
	nCamera.nearFar = glm::vec2(0.1f, range);
	
	// Cycles through all the textures and attaches them to the cubemap object
	for (unsigned int x = 0; x < 6; x++){
		 // should get rid of this btw
		nCamera.Orientation = rqtargets[x];
		nCamera.Up = rqups[x];
		nCamera.updateMatrix();

		glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::cmFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + x, cm->ID, 0);
		renderTarget::clearsmbuffer();
		glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::cmFBO);

		if (!FEImGuiWindow::isWireframe && RenderClass::renderSkybox) // should add skybox.scene
			Skybox::draw(nCamera,  renderTarget::cmFBO, false);
		glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::cmFBO);	


		for (size_t i = 0; i < renderQueueDataVector.size(); i++){
			int index = fetchModelIndex(renderQueueDataVector[i].RenderID);
			if (index != -1 && !renderQueueDataVector[i].isInstanced){
				//drawCount++;
				//int modelShaderIndex = ShaderHandler::fetchShaderIndex(renderQueueDataVector[i].shaderUUID);

				// these are temp
				models[index].model->updatePosition(renderQueueDataVector[i].position);
				models[index].model->updateRotation(renderQueueDataVector[i].rotation);
				models[index].model->updateScale(renderQueueDataVector[i].scale);
				models[index].model->updateTranformation();

				if (!FE_Math::isInRange(renderQueueDataVector[i].position, pos, range)) continue; // range check/cull
				models[index].model->childrenRangeCull(pos, nCamera.nearFar.y);
				LightingHandler::sendToShader(shader);

				//RenderClass::bluenoise->Bind();
				//shader.setInt("BlueNoiseTex", 6);

				shader.setHandleui64ARB("BlueNoiseHandle", RenderClass::bluenoise->handle);
				shader.setHandleui64ARB("bayerMatrixHandle", RenderClass::bayermatrix->handle);

				Skybox::SkyboxCubemap->cubemapToUUIDShader("cmMainHandle", shader);

				// this would normally be in material
				shader.Activate();

				nCamera.Matrix(shader, "camMatrix");

				shader.Activate();
				shader.setTimeVariables();
				shader.setFloat("doBinaryAlpha", RenderClass::doBinaryAlpha);
				// this would normally be in material
				
				if (renderQueueDataVector[i].doCulling == true && !FEImGuiWindow::isWireframe) glEnable(GL_CULL_FACE);
				else glDisable(GL_CULL_FACE);
				if (renderQueueDataVector[i].cullFrontFace) glCullFace(GL_FRONT);
				else glCullFace(GL_BACK);

				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Enable wireframe mode


				shader.Activate();
				shader.setFloat2("uvScale", renderQueueDataVector[i].uvScale);
				shader.setInt("indirectSamples", 0);
				shader.setBool("doReflect", false);

				shader.setInt("drawIndex", i);
				
				shader.Activate();
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);
				// temp
				models[index].model->draw(shader, nCamera);
				

				//glFrontFace(GL_CCW);
				glCullFace(GL_BACK); // Reset culling to default
				glDisable(GL_CULL_FACE);

				glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
			}
		}

		unsigned char* data = new unsigned char[width * height * 4];
		glBindFramebuffer(GL_READ_BUFFER, renderTarget::cmFBO);
		//glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);



		glBindTexture(GL_TEXTURE_CUBE_MAP, cm->ID);
		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + x,
			0,
			GL_RGBA,
			width,
			height,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			data
		);

		delete[] data;
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, cm->ID);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	if (GLAD_GL_ARB_bindless_texture) {
		cm->handle = glGetTextureHandleARB(cm->ID);
		glMakeTextureHandleResidentARB(cm->handle);
	}


	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glViewport(0, 0, viewport[2], viewport[3]);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderHandler::regularDraw(){
	if (renderQueueDataVector.empty()) return;

	if (FEImGuiWindow::isWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Enable wireframe mode
	
	// gpass
	for (size_t i = 0; i < renderQueueDataVector.size(); i++){
		int index = fetchModelIndex(renderQueueDataVector[i].RenderID);
		if (index != -1 && !renderQueueDataVector[i].isInstanced &&  
			RenderClass::currentRendererInd ==  RenderClass::DEFERRED  ||
			RenderClass::currentRendererInd ==  RenderClass::SWRT){
			//drawCount++;
			// whole cull if (Collision::AABBtoSphereRangeCull())
			int modelGPShaderIndex = ShaderHandler::fetchShaderIndex(renderQueueDataVector[i].gpShaderUUID);

			// these are temp
			models[index].model->updatePosition(renderQueueDataVector[i].position);
			models[index].model->updateRotation(renderQueueDataVector[i].rotation);
			models[index].model->updateScale(renderQueueDataVector[i].scale);
			models[index].model->updateTranformation();
			
			// only needed here (previous for velocity)
			models[index].model->updatePrevPosition(renderQueueDataVector[i].pPosition);
			models[index].model->updatePrevRotation(renderQueueDataVector[i].pRotation);
			models[index].model->updatePrevScale(renderQueueDataVector[i].pScale);
			models[index].model->updatePrevTranformation();
			
			// range cull prep before draw
			
			models[index].model->childrenRangeCull(Scene::maincamera.Position, Scene::maincamera.nearFar.y);
			//models[index].model->childrenRangeCull(Scene::maincamera.Position, 15.0f);
			
			ShaderHandler::shaderObjects[modelGPShaderIndex].Shader.Activate();
			Scene::maincamera.Matrix(ShaderHandler::shaderObjects[modelGPShaderIndex].Shader, "camMatrix");
			ShaderHandler::shaderObjects[modelGPShaderIndex].Shader.setTimeVariables();
			ShaderHandler::shaderObjects[modelGPShaderIndex].Shader.setBool("doBinaryAlpha", RenderClass::doBinaryAlpha);
			ShaderHandler::shaderObjects[modelGPShaderIndex].Shader.setBool("animateBinaryAlpha", RenderClass::animateBinaryAlpha);
			// this would normally be in material
			
			ShaderHandler::shaderObjects[modelGPShaderIndex].Shader.setHandleui64ARB("BlueNoiseHandle", RenderClass::bluenoise->handle);
			ShaderHandler::shaderObjects[modelGPShaderIndex].Shader.setHandleui64ARB("bayerMatrixHandle", RenderClass::bayermatrix->handle);
			ShaderHandler::shaderObjects[modelGPShaderIndex].Shader.setHandleui64ARB("ripplesHandle", RenderClass::ripples->handle);
			ShaderHandler::shaderObjects[modelGPShaderIndex].Shader.setHandleui64ARB("dropletsHandle", RenderClass::droplets->handle);
			ShaderHandler::shaderObjects[modelGPShaderIndex].Shader.setHandleui64ARB("puddlesHandle", RenderClass::puddles->handle);
			ShaderHandler::shaderObjects[modelGPShaderIndex].Shader.setFloat2("uvScale", renderQueueDataVector[i].uvScale);
			ShaderHandler::shaderObjects[modelGPShaderIndex].Shader.setInt("drawIndex", i);
			
			
			if (renderQueueDataVector[i].doCulling == true && !FEImGuiWindow::isWireframe) glEnable(GL_CULL_FACE);
			else glDisable(GL_CULL_FACE);
			if (renderQueueDataVector[i].cullFrontFace) glCullFace(GL_FRONT);
			else glCullFace(GL_BACK);
			
			GeometryPass::gPassDraw(models[index].model, ShaderHandler::shaderObjects[modelGPShaderIndex].Shader, Scene::maincamera);
			
			//glFrontFace(GL_CCW);
			glCullFace(GL_BACK); // Reset culling to default
			glDisable(GL_CULL_FACE);
		}
	}

	
	
	if (RenderClass::currentRendererInd == RenderClass::FORWARD) {
		// regular non instanced
		for (size_t i = 0; i < renderQueueDataVector.size(); i++){
			int index = fetchModelIndex(renderQueueDataVector[i].RenderID);
			if (index != -1 && !renderQueueDataVector[i].isInstanced){
				drawCount++;
				int modelShaderIndex = ShaderHandler::fetchShaderIndex(renderQueueDataVector[i].shaderUUID);

				// these are temp
				models[index].model->updatePosition(renderQueueDataVector[i].position);
				models[index].model->updateRotation(renderQueueDataVector[i].rotation);
				models[index].model->updateScale(renderQueueDataVector[i].scale);
				models[index].model->updateTranformation();
				
				models[index].model->childrenRangeCull(Scene::maincamera.Position, Scene::maincamera.nearFar.y);

				LightingHandler::sendToShader(ShaderHandler::shaderObjects[modelShaderIndex].Shader);

				//RenderClass::bluenoise->Bind();
				//ShaderHandler::shaderObjects[modelShaderIndex].Shader.setInt("BlueNoiseTex", 6);
				
				ShaderHandler::shaderObjects[modelShaderIndex].Shader.setHandleui64ARB("BlueNoiseHandle", RenderClass::bluenoise->handle);

				ShaderHandler::shaderObjects[modelShaderIndex].Shader.setHandleui64ARB("bayerMatrixHandle", RenderClass::bayermatrix->handle);

				//Skybox::bind(5);
				//Skybox::cubemapToShader(ShaderHandler::shaderObjects[modelShaderIndex].Shader, 5);

				//glActiveTexture(GL_TEXTURE0 + 5);// + textureUnit
				//glBindTexture(GL_TEXTURE_CUBE_MAP, tempCM->ID);

				ShaderHandler::shaderObjects[modelShaderIndex].Shader.Activate();
				if (renderENV) tempCM->cubemapToUUIDShader("cmMainHandle", ShaderHandler::shaderObjects[modelShaderIndex].Shader);
				else Skybox::SkyboxCubemap->cubemapToUUIDShader("cmMainHandle", ShaderHandler::shaderObjects[modelShaderIndex].Shader);

				//tempCM

				// this would normally be in material

				ShaderHandler::shaderObjects[modelShaderIndex].Shader.setMat4("camMatrix", Scene::maincamera.cameraMatrixAlwaysUnjittered);
				ShaderHandler::shaderObjects[modelShaderIndex].Shader.setTimeVariables();
				ShaderHandler::shaderObjects[modelShaderIndex].Shader.setBool("doBinaryAlpha", RenderClass::doBinaryAlpha);
				ShaderHandler::shaderObjects[modelShaderIndex].Shader.setBool("animateBinaryAlpha", RenderClass::animateBinaryAlpha);
				// this would normally be in material
				
				ShaderHandler::shaderObjects[modelShaderIndex].Shader.Activate();
				ShaderHandler::shaderObjects[modelShaderIndex].Shader.setFloat2("uvScale", renderQueueDataVector[i].uvScale);
				ShaderHandler::shaderObjects[modelShaderIndex].Shader.setInt("indirectSamples", ProbeHandler::indirectSamples);
				ShaderHandler::shaderObjects[modelShaderIndex].Shader.setInt("drawIndex", i);

				glActiveTexture(GL_TEXTURE7);
				glBindTexture(GL_TEXTURE_2D, GeometryPass::depthTexture);
				ShaderHandler::shaderObjects[modelShaderIndex].Shader.setInt("depthMap", 7);
				
				//if (!RenderClass::DoForwardLightingPass && !RenderClass::DoDeferredLightingPass) continue; // Skip rendering if not in regular or lighting pass
				if (renderQueueDataVector[i].doCulling == true && !FEImGuiWindow::isWireframe) glEnable(GL_CULL_FACE);
				else glDisable(GL_CULL_FACE);
				if (renderQueueDataVector[i].cullFrontFace) glCullFace(GL_FRONT);
				else glCullFace(GL_BACK);

				if (FEImGuiWindow::isWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Enable wireframe mode

				//smoothnessValue

				//raytracer::uvScaleUpdate(component.renderHeads.Model->UUID, component.systems.material.uvScale);


				glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::FBO);
				ShaderHandler::shaderObjects[modelShaderIndex].Shader.Activate();
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);

				// temp
				models[index].model->draw(ShaderHandler::shaderObjects[modelShaderIndex].Shader, Scene::maincamera);

				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Enable wireframe mode

				//glFrontFace(GL_CCW);
				glCullFace(GL_BACK); // Reset culling to default
				glDisable(GL_CULL_FACE);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);

				glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
			}
		}
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Enable wireframe mode
}

void RenderHandler::shadowDraw(){
	if (renderQueueDataVector.empty()) return;
	// shadow pass add infomation like culling, facedir
	for (size_t i = 0; i < renderQueueDataVector.size(); i++){
		if (renderQueueDataVector[i].castsShadow && !renderQueueDataVector[i].isInstanced){
			int index = fetchModelIndex(renderQueueDataVector[i].RenderID);
			if (index != -1){
				//drawCount++;
				// these are temp
				models[index].model->updatePosition(renderQueueDataVector[i].position);
				models[index].model->updateRotation(renderQueueDataVector[i].rotation);
				models[index].model->updateScale(renderQueueDataVector[i].scale);
				models[index].model->updateTranformation();
				
				if (renderQueueDataVector[i].doCulling == true && !FEImGuiWindow::isWireframe) glEnable(GL_CULL_FACE);
				else glDisable(GL_CULL_FACE);
				if (renderQueueDataVector[i].cullFrontFace) glCullFace(GL_FRONT);
				else glCullFace(GL_BACK);
				LightingHandler::drawShadowMap(models[index].model, i);
			}
		}
	}
}

void RenderHandler::instancedDraw(){
	// needs to make batches of instanced data, do sep for both shadow and regular, shadow doesnt include shaders or uv
	for (size_t i = 0; i < renderQueueDataVector.size(); i++){
		if (renderQueueDataVector[i].isInstanced){

		}
	}
	// then needs to draw it
}