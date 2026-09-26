#ifndef BVH_H
#define BVH_H
#include "glm/glm.hpp"
#include <vector>
#include <Systems/Physics/Collision.h>
#include <Render/Buffer/VBO.h>

class accelerate{
public:
    
    // uniform gird
    static void uniformSplitEmptySpace(std::vector<Collision::AABB>& aabbs, glm::vec3& p, glm::vec3& e, int dimension);
    
    struct clipmapLevel{
        glm::vec4 ps; // pos & scale
        uint64_t handle;
        uint64_t handle2;
        float thickness;
        uint32_t dirty; // bool but 64 for padding
        float padding0;
        float padding1;
    };
    
    static void transformClips(glm::vec3 p, std::vector<clipmapLevel>&clips);
    
    struct BVH_primitive{
        //GLint i0, i1, i2;
        unsigned int i0, i1, i2;
        glm::vec3 centroid;
        Collision::AABB extents;
    };
    
    struct  leaf{
        // job and bottoms child information
        //int job = 0; // 0 = top, 1 = internal, 2 = bottom
        //int startIndex = 0; // for job 2 (bottoms children (example: triangles))
        //int count = 0; // for job 2 again, the offset of triangles from start
        std::vector<GLuint> indices;
        std::vector<BVH_primitive> prims;
        
        // relationships
        //int parentIndex = 0; // for job 1 and 2, parent that holds them
        // children (if 0 or 1)
        int firstChildIndex = -1; 
        int secondChildIndex = -1; // -1 means it doesnt have children
        
        // leafs bounds
        Collision::AABB aabb;
        
        int depth = 0;
    };
    
    static Collision::AABB primsToBounds(std::vector<BVH_primitive>& prims);
    static std::vector<BVH_primitive> buildIndicesIntoPrims(std::vector<Vertex> &vertices, std::vector<GLuint> &indices);
    
    static std::vector<leaf> blasGenBVH(std::vector<Vertex> &vertices, std::vector<GLuint> &indices, int minTri, glm::mat4 transformation);

    static int aabbTraverseKDtree(std::vector<Vertex>& vertices, std::vector<accelerate::leaf>& leaves, int &fatherLeafIndex, float &minDist, int &minIndex, int &closestPrimIndex, glm::vec3& p, glm::vec3& s);
    
    
    static std::vector<leaf> blasGenKDAccel(std::vector<Vertex> &vertices, std::vector<GLuint> &indices, int minTri, int maxDepth, glm::mat4 transformation);
    // moved here

private:
    

    
    static bool blasInternalKD(std::vector<BVH_primitive>& prims, std::vector<Vertex> &vertices, leaf& rootLeaf, int &minTri, int &maxDepth, int depth, std::vector<leaf>& Leafs);
    
};

#endif