

#ifndef COLLISION_H
#define COLLISION_H

#include <glm/ext/vector_float3.hpp>
#include <vector>
#include <Render/Buffer/VBO.h>
//#include <Systems/Physics/BVH.h>
//#include <Systems/Physics/voxelizer.h>
/*
	static class for handling collision detection between different collider types
	returns true if a collision is detected, false otherwise
	also returns collision data such as collision point, normal, etc.
	which will be returned in a form of a struct? 
*/

class Collision
{
public:
	
	
	enum collisionTypes{
		typeUndefined,
		typeAABB,
		typeSphere,
		typePlane, // todo
		typeMesh,
		typeOOBB, // todo
		typeFrustum // todo
	};

	struct minmax{
		glm::vec3 min;
		glm::vec3 max;
	};

	struct HitResult {
		bool isColliding = false;
		bool isSolved = false; // for solver
		float distance = 0.0f;
		float depth = 0.0f;
		glm::vec3 lastHit = glm::vec3(0.0f);
		glm::vec3 collisionNormal = glm::vec3(0.0f);
	}; 

	struct AABB{
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 size = glm::vec3(1.0f);
	};

	struct Sphere{
		glm::vec3 position = glm::vec3(0.0f);
		float radius = 1.0f;
	};

	struct collisionObject{
		bool isCollider = true;
		AABB aabb;
		Sphere sphere;
		collisionTypes type = typeAABB; // undefined by default
		HitResult hitResult; // for solving
	};

	struct tri{
		glm::vec3 a = glm::vec3(0.0f);
		glm::vec3 b = glm::vec3(0.0f);
		glm::vec3 c = glm::vec3(0.0f);
	};
	
	
	// thanks LOGL
	struct plane{
		glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
		float distance = 1.0f;
	};
	
	struct Frustum{
		plane topFace;
		plane bottomFace;

		plane rightFace;
		plane leftFace;

		plane farFace;
		plane nearFace;
	};


	static bool showBoxCollider;

	// AABB

	struct rubiksCubePoints{
		// based off rubix cube notation
		glm::vec3 ULF = glm::vec3(0.0f); // up left front
		glm::vec3 URF = glm::vec3(0.0f); // up right front
		glm::vec3 URB = glm::vec3(0.0f); // up right back
		glm::vec3 ULB = glm::vec3(0.0f); // up left back
		glm::vec3 DLF = glm::vec3(0.0f); // down left front
		glm::vec3 DRF = glm::vec3(0.0f); // down right front
		glm::vec3 DRB = glm::vec3(0.0f); // down right back
		glm::vec3 DLB = glm::vec3(0.0f); // down left back
	};

	static rubiksCubePoints fetchFurthestPoints(const std::vector<glm::vec3>& points);
	static rubiksCubePoints fetchFurthestVertices(const std::vector<Vertex>& vertices);
	static rubiksCubePoints aabbToRubixCubePoints(const glm::vec3 p, glm::vec3 s);

	static AABB createAABBfromRubiksCubePoints(const rubiksCubePoints& points);
	static AABB createAABBfromVertices(const std::vector<Vertex>& vertices);
	static AABB createAABBfromPoints(const std::vector<glm::vec3>& points);

	static rubiksCubePoints transformRubiks(const rubiksCubePoints& points, glm::mat4 matrix);

	static Collision::AABB rootNodeFromRubixPoints(Collision::rubiksCubePoints points,
		glm::mat4 ModelMatrix); // returns rootnode
	
	// two of the same thing i know bad and spooky, im tired
	static Collision::AABB rootNodeFromRubixPointsNoPadding(Collision::rubiksCubePoints points,
	glm::mat4 ModelMatrix);
	
	struct KDsplit{
		AABB firstSplit;
		AABB secondSplit;
	};
	
	static KDsplit KDsplitVolume(glm::vec3 p, glm::vec3 extents);
	
	struct octSplit{
		glm::vec3 size;
		
		glm::vec3 splitTRF;
		glm::vec3 splitTRB;
		glm::vec3 splitTLF;
		glm::vec3 splitTLB;
		
		glm::vec3 splitDRF;
		glm::vec3 splitDRB;
		glm::vec3 splitDLF;
		glm::vec3 splitDLB;
	};
	
