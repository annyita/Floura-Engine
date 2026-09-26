#ifndef RENDER_HANDLER_CLASS_H
#define RENDER_HANDLER_CLASS_H

#include <iostream>
#include <xhash>
#include <map>
#include<unordered_map>
#include<Render/Object/ModelAssimp.h>
#include<vector>
#include<camera/Camera.h>
#include<Render/Object/Cubemap.h>
#include <Render/Object/Texture.h>

class RenderHandler
{
public:

	struct modelObject
	{
		uint64_t RenderID;
		Model* model;
		int instances;
		std::string path;
	};

	struct renderQueueData
	{
		uint64_t RenderID;
		uint64_t instanceUUID;
		uint64_t shaderUUID;
		uint64_t gpShaderUUID;
		uint64_t entityUUID;
		int isInstanced; // will come into play way later
		glm::vec3 position;
		glm::vec3 rotation;
		glm::vec3 scale;
		
		// previous
		glm::vec3 pPosition;
		glm::vec3 pRotation;
		glm::vec3 pScale;
		
		glm::vec2 uvScale;
		bool doCulling;
		bool cullFrontFace;
		bool castsShadow;
	};

	struct batchOfUUID{
		uint64_t RenderID;
		uint64_t instanceUUID;
	};

	static bool renderENV;
	
	static int drawCount;

	static std::vector<modelObject> models;

	static std::vector<renderQueueData> renderQueueDataVector;

	static uint64_t fetchHandle(std::string path);

	static int fetchModelIndex(uint64_t RenderID);
	
	static void updateLoadUnloadedModels(); // loads fresh models, that are not loaded yet

	static batchOfUUID addModel(std::string path); // returns id for use

	static void addToRenderQueue(renderQueueData data);

	static void clearRenderQueue();

	static void render();

	static void removeInstancewRenderID(uint64_t RenderID, uint64_t instanceID);
	
	static void removeInstance(int index);

	static uint64_t findRenderUUIDwIstanceUUID(uint64_t InstanceUUID);

	static uint64_t findModelUUIDwRenderUUID(uint64_t RenderID);

	static uint64_t findModelUUIDwInstanceUUID(uint64_t InstanceUUID);

	static void init();

	static Cubemap* tempCM; // made pub
private:

	static Shader cmShader;

	static void cmDraw(std::vector<renderQueueData> rqdVector, Cubemap*& cm, Shader& shader, glm::vec2 resolution, glm::vec3 pos, float range);

	static void regularDraw();
	
	static void shadowDraw();

	static void instancedDraw();

	static std::unordered_map<std::string, uint64_t> pKeyHandleMapRender; // takes path as key, returns handle
	
};
#endif // RENDER_HANDLER_CLASS_H