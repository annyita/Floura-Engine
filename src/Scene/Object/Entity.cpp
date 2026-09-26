#include "Entity.h"
#include <scene/scene.h>
#include <utils/FE_math.h>
#include <utils/logConsole.h>
#include <Scene/LightingHandler.h>
#include <Gameplay/Player.h>
#include "Systems/util/UUID.h"
#include <Render/pipeline/prebuilt_pipelines/depreciated/raytracer.h>
#include <Render/Handler/RenderHandler.h>
#include "Systems/util/relationshipManager.h"
#include "Render/Handler/CubeVisualizer.h"
#include "Render/pipeline/prebuilt_pipelines/swrt.h"
//#include "Systems/Physics/physworld.h"


void entity::createwUUID(uint64_t nUUID, ENT_TYPE_ENUM type, const std::string& name, const std::string& path, const std::string& materialPath){
	entity::UUID = nUUID;
	UUIDstring = UUID::UUIDToString(UUID);
	//std::cout << UUIDstring << std::endl;

	// set type
	entity::type = type;
	// set name 
	entity::name = name;
	// set path 
	entity::path = path;

	switch (type){
	case ENT_MODEL_TYPE: // model
		createModel(path, materialPath);
		break;

	case ENT_BILLBOARD_TYPE: // billboard
		createBillBoard(path);
		break;
	default:
		LogConsole::print("Entity Create: Unknown type '" + std::string(1, type) + "' for entity: " + name);
		break;
	}
	
	createGeneralLogic();
}


void entity::create(ENT_TYPE_ENUM type, const std::string& name, const std::string& path, const std::string& materialPath){
	UUID = UUID::returnHandle();
	UUIDstring = UUID::UUIDToString(UUID);
	//std::cout << UUIDstring << std::endl;

	// set type
	entity::type = type;
	// set name 
	entity::name = name;
	// set path 
	entity::path = path;
	
	switch (type){
	case ENT_MODEL_TYPE: // model
		createModel(path, materialPath);
		break;

	case ENT_BILLBOARD_TYPE: // billboard
		createBillBoard(path);
		break;
	default:
		LogConsole::print("Entity Create: Unknown type '" + std::string(1, type) + "' for entity: " + name);
		break;
	}

	createGeneralLogic();
}

void entity::createGeneralLogic(){
	raytracer::RTGlobalTransformFlag = true;
	// within here attach phys object to handler
	
	component.physobject.UUID = UUID::returnHandle(); // atm internal doesnt need to save, just for syncing
	
	physworld::addPhysicsObjectToArray(&component.physobject, &component.systems.transformation.position, &raytracer::RTGlobalTransformFlag);
}

void entity::LoadMaterial(std::string path){
	component.systems.material.Material.LoadMaterial(path);
}

void entity::addScript(std::string path, std::string name)
{
	ScriptObject* newScript = new ScriptObject(name);
	newScript->loadScript(path);
	ScriptObjects.push_back(newScript);
	initEntityTables(ScriptObjects.back());
}

void entity::reloadScript(int index)
{
	ScriptObjects[index]->loadScript(ScriptObjects[index]->path);
}

void entity::removeScript(int index)
{
	if (index < ScriptObjects.size()) {
		delete ScriptObjects[index];
		ScriptObjects.erase(ScriptObjects.begin() + index);
	}
}

void entity::updateScripts()
{
	// update function
	for (size_t i = 0; i < ScriptObjects.size(); i++)
	{
		if (Player::playstate == 1)
		{
			sendEntityUniformsToScripts(ScriptObjects[i]);
			ScriptObjects[i]->scriptUpdate();
			// any other functions go here
			getEntityUniformsToScripts(ScriptObjects[i]);
		}
	}
}

void entity::initScript(int index)
{
	if (Player::playstate == 1)
	{
		ScriptObjects[index]->didInit = false;
	}
}

