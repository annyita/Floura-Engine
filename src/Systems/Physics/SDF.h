#ifndef SDF_CLASS_H
#define SDF_CLASS_H


#include <Render/Buffer/VBO.h>
#include <Render/Object/texture3D.h>
#include <Systems/Physics/Collision.h>
#include <Systems/Physics/accelerate.h>
#include <Render/Shader/shaderClass.h>
#include <Render/Object/Texture.h>

class flouraSDF
{
public:
    
    static Shader mdfCompute;
    static void createShaders();
    static void cleanupShaders();
    
    static void cacheSDF(const char* path, int hash, std::vector<Texture3D *>& meshSDFs);
    
    static GLuint MDFGENERATIONSSBO;
    static void bakeMeshDistanceFieldGPU(std::vector<Vertex> &vertices, std::vector<accelerate::BVH_primitive>& prims, Collision::AABB root, const int sliceSize, Texture3D& texture, Texture3D& talbedo, GLuint slot, float thickness, std::vector<Texture>& textures);
    static void bakeMeshDistanceFieldGridGPU(std::vector<Vertex> &vertices, std::vector<accelerate::BVH_primitive>& prims, Collision::AABB root, const int sliceSize, Texture3D& texture, Texture3D& talbedo, GLuint slot, float thickness, std::vector<Texture>& textures, bool isnotFirst);
    // SDF
    static void bakeMeshSDFAccel(std::vector<Vertex> &vertices, std::vector<accelerate::leaf>& leaves, Collision::AABB root, const int sliceSize, Texture3D& texture, GLuint slot);
    
    static void nearestPointBlasTraversal(std::vector<accelerate::leaf>& leaves, std::vector<Vertex>& vertices, float &minDist,int &minIndex, int& closestPrimIndex, int cLeafIndex, glm::vec3& P);
    static void nearestNeighbourPrims(std::vector<accelerate::BVH_primitive> &prims, int &closestPrimIndex, glm::vec3& P);
    static void nearestNeighbourPrims(std::vector<Vertex> &vertices, std::vector<accelerate::BVH_primitive> &prims, int &closestPrimIndex, glm::vec3& P);
    // cant use this yet until the recursive fucnction is done
    // had enough of collison class so this does 
    static glm::vec3 distanceToClosestPointOnMeshSDFAccel_PlusUV(std::vector<Vertex> &vertices, std::vector<accelerate::leaf>& leaves, glm::vec3& P);  // KD
    
private:

};

#endif