#include "ModelAssimp.h"
#include "Systems/util/UUID.h"
#include  <utils/FE_math.h>
#include <Systems/Physics/accelerate.h>
#include <assimp/pbrmaterial.h>
#include <assimp/material.h>
#include <thread>
#include "utils/imageWrite.h"
#include <chrono>
#include <Systems/Physics/SDF.h>
#include "Render/pipeline/prebuilt_pipelines/swrt.h"
#include <Render/Handler/RenderHandler.h>

void Model::updatePosition(glm::vec3 Position)
{globalTransformation.position = Position;}

void Model::updateRotation(glm::vec3 Rotation)
{globalTransformation.rotation = Rotation;}

void Model::updateScale(glm::vec3 Scale)
{globalTransformation.scale = Scale;}

void Model::updateTranformation(){

    gModelMatrix = FE_Math::composeMatrixWDegrees(globalTransformation.position, globalTransformation.scale, globalTransformation.rotation);

    for (unsigned int i = 0; i < meshes.size(); i++){
        meshes[i].updateMatrix(lModelMatrix[i]);
        meshes[i].updatePosition(localTransformation[i].position);
		meshes[i].updateRotation(localTransformation[i].rotation);
		meshes[i].updateScale(localTransformation[i].scale);

		meshes[i].updateGlobalMatrix(gModelMatrix);
        meshes[i].updateGlobalPosition(globalTransformation.position);
        meshes[i].updateGlobalScale(globalTransformation.scale);
        meshes[i].updateGlobalRotation(globalTransformation.rotation);
	}

}

void Model::updatePrevPosition(glm::vec3 Position)
{previousGlobalTransformation.position = Position;}

void Model::updatePrevRotation(glm::vec3 Rotation)
{previousGlobalTransformation.rotation = Rotation;}

void Model::updatePrevScale(glm::vec3 Scale)
{previousGlobalTransformation.scale = Scale;}

void Model::updatePrevTranformation(){
    pgModelMatrix = FE_Math::composeMatrixWDegrees(previousGlobalTransformation.position, previousGlobalTransformation.scale, previousGlobalTransformation.rotation);

    for (unsigned int i = 0; i < meshes.size(); i++){
        meshes[i].updatePrevGlobalMatrix(pgModelMatrix);
    }
}

void Model::childrenRangeCull(glm::vec3 position, float range){
    //return;
    //rootnodes
    
    for (unsigned int i = 0; i < rootnodes.size(); i++){
        if (!Collision::AABBtoSphereRangeCull(rootnodes[i].position, rootnodes[i].size, position, range) ) meshes[i].culled = true;
    }  
}


Model::Model(const char* file, bool disableConstructorLoading, bool disableInitialMeshUploadToVBO, bool disableInitialTextureUploadToGPU){
    UUID = UUID::returnHandle();
    path = file;
    // do hash here
    unsigned int hash = 5381;
    for (int i = 0; i < path.size(); ++i){
        hash *= 32 + path[i];
    }
    Model::hash = hash;
    
    disableConstructorLoadingModelFlag = disableConstructorLoading;
    disableInitialMeshUploadToVBOFlag = disableInitialMeshUploadToVBO;
    disableInitialTextureUploadToGPUFlag = disableInitialTextureUploadToGPU;
    
    // if not enabled then we can load
    if (!disableConstructorLoadingModelFlag) loadModel(file);
}

Model::~Model() {
    // Delete mesh first then clear all array inside model
    for (size_t i = 0; i < meshes.size(); i++){
        meshes[i].Delete();
        meshes.erase(meshes.begin() + i);
    }
    meshes.clear();

    for (size_t i = 0; i < loadedTexPath.size(); i++){
        loadedTexPath.erase(loadedTexPath.begin() + i);
    }
    loadedTexPath.clear();

    for (size_t i = 0; i < loadedTex.size(); i++){
        loadedTex[i].Delete();
        loadedTex.erase(loadedTex.begin() + i);
    }
    loadedTex.clear();

    for (int i = 0; i < meshSDFs.size(); ++i){
        meshSDFs[i]->Delete();
        meshSDFs.erase(meshSDFs.begin() + i);
    }

    for (int i = 0; i < meshSDFs2.size(); ++i){
        meshSDFs2[i]->Delete();
        meshSDFs2.erase(meshSDFs2.begin() + i);
    }
    
}

