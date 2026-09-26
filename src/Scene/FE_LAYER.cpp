#include "FE_LAYER.h"

#include <typeindex>
#include <Render/Handler/RenderClass.h>
#include "camera/Camera.h"
#include "utils/FE_math.h"
#include "Render/window/WindowHandler.h"
#include <Scene/Object/Entity.h>
#include "Scene/scene.h"
#include "Render/Handler/CubeVisualizer.h"
#include <Render/Animated/animator.h>
#include "Render/Handler/RenderHandler.h"
#include "utils/timeUtil.h"
#include "Systems/Physics/physworld.h"
#include <Systems/Physics/accelerate.h>

#include "Systems/Physics/SDF.h"


//uint64_t gunID = 0;
//uint64_t renderID = 0;

uint64_t FE_LAYER::pivotPointID = 0;
uint64_t victimPointID = 0;

Shader temporaryTerrainShader;
int size = 50;
//Collision::octSplit osv;
//std::vector<Collision::AABB> aabbs;
void FE_LAYER::init(){
	//glm::vec3 p(0.0f); glm::vec3 e(20.0f);
	//voxelizer::octSplitEmptySpace(aabbs, p, e, 2);
	//voxelizer::uniformSplitEmptySpace(aabbs, p, e, 4);
	//osv = Collision::octSplitVolume(glm::vec3(0.0), glm::vec3(3.0, 3.0, 5.0));
	/*
	std::vector<Texture3D*> nTA;
	Texture3D nTexture;
	Scene::voxelizeArea(glm::vec3(8.277,3.111, -9.802), glm::vec3(1.0f), nTexture,32, 15);
	nTA.push_back(&nTexture);
	voxelizer::cacheVXG("Cache/VXG/temp/", 69, nTA);
	*/
	//return;
	//Collision::KDsplit kds = Collision::KDsplitVolume(glm::vec3(0.0), glm::vec3(1.0, 2.0, 1.0));
	
	//return;
	/*
	physworld::emitter estrogenEmitter;
	estrogenEmitter.enabled = true;
	estrogenEmitter.position = glm::vec3(0.0f, 5.0f, 0.0f);
	estrogenEmitter.UUID = UUID::returnHandle();
	estrogenEmitter.lifespan = 1000.0f;
	estrogenEmitter.maxDistance = 20.0f;
	estrogenEmitter.spawnTickrate = 30.0f;
	estrogenEmitter.limit = 64;
	estrogenEmitter.templatePhysicsObject.affectedByGravity = true;
	//estrogenEmitter.gravity  = glm::vec3(0.0, 5.81, 0.0f);
	
	physworld::uploadEmitter(estrogenEmitter);
	//return;
	int radius = 4;
	
	for (int x = -radius; x < radius; x+=2)
		for (int y = -radius; y < radius; y+=2)
		{
			estrogenEmitter.position = glm::vec3(x, 0.0f, y);	
			physworld::uploadEmitter(estrogenEmitter);
		}
	
	
	return;
	*/
	//return;
	//"temp/stanford_dragon_pbr.glb" // "temp/sponzacrytek/sponza.obj" // "Assets/Models/basic shapes 2/sphere.gltf" // temp/stanforddragon/stanforddragon.gltf // temp/fireplace_room/fireplace_room.obj
	//victimPointID = Scene::AddEntityObject(entity::ENT_MODEL_TYPE,"VictimPoint (FE_LAYER.CPP)", "Assets/Models/pdf_teto/scene.gltf", glm::vec3(0.0f, 0.0f, 0.0f),glm::vec3(0.100), glm::vec3(0.0f) );
	// temp/bistro/assets/annasbistro/annasbistro.gltf
	//pivotPointID = Scene::AddEntityObject(entity::ENT_MODEL_TYPE,"PivPoint (FE_LAYER.CPP)", "Assets/Models/basic shapes 2/sphere.gltf", glm::vec3(0.0f, 0.0f, 0.0f),glm::vec3(1.0), glm::vec3(0.0f) );
	//pivotPointID = Scene::AddEntityObject(entity::ENT_MODEL_TYPE,"PivPoint (FE_LAYER.CPP)", "temp/fireplace_room/fireplace_room.obj", glm::vec3(-2.0f, 0.0f, 0.0f),glm::vec3(1.0), glm::vec3(0.0f) );
	//pivotPointID = Scene::AddEntityObject(entity::ENT_MODEL_TYPE,"PivPoint (FE_LAYER.CPP)", "temp/stanforddragon/stanforddragon.gltf", glm::vec3(-2.0f, 0.0f, 0.0f),glm::vec3(1.0), glm::vec3(0.0f) );
	//pivotPointID = Scene::AddEntityObject(entity::ENT_MODEL_TYPE,"PivPoint (FE_LAYER.CPP)", "temp/erato/erato.obj", glm::vec3(-2.0f, 0.0f, 0.0f),glm::vec3(0.1), glm::vec3(0.0f) );
	//pivotPointID = Scene::AddEntityObject(entity::ENT_MODEL_TYPE,"PivPoint (FE_LAYER.CPP)", "temp/sponzacrytek/sponza.obj", glm::vec3(0.0f, 0.0f, 0.0f),glm::vec3(1.0), glm::vec3(0.0f) );
	//pivotPointID = Scene::AddEntityObject(entity::ENT_MODEL_TYPE,"PivPoint (FE_LAYER.CPP)", "temp/bistro/assets/annasbistro/annasbistro.gltf", glm::vec3(0.0f, 0.0f, 0.0f),glm::vec3(1.0), glm::vec3(0.0f) );
	
	//fireplace_room
	
	for (int i = 0; i < Scene::entityObjects.size(); ++i) 
		if (pivotPointID == Scene::entityObjects[i]->UUID){
			int mIndex = RenderHandler::fetchModelIndex(Scene::entityObjects[i]->component.render.renderID);
			//RenderHandler::models[mIndex].model->generateMeshBlases(0, 8); // 8 is the lowest ill steep
			//RenderHandler::models[mIndex].model->SDFgenerateBlas(64, 15); // 
			//RenderHandler::models[mIndex].model->SDFgeneratePrim(64, 15);
			//RenderHandler::models[mIndex].model->SDFgenerateBlas(64, 15, "Cache/SDF/temp/"); // 
			//RenderHandler::models[mIndex].model->SDFgeneratePrim(32, 15, "Cache/SDF/temp/");
			//flouraSDF::uploadToLSDFScene(Scene::entityObjects[i]->component.render.instanceUUID);

			//RenderHandler::models[mIndex].model->SDFgenerate(12, 4, 32,  0,"Cache/SDF/temp/");
			
			//blasT = BVH::blasGenKD(RenderHandler::models[mIndex].model->meshes[0].vertices, RenderHandler::models[mIndex].model->meshes[0].indices, RenderHandler::models[mIndex].model->ModelBounds, 48, 4, glm::mat4(1.0f));
			//std::cout << "leaf count: " << blasT.size() << std::endl;
			
				//voxelizer::bakeMeshSDF(RenderHandler::models[mIndex].model->meshes[0].vertices, RenderHandler::models[mIndex].model->meshes[0].indices, RenderHandler::models[mIndex].model->ModelBounds, 64, *RenderClass::Susanne64, 0);
				//RenderHandler::models[mIndex].model->meshes[0].vertices;
				//RenderHandler::models[mIndex].model->createVoxelMesh(8, 0, glm::vec3(0.001f), false); // voxelize model instead of mesh
				//RenderHandler::models[mIndex].model->createVoxelMesh(12, 0, glm::vec3(0.1f), false); // voxelize model instead of mesh
			//}
		}
	//GL_RED, GL_UNSIGNED_BYTE, 1
	//FlouraImageWrite::writeImage3DToDisk(RenderClass::Susanne64->ID, RenderClass::Susanne64->width, RenderClass::Susanne64->height, RenderClass::Susanne64->depth, "Assets/volume/SDF/standfordpbr_64.png", GL_RGB, GL_UNSIGNED_BYTE, 3);
	
	// upload to buffer (cause not on entity yet)
	//for (int i = 0; i < Scene::entityObjects.size(); ++i) 
		//if (pivotPointID == Scene::entityObjects[i]->UUID){
				//SceneDescription::uploadToVoxelScene(Scene::entityObjects[i]->component.render.instanceUUID);
			//}
	
	
	/*
	for (int i = 0; i < Scene::entityObjects.size(); ++i) 
		if (victimPointID == Scene::entityObjects[i]->UUID){
			Scene::entityObjects[i]->component.physobject.hasRigidbody = true;
			Scene::entityObjects[i]->component.physobject.affectedByGravity = true;
		}
	*/
	
	return;
	
	temporaryTerrainShader.LoadShaderGeom("Assets/Shaders/Db/VerticeViewer.vert","Assets/Shaders/Db/VertexViewer.frag", "Assets/Shaders/Db/VerticeViewer.geom"); 
	//gunID = Scene::AddEntityObject(entity::ENT_MODEL_TYPE,"gun_loaded_from_cpp", "temp/Capoeira.fbx", glm::vec3(0.0f),glm::vec3(0.01), glm::vec3(0.0f) );

	//for (int i = 0; i < Scene::entityObjects.size(); ++i) if (gunID == Scene::entityObjects[i]->UUID) renderID = Scene::entityObjects[i]->component.render.renderID;
	
	//int modelIndex =RenderHandler::fetchModelIndex(renderID);
	//if (modelIndex != -1)
	//{
		
		// outside here one of these VV doesnt intergrade well, hangs ev everything
		
		
		//Animation danceAnimation("temp/Capoeira.fbx",RenderHandler::models[modelIndex].model);
		//Animator animator(&danceAnimation);
		//animator.PlayAnimation(&danceAnimation);
		//animator.UpdateAnimation(TimeUtil::deltatime);
		//Animator animator(&danceAnimation);
		
		//animator.init(&danceAnimation);
	//}
	
}

