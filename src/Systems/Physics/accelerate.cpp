#include "accelerate.h"
#include "utils/FE_math.h"
#include <Render/Handler/RenderHandler.h>


void accelerate::uniformSplitEmptySpace(std::vector<Collision::AABB>& aabbs, glm::vec3& p, glm::vec3& e, int dimension){
    const glm::vec3 youHappyC(static_cast<float>(dimension));
    glm::vec3 min(p - e); glm::vec3 boundSize = glm::vec3(2.0) * e;
    Collision::AABB naabb; naabb.size = e / youHappyC;
    
    for (int x = 0; x < dimension; ++x)
        for (int y = 0; y < dimension; ++y)
            for (int z = 0; z < dimension; ++z){
                glm::vec3 uvw(x, y, z);
                
                glm::vec3 normalizedUVW = (uvw + glm::vec3(0.5f)) / youHappyC;
                naabb.position = min + normalizedUVW * boundSize;
                
                aabbs.push_back(naabb);
            }
}

void accelerate::transformClips(glm::vec3 p, std::vector<clipmapLevel>& clips){
    for (int i = 0; i < clips.size(); ++i){
        glm::vec3 np = p;
        np = FE_Math::snapToGrid(np, clips[i].ps.w);
        if (glm::vec3(clips[i].ps) != np){
            // dirty
            clips[i].ps = glm::vec4(np, clips[i].ps.w);
            clips[i].dirty = 1;
            
            //std::cout << "abcdefg" << std::endl;
        }
    }
}

Collision::AABB accelerate::primsToBounds(std::vector<BVH_primitive>& prims){
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
    for (int i = 0; i < prims.size(); ++i){
        min = glm::min(min ,(prims[i].extents.position - prims[i].extents.size) );
        max = glm::max(max ,(prims[i].extents.position + prims[i].extents.size) );
    }
    
    Collision::AABB nAABB;
    nAABB.position = (max + min) * 0.5f;
    nAABB.size = (max - min) * 0.5f;
    return nAABB;
}

std::vector<accelerate::BVH_primitive> accelerate::buildIndicesIntoPrims(std::vector<Vertex>& vertices, std::vector<GLuint>& indices){
    std::vector<BVH_primitive> nPrims;
    for (int i = 0; i < indices.size(); i += 3){
        
        unsigned int &i0 = indices[i];
        unsigned int &i1 = indices[i + 1];
        unsigned int &i2 = indices[i + 2];
        
        if (i0 >= vertices.size() ||
        i1 >= vertices.size() ||
        i2 >= vertices.size())
            continue;
        
        // transform point can go here, but not yet
        glm::vec3 &a = vertices[i0].position;
        glm::vec3 &b = vertices[i1].position;
        glm::vec3 &c = vertices[i2].position;
        
        BVH_primitive nPrim;
        nPrim.centroid = (a + b + c) / 3.0f;
        nPrim.i0 = i0; nPrim.i1 = i1; nPrim.i2 = i2;
        
        glm::vec3 min = glm::min(a, glm::min(b, c));
        glm::vec3 max = glm::max(a, glm::max(b, c));
        
        nPrim.extents.position = (max + min) * 0.5f;
        nPrim.extents.size = (max - min) * 0.5f;
        
        nPrims.push_back(nPrim);
    }
    return nPrims;
}

std::vector<accelerate::leaf> accelerate::blasGenBVH(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, 
    int minTri, glm::mat4 transformation){
            
    std::vector<leaf> nLeafs;
    std::vector<Vertex> nVertices = vertices;
    
    // transform
    for (int i = 0; i < nVertices.size(); ++i)
        FE_Math::transformPoint(nVertices[i].position, transformation);
    
    std::vector<accelerate::BVH_primitive> prims = buildIndicesIntoPrims(nVertices, indices);
    if (prims.empty()) return nLeafs;
    
    leaf fatherLeaf;
    fatherLeaf.aabb = primsToBounds(prims);
    //fatherLeaf.indices = indices;
    //fatherLeaf.parentIndex = -1;
    nLeafs.push_back(fatherLeaf);
    
    //blasInternalKD(nVertices, indices, fatherLeaf, steps, minTri, nLeafs);
    return nLeafs;
}