void Model::loadModelPathless(){
    loadModel(path);
}

void Model::draw(Shader& shader, Camera Camera){
    if (!loaded) return;
    //doLodsDraw
	// draw all meshes and parse in data
    for (unsigned int i = 0; i < meshes.size(); i++){
        //Collision::Sphere nSphere = Collision::AABBtoSphere(rootnodes[i].position, rootnodes[i].size);
        bool tLod = meshes[i].hasLod; //get
        bool tforceLodLevel = meshes[i].forceLodLevel;
        meshes[i].hasLod = doLodsDraw;
        meshes[i].forceLodLevel = forceLodLevel;
      //  if (Camera.isRadiusInFrustum(nSphere.position, nSphere.radius))
        
       // if (!meshAabbPoints.empty()) meshes[i].drawRoot = 
        
        RenderHandler::drawCount++;
        
        shader.setInt("meshIndex", i);
        shader.setInt("totalDrawCount", RenderHandler::drawCount);
        meshes[i].draw(shader, Camera);
        
        meshes[i].hasLod = tLod; //set
        meshes[i].forceLodLevel = tforceLodLevel;
    }  
}

void Model::drawInstance(Shader& shader, Camera Camera, int instanceCount){
    if (!loaded) return;
    for (unsigned int i = 0; i < meshes.size(); i++){
        meshes[i].drawInstanced(shader, Camera, instanceCount);
    }
}

void Model::createMeshAABBs(){
    if (!loaded) return;
    meshAabbPoints.clear();
    rootnodes.clear();

    meshAabbPoints.reserve(meshes.size());
    rootnodes.reserve(meshes.size());
    
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
    
    for (size_t i = 0; i < meshes.size(); i++){
        Collision::rubiksCubePoints newRubikzCube;
        newRubikzCube = Collision::fetchFurthestVertices(meshes[i].vertices);
        meshes[i].meshAabbPoints = newRubikzCube;
        meshAabbPoints.push_back(newRubikzCube);
        rootnodes.emplace_back();
        
        // worse way to find min max in history (but pre comp so who cares)
        
        // transform
        newRubikzCube = Collision::transformRubiks(newRubikzCube, lModelMatrix[i]);
        
        glm::vec3 nmin = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 nmax = glm::vec3(std::numeric_limits<float>::lowest());
        
        // buttom () these all gotta be trnasformed :/
        nmin = glm::min(nmin, newRubikzCube.DLB);
        nmin = glm::min(nmin, newRubikzCube.DLF);
        nmin = glm::min(nmin, newRubikzCube.DRB);
        nmin = glm::min(nmin, newRubikzCube.DRF);
        nmax = glm::max(nmax, newRubikzCube.DLB);
        nmax = glm::max(nmax, newRubikzCube.DLF);
        nmax = glm::max(nmax, newRubikzCube.DRB);
        nmax = glm::max(nmax, newRubikzCube.DRF);
        // top
        nmin = glm::min(nmin, newRubikzCube.ULB);
        nmin = glm::min(nmin, newRubikzCube.ULF);
        nmin = glm::min(nmin, newRubikzCube.URB);
        nmin = glm::min(nmin, newRubikzCube.URF);
        nmax = glm::max(nmax, newRubikzCube.ULB);
        nmax = glm::max(nmax, newRubikzCube.ULF);
        nmax = glm::max(nmax, newRubikzCube.URB);
        nmax = glm::max(nmax, newRubikzCube.URF);

        
        min = glm::min(min, nmin);
        max = glm::max(max, nmax);
	}
    
    ModelBounds.position = (min + max) * 0.5f;
    ModelBounds.size = (max - min) * 0.5f;
}