void entity::update()
{
	//pollPositionUpdates();
	
	
	updateMeshAABBs();

	// update mesh positions here vv
	switch (type)
	{
	case ENT_MODEL_TYPE: // model
	{
		int index = RenderHandler::fetchModelIndex(component.render.renderID);
		if (index != -1){
			RenderHandler::models[index].model->updatePosition(component.systems.transformation.position);
			RenderHandler::models[index].model->updateRotation(component.systems.transformation.rotation);
			RenderHandler::models[index].model->updateScale(component.systems.transformation.scale);
			RenderHandler::models[index].model->updateTranformation();
			RenderHandler::models[index].model->updateMeshAABBs();
			
			
			// wait for dirty
			
			if (raytracer::RTGlobalTransformFlag){ // all of these are wasteful, make sure to look into these
				//raytracer::updateboundingboxes(component.render.instanceUUID, component.collider.rootnodes);
				//raytracer::modelMatrixUpdate(component.render.instanceUUID, RenderHandler::models[index].model->gModelMatrix);
				FlouraSWRT::updateGlobalTransformation(component.render.instanceUUID, RenderHandler::models[index].model->gModelMatrix, component.systems.transformation.rotation);
			}
		}
		break;
	}
	case ENT_BILLBOARD_TYPE: // billboard
		component.render.BillBoard->updatePosition(component.systems.transformation.position);
		component.render.BillBoard->updateScale(component.systems.transformation.scale);
		break;
	default:
		break;
	}

	updateScripts();
	
	if (component.systems.transformation.position != component.systems.htransform.position||
		component.systems.transformation.scale != component.systems.htransform.scale ||
		component.systems.transformation.rotation != component.systems.htransform.rotation){
		
		component.systems.htransform.position = component.systems.transformation.position;
		component.systems.htransform.scale = component.systems.transformation.scale;
		component.systems.htransform.rotation = component.systems.transformation.rotation;
		FlouraSWRT::dirtyGDF = true;
	}
}

void entity::updatePhysicsDynamics(float deltatime){
	
	//updateMeshAABBs();
	if (component.physobject.hasRigidbody) // change name to hasdynamics
	{
		component.render.dirtyTransform = true;

		if (component.physobject.affectedByGravity)
		{
			glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);

			component.physobject.force += component.physobject.mass * gravity; // applying foce
		}

		component.physobject.velocity += component.physobject.force / component.physobject.mass * deltatime;
		glm::vec3 newPos = component.systems.transformation.position + component.physobject.velocity * deltatime;
		setPosition(newPos);

		component.physobject.force = glm::vec3(0.0f); // reset force at end
	}
}

void entity::Delete()
{
	// right now logic (raise flags, queue for the deletion event (lock physics thread etc) )
	queuedForDeletion = true;
	Scene::entityDeletionUnderGoing = true;
	Main::lockPhysicsThread = true;
	
	// instead of deleting should queue a delete (make new function since i dont wanna change existing architecture), this will prevent thread issues
	//entity::queuedDeletion(); // sit here for now
}

void entity::queuedDeletion(){
	physworld::bundleArrayDeleteWithUUID(component.physobject.UUID);
	switch (type){
	case ENT_MODEL_TYPE: // model
		{

			int index = RenderHandler::fetchModelIndex(component.render.renderID);
			if (index != -1){
				//raytracer::removeFromRaytracer(component.render.instanceUUID);
				FlouraSWRT::removeFromLSDFScene(component.render.instanceUUID);
				//SceneDescription::removeFromVoxelScene(component.render.instanceUUID); // not setup but still, just putting this here
			}
			RenderHandler::removeInstancewRenderID(component.render.renderID, component.render.instanceUUID);
			//delete component.renderHeads.Model;
			//component.renderHeads.Model = nullptr;
			component.systems.material.Material.ClearMaterial();
			break;
		}
	case ENT_BILLBOARD_TYPE: // billboard
		delete component.render.BillBoard;
		component.render.BillBoard = nullptr;
		break;
	}

	for (size_t i = 0; i < ScriptObjects.size(); i++){
		removeScript(i);
	}

	// decouple children
	int thisIndex = RelationshipManager::indexFromUUIDEntity(entity::UUID);
	if (thisIndex != -1){
		for (size_t i = 0; i < component.relationship.childUUID.size(); i++){
			int childIndex = RelationshipManager::indexFromUUIDEntity(component.relationship.childUUID[i]);
			if (childIndex != -1) RelationshipManager::removeParent(childIndex);
		}
		// erase paremt i should do too
		if (component.relationship.hasParent){
			RelationshipManager::removeParent(thisIndex);
		}
	
	}

	raytracer::RTGlobalTransformFlag = true;
}