	static octSplit octSplitVolume(glm::vec3 p, glm::vec3 extents);
	
	// takes AABB return MinMax
	static Collision::minmax returnMinMax(glm::vec3 p, glm::vec3 s);
	
	static glm::vec3 constrainPoint(glm::vec3 vp, glm::vec3 c2, float cRadius);
	
	static HitResult advancedConstrainPoint(glm::vec3 vp, glm::vec3 c2, float cRadius);

	static glm::vec3 nearestPointOnAABB(const glm::vec3& p, const glm::vec3& pos, const glm::vec3& extents);
	
	// AABB collision detection
	// AABB vs AABB
	static HitResult AABBvsAABB(
		const glm::vec3& posA, const glm::vec3& sizeA,
		const glm::vec3& posB, const glm::vec3& SizeB);

	// AABB vs Point
	static HitResult AABBvsPoint(
		const glm::vec3& posA, const glm::vec3& sizeA,
		const glm::vec3& point);

	// AABB vs Ray
	static HitResult AABBvsRay(const glm::vec3& posA, const glm::vec3& sizeA,
		glm::vec3 rayOrigin, glm::vec3 rayDir);
	
	static HitResult AABBvsSphere(const glm::vec3 posB, const glm::vec3 scaleB, const glm::vec3 pointS, const float radius);

	// triangle
	
	static glm::vec3 closestPointOnTriangle(const glm::vec3& P, const glm::vec3& A, const glm::vec3& B, const glm::vec3& C);
	
	struct voxelAccel{
		Collision::AABB voxel;
		std::vector<GLuint> indices;
	};
	
	static bool TrianglevsPoint(
		const glm::vec3& P, const glm::vec3& A, const glm::vec3& B, const glm::vec3& C, float epsilon);

	static bool RayVsTriangleSimple(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 A, glm::vec3 B, glm::vec3 C);

	static HitResult RayVsTriangle(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 A, glm::vec3 B, glm::vec3 C);

	
	// label as SAT
	static HitResult SATTrianglevsTriangle(
		const glm::vec3& v0A, const glm::vec3& v1A, const glm::vec3& v2A,
		const glm::vec3& v0B, const glm::vec3& v1B, const glm::vec3& v2B);

	static HitResult SATTriangleVSAABB(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
																		const glm::vec3& AABBpos, const glm::vec3& AABBsize);
	
	static HitResult NearestPointTriangleVSAABB(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
																	const glm::vec3& AABBpos, const glm::vec3& AABBsize);

private:

	static void projectAABB(const glm::vec3& positon, const glm::vec3& scale,
		const glm::vec3& normal, float& min, float& max);
	
	static void projectVertex(
		const glm::vec3& A, const glm::vec3& B, const glm::vec3& C,
		const glm::vec3& normal,
		float& min, float& max);

	static bool doesVertexOverlap(const glm::vec3& v0A, const glm::vec3& v1A, const glm::vec3& v2A,
		const glm::vec3& v0B, const glm::vec3& v1B, const glm::vec3& v2B, const glm::vec3& normal);

public:

	// sphere

	// sphere vs sphere
	static HitResult SpherevsSphere( // pent depth would be nice for solvin
		const glm::vec3& P1, const glm::vec3& P2,
		float R1, float R2); // two points and radius

	static bool SphereVsPoint(glm::vec3 spherePoint, float radius, glm::vec3 point);

	//
	static bool isBoxInFrustum(const glm::vec3& worldPos, const glm::vec3& Scale, const glm::mat4& view, const glm::mat4& projection);
	
	static Sphere AABBtoSphere(const glm::vec3& posA, const glm::vec3& sizeA);
	
	static bool AABBtoSphereRangeCull(const glm::vec3& posA, const glm::vec3& sizeA, glm::vec3 point, float distance);

	static HitResult resolveCollision(collisionObject& A, collisionObject& B);
	
	static HitResult resolveCollisionW_NewPositons(collisionObject& A, collisionObject& B, const glm::vec3& posA, const glm::vec3& posB);
	
	static bool meshAABBCheck(std::vector<Vertex> &vertices, std::vector<GLuint> &indices, AABB aabb);

private:

};

#endif