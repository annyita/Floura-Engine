#include "Mesh.h"
#include <utils/logConsole.h>
#include <limits>
#include <utils/FE_math.h>
#include <Render/Object/Skybox.h>
#include "Scene/scene.h"
#include "Systems/util/UUID.h"
#include <Render/render_util/meshTools.h>

#include "Render/Handler/RenderHandler.h"


void Mesh::create(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, std::vector<Texture>& textures){
    UUID = UUID::returnHandle();

    // err checking
    if (vertices.empty()) LogConsole::print("mesh.cpp Vertices are empty");
    if (indices.empty()) LogConsole::print("mesh.cpp indices are empty");
    if (textures.empty()) LogConsole::print("mesh.cpp textures are empty");

    if (vertices.empty() && indices.empty())
    {
        LogConsole::print("mesh.cpp vertices are empty");
        return; // avoid passing on cause will cause issues
    }

    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;

    if (!suppressSetupMeshCall) setupMesh();
}

void Mesh::createWithoutTexture(std::vector<Vertex>& vertices, std::vector<GLuint>& indices){
    // err checking
    if (vertices.empty()) LogConsole::print("mesh.cpp Vertices are empty");
    if (indices.empty())LogConsole::print("mesh.cpp indices are empty");
    
    if (vertices.empty() && indices.empty())
    {
        LogConsole::print("mesh.cpp vertices are empty");
        return; // avoid passing on cause will cause issues
    }

    this->vertices = vertices;
    this->indices = indices;

    if (!suppressSetupMeshCall) setupMesh();
}

void Mesh::genBlas(int mintri, int maxDepth){
    blas = accelerate::blasGenKDAccel(vertices, indices, mintri, maxDepth, glm::mat4(1.0f));
}

void Mesh::bindMaterial(Shader& shader){
    unsigned int numDiffuse = 0;
    unsigned int numSpecular = 0;
    unsigned int numNormal = 0;
    unsigned int numEmissive = 0;

    for (unsigned int i = 0; i < textures.size(); i++){
        std::string num;
        std::string type = textures[i].type;
        if (type == "texture_diffuse") {num = std::to_string(numDiffuse++);}
        else if (type == "texture_roughness") {num = std::to_string(numSpecular++);}
        else if (type == "texture_normal") {num = std::to_string(numNormal++);}
        else if (type == "texture_emission") {num = std::to_string(numEmissive++);}
        shader.Activate(); 
        shader.setHandleui64ARB((type + "_Handle").c_str(), textures[i].handle);
        textures[i].texUnit(shader, (type).c_str(), textures[i].unit);
        textures[i].Bind();
    }
}

void Mesh::draw(Shader& shader, Camera Camera){
    if (!created) return;
    
    if (culled){ culled = false; return;}
    
    shader.Activate();
    VAO.Bind();
    
    // bind material
    bindMaterial(shader);
    
    // Camera Matrix
    shader.Activate();
    //glUniform3f(glGetUniformLocation(shader.ID, "camPos"), Camera.Position.x, Camera.Position.y, Camera.Position.z);
    shader.setFloat3("camPos", Camera.Position);
    Camera.Matrix(shader, "camMatrix");
    shader.setMat4("u_ViewMatrix", Camera.view);
    shader.setMat4("u_ProjectionMatrix", Camera.projection);

    glm::mat4 finalMeshMat = globalMeshMatrix * meshMatrix;
    glm::mat4 finishedPrevMeshMat = globalPrevMeshMatrix * meshMatrix;

    //glm::mat4 newLMat = FE_Math::composeMatrixWDegrees(position, scale, rotation);
    //glm::mat4 newGMat = FE_Math::composeMatrixWDegrees(globalPosition, globalScale, globalRotation);

	//glm::mat4 newMat = newGMat * newLMat;

    // Push model matrix to the vertex shader
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "model"), 1, GL_FALSE, glm::value_ptr(finalMeshMat));
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "previousModel"), 1, GL_FALSE, glm::value_ptr(finishedPrevMeshMat));
    glm::mat3 model3x3 = glm::mat3(finalMeshMat);
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(model3x3));
    glUniformMatrix3fv(glGetUniformLocation(shader.ID, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

    //glEnable(GL_BLEND);
    //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw the mesh

    size_t count = indices.size();
    
    if (hasLod){
        //glm::vec3 childPosition = glm::vec3(globalMeshMatrix * glm::vec4(position, 1.0));
        //		glm::vec3 np = glm::clamp(P, v.position - v.size, v.position + v.size); // should use mesh extents here
        Collision::AABB aabb = Collision::rootNodeFromRubixPointsNoPadding(meshAabbPoints, globalMeshMatrix);
        glm::vec3 np = Collision::nearestPointOnAABB(Camera.Position , aabb.position, aabb.size);
        int lodLevel = 0;
        if (forceLodLevel < 0) lodLevel = FE_Math::calculateLODLevel(np, Camera.Position, transitionDistance, 4) - 1;
        else lodLevel = forceLodLevel;
        //std::cout << "lodLevel = " << lodLevel << std::endl;
        
        lodLevel = 3;
        
        if (lodLevel < 0){
            nEBO.Bind();
        }
        else{
            lodEBOs[lodLevel].Bind();
            count = lodSizes[lodLevel];    
        }
        
    }
    else nEBO.Bind();
    
    switch (drawType){
    case TRIANGLE: glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0); break;
    case LINE: glDrawElements(GL_LINES, count, GL_UNSIGNED_INT, 0); break;
    case POINT: glDrawElements(GL_POINTS, count, GL_UNSIGNED_INT, 0); break;
    default: glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0); break;
    }
    
    //glDisable(GL_BLEND);

    //glBindTexture(GL_TEXTURE_2D, 0);
}