int accelerate::aabbTraverseKDtree(std::vector<Vertex>& vertices, std::vector<accelerate::leaf>& leaves, int &fatherLeafIndex, float &minDist, int &minIndex, int &closestPrimIndex, glm::vec3& p, glm::vec3& s){
    if (leaves.empty() || fatherLeafIndex < 0) return -1;
        
    glm::vec3 np = Collision::nearestPointOnAABB(p, leaves[fatherLeafIndex].aabb.position, leaves[fatherLeafIndex].aabb.size);
    float nd = glm::distance(np, p); // get the distance
    
    if (nd >= minDist) return -1;
    
    Collision::HitResult hr = Collision::AABBvsAABB(leaves[fatherLeafIndex].aabb.position, leaves[fatherLeafIndex].aabb.size, p, s);
    if (!hr.isColliding) return -1;
    
    // if we are bottom
    if  (leaves[fatherLeafIndex].firstChildIndex <= -1 && leaves[fatherLeafIndex].secondChildIndex <= -1){
        //minDist = nd;
        //minIndex = fatherLeafIndex;
        float primMinDist = minDist;
        for (int i = 0; i < leaves[fatherLeafIndex].prims.size(); ++i){
            const glm::vec3 npp = Collision::nearestPointOnAABB(p, leaves[fatherLeafIndex].prims[i].extents.position, leaves[fatherLeafIndex].prims[i].extents.size);
            float npd = glm::distance(npp, p);
            if (npd < primMinDist){
                const unsigned int &i0 = leaves[fatherLeafIndex].prims[i].i0;
                const unsigned int &i1 = leaves[fatherLeafIndex].prims[i].i1;
                const unsigned int &i2 = leaves[fatherLeafIndex].prims[i].i2;
				
                if (i0 >= vertices.size() ||
                    i1 >= vertices.size() ||
                    i2 >= vertices.size())
                    continue;
				
                const Vertex* cV1 = &vertices[i0];
                const Vertex* cV2 = &vertices[i1];
                const Vertex* cV3 = &vertices[i2];
	
                const glm::vec3 tnp = Collision::closestPointOnTriangle(p, cV1->position, cV2->position, cV3->position);
                float tnd = glm::distance(tnp, p);
                if (tnd < minDist){
                    closestPrimIndex = i;
                    minIndex = fatherLeafIndex;
                    minDist = tnd;
                }
            }
        }
        return 1;
    }
    
    int firstChildInd = leaves[fatherLeafIndex].firstChildIndex;
    int secondChildInd = leaves[fatherLeafIndex].secondChildIndex;
    
    float fnd = std::numeric_limits<float>::max();
    float snd = std::numeric_limits<float>::max();
    
    if (firstChildInd >= 0){
        glm::vec3 fnp = Collision::nearestPointOnAABB(p, leaves[firstChildInd].aabb.position, leaves[firstChildInd].aabb.size);
        fnd = glm::distance(fnp, p); // get the distance
    }
    if (secondChildInd >= 0){
        glm::vec3 snp = Collision::nearestPointOnAABB(p, leaves[secondChildInd].aabb.position, leaves[secondChildInd].aabb.size);
        snd = glm::distance(snp, p); // get the distance
    }
    
    if (fnd < snd){
        if (firstChildInd >= 0 && fnd < minDist)aabbTraverseKDtree(vertices, leaves, leaves[fatherLeafIndex].firstChildIndex, minDist, minIndex, closestPrimIndex, p, s);
        if (secondChildInd >= 0 && snd < minDist)aabbTraverseKDtree(vertices, leaves, leaves[fatherLeafIndex].secondChildIndex, minDist,minIndex, closestPrimIndex, p, s);
    }
    else{
        if (secondChildInd >= 0 && snd < minDist)aabbTraverseKDtree(vertices, leaves, leaves[fatherLeafIndex].secondChildIndex, minDist,minIndex, closestPrimIndex, p, s);
        if (firstChildInd >= 0 && fnd < minDist)aabbTraverseKDtree(vertices, leaves, leaves[fatherLeafIndex].firstChildIndex, minDist, minIndex, closestPrimIndex, p, s);
    }
    return 0;
}

