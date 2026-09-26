#ifndef RENDER_CLASS_H
#define RENDER_CLASS_H

#include <iostream>
#include <camera/Camera.h>
#include <Editor/UI/ImGui/ImGuiWindow.h>
#include <Render/Object/Skybox.h>
//#include <Render/Cube/Billboard.h>
#include <Render/Object/line.h>
#include "Render/Object/Texture.h"
#include "Render/Object/texture3D.h"
#include <glad/gl.h>

class RenderClass
{
public:

	// billboard
	static Shader taaShader;
	static Shader skyGadientShader;
	static Shader billBoardShader;
	static Shader gPassShaderBillBoard;
	static Shader LineShader;
	static bool renderSkybox;
	static bool doReflections;
	static bool doSSR;
	static bool doContactShadows;
	static bool doFog;
	static GLfloat DepthDistance;
	static GLfloat DepthPlane[];
	static glm::vec3 skyRGBA;
	static glm::vec3 fogRGBA;
	static Line3D* line;
	static Texture* bluenoise;
	static Texture* bayermatrix;
	static Texture* caustic;
	static Texture* ripples;
	static Texture* droplets;
	static Texture* puddles;
	static Texture3D* LUT;
	static bool doTAA;
	static float jitterFactor;
	static bool doBinaryAlpha;
	static bool animateBinaryAlpha;
	
	struct transformation {
		glm::vec3 position = glm::vec3(0.0f);
		glm::vec3 rotation = glm::vec3(0.0f);
		glm::quat qRotation = glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 scale = glm::vec3(1.0f);
	};
	
	enum renderersEnum{
		NONE = 0,
		DEFERRED = 1,
		FORWARD = 2,
		SWRT = 3
	};
	
	static renderersEnum currentRenderer;
	static int currentRendererInd;

	static void init(unsigned int width, unsigned int height);
	static void initGlobalShaders();
	static void Cleanup();
	
	static void ClearFramebuffers();
	static void Render(GLFWwindow* window, unsigned int width, unsigned int height);
	static void taaPass();
	static void skyGraidentPass();
	static void compileShaderUniforms();

};
#endif