void Model::generateMeshBlases(int mintri, int maxDepth){
    std::cout << "blas generation" <<  std::endl;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < meshes.size(); ++i){
        //Collision::AABB nRootNode = Collision::rootNodeFromRubixPointsNoPadding(meshAabbPoints[i], glm::mat4(1.0));
        meshes[i].genBlas(mintri, maxDepth);
                
        //std::cout << "index: " << i << " - " << abs((((meshes.size() -float(i)) - meshes.size()) / meshes.size()) * 100.0f ) << "%"<< std::endl;
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> seconds = end - start;
    std::cout << "time elapsed (seconds): " << seconds.count() << std::endl;
}

void Model::MDFgenerateBlas(int sliceSize, GLuint slot){
    if (!loaded) return;
    
    if (meshes.size() > FlouraSWRT::localPerModelMeshCountCap){
        sdfCompatible = false;
        return;
    }
    
    for (int i = 0; i < meshSDFs.size(); ++i)
        meshSDFs[i]->Delete();
    meshSDFs.clear();
    
    auto start = std::chrono::steady_clock::now();
    // should be some kinda hash thingy to check if we have sdf already, and then if so load them from disk
    // maybe use renderID_index_size.png
    std::cout << "count: " << meshes.size()<< std::endl;
    for (int i = 0; i < meshes.size(); ++i){
        Collision::AABB nRootNode = Collision::rootNodeFromRubixPointsNoPadding(meshAabbPoints[i], glm::mat4(1.0));
        
        float normalizedScale = FE_Math::normalizeScale(nRootNode.size, 1.0f); // 1.0 is the area but a .2 pad would be good
        // min + max * 0.5
        //glm::vec3 centre = ( (nRootNode.position - nRootNode.size) + (nRootNode.position + nRootNode.size) * 0.5f);
        
        glm::mat4 normalizedMatrix(1.0);
        normalizedMatrix = glm::scale(normalizedMatrix, glm::vec3(normalizedScale));
        //normalizedMatrix = glm::translate(normalizedMatrix, -centre);
        
        Collision::AABB transformedRootNode = Collision::rootNodeFromRubixPointsNoPadding(meshAabbPoints[i], normalizedMatrix);
        
        std::vector<Vertex> nVertices = meshes[i].vertices;

        for (int x = 0; x< meshes[i].vertices.size(); ++x)
            FE_Math::transformPoint(nVertices[x].position, normalizedMatrix);
        
        // need to scale blas for this
        
        std::vector<accelerate::leaf> nBLAS = meshes[i].blas;
        
        // transform blas
        for (int x = 0; x< nBLAS.size(); ++x){
            nBLAS[x].aabb = Collision::rootNodeFromRubixPointsNoPadding(Collision::aabbToRubixCubePoints(nBLAS[x].aabb.position, nBLAS[x].aabb.size), normalizedMatrix);
        }
        
        Texture3D* nT3D;
        nT3D = new Texture3D();
        meshSDFs.push_back(nT3D);
        // still pushbak the texture lets just not do anything with it
        if (nBLAS.empty()) continue;
        
        // sdf generate function using voxel accel
        //flouraSDF::bakeMeshSDF(nVertices, meshes[i].indices, transformedRootNode, sliceSize, *meshSDFs.back(), slot);
        //flouraSDF::bakeMeshSDFAccel(nVertices, nVA, transformedRootNode, sliceSize, *meshSDFs.back(), slot);
        flouraSDF::bakeMeshSDFAccel(nVertices, nBLAS, transformedRootNode, sliceSize, *meshSDFs.back(), slot);
        
        std::cout << "index: " << i << " - " << abs((((meshes.size() -float(i)) - meshes.size()) / meshes.size()) * 100.0f ) << "%"<<"\n";
    }
        
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> seconds = end - start;
    std::cout << "time elapsed (seconds): " << seconds.count() << std::endl;
}

