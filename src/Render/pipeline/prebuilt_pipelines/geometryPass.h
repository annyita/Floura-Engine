#ifndef GEOMETRYPASS_CLASS_H
#define GEOMETRYPASS_CLASS_H

#include<iostream>
#include <Render/Object/ModelAssimp.h>
#include <camera/Camera.h>
class GeometryPass
{
public:
	static unsigned int depthTexture, gBuffer, gAlbedo, gNormal, gSpecular, gPosition, DBO, gNoise, gVelocity, gEmission;
	
	static void updateGbufferResolution(unsigned int width, unsigned int height);

	static void setupGbuffers(unsigned int width, unsigned int height);
	
	static void cleanupGbuffers();

	static void gPassDraw(Model*& model, Shader& GPass, Camera camera);
};

#endif // GEOMETRYPASS_CLASS_H