//glm::vec3 position = glm::vec3(0.0f, 0.0f, 2.0f);
void FE_LAYER::Update(){
	
	return;
	glm::vec3 pPos = glm::vec3(0.0f, 0.0f, 0.0f);
	
	for (int i = 0; i < Scene::entityObjects.size(); ++i)  if (pivotPointID == Scene::entityObjects[i]->UUID) pPos = Scene::entityObjects[i]->fetchPosition();
	
	for (int i = 0; i < Scene::entityObjects.size(); ++i) 
		if (victimPointID == Scene::entityObjects[i]->UUID){
			// advancedConstrainPoint
			Collision::HitResult HR = Collision::advancedConstrainPoint(Scene::entityObjects[i]->fetchPosition(),glm::vec3(pPos), 10.0f);
			Scene::entityObjects[i]->setPosition(HR.lastHit);
			
			//Scene::entityObjects[i]->setPosition(Collision::constrainPoint(Scene::entityObjects[i]->fetchPosition(),glm::vec3(pPos), 10.0f));
			
			if (HR.isColliding){
				glm::vec3& vel = Scene::entityObjects[i]->component.physobject.velocity;
				float dot = glm::dot(vel, HR.collisionNormal);
				
				if (dot < 0.0f) vel -= dot * HR.collisionNormal;

				Scene::entityObjects[i]->component.physobject.force += (-HR.collisionNormal * HR.depth * 50.0f);
			}
		}
	
	return;
	
	/*
	glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer::FBO);

	float closestDist = std::numeric_limits<float>::max();
	Collision::HitResult closestResult;
	bool hitAnything = false;

	glm::vec3 rayPos = Camera::Position;
	glm::vec3 rayDir = Camera::Orientation;

	//glm::vec3 rayPos = glm::vec3(0.0f, 10.0f, 0.0f);
	//glm::vec3 rayDir = glm::vec3(0.0f, -1.0f, 0.0f);

	for (size_t i = 0; i < Scene::entityObjects.size(); i++)
	{
		if (glfwGetMouseButton(windowHandler::window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{

			Collision::HitResult result = Scene::entityObjects[i]->RayVsTriangle(rayPos, rayDir);

			if (result.isColliding && result.distance < closestDist) {
				{
					closestDist = result.distance;
					closestResult = result;
					hitAnything = true;
					
				}
			}
		}

		if (hitAnything) {
			cube->draw(closestResult.lastHit, glm::vec3(0.1f), glm::vec3(1.0f, 0.0f, 0.0f));
			std::cout << "hit" << std::endl;
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	*/

}
void FE_LAYER::onBeginningOfFrame(){
}