void Model::MDFgeneratePrimGPU(int sliceSize, GLuint slot, float thickness){
        if (!loaded) return;
    
        if (meshes.size() > FlouraSWRT::localPerModelMeshCountCap){
            sdfCompatible = false;
            return;
        }
        
        for (int i = 0; i < meshSDFs.size(); ++i)
            meshSDFs[i]->Delete();
        meshSDFs.clear();

        for (int i = 0; i < meshSDFs2.size(); ++i)
            meshSDFs2[i]->Delete();
    meshSDFs2.clear();
    
    auto start = std::chrono::steady_clock::now();
    
    std::cout << "Amount to generate: " << meshes.size() << std::endl;
    for (int i = 0; i < meshes.size(); ++i){
        Collision::AABB nRootNode = Collision::rootNodeFromRubixPointsNoPadding(meshAabbPoints[i], glm::mat4(1.0));
        
        float normalizedScale = FE_Math::normalizeScale(nRootNode.size, 1.0f); // 1.0 is the area but a .2 pad would be good
        // min + max * 0.5
        //glm::vec3 centre = ( (nRootNode.position - nRootNode.size) + (nRootNode.position + nRootNode.size) * 0.5f);
        
        glm::mat4 normalizedMatrix = glm::scale(glm::mat4(1.0), glm::vec3(normalizedScale));
        //normalizedMatrix = glm::translate(normalizedMatrix, -centre);
        
        Collision::AABB transformedRootNode = Collision::rootNodeFromRubixPointsNoPadding(meshAabbPoints[i], normalizedMatrix);
        
        std::vector<Vertex> nVertices = meshes[i].vertices;

        for (int x = 0; x< meshes[i].vertices.size(); ++x)
            FE_Math::transformPoint(nVertices[x].position, normalizedMatrix);
        
        std::vector<accelerate::BVH_primitive> nPrims = accelerate::buildIndicesIntoPrims(meshes[i].vertices, meshes[i].indices);
        
        // transform blas
        for (int x = 0; x< nPrims.size(); ++x){
            nPrims[x].extents = Collision::rootNodeFromRubixPointsNoPadding(Collision::aabbToRubixCubePoints(nPrims[x].extents.position, nPrims[x].extents.size), normalizedMatrix);
            //std::cout<<nBLAS[x].prims.size() << std::endl;
        }
        
        Texture3D* nT3D; nT3D = new Texture3D();
        meshSDFs.push_back(nT3D);
        Texture3D* nT3D2; nT3D2 = new Texture3D();
        meshSDFs2.push_back(nT3D2);
        //meshSDFs2
        
        // still pushbak the texture lets just not do anything with it
        if (nPrims.empty()) continue;
        
       //
        
        // sdf generate function using voxel accel
        //flouraSDF::bakeMeshSDF(nVertices, meshes[i].indices, transformedRootNode, sliceSize, *meshSDFs.back(), slot);
        //flouraSDF::bakeMeshSDFAccel(nVertices, nVA, transformedRootNode, sliceSize, *meshSDFs.back(), slot);
        flouraSDF::bakeMeshDistanceFieldGPU(nVertices, nPrims, transformedRootNode, sliceSize, *meshSDFs.back(), *meshSDFs2.back(), slot, thickness, meshes[i].textures);
        
        std::cout << "index: " << i << " - " << abs((((meshes.size() -float(i)) - meshes.size()) / meshes.size()) * 100.0f ) << "%"<< std::endl;
    }
        
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> seconds = end - start;
    std::cout << "time elapsed (seconds): " << seconds.count() << std::endl;
}