Collision::HitResult entity::AABBVsEntity(glm::vec3 pos, glm::vec3 scale){
	Collision::HitResult finalResult;
	finalResult.isColliding = false;
	finalResult.distance = std::numeric_limits<float>::max();	
	int index = RenderHandler::fetchModelIndex(component.render.renderID);
	if (index != -1){

		glm::mat4 gModelMatrix = FE_Math::composeMatrixWDegrees(RenderHandler::models[index].model->globalTransformation.position,
			RenderHandler::models[index].model->globalTransformation.scale, RenderHandler::models[index].model->globalTransformation.rotation);

		// for each mesh
		for (size_t x = 0; x < RenderHandler::models[index].model->meshes.size(); x++){
			// final transformation
			glm::mat4 finalMatrix = gModelMatrix * RenderHandler::models[index].model->lModelMatrix[x]; // * by local transform

			// AABB to speed things up
			Collision::HitResult AABB = Collision::AABBvsAABB(component.collider.rootnodes[x].position, component.collider.rootnodes[x].size, pos, scale);
			if (AABB.isColliding){
				for (size_t y = 0; y < RenderHandler::models[index].model->meshes[x].indices.size(); y += 3){
					
					unsigned int i0 = RenderHandler::models[index].model->meshes[x].indices[y];
					unsigned int i1 = RenderHandler::models[index].model->meshes[x].indices[y + 1];
					unsigned int i2 = RenderHandler::models[index].model->meshes[x].indices[y + 2];

					if (i0 >= RenderHandler::models[index].model->meshes[x].vertices.size() ||
						i1 >= RenderHandler::models[index].model->meshes[x].vertices.size() ||
						i2 >= RenderHandler::models[index].model->meshes[x].vertices.size()) {
						continue;
					}

					glm::vec3 a = RenderHandler::models[index].model->meshes[x].vertices[i0].position;
					glm::vec3 b = RenderHandler::models[index].model->meshes[x].vertices[i1].position;
					glm::vec3 c = RenderHandler::models[index].model->meshes[x].vertices[i2].position;

					FE_Math::transformPoint(a, finalMatrix);
					FE_Math::transformPoint(b, finalMatrix);
					FE_Math::transformPoint(c, finalMatrix);

					// run hit test and return result
					Collision::HitResult trihit = Collision::SATTriangleVSAABB(a, b, c, pos, scale);

					if (trihit.isColliding && trihit.distance < finalResult.distance){
						finalResult = trihit;
					}
				}
			}
		}
	}
	return finalResult;
}


