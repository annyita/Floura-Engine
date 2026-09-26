#ifndef FE_OBJECT_H
#define FE_OBJECT_H

#include <string>
#include <Render/Object/ModelAssimp.h>
#include <Render/Object/Billboard.h>
#include <Render/Shader/Material.h>
#include "Systems/Physics/Collision.h"
#include <Render/pipeline/prebuilt_pipelines/depreciated/raytracer.h>
#include <vector>
#include <Scripting/ScriptObject.h>
#include <Scene/ProbeHandler.h>
//#include <Scene/scene.h>
#include "Systems/Physics/physworld.h"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

class entity
{
public:

	//structs
	struct material {
		Material Material;
		glm::vec2 uvScale = glm::vec2(1.0f);
	};

	struct flags {
		bool isStatic = false;
		bool castsShadow = true;
		bool render = true;
		bool doCulling = true;
		bool cullFrontFace = false;
	};

	struct transformation {
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 rotation = glm::vec3(0.0f);
		glm::vec3 scale = glm::vec3(1.0f);
	};

	struct systems {
		transformation htransform;
		transformation previousTransformation;
		transformation transformation;
		material material;
	};

	struct collider { // this is stupid but will stay for broad phase
		Collision::AABB modelNode;
		float range = 0.0f;
		std::vector<Collision::AABB> rootnodes;
	};

	struct render {
		//Model* Model;
		std::string renderIDString;
		std::string instanceIDString;
			uint64_t renderID;
			uint64_t instanceUUID;
		BillBoard* BillBoard;
		bool dirtyTransform = false;
		bool drawInstanced = false;
	};

	struct relationship{
		bool hasParent = false;
		uint64_t parentUUID;
		std::vector<uint64_t> childUUID;
	};

	struct components {
		flags flags;
		physworld::object physobject;
		collider collider;
		systems systems;
		render render;
		relationship relationship;
	};
	std::string name;

	enum ENT_TYPE_ENUM{
		ENT_MODEL_TYPE	    =  0,
		ENT_BILLBOARD_TYPE  = 1,
		ENT_EMPTY_TYPE			=  2
	};
	
	ENT_TYPE_ENUM type;
	
	// replace type with int and enum
	//char type;
	std::string path;
	std::vector<ScriptObject*> ScriptObjects;

	//

	void createwUUID(uint64_t nUUID, ENT_TYPE_ENUM type, const std::string& name, const std::string& path, const std::string& materialPath);

	void create(ENT_TYPE_ENUM type, const std::string& name, const std::string& path, const std::string& materialPath);
	
private:
	void createGeneralLogic();
public:

	void LoadMaterial(std::string path);

	void addScript(std::string path, std::string name);

	void reloadScript(int index);

	void removeScript(int index);

	void updateScripts();

	void initScript(int index);

	void update();
	
	void updatePhysicsDynamics(float deltatime);

	void draw();

	void drawShadowMap();

	void Delete();
	
	bool queuedForDeletion = false;
	
	void queuedDeletion(); // the real deletion, delete stays for existing architecture

	//void addParent();

	// aabb vs entity here
	Collision::HitResult AABBVsEntity(glm::vec3 pos, glm::vec3 scale);
	Collision::HitResult RayVsEntity(glm::vec3 rayPos, glm::vec3 rayDir);
		void updateCollision();
		void updateMeshAABBs();
		void updateModelBounds();

	components component;
	uint64_t UUID;
	std::string UUIDstring;

	private:
	void createModel(const std::string& path, const std::string& materialPath);
	void createBillBoard(const std::string& path);

	public:

	// transformations
	glm::vec3 fetchPosition() {return component.systems.transformation.position;}
	glm::vec3 fetchRotation() {return component.systems.transformation.rotation;}
	glm::vec3 fetchScale() {return component.systems.transformation.scale;}
	
	void setPosition(const glm::vec3& position) {
		if (position == component.systems.transformation.position) return;
		component.systems.transformation.position = position;
		raytracer::RTGlobalTransformFlag = true;
		component.render.dirtyTransform = true;
		ProbeHandler::dirtyScene = true;
	}
	void setRotation(const glm::vec3& rotation) {
		if (rotation == component.systems.transformation.rotation) return;
		glm::vec3 nr = rotation;
		if (nr.x > 360) nr.x = 0.0f;
		if (nr.y > 360) nr.y = 0.0f;
		if (nr.z > 360) nr.z = 0.0f;
		component.systems.transformation.rotation = nr;
		raytracer::RTGlobalTransformFlag = true;
		component.render.dirtyTransform = true;
		ProbeHandler::dirtyScene = true;
	}
	void setScale(const glm::vec3& scale) {
		if (scale == component.systems.transformation.scale) return;
		component.systems.transformation.scale = scale;
		raytracer::RTGlobalTransformFlag = true;
		component.render.dirtyTransform = true;
		ProbeHandler::dirtyScene = true;
	}
	private:
		void sendEntityUniformsToScripts(ScriptObject* obj);
		void getEntityUniformsToScripts(ScriptObject* obj);
		void initEntityTables(ScriptObject* obj);
};

#endif