void Model::MDFgenerateGridPrimGPU(int sliceSize, GLuint slot, float thickness, int div){
    if (!loaded) return;
    
    if (meshes.size() > FlouraSWRT::localPerModelMeshCountCap){
        sdfCompatible = false;
        return;
    }
        
    for (int i = 0; i < meshSDFs.size(); ++i) meshSDFs[i]->Delete();
    meshSDFs.clear();

    for (int i = 0; i < meshSDFs2.size(); ++i) meshSDFs2[i]->Delete();
    meshSDFs2.clear();
    
    auto start = std::chrono::steady_clock::now();
    
    // yea i know bad, but im lazy
    // build model brounds
    std::vector<glm::vec3> points; 
    for (int i = 0; i < meshAabbPoints.size(); ++i){
        points.push_back(meshAabbPoints[i].URF);
        points.push_back(meshAabbPoints[i].ULF);
        points.push_back(meshAabbPoints[i].URB);
        points.push_back(meshAabbPoints[i].ULB);
        
        points.push_back(meshAabbPoints[i].DRF);
        points.push_back(meshAabbPoints[i].DLF);
        points.push_back(meshAabbPoints[i].DRB);
        points.push_back(meshAabbPoints[i].DLB);
    }
    Collision::AABB meshBounds = Collision::createAABBfromPoints(points);
    // uniform grid gen
    std::vector<Collision::AABB> cells;
    accelerate::uniformSplitEmptySpace(cells, meshBounds.position, meshBounds.size, div); // settle for splits in model bounds, and then normalize each cell themsleves
        
    std::cout << "Amount to attempt to generate: " << cells.size() << std::endl;
    for (int i = 0; i < cells.size(); ++i){
        std::vector<Mesh*> nMeshes;
        std::vector<glm::mat4> nLocalTrans;
        
        // tag touching nodes and meshes (could try isolating tagged meshes)
        for (int x = 0; x < meshes.size(); ++x){
            Collision::AABB nRootNode = Collision::rootNodeFromRubixPointsNoPadding(meshAabbPoints[x], glm::mat4(1.0)); // glm::mat4(1.0)
            Collision::HitResult hr = Collision::AABBvsAABB(nRootNode.position, nRootNode.size, cells[i].position, cells[i].size);
            if (hr.isColliding || true){ // temporary
                nMeshes.push_back(&meshes[x]);
                nLocalTrans.push_back(lModelMatrix[x]);
                //std::cout << x <<std::endl;
            }
        }
        if (nMeshes.empty()) continue;
        
        std::cout << "nmesh size: " << nMeshes.size() << std::endl;
        
        float normalizedScale = FE_Math::normalizeScale(cells[i].size, 1.0f);
        //glm::mat4 normalizedMatrix(1.0); // small in a corner????
        glm::mat4 normalizedMatrix = glm::scale(glm::mat4(1.0), glm::vec3(normalizedScale)); // commenting this out brings me more so 
        
   //     Collision::AABB normalizedRootNode = Collision::rootNodeFromRubixPointsNoPadding(
   //     Collision::aabbToRubixCubePoints(meshBounds.position, meshBounds.size), normalizedMatrix);

        Collision::AABB normalizedRootNode = Collision::rootNodeFromRubixPointsNoPadding(
        Collision::aabbToRubixCubePoints(cells[i].position, cells[i].size), normalizedMatrix);
        
        bool cellCreation = false;
        // now where we actually do every mesh
        for (int x = 0; x < nMeshes.size(); ++x){
            std::vector<Vertex> nVertices = nMeshes[x]->vertices;

            //nMeshes

            for (int z = 0; z< nVertices.size(); ++z)
                FE_Math::transformPoint(nVertices[z].position, normalizedMatrix * nLocalTrans[x]);
            
            std::vector<accelerate::BVH_primitive> nPrims = accelerate::buildIndicesIntoPrims(nVertices, nMeshes[x]->indices);
            if (nPrims.empty()) continue;
            
            // temporary
            // transform blas
            //for (int z = 0; z< nPrims.size(); ++z)
            //    nPrims[z].extents = Collision::rootNodeFromRubixPointsNoPadding(Collision::aabbToRubixCubePoints(nPrims[z].extents.position, nPrims[z].extents.size), normalizedMatrix);
            
            // triangle test (commented out temporarily)
            /*
            bool hitPrim = false;
            for (int z = 0; z < nPrims.size(); ++z){
                Collision::HitResult hr = Collision::AABBvsAABB(nPrims[z].extents.position, nPrims[z].extents.size, cells[i].position, cells[i].size);
                if (!hr.isColliding) continue;
                hitPrim = true;// break; // if any hit then we break
            }
            if (!hitPrim) continue;
            */
            
            
            // if even one mesh has a triangle inside, counter will be one and we create volume, itll go over one if theres more than 1 mesh hit
            bool isnotFirst(cellCreation);
            if (!isnotFirst){
                Texture3D* nT3D; nT3D = new Texture3D();
                meshSDFs.push_back(nT3D);
                Texture3D* nT3D2; nT3D2 = new Texture3D();
                meshSDFs2.push_back(nT3D2);
                cellCreation = true;
                
                mdfNodes.push_back(cells[i]);
                
            }
            if (meshSDFs.empty() || meshSDFs2.empty()) continue;
            
            flouraSDF::bakeMeshDistanceFieldGridGPU(nVertices, nPrims, normalizedRootNode, sliceSize, *meshSDFs.back(), *meshSDFs2.back(), slot, thickness, nMeshes[x]->textures, isnotFirst);
            
            std::cout << "cell: " << i <<  "- mesh index: " << x << " - " << abs((((nMeshes.size() -float(x)) - nMeshes.size()) / nMeshes.size()) * 100.0f ) << "%"<< std::endl;
        }
        std::cout << "cell index: " << i << " - " << abs((((cells.size() -float(i)) - cells.size()) / cells.size()) * 100.0f ) << "%"<< std::endl;
    }
    
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> seconds = end - start;
    std::cout << "time elapsed (seconds): " << seconds.count() << std::endl;
}