Collision::HitResult entity::RayVsEntity(glm::vec3 rayPos, glm::vec3 rayDir)
{
	Collision::HitResult finalResult;
	finalResult.isColliding = false;
	finalResult.distance = std::numeric_limits<float>::max();	
	int index = RenderHandler::fetchModelIndex(component.render.renderID);
	if (index != -1)
	{

		glm::mat4 gModelMatrix = FE_Math::composeMatrixWDegrees(RenderHandler::models[index].model->globalTransformation.position,
			RenderHandler::models[index].model->globalTransformation.scale, RenderHandler::models[index].model->globalTransformation.rotation);

		// for each mesh
		for (size_t x = 0; x < RenderHandler::models[index].model->meshes.size(); x++)
		{
			// final transformation
			glm::mat4 finalMatrix = gModelMatrix * RenderHandler::models[index].model->lModelMatrix[x]; // * by local transform

			// AABB to speed things up
			Collision::HitResult AABB = Collision::AABBvsRay(component.collider.rootnodes[x].position, component.collider.rootnodes[x].size, rayPos, rayDir);
			if (AABB.isColliding)
			{
				for (size_t y = 0; y < RenderHandler::models[index].model->meshes[x].indices.size(); y += 3)
				{
					unsigned int i0 = RenderHandler::models[index].model->meshes[x].indices[y];
					unsigned int i1 = RenderHandler::models[index].model->meshes[x].indices[y + 1];
					unsigned int i2 = RenderHandler::models[index].model->meshes[x].indices[y + 2];

					if (i0 >= RenderHandler::models[index].model->meshes[x].vertices.size() ||
						i1 >= RenderHandler::models[index].model->meshes[x].vertices.size() ||
						i2 >= RenderHandler::models[index].model->meshes[x].vertices.size()) {
						continue;
					}


					glm::vec3 a = RenderHandler::models[index].model->meshes[x].vertices[i0].position;
					glm::vec3 b = RenderHandler::models[index].model->meshes[x].vertices[i1].position;
					glm::vec3 c = RenderHandler::models[index].model->meshes[x].vertices[i2].position;

					FE_Math::transformPoint(a, finalMatrix);
					FE_Math::transformPoint(b, finalMatrix);
					FE_Math::transformPoint(c, finalMatrix);

					// run hit test and return result
					Collision::HitResult triHit = Collision::RayVsTriangle(rayPos, rayDir, a, b, c);

					if (triHit.isColliding && triHit.distance < finalResult.distance)
					{
						finalResult = triHit;
					}
				}
			}
		}
	}
	return finalResult;
}