void Mesh::drawInstanced(Shader& shader, Camera Camera, int instanceCount){
    if (!created) return;
    std::cout << "drawing instance" << instanceCount << std::endl;
}

void Mesh::setupMesh(){
    VAO.Bind();
    
    nVBO.generateVBO(vertices);
    nEBO.generateEBO(indices);
    
    const size_t stride = sizeof(Vertex);

    // first is slot then how many varables, these are all vec3 and 2
    VAO.LinkAttrib(nVBO, 0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, position));
    VAO.LinkAttrib(nVBO, 1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, normal));
    //VAO.LinkAttrib(nVBO, 2, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, color));
    VAO.LinkAttrib(nVBO, 3, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, texUV));
    VAO.LinkAttrib(nVBO, 4, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, tangent));
    VAO.LinkAttrib(nVBO, 5, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, biTangent));
    if (HasBones){
        VAO.LinkAttribI(nVBO, 6, 4, GL_INT, stride, (void*)offsetof(Vertex, m_BoneIDs));
        VAO.LinkAttrib(nVBO, 7, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, m_Weights));
    }
    VAO.Unbind();
    nVBO.Unbind();
    nEBO.Unbind();
    
    // LOD Gen
    if (generateLod){
        std::vector<GLuint> lod1 = MeshTools::simplifyIndices(indices, vertices, 0.75f, 0.01); // near
        std::vector<GLuint> lod2 = MeshTools::simplifyIndices(indices, vertices, 0.50f, 0.01); // mid
        std::vector<GLuint> lod3 = MeshTools::simplifyIndices(indices, vertices, 0.25f, 0.01); // far
        std::vector<GLuint> lod4 = MeshTools::simplifyIndices(indices, vertices, 0.10f, 0.01); // furthest
        
        lodSizes[0] = lod1.size();
        lodSizes[1] = lod2.size();
        lodSizes[2] = lod3.size();
        lodSizes[3] = lod4.size();
        
        lodEBOs[0].generateEBO(lod1);
        lodEBOs[1].generateEBO(lod2);
        lodEBOs[2].generateEBO(lod3);
        lodEBOs[3].generateEBO(lod4);
        
        // unbind
        lodEBOs[3].Unbind();
    }
    
    created = true;
}

void Mesh::Delete(){
    VAO.Delete();
    nVBO.Delete();
    nEBO.Delete();
            
    lodEBOs[0].Delete();
    lodEBOs[1].Delete();
    lodEBOs[2].Delete();
    lodEBOs[3].Delete();
    
    for (size_t i = 0; i < textures.size(); i++){
        textures[i].Delete();
    }
    textures.clear();

    vertices.clear();
    indices.clear();
    
    blas.clear();
}

void Mesh::updateMatrix(glm::mat4 matrix){
    Mesh::meshMatrix = matrix;
}
void Mesh::updatePosition(glm::vec3 position){
    Mesh::position = position;
}
void Mesh::updateRotation(glm::vec3 rotation){
    Mesh::rotation = rotation;
}
void Mesh::updateScale(glm::vec3 scale){
    Mesh::scale = scale;
}
void Mesh::updateGlobalMatrix(glm::mat4 matrix){
	Mesh::globalMeshMatrix = matrix;
}
void Mesh::updatePrevGlobalMatrix(glm::mat4 matrix){
    Mesh::globalPrevMeshMatrix = matrix;
}
void Mesh::updateGlobalPosition(glm::vec3 position){
    Mesh::globalPosition = position;
}
void Mesh::updateGlobalScale(glm::vec3 scale){
    Mesh::globalScale = scale;
}
void Mesh::updateGlobalRotation(glm::vec3 rotation){
    Mesh::globalRotation = rotation;
}