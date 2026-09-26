#include "voxelizer.h"
#include "utils/FE_math.h"
#include "Systems/Physics/accelerate.h"
#include <utils/imageWrite.h>

void voxelizer::bakeMeshVXGAccel(std::vector<Vertex>& vertices, std::vector<accelerate::BVH_primitive>& prims,
    Collision::AABB root, const int sliceSize, Texture3D& texture, GLuint slot, std::vector<Texture>& textures){
    
    Collision::minmax mm = Collision::returnMinMax(root.position, root.size);
    glm::vec3 boundssize = mm.max - mm.min;
    
    // still here incase i change my mind
    const int sX = sliceSize;
    const int sY = sliceSize;
    const int sZ = sliceSize;
    
    glm::vec3 s = boundssize / glm::vec3(sX, sY, sZ);
    //std::vector<float> sdfData(sX * sY * sZ);
    std::vector<float> sdfData(sX * sY * sZ * 4);

    for (int x = 0; x < sX; ++x)
        for (int y = 0; y < sY; ++y)
            for (int z = 0; z < sZ; ++z)
            {
                float u = (x + 0.5f) / sX;
                float v = (y + 0.5f) / sY;
                float w = (z + 0.5f) / sZ;
                
                glm::vec3 p = mm.min + glm::vec3(u, v, w) * boundssize;
                
                //float dist = Collision::distanceToClosestPointOnMeshSDF(vertices, indices, p);
                //glm::vec3 dist_pUV = Collision::distanceToClosestPointOnMeshSDFAccel_PlusUV(vertices, voxelAccel, p);
                glm::vec4 vox = VoxelizeMeshVXG(vertices, prims, p, s, textures);
                //glm::vec3 dist_pUV = distanceToClosestPointOnMeshSDFAccel_PlusUV(vertices, prims, p);
            	//dist_pUV = glm::vec3(dist_pUV.r);
                //int index = x + (y * sX) + (z * sX * sY);
                //sdfData[index] = dist_pUV.x;
                
                int index = (x + (y * sX) + (z * sX * sY)) * 4; // one for each component
                sdfData[index] = vox.x;
                sdfData[index + 1] = vox.y;
                sdfData[index + 2] = vox.z;
                sdfData[index + 3] = vox.w;
            }
    
    texture.Delete();
    texture.path = "NULL";
    texture.type = "SDF";
    texture.slot = slot;
    texture.width = sX;
    texture.height = sY;
    texture.depth = sZ;
    
    glGenTextures(1, &texture.ID);
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_3D, texture.ID);
    
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // GL_NEAREST_MIPMAP_LINEAR
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    glTexImage3D(
    GL_TEXTURE_3D,
    0,
    GL_RGBA32F, //     GL_R32F,
    sX,
    sY,
    sZ,
    0,
    GL_RGBA,
    GL_FLOAT,
    sdfData.data()
    );
    glGenerateMipmap(GL_TEXTURE_3D);

    if (GLAD_GL_ARB_bindless_texture) {
        texture.handle = glGetTextureHandleARB(texture.ID);
        glMakeTextureHandleResidentARB(texture.handle);
    }
    glBindTexture(GL_TEXTURE_3D, 0);
}