void Model::updateMeshAABBs(){
    if (!loaded) return;
    for (size_t i = 0; i < meshes.size(); i++){
        glm::mat4 finalMeshMat = gModelMatrix * lModelMatrix[i];
        rootnodes[i] = Collision::rootNodeFromRubixPoints(meshAabbPoints[i], finalMeshMat);
    }
}

void Model::loadModel(std::string path){
    if (loaded) return; // prevent loading loop
    
    loaded = true; // im putting this first to avoid any loading loops with the load function i wanna use
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);
    //const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return;
    }
    directory = path.substr(0, path.find_last_of('/'));
    
    processNode(scene->mRootNode, scene);
    
    //std::cout << "bone count " << m_BoneCounter << std::endl;
}

void Model::processNode(aiNode* node, const aiScene* scene){
    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++){
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        totalVertices += mesh->mNumVertices;
        totalIndices  += mesh->mNumFaces * 3;
        totalBones += mesh->mNumBones;
        
        processPositions(node);
        meshes.push_back(processMesh(mesh, scene));
    }
    
    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++){
        processNode(node->mChildren[i], scene);
    }
}

void Model::processPositions(aiNode* node){
    aiMatrix4x4 localTransform = node->mTransformation;
    aiVector3D position; aiVector3D scale; aiQuaternion rotation;
    localTransform.Decompose(scale, rotation, position);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(position.x, position.y, position.z));
    glm::mat4 rotation_matrix = glm::mat4_cast(glm::quat(rotation.w, rotation.x, rotation.y, rotation.z));
    model = model * rotation_matrix;
    model = glm::scale(model, glm::vec3(scale.x, scale.y, scale.z));
    
    //localTransformation
    transformation newTransformation;
    newTransformation.position = glm::vec3(position.x, position.y, position.z);
    newTransformation.scale = glm::vec3(scale.x, scale.y, scale.z);
    newTransformation.qRotation = glm::quat(rotation.w, rotation.x, rotation.y, rotation.z);
    glm::vec3 euler_radians = glm::eulerAngles(newTransformation.qRotation);
    newTransformation.rotation = glm::degrees(euler_radians);
    
    localTransformation.push_back(newTransformation);
    lModelMatrix.push_back(model);
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene){
    Mesh nMesh;
    // primary mesh data extraction
    std::vector<Vertex> vertices = assembleVertices(mesh);
    std::vector<GLuint> indices = assembleIndices(mesh);
    std::vector<Texture> textures = assembleMaterials(mesh, scene);
    
    nMesh.HasBones = mesh->HasBones();
    
    ExtractBoneWeightForVertices(vertices, mesh, scene);
    
    aiString name = mesh->mName;
	nMesh.name = name.C_Str();
    //std::cout << "disableInitialMeshUploadToVBOFlag: " << disableInitialMeshUploadToVBOFlag << std::endl;
    if (disableInitialMeshUploadToVBOFlag) nMesh.suppressSetupMeshCall = true; // if flag is enabled, suppress uploading to the gpu
    nMesh.create(vertices, indices, textures);
    return Mesh(nMesh);
}

