#include "SDF.h"
#include <utils/FE_math.h>

#include "Render/Handler/RenderHandler.h"
#include "utils/logConsole.h"
#include <utils/imageWrite.h>
#include <glm/gtx//matrix_decompose.hpp>

Shader flouraSDF::mdfCompute;

void flouraSDF::createShaders(){
	mdfCompute.LoadComputeShader("Assets/Shaders/compute/mdfBake.comp");
}

void flouraSDF::cleanupShaders(){
	mdfCompute.Delete();
}

void flouraSDF::cacheSDF(const char* path, int hash, std::vector<Texture3D *>& meshSDFs){
	for (int i = 0; i < meshSDFs.size(); ++i){
		std::string nPath = path + std::to_string(hash) + "_" + std::to_string(i) +".SDF";
		FlouraImageWrite::writeImage3DToDiskJSON(meshSDFs[i]->ID,
			meshSDFs[i]->width, meshSDFs[i]->height, meshSDFs[i]->depth,
			nPath.c_str(), GL_RGB, GL_FLOAT, 3); // GB
	}
}

GLuint flouraSDF::MDFGENERATIONSSBO;

void flouraSDF::bakeMeshDistanceFieldGPU(std::vector<Vertex> &vertices, std::vector<accelerate::BVH_primitive>& prims, Collision::AABB root, const int sliceSize, Texture3D& texture, Texture3D& talbedo, GLuint slot, float thickness, std::vector<Texture>& textures){
	// yeah i know, tons of wasted empty components, idc tho
	struct triangle{
		glm::vec4 a;
		glm::vec4 b;
		glm::vec4 c;
		
		glm::vec4 uvAB;
		glm::vec4 uvC;
		
		glm::vec4 primPos;
		glm::vec4 primExt;
	};
	
	std::vector<triangle> triangles;
	
	for (int i = 0; i < prims.size(); ++i){
		//prims[i].extents
		
		const unsigned int &i0 = prims[i].i0;
		const unsigned int &i1 = prims[i].i1;
		const unsigned int &i2 = prims[i].i2;
			
		if (i0 >= vertices.size() ||
			i1 >= vertices.size() ||
			i2 >= vertices.size())
			continue;
		
		triangle nTriangle;
		nTriangle.a = glm::vec4(vertices[i0].position, 1.0);
		nTriangle.b = glm::vec4(vertices[i1].position, 1.0);
		nTriangle.c = glm::vec4(vertices[i2].position, 1.0);
		
		nTriangle.uvAB = glm::vec4(vertices[i0].texUV, vertices[i1].texUV);
		nTriangle.uvC = glm::vec4(vertices[i2].texUV,1.0, 1.0);
		
		nTriangle.primPos = glm::vec4(prims[i].extents.position, 1.0);
		nTriangle.primExt = glm::vec4(prims[i].extents.size, 1.0);
		
		triangles.push_back(nTriangle);
	}
	
	if (triangles.empty()) return;
	/**/
	// generate
	glGenBuffers(1, &MDFGENERATIONSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, MDFGENERATIONSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, triangles.size() * sizeof(triangle), triangles.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, MDFGENERATIONSSBO); // 6
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind
	

	texture.Delete(); texture.createImage3D(sliceSize,sliceSize,sliceSize, "SDF", slot, GL_RGBA16F); // GL_RGBA32F GL_R8_SNORM GL_RGBA16F
	talbedo.Delete(); talbedo.createImage3D(sliceSize,sliceSize,sliceSize, "SDF", slot + 1, GL_RGBA8);
	/**/
	glBindImageTexture(0, texture.ID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F); // GL_RGBA32F GL_R8_SNORM
	glBindImageTexture(1, talbedo.ID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
	mdfCompute.Activate();
	mdfCompute.setInt3("volumeSize", texture.width,texture.height,texture.depth);
	mdfCompute.setFloat3("ivScale", root.size);
	mdfCompute.setFloat3("ivPosition", root.position);
	mdfCompute.setFloat("thickness", thickness);
	mdfCompute.setBool("accumulateMode", false);
	
	for (unsigned int x = 0; x < textures.size(); x++){
		if (textures[x].type == "texture_diffuse"){
			mdfCompute.setHandleui64ARB("AlbedoHandle", textures[x].handle); 
			continue;    
		}
		if (textures[x].type == "texture_emission"){
			mdfCompute.setHandleui64ARB("EMHandle", textures[x].handle);
			continue;    
		}
	}
	
	glDispatchCompute((texture.width + 7) / 8, (texture.height + 7) / 8, ((texture.depth + 7) / 8));
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	
	// cleanup
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glDeleteBuffers(1, &MDFGENERATIONSSBO);
	
	for (int i = 0; i < triangles.size(); ++i){
		triangles.erase(triangles.begin() + i);
	}
	triangles.clear();
}

void flouraSDF::bakeMeshDistanceFieldGridGPU(std::vector<Vertex>& vertices,
	std::vector<accelerate::BVH_primitive>& prims, Collision::AABB root, const int sliceSize, Texture3D& texture,
	Texture3D& talbedo, GLuint slot, float thickness, std::vector<Texture>& textures, bool isnotFirst)
{
	// yeah i know, tons of wasted empty components, idc tho
	struct triangle{
		glm::vec4 a;
		glm::vec4 b;
		glm::vec4 c;
		
		glm::vec4 uvAB;
		glm::vec4 uvC;
		
		glm::vec4 primPos;
		glm::vec4 primExt;
	};
	
	std::vector<triangle> triangles;
	
	for (int i = 0; i < prims.size(); ++i){
		//prims[i].extents
		
		const unsigned int &i0 = prims[i].i0;
		const unsigned int &i1 = prims[i].i1;
		const unsigned int &i2 = prims[i].i2;
			
		if (i0 >= vertices.size() ||
			i1 >= vertices.size() ||
			i2 >= vertices.size())
			continue;
		
		triangle nTriangle;
		nTriangle.a = glm::vec4(vertices[i0].position, 1.0);
		nTriangle.b = glm::vec4(vertices[i1].position, 1.0);
		nTriangle.c = glm::vec4(vertices[i2].position, 1.0);
		
		nTriangle.uvAB = glm::vec4(vertices[i0].texUV, vertices[i1].texUV);
		nTriangle.uvC = glm::vec4(vertices[i2].texUV,1.0, 1.0);
		
		nTriangle.primPos = glm::vec4(prims[i].extents.position, 1.0);
		nTriangle.primExt = glm::vec4(prims[i].extents.size, 1.0);
		
		triangles.push_back(nTriangle);
	}
	
	if (triangles.empty()) return;
	/**/
	// generate
	glGenBuffers(1, &MDFGENERATIONSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, MDFGENERATIONSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, triangles.size() * sizeof(triangle), triangles.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, MDFGENERATIONSSBO); // 6
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind
	
	if (!isnotFirst){
		texture.Delete(); 
		texture.createImage3D(sliceSize,sliceSize,sliceSize, "SDF", slot, GL_RGBA16F); // GL_RGBA32F GL_R8_SNORM GL_RGBA16F
		talbedo.Delete(); 
		talbedo.createImage3D(sliceSize,sliceSize,sliceSize, "SDF", slot + 1, GL_RGBA8);
	}
	
	/**/
	glBindImageTexture(0, texture.ID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F); // GL_RGBA32F GL_R8_SNORM
	glBindImageTexture(1, talbedo.ID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
	mdfCompute.Activate();
	mdfCompute.setInt3("volumeSize", texture.width,texture.height,texture.depth);
	mdfCompute.setFloat3("ivScale", root.size);
	mdfCompute.setFloat3("ivPosition", root.position);
	mdfCompute.setFloat("thickness", thickness);
	mdfCompute.setBool("accumulateMode", isnotFirst);

	for (unsigned int x = 0; x < textures.size(); x++){
		if (textures[x].type == "texture_diffuse"){
			mdfCompute.setHandleui64ARB("AlbedoHandle", textures[x].handle); 
			continue;    
		}
		if (textures[x].type == "texture_emission"){
			mdfCompute.setHandleui64ARB("EMHandle", textures[x].handle);
			continue;    
		}
	}
	
	glDispatchCompute((texture.width + 7) / 8, (texture.height + 7) / 8, ((texture.depth + 7) / 8));
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	
	// cleanup
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glDeleteBuffers(1, &MDFGENERATIONSSBO);
	
	for (int i = 0; i < triangles.size(); ++i){
		triangles.erase(triangles.begin() + i);
	}
	triangles.clear();
}

void flouraSDF::bakeMeshSDFAccel(std::vector<Vertex>& vertices, std::vector<accelerate::leaf>& leaves,
                                 Collision::AABB root, const int sliceSize, Texture3D& texture, GLuint slot){
    Collision::minmax mm = Collision::returnMinMax(root.position, root.size);
    glm::vec3 boundssize = mm.max - mm.min;
    
    // still here incase i change my mind
    const int sX = sliceSize;
    const int sY = sliceSize;
    const int sZ = sliceSize;

    //std::vector<float> sdfData(sX * sY * sZ);
    std::vector<float> sdfData(sX * sY * sZ * 3);

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
                
                glm::vec3 dist_pUV = distanceToClosestPointOnMeshSDFAccel_PlusUV(vertices, leaves, p);
                //int index = x + (y * sX) + (z * sX * sY);
                //sdfData[index] = dist_pUV.x;
                
                int index = (x + (y * sX) + (z * sX * sY)) * 3; // one for each component
                sdfData[index] = dist_pUV.x;
                sdfData[index + 1] = dist_pUV.y;
                sdfData[index + 2] = dist_pUV.z;
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
    GL_RGB32F, //     GL_R32F,
    sX,
    sY,
    sZ,
    0,
    GL_RGB, //GL_RED
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


// this function is broken


void flouraSDF::nearestPointBlasTraversal(std::vector<accelerate::leaf>& leaves, std::vector<Vertex>& vertices, float &minDist, int &minIndex, int& closestPrimIndex, int cLeafIndex, glm::vec3& P){
    if (leaves.empty() || cLeafIndex < 0) return;
    
    // nearest point on node
	glm::vec3 np = Collision::nearestPointOnAABB(P, leaves[cLeafIndex].aabb.position, leaves[cLeafIndex].aabb.size);
    float nd = glm::distance(np, P); // get the distance
	
	if (nd >= minDist)return;
	
	if  (leaves[cLeafIndex].firstChildIndex <= -1 && leaves[cLeafIndex].secondChildIndex <= -1){
		//minIndex = cLeafIndex;
		//minDist = nd;
		
		float primMinDist = minDist;
		for (int i = 0; i < leaves[cLeafIndex].prims.size(); ++i){
			const glm::vec3 npp = Collision::nearestPointOnAABB(P, leaves[cLeafIndex].prims[i].extents.position, leaves[cLeafIndex].prims[i].extents.size);
			float npd = glm::distance(npp, P);
			if (npd < primMinDist){
				const unsigned int &i0 = leaves[cLeafIndex].prims[i].i0;
				const unsigned int &i1 = leaves[cLeafIndex].prims[i].i1;
				const unsigned int &i2 = leaves[cLeafIndex].prims[i].i2;
				
				if (i0 >= vertices.size() ||
					i1 >= vertices.size() ||
					i2 >= vertices.size())
					continue;
				
				const Vertex* cV1 = &vertices[i0];
				const Vertex* cV2 = &vertices[i1];
				const Vertex* cV3 = &vertices[i2];
	
				const glm::vec3 tnp = Collision::closestPointOnTriangle(P, cV1->position, cV2->position, cV3->position);
				float tnd = glm::distance(tnp, P);
				if (tnd < minDist){
					closestPrimIndex = i;
					minIndex = cLeafIndex;
					minDist = tnd;
				}
			}
		}
		return;
	}
	//minDist = nd;
	//minDistIndex = cLeafIndex;
	//std::cout << "index: " << cLeafIndex<< std::endl;
	int firstChildInd = leaves[cLeafIndex].firstChildIndex;
	int secondChildInd = leaves[cLeafIndex].secondChildIndex;
	
	float fnd = std::numeric_limits<float>::max();
	float snd = std::numeric_limits<float>::max();
	
	if (firstChildInd >= 0){
		glm::vec3 fnp = Collision::nearestPointOnAABB(P, leaves[firstChildInd].aabb.position, leaves[firstChildInd].aabb.size);
		fnd = glm::distance(fnp, P); // get the distance
	}
	if (secondChildInd >= 0){
		glm::vec3 snp = Collision::nearestPointOnAABB(P, leaves[secondChildInd].aabb.position, leaves[secondChildInd].aabb.size);
		snd = glm::distance(snp, P); // get the distance
	}
	
	if (fnd < snd){
		if (firstChildInd >= 0 && fnd < minDist)nearestPointBlasTraversal(leaves, vertices, minDist, minIndex, closestPrimIndex, leaves[cLeafIndex].firstChildIndex, P);
		if (secondChildInd >= 0 && snd < minDist)nearestPointBlasTraversal(leaves, vertices, minDist,minIndex, closestPrimIndex, leaves[cLeafIndex].secondChildIndex, P);
	}
	else{
		if (secondChildInd >= 0 && snd < minDist)nearestPointBlasTraversal(leaves, vertices, minDist,minIndex, closestPrimIndex, leaves[cLeafIndex].secondChildIndex, P);
		if (firstChildInd >= 0 && fnd < minDist)nearestPointBlasTraversal(leaves, vertices, minDist, minIndex, closestPrimIndex, leaves[cLeafIndex].firstChildIndex, P);
	}
}

void flouraSDF::nearestNeighbourPrims(std::vector<accelerate::BVH_primitive>& prims, int& closestPrimIndex, glm::vec3& P){
	float minDist = std::numeric_limits<float>::max();
        
	for (int i = 0; i < prims.size(); ++i){
		const glm::vec3 np = Collision::nearestPointOnAABB(P, prims[i].extents.position, prims[i].extents.size);
		float nd = glm::distance(np, P);
		if (nd < minDist){
			minDist = nd;
			closestPrimIndex = i;
		}
	}
}

void flouraSDF::nearestNeighbourPrims(std::vector<Vertex>& vertices, std::vector<accelerate::BVH_primitive>& prims,
	int& closestPrimIndex, glm::vec3& P){
	float minDist = std::numeric_limits<float>::max();
        
	for (int i = 0; i < prims.size(); ++i){
		const glm::vec3 np = Collision::nearestPointOnAABB(P, prims[i].extents.position, prims[i].extents.size);
		float nd = glm::distance(np, P);
		if (nd < minDist){
			//minDist = nd;
			
			const unsigned int &i0 = prims[i].i0;
			const unsigned int &i1 = prims[i].i1;
			const unsigned int &i2 = prims[i].i2;
			
			if (i0 >= vertices.size() ||
				i1 >= vertices.size() ||
				i2 >= vertices.size())
				continue;
			
			const Vertex* cV1 = &vertices[i0];
			const Vertex* cV2 = &vertices[i1];
			const Vertex* cV3 = &vertices[i2];
	
			const glm::vec3 tnp = Collision::closestPointOnTriangle(P, cV1->position, cV2->position, cV3->position);
			float tnd = glm::distance(tnp, P);
			if (tnd < minDist){
				closestPrimIndex = i;
				minDist = tnd;
			}
		}
	}
}

glm::vec3 flouraSDF::distanceToClosestPointOnMeshSDFAccel_PlusUV(std::vector<Vertex>& vertices,
                                                                 std::vector<accelerate::leaf>& leaves, glm::vec3& P){
	if (vertices.empty()) return glm::vec3(0.0f);
	
	float minDist = std::numeric_limits<float>::max();
	int closestLeafIndex = -1;
	int closestPrimIndex = -1;
	
	nearestPointBlasTraversal(leaves, vertices, minDist, closestLeafIndex, closestPrimIndex, static_cast<int>(leaves.size()) - 1, P);
	
	//nearestPointBlasTraversal(leaves, minDist, closestLeafIndex, 0 , P);
	
	if (closestLeafIndex < 0){		
		std::cout <<  "closestLeafIndex below zero" << std::endl;
		return glm::vec3(0.0f);
	}
	
	//int closestPrimIndex = -1;
	//closestPrimIndex = closestLeafIndex;
	
	//nearestNeighbourPrims(leaves[closestLeafIndex].prims, closestPrimIndex, P);
	//nearestNeighbourPrims(vertices, leaves[closestLeafIndex].prims, closestPrimIndex, P);
	
	if (closestPrimIndex < 0){
		std::cout <<  "closestPrimIndex below zero" << std::endl;
		return glm::vec3(0.0f);
	}
	
	const unsigned int &i0 = leaves[closestLeafIndex].prims[closestPrimIndex].i0;
	const unsigned int &i1 = leaves[closestLeafIndex].prims[closestPrimIndex].i1;
	const unsigned int &i2 = leaves[closestLeafIndex].prims[closestPrimIndex].i2;
        
	if (i0 >= vertices.size() ||
	i1 >= vertices.size() ||
	i2 >= vertices.size())
		return glm::vec3(0.0f);
    
	minDist = std::numeric_limits<float>::max();
	glm::vec3 cP(0.0f);
	bool anyHit(false);
	
	const Vertex* cV1 = &vertices[i0];
	const Vertex* cV2 = &vertices[i1];
	const Vertex* cV3 = &vertices[i2];
	
	const glm::vec3 np = Collision::closestPointOnTriangle(P, cV1->position, cV2->position, cV3->position);
	float nd = glm::distance(np, P);
	if (nd < minDist){
		anyHit = true;
		minDist = nd;
		cP = np;
	}
	
	if (!anyHit){
		std::cout <<  "anyHit fail" << std::endl;
		return glm::vec3(0.0f);
	}

	// if directions are opossing, then flip ld (calc face normal here to cut down on calcs)
	//if (glm::dot(P - cP, FE_Math::faceNormalFromTriangle(cV1->position, cV2->position, cV3->position, cV1->normal, cV2->normal, cV3->normal)) < 0.0f)
	if (glm::dot(P - cP, FE_Math::faceNormalFromTriangle(cV1->position, cV2->position, cV3->position)) < 0.0f)
		minDist = -minDist;
	
	// calc uv with return instead of every cycle to cut down on calcs
	//return glm::vec3(minDist);
	return glm::vec3(minDist, FE_Math::uvPosFromVertexAndPoint(cV1->position,cV2->position,cV3->position, cV1->texUV, cV2->texUV, cV3->texUV, cP));
}