void voxelizer::bakeMeshVXGAccel(std::vector<Vertex>& vertices, std::vector<accelerate::leaf>& leaves, Collision::AABB root,
    const int sliceSize, Texture3D& texture, GLuint slot, std::vector<Texture>& textures){
    
    Collision::minmax mm = Collision::returnMinMax(root.position, root.size);
    glm::vec3 boundssize = mm.max - mm.min;
    
    // still here incase i change my mind
    const int sX = sliceSize;
    const int sY = sliceSize;
    const int sZ = sliceSize;
    
    glm::vec3 s = boundssize / glm::vec3(sX, sY, sZ);
    //std::vector<float> sdfData(sX * sY * sZ);
    std::vector<float> sdfData(sX * sY * sZ * 4);

    for (int x = 0; x < sX; ++x)
        for (int y = 0; y < sY; ++y)
            for (int z = 0; z < sZ; ++z)
            {
                float u = (x + 0.5f) / sX;
                float v = (y + 0.5f) / sY;
                float w = (z + 0.5f) / sZ;
                
                glm::vec3 p = mm.min + glm::vec3(u, v, w) * boundssize;
                
                //float dist = Collision::distanceToClosestPointOnMeshSDF(vertices, indices, p);
                //glm::vec3 dist_pUV = Collision::distanceToClosestPointOnMeshSDFAccel_PlusUV(vertices, voxelAccel, p);
                glm::vec4 vox = VoxelizeMeshVXG(vertices, leaves, p, s, textures);
                //glm::vec3 dist_pUV = distanceToClosestPointOnMeshSDFAccel_PlusUV(vertices, prims, p);
            	//dist_pUV = glm::vec3(dist_pUV.r);
                //int index = x + (y * sX) + (z * sX * sY);
                //sdfData[index] = dist_pUV.x;
                
                int index = (x + (y * sX) + (z * sX * sY)) * 4; // one for each component
                sdfData[index] = vox.x;
                sdfData[index + 1] = vox.y;
                sdfData[index + 2] = vox.z;
                sdfData[index + 3] = vox.w;
            }
    
    texture.Delete();
    texture.path = "NULL";
    texture.type = "SDF";
    texture.slot = slot;
    texture.width = sX;
    texture.height = sY;
    texture.depth = sZ;
    
    glGenTextures(1, &texture.ID);
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_3D, texture.ID);
    
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // GL_NEAREST_MIPMAP_LINEAR
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    glTexImage3D(
    GL_TEXTURE_3D,
    0,
    GL_RGBA32F, //     GL_R32F,
    sX,
    sY,
    sZ,
    0,
    GL_RGBA,
    GL_FLOAT,
    sdfData.data()
    );
    glGenerateMipmap(GL_TEXTURE_3D);

    if (GLAD_GL_ARB_bindless_texture) {
        texture.handle = glGetTextureHandleARB(texture.ID);
        glMakeTextureHandleResidentARB(texture.handle);
    }
    glBindTexture(GL_TEXTURE_3D, 0);
}