void entity::draw(){
	if (!component.flags.render) return;

	switch (type){
		case ENT_MODEL_TYPE:{
			RenderHandler::renderQueueData newRenderData;
			newRenderData.RenderID = component.render.renderID;
			newRenderData.shaderUUID = component.systems.material.Material.modelShaderUUID;
			newRenderData.gpShaderUUID = component.systems.material.Material.modelGpassShaderUUID;
			newRenderData.entityUUID = UUID;
			newRenderData.castsShadow = component.flags.castsShadow;
			newRenderData.cullFrontFace = component.flags.cullFrontFace;
			newRenderData.doCulling = component.flags.doCulling;
			newRenderData.isInstanced = component.render.drawInstanced;
			newRenderData.position = component.systems.transformation.position;
			newRenderData.rotation = component.systems.transformation.rotation;
			newRenderData.scale = component.systems.transformation.scale;
			newRenderData.pPosition = component.systems.previousTransformation.position;
			newRenderData.pRotation = component.systems.previousTransformation.rotation;
			newRenderData.pScale = component.systems.previousTransformation.scale;
				
			newRenderData.uvScale = component.systems.material.uvScale;
			RenderHandler::addToRenderQueue(newRenderData);

			// these need to be dirty
			int index = RenderHandler::fetchModelIndex(component.render.renderID);
			if (index != -1){
				//raytracer::uvScaleUpdate(component.render.instanceUUID, component.systems.material.uvScale);
				//FlouraSWRT::updateUVscale(component.render.instanceUUID, component.systems.material.uvScale);
			}
			for (size_t i = 0; i < component.collider.rootnodes.size(); i++){
				if (Collision::showBoxCollider){
					CubeVisualizer::draw(component.collider.rootnodes[i].position,
		component.collider.rootnodes[i].size, glm::vec3(1.0f, 0.0f, 1.0f), 1.0, true, false);
					CubeVisualizer::draw(component.collider.modelNode.position,
		component.collider.modelNode.size, glm::vec3(0.0f, 1.0f, 1.0f), 2.0, true, false);
				}

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		break;
	}
		case ENT_BILLBOARD_TYPE:{
			component.render.BillBoard->draw();
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		break;
		}
	}
}

void entity::drawShadowMap(){
	if (!component.flags.castsShadow) return;
	if (type == ENT_BILLBOARD_TYPE)
		LightingHandler::drawShadowMapBillboard(component.render.BillBoard, component.systems.transformation.position, component.systems.transformation.scale);
}

// fetch model matrix,
// fetch model data

void entity::updateCollision(){
	entity::updateMeshAABBs();
	switch (type){
	case ENT_MODEL_TYPE:{
		int index = RenderHandler::fetchModelIndex(component.render.renderID);
		if (index != -1){

			// just for now do camera vs aabb from meshes, in future there should
		// be a collision handler, that does objects vs objects for aabbs
			for (size_t i = 0; i < component.collider.rootnodes.size(); i++){
				// Calculate global position and scale

				// these two are bugged

				glm::vec3 globalPosition = component.collider.rootnodes[i].position;
				//glm::vec3 globalPosition = component.systems.transformation.position * component.renderHeads.Model->MeshAABBs[i].position;
				//glm::vec3 globalPosition = component.renderHeads.Model->MeshAABBs[i].position;
				//glm::vec3 globalPosition = component.systems.transformation.position;

				glm::vec3 globalSize = component.collider.rootnodes[i].size;


				//std::cout << "x" << component.renderHeads.Model->MeshAABBs[i].position.x << std::endl;

				// Collision check using global position and scale
				Collision::HitResult collisionData = Collision::AABBvsAABB(globalPosition, globalSize,
					glm::vec3(Scene::maincamera.Position.x, (Scene::maincamera.Position.y - (Player::cameraColliderScale.y / 2.0f)), Scene::maincamera.Position.z),
					Player::cameraColliderScale);

				// Handle collision logic
				if (collisionData.isColliding){
					Scene::maincamera.Position = glm::vec3(collisionData.lastHit.x,
						(collisionData.lastHit.y + (Player::cameraColliderScale.y / 2.0f)),
						collisionData.lastHit.z);
					CubeVisualizer::draw(collisionData.lastHit, glm::vec3(0.1f), glm::vec3(1.0f, 0.0f, 0.0f),2.0, true, false);

					if (collisionData.collisionNormal == glm::vec3(0.0f, 1.0f, 0.0f)) // up or down
						Player::isColliding = true; // Set collision state
				}
			}
		}
		//RenderClass::line->draw(glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	}

	default:
		break;
	}


}

void entity::updateMeshAABBs()
{
	if (component.render.dirtyTransform){
		component.render.dirtyTransform = false;

		if (type == ENT_MODEL_TYPE){
			int index = RenderHandler::fetchModelIndex(component.render.renderID);
			if (index != -1){

				RenderHandler::models[index].model->updatePosition(component.systems.transformation.position);
				RenderHandler::models[index].model->updateRotation(component.systems.transformation.rotation);
				RenderHandler::models[index].model->updateScale(component.systems.transformation.scale);
				RenderHandler::models[index].model->updateTranformation();
				RenderHandler::models[index].model->updateMeshAABBs();
				component.collider.rootnodes = RenderHandler::models[index].model->rootnodes;
				updateModelBounds();
			}
			//component.renderHeads.Model->updateMeshAABBs();
		}
	}

}

void entity::updateModelBounds(){
	std::vector<glm::vec3> points;
	
	for (size_t i = 0; i < component.collider.rootnodes.size(); i++){
		Collision::minmax newMinMax = Collision::returnMinMax(component.collider.rootnodes[i].position, component.collider.rootnodes[i].size);

		// push points into array
		points.push_back(newMinMax.max);
		points.push_back(newMinMax.min);
	}
	component.collider.modelNode = Collision::createAABBfromPoints(points);
	
	// give max extent and turn into range
	component.collider.range = glm::max(component.collider.modelNode.size.x,  glm::max(component.collider.modelNode.size.y, component.collider.modelNode.size.z));
	
	// set colliders
	component.physobject.collisionObject.aabb = component.collider.modelNode;
	component.physobject.collisionObject.sphere.position = component.collider.modelNode.position;
	component.physobject.collisionObject.sphere.radius = component.collider.range;
	
	//component.physobject.collisionObject.aabb.size *= 2.0f;
	//component.physobject.collisionObject.sphere.radius *= 2.0f;
}

void entity::createModel(const std::string& path, const std::string& materialPath){
	RenderHandler::batchOfUUID newBatchOfUUID = RenderHandler::addModel(path);
	component.render.renderID = newBatchOfUUID.RenderID;
	component.render.instanceUUID = newBatchOfUUID.instanceUUID;
	component.render.renderIDString = UUID::UUIDToString(newBatchOfUUID.RenderID);
	component.render.instanceIDString = UUID::UUIDToString(newBatchOfUUID.instanceUUID);
	component.render.dirtyTransform = true;
	entity::updateMeshAABBs();
	component.systems.material.Material.LoadMaterial(materialPath);
	
	int index = RenderHandler::fetchModelIndex(component.render.renderID);
	if (index != -1){
		FlouraSWRT::uploadToLSDFScene(component.render.instanceUUID);
	}
}

void entity::createBillBoard(const std::string& path){
	component.render.BillBoard = new BillBoard(path);
}

void entity::sendEntityUniformsToScripts(ScriptObject* obj){
	sol::table transform = obj->getOrCreateTable("transform");
	obj->setUniform("positionX", transform, sol::make_object(obj->luaState, component.systems.transformation.position.x));
	obj->setUniform("positionY", transform, sol::make_object(obj->luaState, component.systems.transformation.position.y));
	obj->setUniform("positionZ", transform, sol::make_object(obj->luaState, component.systems.transformation.position.z));
	obj->setUniform("scaleX", transform, sol::make_object(obj->luaState, component.systems.transformation.scale.x));
	obj->setUniform("scaleY", transform, sol::make_object(obj->luaState, component.systems.transformation.scale.y));
	obj->setUniform("scaleZ", transform, sol::make_object(obj->luaState, component.systems.transformation.scale.z));
	obj->setUniform("rotationX", transform, sol::make_object(obj->luaState, component.systems.transformation.rotation.x));
	obj->setUniform("rotationY", transform, sol::make_object(obj->luaState, component.systems.transformation.rotation.y));
	obj->setUniform("rotationZ", transform, sol::make_object(obj->luaState, component.systems.transformation.rotation.z));
	
	sol::table camera = obj->getOrCreateTable("camera");
	obj->setUniform("positionX", camera, sol::make_object(obj->luaState, Scene::maincamera.Position.x));
	obj->setUniform("positionY", camera, sol::make_object(obj->luaState, Scene::maincamera.Position.y));
	obj->setUniform("positionZ", camera, sol::make_object(obj->luaState, Scene::maincamera.Position.z));
}

void entity::getEntityUniformsToScripts(ScriptObject* obj){
	sol::table transform = obj->getOrCreateTable("transform");
	setPosition(glm::vec3(transform["positionX"].get<float>(), transform["positionY"].get<float>(), transform["positionZ"].get<float>()));
	setScale(glm::vec3(transform["scaleX"].get<float>(), transform["scaleY"].get<float>(), transform["scaleZ"].get<float>()));
	setRotation(glm::vec3(transform["rotationX"].get<float>(), transform["rotationY"].get<float>(), transform["rotationZ"].get<float>()));
}

void entity::initEntityTables(ScriptObject* obj){
	obj->createTable("transform");
}