void Model::SetVertexBoneDataToDefault(Vertex& vertex){
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++){
        vertex.m_BoneIDs[i] = -1; // invalid ID -1
        vertex.m_Weights[i] = 0.0f;
    }
}

void Model::SetVertexBoneData(Vertex& vertex, int boneID, float weight){
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i){
        if (vertex.m_BoneIDs[i] < 0){
            vertex.m_Weights[i] = weight;
            vertex.m_BoneIDs[i] = boneID;
            break;
        }
    }
}

static inline glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from) // thanks learnopengl
{
    glm::mat4 to;
    //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

void Model::ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene){
    auto& boneInfoMap = m_BoneInfoMap;
    int& boneCount = m_BoneCounter;
    
    for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex){
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
        if (boneInfoMap.find(boneName) == boneInfoMap.end()){
            BoneInfo newBoneInfo;
            newBoneInfo.id = boneCount;
            newBoneInfo.offset = ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
            boneInfoMap[boneName] = newBoneInfo;
            boneID = boneCount;
            boneCount++;
        }
        else{
            boneID = boneInfoMap[boneName].id;
        }
        assert(boneID != -1);
        auto weights = mesh->mBones[boneIndex]->mWeights;
        int numWeights = mesh->mBones[boneIndex]->mNumWeights;

        for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex){
            int vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;
            assert(vertexId <= vertices.size());
            SetVertexBoneData(vertices[vertexId], boneID, weight);
        }
    }
    
}


std::vector<Vertex> Model::assembleVertices(aiMesh* mesh){
    std::vector<Vertex> vertices;
    
    for (unsigned int i = 0; i < mesh->mNumVertices; i++){
        Vertex vertex;
        SetVertexBoneDataToDefault(vertex); // feed dummy data
        
        // process vertex positions, normals and texture coordinates
        glm::vec3 vector;
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.position = vector;

        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;
        vertex.normal = vector;

        vector.x = mesh->mTangents[i].x;
        vector.y = mesh->mTangents[i].y;
        vector.z = mesh->mTangents[i].z;
        vertex.tangent = vector;

        vector.x = mesh->mBitangents[i].x;
        vector.y = mesh->mBitangents[i].y;
        vector.z = mesh->mBitangents[i].z;
        vertex.biTangent = vector;

        //if (mesh->GetNumColorChannels() > 0)
        //{
        //vertex.color.x = mesh->mColors[1][i].r;
        //vertex.color.y = mesh->mColors[1][i].g;
        //vertex.color.z = mesh->mColors[1][i].b;
        //}
        //else
        vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);

        if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates? // determine colour here
        {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.texUV = vec;
        }
        else
            vertex.texUV = glm::vec2(0.0f, 0.0f);
            
            
        vertices.push_back(vertex);
    }
    
    return vertices;
}