void voxelizer::bakeMeshVXGAccelMeshPrims(std::vector<Mesh*>& meshes, std::vector<glm::mat4>& transforms,
    Collision::AABB root, const int sliceSize, Texture3D& texture, GLuint slot){
    
    std::vector<std::vector<Vertex>> vertices;
    std::vector<std::vector<accelerate::BVH_primitive>> prims;
    std::vector<std::vector<Texture>> textures; 
    
    for (int i = 0; i < meshes.size(); ++i){
        std::vector<Vertex> nVertices = meshes[i]->vertices;
        
        for (int x = 0; x < nVertices.size(); ++x)
            FE_Math::transformPoint(nVertices[x].position, transforms[i]);
        
        // generate prims
        std::vector<accelerate::BVH_primitive> nPrims = accelerate::buildIndicesIntoPrims(nVertices,
             meshes[i]->indices);
        
        vertices.push_back(nVertices);
        prims.push_back(nPrims);
        textures.push_back(meshes[i]->textures);
    }
    std::cout << vertices.size() << std::endl;
    
    Collision::minmax mm = Collision::returnMinMax(root.position, root.size);
    glm::vec3 boundssize = mm.max - mm.min;
    
    // still here incase i change my mind
    const int sX = sliceSize;
    const int sY = sliceSize;
    const int sZ = sliceSize;
    
    glm::vec3 s = boundssize / glm::vec3(sX, sY, sZ);
    //std::vector<float> sdfData(sX * sY * sZ);
    std::vector<float> sdfData(sX * sY * sZ * 4);

    for (int x = 0; x < sX; ++x)
        for (int y = 0; y < sY; ++y)
            for (int z = 0; z < sZ; ++z)
            {
                float u = (x + 0.5f) / sX;
                float v = (y + 0.5f) / sY;
                float w = (z + 0.5f) / sZ;
                
                glm::vec3 p = mm.min + glm::vec3(u, v, w) * boundssize;
                
                //float dist = Collision::distanceToClosestPointOnMeshSDF(vertices, indices, p);
                //glm::vec3 dist_pUV = Collision::distanceToClosestPointOnMeshSDFAccel_PlusUV(vertices, voxelAccel, p);
                glm::vec4 vox = VoxelizeMeshVXGmeshPrims(vertices, prims, p, s, textures);
                //glm::vec4 vox = glm::vec4(1.0f);
                
                //glm::vec3 dist_pUV = distanceToClosestPointOnMeshSDFAccel_PlusUV(vertices, prims, p);
            	//dist_pUV = glm::vec3(dist_pUV.r);
                //int index = x + (y * sX) + (z * sX * sY);
                //sdfData[index] = dist_pUV.x;
                
                int index = (x + (y * sX) + (z * sX * sY)) * 4; // one for each component
                sdfData[index] = vox.x;
                sdfData[index + 1] = vox.y;
                sdfData[index + 2] = vox.z;
                sdfData[index + 3] = vox.w;
            }
    
    texture.Delete();
    texture.path = "NULL";
    texture.type = "SDF";
    texture.slot = slot;
    texture.width = sX;
    texture.height = sY;
    texture.depth = sZ;
    
    glGenTextures(1, &texture.ID);
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_3D, texture.ID);
    
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // GL_NEAREST_MIPMAP_LINEAR
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    glTexImage3D(
    GL_TEXTURE_3D,
    0,
    GL_RGBA32F, //     GL_R32F,
    sX,
    sY,
    sZ,
    0,
    GL_RGBA,
    GL_FLOAT,
    sdfData.data()
    );
    glGenerateMipmap(GL_TEXTURE_3D);

    if (GLAD_GL_ARB_bindless_texture) {
        texture.handle = glGetTextureHandleARB(texture.ID);
        glMakeTextureHandleResidentARB(texture.handle);
    }
    glBindTexture(GL_TEXTURE_3D, 0);
}

glm::vec4 voxelizer::VoxelizeMeshVXG(std::vector<Vertex>& vertices, std::vector<accelerate::BVH_primitive>& prims,
                                     glm::vec3& P, glm::vec3& S, std::vector<Texture>& textures){
    
    for (int i = 0; i < prims.size(); ++i){
        Collision::HitResult aabbHR = Collision::AABBvsAABB(P, S, prims[i].extents.position, prims[i].extents.size);
        if (!aabbHR.isColliding) continue;
        
        const unsigned int &i0 = prims[i].i0;
        const unsigned int &i1 = prims[i].i1;
        const unsigned int &i2 = prims[i].i2;
        
        if (i0 >= vertices.size() ||
            i1 >= vertices.size() ||
            i2 >= vertices.size()) 
            continue;
        
        const Vertex* a = &vertices[i0];
        const Vertex* b = &vertices[i1];
        const Vertex* c = &vertices[i2];
        
        Collision::HitResult trihit = Collision::SATTriangleVSAABB(a->position, b->position, c->position, P, S);
        if (!trihit.isColliding) continue;
        
        const glm::vec3 np = Collision::closestPointOnTriangle(P, a->position, b->position, c->position);
        glm::vec2 uv = FE_Math::uvPosFromVertexAndPoint(a->position,b->position,c->position, 
            a->texUV, b->texUV, c->texUV, np);
        glm::vec4 dColour(1.0);
        // needs to sample textures
        for (unsigned int x = 0; x < textures.size(); x++){
            if (textures[x].type == "texture_diffuse"){
                dColour = textures[x].rgbaSample(uv, true);
                break;    
            }
        }
        
        return dColour;
    }
    
    return glm::vec4(0.0);
}

