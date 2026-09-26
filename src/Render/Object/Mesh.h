#ifndef MESH_CLASS_H
#define MESH_CLASS_H

#include "Systems/Physics/Collision.h"
#include <Render/Shader/shaderClass.h>
#include "string"
#include "iostream"
#include "Render/Buffer/VAO.h"
#include "Render/Buffer/EBO.h"
#include "Render/Object/Texture.h"
#include <xhash>
#include <camera/Camera.h>
#include <Systems/Physics/accelerate.h>

class Mesh {

	private:
	
	enum  drawTypes{
		POINT = 0,
		LINE = 1,
		TRIANGLE = 2
	};
	
	public:
	
	drawTypes drawType = TRIANGLE;
	
    uint64_t UUID;

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;
	
    void create(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, std::vector<Texture>& textures);
    void createWithoutTexture(std::vector<Vertex>& vertices, std::vector<GLuint>& indices);
	
	void genBlas(int mintri, int maxDepth);
	
	void bindMaterial(Shader &shader);
    void draw(Shader &shader, Camera Camera);
    void drawInstanced(Shader& shader, Camera Camera, int instanceCount);
    void Delete();

    void updateMatrix(glm::mat4 matrix);
	//void updatePrevMatrix(glm::mat4 matrix);
    void updatePosition(glm::vec3 position);
	void updateRotation(glm::vec3 rotation);
	void updateScale(glm::vec3 scale);
	void updateGlobalMatrix(glm::mat4 matrix);
	void updatePrevGlobalMatrix(glm::mat4 matrix);
    void updateGlobalPosition(glm::vec3 position);
    void updateGlobalScale(glm::vec3 scale);
    void updateGlobalRotation(glm::vec3 rotation);

    std::string name = "empty";
	bool culled = false; // reset on end of draw;
	bool suppressSetupMeshCall = false;
	bool hasLod = false;
	bool generateLod = false;
	int forceLodLevel = -1; // -1 = off
	bool created = false;
	bool HasBones = false;
	float transitionDistance = 5.0f;
	
	// exposed for threadding
	void setupMesh();
	
	std::vector<accelerate::leaf> blas;
	Collision::rubiksCubePoints meshAabbPoints;
    
private:

	
    glm::mat4 meshMatrix = glm::mat4(1.0f);
	//glm::mat4 meshPrevMatrix = glm::mat4(1.0f);
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::mat4 globalMeshMatrix = glm::mat4(1.0f);
	glm::mat4 globalPrevMeshMatrix = glm::mat4(1.0f);
    glm::vec3 globalPosition = glm::vec3(0.0f);
    glm::vec3 globalScale = glm::vec3(1.0f);
    glm::vec3 globalRotation = glm::vec3(0.0f, 0.0f, 0.0f);
	
    VAO VAO;
	VBO nVBO;
	EBO nEBO;
	
	EBO lodEBOs[4];
	size_t lodSizes[4];
	
};

#endif