std::vector<GLuint> Model::assembleIndices(aiMesh* mesh){
    std::vector<GLuint> indices;
    
    for (unsigned int i = 0; i < mesh->mNumFaces; i++){
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    
    return indices;
}


std::vector<Texture> Model::assembleMaterials(aiMesh* mesh, const aiScene* scene){
    std::vector<Texture> textures;
    
    if (mesh->mMaterialIndex >= 0){ // needs to check material type like "vec4 col instead of texture"

        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::vector<Texture> diffuseMaps = loadMaterialTextures(material,
            aiTextureType_DIFFUSE, "texture_diffuse", 0);
        std::vector<Texture> roughnessMaps = loadMaterialTextures(material,
            aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness", 1); // Note: Could also be aiTextureType_SHININESS
        std::vector<Texture> normalMaps = loadMaterialTextures(material,
            aiTextureType_NORMALS, "texture_normal", 2);
        std::vector<Texture> emissionMaps = loadMaterialTextures(material,
            aiTextureType_EMISSIVE, "texture_emission", 3);

        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        textures.insert(textures.end(), emissionMaps.begin(), emissionMaps.end());

    }
    
    return textures;
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type,
    std::string typeName, int slot){
    std::vector<Texture> textures;

    aiTextureType targetType = type;

    if (mat->GetTextureCount(targetType) == 0){
        glm::vec4 colourFloat = glm::vec4(0.0f);

        switch (type){
        case aiTextureType_DIFFUSE:{
                aiColor4D albedoFactor(1.0f, 1.0f, 1.0f, 1.0f);
                mat->Get(AI_MATKEY_COLOR_DIFFUSE, albedoFactor);
                colourFloat = glm::vec4(albedoFactor[0], albedoFactor[1], albedoFactor[2], albedoFactor[3]);
                break;
            }
        case aiTextureType_DIFFUSE_ROUGHNESS:{
                ai_real metallic = 1.0f; ai_real roughness = 1.0f;
                mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
                mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
                colourFloat = glm::vec4(1.0f, roughness, metallic, 1.0 - glm::sqrt(roughness));
                break;
            }
        case aiTextureType_NORMALS:{
                colourFloat = glm::vec4(0.5f, 0.5f, 1.0f, 1.0f);
                break;
            }
        case aiTextureType_EMISSIVE:{
                aiColor3D emissionColor(0.0f, 0.0f, 0.0f);             ai_real emissionFactor = 1.0f;
                mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissionColor);
                mat->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissionFactor);
                colourFloat = glm::vec4(emissionColor.r,emissionColor.g,emissionColor.b, emissionFactor);
                break;
            }
            default:{
                colourFloat = glm::vec4(1.0f);
                break;
            }
        }

        Texture texture;
        if (disableInitialTextureUploadToGPUFlag) texture.suppressCreation = true; // if the flag is enabled, suppress the entire texture from being loaded, and store the vars to be reused
        texture.createColour(colourFloat, (typeName).c_str(), slot);
        textures.push_back(texture);
        loadedTex.push_back(texture);
        loadedTexPath.push_back("null");
    }
    else
    {
        for (unsigned int i = 0; i < mat->GetTextureCount(targetType); i++){
        
            aiString str; // str is path
            mat->GetTexture(targetType, i, &str);
            bool skip = false;
            for (unsigned int j = 0; j < loadedTex.size(); j++){
                // check if already exsists, if so pushback texture and break
                if (loadedTexPath[j] == (str).C_Str()) {// dont have a thing for that yet
                    textures.push_back(loadedTex[j]);
                    skip = true;
                    break;
                }
            }
            if (!skip){
                Texture texture;
                if (disableInitialTextureUploadToGPUFlag) texture.suppressCreation = true; // if the flag is enabled, suppress the entire texture from being loaded, and store the vars to be reused
                std::string path = directory + "/" + str.C_Str();
                texture.linearFilter = true;
                texture.createTexture(path.c_str(), (typeName).c_str(), slot);
                
                // detect failure and push def value (if not diffuse)
                if (texture.skipstbi && type != aiTextureType_DIFFUSE){
                    glm::vec4 colourFloat = glm::vec4(0.0f);
                    
                    switch (type){
                    case aiTextureType_DIFFUSE_ROUGHNESS:{ colourFloat = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f); break; }
                    case aiTextureType_NORMALS:{ colourFloat = glm::vec4(0.5f, 0.5f, 1.0f, 1.0f); break;}
                    case aiTextureType_EMISSIVE:{ colourFloat = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); break; }
                    default:{ colourFloat = glm::vec4(1.0f); break; }
                    }
                    
                    // push new
                    texture.Delete();
                    texture.createColour(colourFloat, (typeName).c_str(), slot);
                }
                
                // push texture
                textures.push_back(texture);
                loadedTex.push_back(texture);
                loadedTexPath.push_back(str.C_Str());
            }   
        }
    }
    return textures;
}