glm::vec4 voxelizer::VoxelizeMeshVXG(std::vector<Vertex>& vertices, std::vector<accelerate::leaf>& leaves, glm::vec3& P,
    glm::vec3& S, std::vector<Texture>& textures){
    float minDist = std::numeric_limits<float>::max();
    int closestLeafIndex = -1;
    int closestPrimIndex = -1;
    int startingIndex = static_cast<int>(leaves.size()) - 1;
    //startingIndex = 0;
    accelerate::aabbTraverseKDtree(vertices, leaves, startingIndex, minDist, closestLeafIndex, closestPrimIndex, P, S);
    
    if (closestLeafIndex < 0 || closestPrimIndex < 0|| leaves[closestLeafIndex].prims.empty()) return glm::vec4(1.0, 0.0f, 0.0f, 1.0f);
    
    // yeah i know bad, cutting corners rn
    std::vector<accelerate::BVH_primitive> singleprim;
    singleprim.push_back(leaves[closestLeafIndex].prims[closestPrimIndex]);
    return VoxelizeMeshVXG(vertices, singleprim, P, S, textures);
}

glm::vec4 voxelizer::VoxelizeMeshVXGmeshPrims(std::vector<std::vector<Vertex>>& vertices,
    std::vector<std::vector<accelerate::BVH_primitive>>& prims, glm::vec3& P, glm::vec3& S,
    std::vector<std::vector<Texture>>& textures){
        
    for (int meshInd = 0; meshInd < prims.size(); ++meshInd) //meshes
        for (int i = 0; i < prims[meshInd].size(); ++i){ // prims
            
            Collision::HitResult aabbHR = Collision::AABBvsAABB(P, S, prims[meshInd][i].extents.position, prims[meshInd][i].extents.size);
            if (!aabbHR.isColliding) continue;
            
            //return glm::vec4(1.0, 0.0, 0.0, 1.0);
            const unsigned int &i0 = prims[meshInd][i].i0;
            const unsigned int &i1 = prims[meshInd][i].i1;
            const unsigned int &i2 = prims[meshInd][i].i2;
        
            if (i0 >= vertices[meshInd].size() ||
                i1 >= vertices[meshInd].size() ||
                i2 >= vertices[meshInd].size()) 
                continue;
        
            const Vertex* a = &vertices[meshInd][i0];
            const Vertex* b = &vertices[meshInd][i1];
            const Vertex* c = &vertices[meshInd][i2];
        
            Collision::HitResult trihit = Collision::SATTriangleVSAABB(a->position, b->position, c->position, P, S);
            if (!trihit.isColliding) continue;
        
            const glm::vec3 np = Collision::closestPointOnTriangle(P, a->position, b->position, c->position);
            glm::vec2 uv = FE_Math::uvPosFromVertexAndPoint(a->position,b->position,c->position, 
                a->texUV, b->texUV, c->texUV, np);
            glm::vec4 dColour(1.0);
            // needs to sample textures
            for (unsigned int x = 0; x < textures.size(); x++){
                if (textures[meshInd][x].type == "texture_diffuse"){
                    dColour = textures[meshInd][x].rgbaSample(uv, true);
                    break;    
                }
            }
        
            return dColour;
        }
    
    return glm::vec4(0.0);
}

void voxelizer::cacheVXG(const char* path, int hash, std::vector<Texture3D *>& meshSDFs){
    for (int i = 0; i < meshSDFs.size(); ++i){
        std::string nPath = path + std::to_string(hash) + "_" + std::to_string(i) +".png";
        FlouraImageWrite::writeImage3DToDiskPNG(meshSDFs[i]->ID,
            meshSDFs[i]->width, meshSDFs[i]->height, meshSDFs[i]->depth,
            nPath.c_str(), GL_RGBA, GL_UNSIGNED_BYTE, 4); // GB
    }
}