void FE_LAYER::draw(){
	//CubeVisualizer::draw(glm::vec3(0.0), glm::vec3(20.0), glm::vec3(1.0, 0.0, 0.0), 3.5, true, false);

	//for (int i = 0; i < aabbs.size(); ++i)
	//	CubeVisualizer::draw(aabbs[i].position, aabbs[i].size, glm::vec3(1.0), 0.5, true, false);
	/*

	CubeVisualizer::draw(osv.splitTRF, osv.size, glm::vec3(1.0), 0.5, true, false);
	CubeVisualizer::draw(osv.splitTLF, osv.size, glm::vec3(1.0), 0.5, true, false);
	CubeVisualizer::draw(osv.splitTRB, osv.size, glm::vec3(1.0), 0.5, true, false);
	CubeVisualizer::draw(osv.splitTLB, osv.size, glm::vec3(1.0), 0.5, true, false);
	
	CubeVisualizer::draw(osv.splitDRF, osv.size, glm::vec3(1.0), 0.5, true, false);
	CubeVisualizer::draw(osv.splitDLF, osv.size, glm::vec3(1.0), 0.5, true, false);
	CubeVisualizer::draw(osv.splitDRB, osv.size, glm::vec3(1.0), 0.5, true, false);
	CubeVisualizer::draw(osv.splitDLB, osv.size, glm::vec3(1.0), 0.5, true, false);
	*/
}

void FE_LAYER::Delete(){
}
