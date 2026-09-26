#ifndef VOXELIZER_CLASS_H
#define VOXELIZER_CLASS_H


#include <Render/Buffer/VBO.h>
#include <Render/Object/texture3D.h>
#include <Systems/Physics/Collision.h>
#include <Systems/Physics/accelerate.h>
#include "Render/Object/Texture.h"
#include <Render/Object/Mesh.h>
class voxelizer
{
public:
    
    // do voxelization with texture3D now as a demo. then later try the gpu rasterizer
    static void bakeMeshVXGAccel(std::vector<Vertex> &vertices, std::vector<accelerate::BVH_primitive>& prims, Collision::AABB root, const int sliceSize,
        Texture3D& texture, GLuint slot, std::vector<Texture>& textures);
    static void bakeMeshVXGAccel(std::vector<Vertex> &vertices, std::vector<accelerate::leaf>& leaves, Collision::AABB root, const int sliceSize, 
        Texture3D& texture, GLuint slot, std::vector<Texture>& textures);
    
    static void bakeMeshVXGAccelMeshPrims(std::vector<Mesh*> &meshes, std::vector<glm::mat4>& transforms, Collision::AABB root, const int sliceSize,
    Texture3D& texture, GLuint slot);
    
    
    static glm::vec4 VoxelizeMeshVXG(std::vector<Vertex> &vertices, std::vector<accelerate::BVH_primitive>& prims, glm::vec3& P, glm::vec3& S, 
        std::vector<Texture>& textures);  // BVH PRIM
    static glm::vec4 VoxelizeMeshVXG(std::vector<Vertex> &vertices,
        std::vector<accelerate::leaf>& leaves, glm::vec3& P, glm::vec3& S, std::vector<Texture>& textures);  // KD
    static glm::vec4 VoxelizeMeshVXGmeshPrims(std::vector<std::vector<Vertex>> &vertices, std::vector<std::vector<accelerate::BVH_primitive>>& prims, glm::vec3& P, glm::vec3& S, 
    std::vector<std::vector<Texture>>& textures);  // BVH PRIM
    
    static void cacheVXG(const char* path, int hash, std::vector<Texture3D *>& meshSDFs);
    
    // oct style here
    static void octSplitEmptySpace(std::vector<Collision::AABB>& aabbs, glm::vec3& p, glm::vec3& e, int iter);
    
    
    // kd style down here
    
    struct voxelMaterial{
        glm::vec4 albedo;
        glm::vec3 arm; 
        glm::vec3 emission;
    };

    struct voxelObj{
        Collision::AABB voxel;
        voxelMaterial material;
    };
    
    static std::vector<Collision::AABB> voxelizeMeshKD(std::vector<Vertex> &vertices, std::vector<GLuint> &indices, Collision::AABB root, int steps, int minTri, glm::vec3 minSize, bool doVertexSnap);
    static std::vector<Collision::AABB> voxelizeMeshKD(std::vector<Vertex> &vertices, std::vector<GLuint> &indices, Collision::AABB root, int steps, int minTri, glm::vec3 minSize, bool doVertexSnap, glm::mat4 transformation);
    
    static std::vector<Collision::voxelAccel> voxelizeMeshKDAccel(std::vector<Vertex> &vertices, std::vector<GLuint> &indices, Collision::AABB root, int steps, int minTri, glm::vec3 minSize, glm::mat4 transformation);
    
private:
    
    static bool voxelizerInternalKD(std::vector<Vertex> &vertices, std::vector<GLuint> &indices, Collision::AABB root, int steps, int minTri, glm::vec3 minSize, std::vector<Collision::AABB>& AABBs); // needs some sorta AABB array input, mesh geometry
    static bool voxelizerInternalKDwVertSnap(std::vector<Vertex> &vertices, std::vector<GLuint> &indices, Collision::AABB root, int steps, int minTri, glm::vec3 minSize, std::vector<Collision::AABB>& AABBs); // needs some sorta AABB array input, mesh geometry
        
    static bool voxelizerInternalKDAccel(std::vector<Vertex> &vertices, std::vector<GLuint> &indices, Collision::AABB root, int steps, int minTri, glm::vec3 minSize, std::vector<Collision::voxelAccel>& AABBs); // needs some sorta AABB array input, mesh geometry
    
    
    static void axisSnap(Collision::AABB& aabb, int &countBelowMinimum, glm::vec3 minSize);

};

#endif