void voxelizer::octSplitEmptySpace(std::vector<Collision::AABB>& aabbs, glm::vec3& p, glm::vec3& e, int iter){
    if (iter <= 0) return;
    
    Collision::octSplit newSplit = Collision::octSplitVolume(p, e);
    
    int nCount = iter - 1;
    
    if (nCount == 0){
        Collision::AABB naabb; naabb.size = newSplit.size;
        
        naabb.position = newSplit.splitTRF; aabbs.push_back(naabb);
        naabb.position = newSplit.splitTLF; aabbs.push_back(naabb);
        naabb.position = newSplit.splitTRB; aabbs.push_back(naabb);
        naabb.position = newSplit.splitTLB; aabbs.push_back(naabb);
        
        naabb.position = newSplit.splitDRF; aabbs.push_back(naabb);
        naabb.position = newSplit.splitDLF; aabbs.push_back(naabb);
        naabb.position = newSplit.splitDRB; aabbs.push_back(naabb);
        naabb.position = newSplit.splitDLB; aabbs.push_back(naabb);
        return;
    }
    
    octSplitEmptySpace(aabbs, newSplit.splitTRF, newSplit.size, nCount);
    octSplitEmptySpace(aabbs, newSplit.splitTLF, newSplit.size, nCount);
    octSplitEmptySpace(aabbs, newSplit.splitTRB, newSplit.size, nCount);
    octSplitEmptySpace(aabbs, newSplit.splitTLB, newSplit.size, nCount);
    
    octSplitEmptySpace(aabbs, newSplit.splitDRF, newSplit.size, nCount);
    octSplitEmptySpace(aabbs, newSplit.splitDLF, newSplit.size, nCount);
    octSplitEmptySpace(aabbs, newSplit.splitDRB, newSplit.size, nCount);
    octSplitEmptySpace(aabbs, newSplit.splitDLB, newSplit.size, nCount);
}

std::vector<Collision::AABB> voxelizer::voxelizeMeshKD(std::vector<Vertex>& vertices, std::vector<GLuint>& indices,
                                                       Collision::AABB root, int steps, int minTri, glm::vec3 minSize, bool doVertexSnap){
    std::vector<Collision::AABB> nAABs;
    
    if (!doVertexSnap){
        voxelizerInternalKD(vertices, indices, root, steps, minTri, minSize, nAABs);
        return nAABs;
    }
    voxelizerInternalKDwVertSnap(vertices, indices, root, steps, minTri, minSize, nAABs);
    return nAABs;
}

std::vector<Collision::AABB> voxelizer::voxelizeMeshKD(std::vector<Vertex>& vertices,
    std::vector<GLuint>& indices, Collision::AABB root, int steps, int minTri,  glm::vec3 minSize, bool doVertexSnap, glm::mat4 transformation){
    
    std::vector<Collision::AABB> nAABs;
    std::vector<Vertex> nVertices = vertices;
    
    // transform
    for (int i = 0; i < nVertices.size(); ++i)
        FE_Math::transformPoint(nVertices[i].position, transformation);
    
    if (!doVertexSnap){
        voxelizerInternalKD(nVertices, indices, root, steps, minTri, minSize, nAABs);
        return nAABs;
    }
    voxelizerInternalKDwVertSnap(vertices, indices, root, steps, minTri, minSize, nAABs);
    return nAABs;
}

std::vector<Collision::voxelAccel> voxelizer::voxelizeMeshKDAccel(std::vector<Vertex>& vertices,
    std::vector<GLuint>& indices, Collision::AABB root, int steps, int minTri, glm::vec3 minSize,
    glm::mat4 transformation){
    std::vector<Collision::voxelAccel> nAABs;
    std::vector<Vertex> nVertices = vertices;
    
    // transform
    for (int i = 0; i < nVertices.size(); ++i)
        FE_Math::transformPoint(nVertices[i].position, transformation);
    
    voxelizerInternalKDAccel(nVertices, indices, root, steps, minTri, minSize, nAABs);
    return nAABs;
}