std::vector<accelerate::leaf> accelerate::blasGenKDAccel(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, int minTri, int maxDepth, glm::mat4 transformation){
            
    std::vector<leaf> nLeafs;
    std::vector<Vertex> nVertices = vertices;
    
    // transform
    for (int i = 0; i < nVertices.size(); ++i)
        FE_Math::transformPoint(nVertices[i].position, transformation);
    
    //std::vector<GLuint>& nindices = indices;
    
    std::vector<accelerate::BVH_primitive> prims = buildIndicesIntoPrims(nVertices, indices);
    if (prims.empty()) return nLeafs;
    //std::cout << prims.size() << std::endl;
    
    leaf fatherLeaf;
    fatherLeaf.aabb = primsToBounds(prims);
    //std::cout << "ext: "<< fatherLeaf.aabb.size.x << " "<< fatherLeaf.aabb.size.y <<" "<< fatherLeaf.aabb.size.z;;
    //std::cout << "pos: "<< fatherLeaf.aabb.position.x << " "<< fatherLeaf.aabb.position.y <<" "<< fatherLeaf.aabb.position.z;;
    //fatherLeaf.aabb = root;
    fatherLeaf.indices = indices;
    fatherLeaf.prims = prims;
    //fatherLeaf.parentIndex = -1;
    
    blasInternalKD(prims, nVertices, fatherLeaf, minTri, maxDepth, 0, nLeafs);
    fatherLeaf.indices = indices;
    fatherLeaf.prims = prims;
    nLeafs.push_back(fatherLeaf);
    //blasInternalKD(prims, nVertices, nLeafs[0], minTri, nLeafs);
    return nLeafs;
}

bool accelerate::blasInternalKD(std::vector<BVH_primitive>& prims, std::vector<Vertex> &vertices,
                         leaf& rootLeaf, int& minTri, int &maxDepth, int depth, std::vector<leaf>& Leafs){
        
        if (minTri <= 0 || depth > maxDepth) return false;
    
        Collision::AABB cAABB =rootLeaf.aabb;
        
        int hitCount = 0; // use for triangles hit
        //int count = 0;

        std::vector<BVH_primitive> nprims;
        std::vector<GLuint> nIndices;

        // does collision
        for (int i = 0; i < prims.size(); ++i){
            Collision::HitResult abbbHit =Collision::AABBvsAABB(cAABB.position, cAABB.size, prims[i].extents.position, prims[i].extents.size);
            //std::cout << "touched this deep" << std::endl;
            if (abbbHit.isColliding){ // we are failing to collide
                if (prims[i].i0 >= vertices.size() || prims[i].i1 >= vertices.size() || prims[i].i2 >= vertices.size())
                    continue;
                // transform point can go here, but not yet
                glm::vec3 &a = vertices[prims[i].i0].position;
                glm::vec3 &b = vertices[prims[i].i1].position;
                glm::vec3 &c = vertices[prims[i].i2].position;
                
                Collision::HitResult trihit = Collision::SATTriangleVSAABB(a, b, c, cAABB.position, cAABB.size);
                if (trihit.isColliding){
                    nprims.push_back(prims[i]);
                    nIndices.push_back(prims[i].i0);
                    nIndices.push_back(prims[i].i1);
                    nIndices.push_back(prims[i].i2);
                    hitCount++;
                }
            }
        }
        // if under min or nohit
        if (hitCount < minTri || hitCount <= 0 || nprims.empty() || nIndices.empty()) return false;
    //std::cout << "touched this deep" << std::endl;
        //rootLeaf.indices = nIndices;
        //rootLeaf.prims = nprims;
        
        // should kd split and test both
        Collision::KDsplit kds = Collision::KDsplitVolume(cAABB.position, cAABB.size);

        leaf firstLeaf; firstLeaf.aabb = kds.firstSplit; 
        leaf secondLeaf; secondLeaf.aabb = kds.secondSplit;
        rootLeaf.depth = depth;
        //kds.firstSplit.size
        // traverse down
        bool rA = blasInternalKD(nprims, vertices, firstLeaf, minTri, maxDepth, depth +=1, Leafs); // nprims should be here but i am testing
        if (rA){
            Leafs.push_back(firstLeaf);
            rootLeaf.firstChildIndex = static_cast<int>(Leafs.size() - 1);
            //std::cout << rootLeaf.firstChildIndex<< " - first child" << std::endl;
        }
        bool rB = blasInternalKD(nprims, vertices, secondLeaf, minTri, maxDepth, depth +=1, Leafs);
        if (rB){
            Leafs.push_back(secondLeaf);
            rootLeaf.secondChildIndex = static_cast<int>(Leafs.size() - 1);
            //std::cout << rootLeaf.secondChildIndex << " - sec child" << std::endl;
        }
    
        if (!rA && !rB){
            rootLeaf.indices = nIndices;
            rootLeaf.prims = nprims;
            rootLeaf.firstChildIndex = -2;
            rootLeaf.secondChildIndex = -2;
            
        }
    
         //if both are false, pushback the root
        //if (!rA && !rB) Leafs.push_back(root);
        
    return true;
    //return rA || rB;
    //return rA && rB;
}
