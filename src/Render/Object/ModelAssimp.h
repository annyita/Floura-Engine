#ifndef Model_CLASS_H
#define Model_CLASS_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <Render/Shader/shaderClass.h>
#include "string"
#include "iostream"
#include "Mesh.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <Render/Handler/RenderClass.h>
#include "Systems/Physics/Collision.h"
#include <map>
#include <iterator>
#include "Render/Animated/animdata.h"
#include <Systems/Physics/voxelizer.h>
#include <Render/Object/texture3D.h>

#define MAX_BONE_INFLUENCE 4

class Model {
public:

	uint64_t UUID;// modelID
	uint64_t renderID;
	//std::vector<uint64_t>instanceUUIDs;
	struct instaceData{
		uint64_t ID;
	};
	std::vector<instaceData> instacesData;

	bool disableConstructorLoadingModelFlag = false; // for threadding, if true it'll stop the class constructor calling the load function 
	bool disableInitialMeshUploadToVBOFlag = false; // for threading, stops the uploading to the gpu outside the opengl thread (very big nono, opengl doesnt like)
	bool disableInitialTextureUploadToGPUFlag = false; // for threading again, stops the textures uploading to the gpu again
	bool doLodsDraw = false;
	int forceLodLevel = -1;
	bool loaded = false; // blocks certain calls unless loaded
	bool sdfCompatible = true;;
	
	struct transformation {
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 rotation = glm::vec3(0.0f);
		glm::quat qRotation = glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 scale = glm::vec3(1.0f);
	};

	transformation globalTransformation;
	transformation previousGlobalTransformation;
	glm::mat4 gModelMatrix = glm::mat4(1.0f);
	glm::mat4 pgModelMatrix = glm::mat4(1.0f);

	void updatePosition(glm::vec3 Position);
	void updateRotation(glm::vec3 Rotation);
	void updateScale(glm::vec3 Scale);
	void updateTranformation();
	
	void updatePrevPosition(glm::vec3 Position);
	void updatePrevRotation(glm::vec3 Rotation);
	void updatePrevScale(glm::vec3 Scale);
	void updatePrevTranformation();
	
	
	void childrenRangeCull(glm::vec3 position, float range);

	Model(const char* file, bool disableConstructorLoading, bool disableInitialMeshUploadToVBO, bool disableInitialTextureUploadToGPU);
	~Model();
	
	void loadModelPathless(); // takes from path string
	
	auto& GetBoneInfoMap() { return m_BoneInfoMap; }
	int& GetBoneCount() { return m_BoneCounter; }

	void draw(Shader &shader, Camera Camera);

	void drawInstance(Shader& shader, Camera Camera, int instanceCount);

	void createMeshAABBs();
	
	void generateMeshBlases(int mintri, int maxDepth);
	
	void MDFgenerateBlas(int sliceSize, GLuint slot);
	void MDFgeneratePrimGPU(int sliceSize, GLuint slot, float thickness);
	void MDFgenerateGridPrimGPU(int sliceSize, GLuint slot, float thickness, int div);

	void updateMeshAABBs();

	// AABBS position stored in local space
	std::vector<Collision::AABB> rootnodes;
	Collision::AABB ModelBounds;
	int mdfFormation = 0;
	std::vector<Collision::AABB> mdfNodes;
	std::vector<Texture3D *> meshSDFs;
	std::vector<Texture3D *> meshSDFs2;
	std::vector<Mesh> meshes;
	std::vector<Collision::rubiksCubePoints> meshAabbPoints;

	std::vector<transformation> localTransformation;
	std::vector <glm::mat4> lModelMatrix;
	
	// informative
	int totalVertices = 0;
	int totalIndices = 0;
	int totalBones = 0;

	
	std::string path; 
	unsigned int hash;
	
private:
	std::vector<std::string> loadedTexPath;
	std::vector<Texture> loadedTex;
	std::string directory;
	
	// bones
	std::map<std::string, BoneInfo> m_BoneInfoMap;
	int m_BoneCounter = 0;
	
	void loadModel(std::string path);
	void processNode(aiNode* node, const aiScene* scene);
	void processPositions(aiNode* node);
	
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);

	void SetVertexBoneDataToDefault(Vertex& vertex);
	void SetVertexBoneData(Vertex& vertex, int boneID, float weight);
	void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene);
	
	std::vector<Vertex> assembleVertices(aiMesh* mesh);
	std::vector<GLuint> assembleIndices(aiMesh* mesh);
	std::vector<Texture> assembleMaterials(aiMesh* mesh, const aiScene* scene);
	
	
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type,
		std::string typeName, int slot);
	
};

#endif