bool voxelizer::voxelizerInternalKD(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, Collision::AABB root,
                                    int steps, int minTri, glm::vec3 minSize, std::vector<Collision::AABB>& AABBs){
    
    if (steps <= 0 && minTri <= 0) return false;
    
    int hitCount = 0; // use for triangles hit
    
    std::vector<GLuint> nIndices;
    
    // does collision
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
		
        // collision sat here
        Collision::HitResult trihit = Collision::SATTriangleVSAABB(a, b, c, root.position, root.size);
        if (trihit.isColliding){
            nIndices.push_back(i0);
            nIndices.push_back(i1);
            nIndices.push_back(i2);
            
            hitCount++;
        }
    }
    
    // if under min or nohit
    if (hitCount < minTri || hitCount <= 0) return false;
    //if (hitCount <= 0) return false;
    
    // should kd split and test both
    Collision::KDsplit kds = Collision::KDsplitVolume(root.position, root.size);
    // if children have any axis below min size snap them, and if all are return false
    // going for if statements so i can filter which axis are min
    int countBelowMinimum = 0;
    if (kds.firstSplit.size.x < minSize.x){ kds.firstSplit.size.x  = minSize.x; countBelowMinimum++;}
    if (kds.firstSplit.size.y < minSize.y){ kds.firstSplit.size.y  = minSize.y; countBelowMinimum++;}
    if (kds.firstSplit.size.z < minSize.z){ kds.firstSplit.size.z  = minSize.z; countBelowMinimum++;}
    if (kds.secondSplit.size.x < minSize.x){ kds.secondSplit.size.x  = minSize.x; countBelowMinimum++;}
    if (kds.secondSplit.size.y < minSize.y){ kds.secondSplit.size.y  = minSize.y; countBelowMinimum++;}
    if (kds.secondSplit.size.z < minSize.z){ kds.secondSplit.size.z  = minSize.z; countBelowMinimum++;}
    
    if (countBelowMinimum >= 6) return false;
    // ^^ yandere dev ahh code
    
    // for 0 = infinte steps
    int nSize = 1; if (steps > 0) nSize = steps - 1;
    
    //kds.firstSplit.size
    // traverse down
    bool rA = voxelizerInternalKD(vertices, nIndices, kds.firstSplit, nSize, minTri, minSize, AABBs);
    bool rB = voxelizerInternalKD(vertices, nIndices, kds.secondSplit, nSize, minTri, minSize, AABBs);
    
    // if both are false, pushback the root
    if (!rA && !rB) AABBs.push_back(root);
    
    return true;
}

bool voxelizer::voxelizerInternalKDwVertSnap(std::vector<Vertex>& vertices, std::vector<GLuint>& indices,
    Collision::AABB root, int steps, int minTri, glm::vec3 minSize, std::vector<Collision::AABB>& AABBs)
{
        
    if (steps <= 0 && minTri <= 0) return false;
    
    int hitCount = 0; // use for triangles hit
    
    std::vector<GLuint> nIndices;
    std::vector<Vertex> nVertices;
    
    // does collision
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
		
        // collision sat here
        Collision::HitResult trihit = Collision::SATTriangleVSAABB(a, b, c, root.position, root.size);
        if (trihit.isColliding){
            nIndices.push_back(i0);
            nIndices.push_back(i1);
            nIndices.push_back(i2);
            
            nVertices.push_back(vertices[i0]);
            nVertices.push_back(vertices[i1]);
            nVertices.push_back(vertices[i2]);
            
            hitCount++;
        }
    }
    
    // if under min or nohit
    if (hitCount < minTri || hitCount <= 0) return false;
    //if (hitCount <= 0) return false;
    
    Collision::rubiksCubePoints nRubikzCube = Collision::fetchFurthestVertices(nVertices);
    Collision::AABB nAABB = Collision::rootNodeFromRubixPoints(nRubikzCube, glm::mat4(1.0f));
    
    // should kd split and test both
    Collision::KDsplit kds = Collision::KDsplitVolume(nAABB.position, nAABB.size);
    // if children have any axis below min size snap them, and if all are return false
    // going for if statements so i can filter which axis are min
    int countBelowMinimum = 0;
    axisSnap(kds.firstSplit, countBelowMinimum, minSize);
    axisSnap(kds.secondSplit, countBelowMinimum, minSize);
    
    if (countBelowMinimum >= 6) return false;
    // ^^ yandere dev ahh code
    
    // for 0 = infinte steps
    int nSize = 1; if (steps > 0) nSize = steps - 1;
    
    // traverse down
    bool rA = voxelizerInternalKDwVertSnap(vertices, nIndices, kds.firstSplit, nSize, minTri, minSize, AABBs);
    bool rB = voxelizerInternalKDwVertSnap(vertices, nIndices, kds.secondSplit, nSize, minTri, minSize, AABBs);
    
    // if both are false, pushback the root
    if (!rA && !rB) AABBs.push_back(root);
    
    return true;
}

bool voxelizer::voxelizerInternalKDAccel(std::vector<Vertex>& vertices, std::vector<GLuint>& indices,
    Collision::AABB root, int steps, int minTri, glm::vec3 minSize, std::vector<Collision::voxelAccel>& AABBs){
        
    if (steps <= 0 && minTri <= 0) return false;
    
    int hitCount = 0; // use for triangles hit
    
    std::vector<GLuint> nIndices;
    
    // does collision
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
		
        // collision sat here
        Collision::HitResult trihit = Collision::SATTriangleVSAABB(a, b, c, root.position, root.size);
        if (trihit.isColliding){
            nIndices.push_back(i0);
            nIndices.push_back(i1);
            nIndices.push_back(i2);
            
            hitCount++;
        }
    }
    
    // if under min or nohit
    if (hitCount < minTri || hitCount <= 0) return false;
    //if (hitCount <= 0) return false;
    
    // should kd split and test both
    Collision::KDsplit kds = Collision::KDsplitVolume(root.position, root.size);
    // if children have any axis below min size snap them, and if all are return false
    // going for if statements so i can filter which axis are min
    int countBelowMinimum = 0;
    if (kds.firstSplit.size.x < minSize.x){ kds.firstSplit.size.x  = minSize.x; countBelowMinimum++;}
    if (kds.firstSplit.size.y < minSize.y){ kds.firstSplit.size.y  = minSize.y; countBelowMinimum++;}
    if (kds.firstSplit.size.z < minSize.z){ kds.firstSplit.size.z  = minSize.z; countBelowMinimum++;}
    if (kds.secondSplit.size.x < minSize.x){ kds.secondSplit.size.x  = minSize.x; countBelowMinimum++;}
    if (kds.secondSplit.size.y < minSize.y){ kds.secondSplit.size.y  = minSize.y; countBelowMinimum++;}
    if (kds.secondSplit.size.z < minSize.z){ kds.secondSplit.size.z  = minSize.z; countBelowMinimum++;}
    
    if (countBelowMinimum >= 6) return false;
    // ^^ yandere dev ahh code
    
    // for 0 = infinte steps
    int nSize = 1; if (steps > 0) nSize = steps - 1;
    
    //kds.firstSplit.size
    // traverse down
    bool rA = voxelizerInternalKDAccel(vertices, nIndices, kds.firstSplit, nSize, minTri, minSize, AABBs);
    bool rB = voxelizerInternalKDAccel(vertices, nIndices, kds.secondSplit, nSize, minTri, minSize, AABBs);
    
    Collision::voxelAccel nVA;
    nVA.voxel = root;
    nVA.indices = nIndices;
    
    // if both are false, pushback the root
    if (!rA && !rB) AABBs.push_back(nVA);
    
    return true;
}

void voxelizer::axisSnap(Collision::AABB& aabb, int& countBelowMinimum, glm::vec3 minSize){
    if (aabb.size.x < minSize.x){ aabb.size.x  = minSize.x; countBelowMinimum++;}
    if (aabb.size.y < minSize.y){aabb.size.y  = minSize.y; countBelowMinimum++;}
    if (aabb.size.z < minSize.z){ aabb.size.z  = minSize.z; countBelowMinimum++;}
}
