#include "flouraDeferred.h"
#include <Render/pipeline/prebuilt_pipelines/geometryPass.h>
#include <Render/pipeline/prebuilt_pipelines/historyPass.h>
#include <Render/Shader/renderTarget.h>
#include <Render/Handler/RenderClass.h>
#include <Scene/LightingHandler.h>
#include <Scene/Scene.h>

#include "swrt.h"
#include "Render/Handler/RenderHandler.h"


void FlouraDeferred::init(){
}

void FlouraDeferred::createShaders(){
    DFL_Shader.LoadShader("Assets/Shaders/Deferred/DFR_Phon.vert", "Assets/Shaders/Deferred/DFR_Phon.frag");
    SSR_Shader.LoadShader("Assets/Shaders/Deferred/screenspaceReflections.vert", "Assets/Shaders/Deferred/screenspaceReflections.frag");
}

void FlouraDeferred::draw(){

}

void FlouraDeferred::Delete()
{
	DFL_Shader.Delete();
	SSR_Shader.Delete();
}

Shader FlouraDeferred::DFL_Shader;
Shader FlouraDeferred::SSR_Shader;

void FlouraDeferred::DeferredLightingPass(){
	glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::FBO);
	DFL_Shader.Activate();
	/*
	FlouraSWRT::indirectProbePass();
	DFL_Shader.setHandleui64ARB("indirectVolume", FlouraSWRT::testChunk.indirectVolume->handle);
	DFL_Shader.setHandleui64ARB("emissionVolume", FlouraSWRT::testChunk.emissionVolume->handle);
	DFL_Shader.setFloat3("ivPosition", FlouraSWRT::testChunk.position);
	DFL_Shader.setFloat("ivScale", FlouraSWRT::testChunk.scale);
	*/
	
	DFL_Shader.setTexture2D("gPosition", 1, GeometryPass::gPosition);
	DFL_Shader.setTexture2D("gNormal", 2, GeometryPass::gNormal);
	DFL_Shader.setTexture2D("gAlbedoSpec", 3, GeometryPass::gAlbedo);
	DFL_Shader.setTexture2D("depthMap", 5, GeometryPass::depthTexture);
	DFL_Shader.setTexture2D("gSpecular", 6, GeometryPass::gSpecular);
	DFL_Shader.setTexture2D("gVelocity", 7, GeometryPass::gVelocity);
	// skip 8 because of shadow map
	DFL_Shader.setTexture2D("gEmission", 10, GeometryPass::gEmission);
	

	glActiveTexture(GL_TEXTURE11);
	glBindTexture(GL_TEXTURE_2D, HistoryPass::hColour);
	glGenerateMipmap(GL_TEXTURE_2D); // remove later
	DFL_Shader.setInt("hColour", 11);
	DFL_Shader.setTexture2D("hDepthTexture", 12, HistoryPass::hDepthTexture);
	
	DFL_Shader.setFloat("NearPlane", Scene::maincamera.nearFar.x);
	DFL_Shader.setFloat("FarPlane", Scene::maincamera.nearFar.y);

	DFL_Shader.setFloat("fogDepthDistance", RenderClass::DepthDistance);
	DFL_Shader.setFloat("fogNearPlane", RenderClass::DepthPlane[0]);
	DFL_Shader.setFloat("fogFarPlane", RenderClass::DepthPlane[1]);
	DFL_Shader.setBool("doFog", RenderClass::doFog);

	DFL_Shader.setFloat3("fogColor", RenderClass::fogRGBA);
	//GBLpass.Activate();
	//mat4
	DFL_Shader.setMat4("cameraMatrix", Scene::maincamera.cameraMatrix);
	DFL_Shader.setMat4("projectionMatrix", Scene::maincamera.projection);
	DFL_Shader.setMat4("viewMatrix", Scene::maincamera.view);
	DFL_Shader.setMat4("inverseViewMatrix",glm::inverse(Scene::maincamera.view));
	DFL_Shader.setMat4("inverseProjection", glm::inverse(Scene::maincamera.projection));

	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(Scene::maincamera.cameraMatrix)));
	DFL_Shader.setMat3("normalMatrix", normalMatrix);

	DFL_Shader.setFloat3("orientation", Scene::maincamera.Orientation);
	DFL_Shader.setFloat3("cameraPos", Scene::maincamera.Position);
	DFL_Shader.setFloat3("cameraDirection", Scene::maincamera.Orientation);
	DFL_Shader.setFloat3("camPos", Scene::maincamera.Position);
	DFL_Shader.setInt("indirectSamples", ProbeHandler::indirectSamples);

	
	DFL_Shader.setHandleui64ARB("BlueNoiseHandle", RenderClass::bluenoise->handle);
	DFL_Shader.setHandleui64ARB("bayerMatrixHandle", RenderClass::bayermatrix->handle);
	DFL_Shader.setHandleui64ARB("caustic", RenderClass::caustic->handle);
	
	//DFL_Shader.setBool("doSSR", RenderClass::doSSR);
	DFL_Shader.setBool("doContactShadows", RenderClass::doContactShadows);
	
	DFL_Shader.setTimeVariables();
	
	LightingHandler::sendToShader(DFL_Shader);
	
	//shader.
	RenderQuad::draw();
}

void FlouraDeferred::ssrPass(){
	glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::FBO);
	SSR_Shader.Activate();
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderTarget::screentexture);
	glGenerateMipmap(GL_TEXTURE_2D);
	SSR_Shader.setInt("screentexture", 0);
	SSR_Shader.setTexture2D("gPosition", 1, GeometryPass::gPosition);
	SSR_Shader.setTexture2D("gNormal", 2, GeometryPass::gNormal);
	SSR_Shader.setTexture2D("gAlbedoSpec", 3, GeometryPass::gAlbedo);
	SSR_Shader.setTexture2D("depthMap", 5, GeometryPass::depthTexture);
	SSR_Shader.setTexture2D("gSpecular", 6, GeometryPass::gSpecular);
	SSR_Shader.setTexture2D("gVelocity", 7, GeometryPass::gVelocity);
	// skip 8 because of shadow map
	SSR_Shader.setTexture2D("gEmission", 10, GeometryPass::gEmission);
	
	glActiveTexture(GL_TEXTURE11);
	glBindTexture(GL_TEXTURE_2D, HistoryPass::hColour);
	glGenerateMipmap(GL_TEXTURE_2D); // remove later
	SSR_Shader.setInt("hColour", 11);
	
	SSR_Shader.setTexture2D("hDepthTexture", 12, HistoryPass::hDepthTexture);
	SSR_Shader.setFloat("NearPlane", Scene::maincamera.nearFar.x);
	SSR_Shader.setFloat("FarPlane", Scene::maincamera.nearFar.y);
	
	//ssPass.Activate();
	//mat4
	SSR_Shader.setMat4("cameraMatrix", Scene::maincamera.cameraMatrix);
	SSR_Shader.setMat4("projectionMatrix", Scene::maincamera.projection);
	SSR_Shader.setMat4("viewMatrix", Scene::maincamera.view);
	SSR_Shader.setMat4("inverseViewMatrix",glm::inverse(Scene::maincamera.view));
	SSR_Shader.setMat4("inverseProjection", glm::inverse(Scene::maincamera.projection));

	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(Scene::maincamera.cameraMatrix)));
	SSR_Shader.setMat3("normalMatrix", normalMatrix);

	SSR_Shader.setFloat3("orientation", Scene::maincamera.Orientation);
	SSR_Shader.setFloat3("cameraPos", Scene::maincamera.Position);
	SSR_Shader.setFloat3("cameraDirection", Scene::maincamera.Orientation);
	SSR_Shader.setFloat3("camPos", Scene::maincamera.Position);
	SSR_Shader.setInt("indirectSamples", ProbeHandler::indirectSamples);
	
	SSR_Shader.setHandleui64ARB("BlueNoiseHandle", RenderClass::bluenoise->handle);
	SSR_Shader.setHandleui64ARB("bayerMatrixHandle", RenderClass::bayermatrix->handle);

	SSR_Shader.setBool("doSSR", RenderClass::doSSR);
	SSR_Shader.setBool("doContactShadows", RenderClass::doContactShadows);
	SSR_Shader.setBool("doReflections", RenderClass::doReflections);
	
	SSR_Shader.setTimeVariables();
	//RenderHandler::tempCM->cubemapToUUIDShader("cmMainHandle", RenderClass::ssPass);
	
	Skybox::SkyboxCubemap->cubemapToUUIDShader("SB_Handle", SSR_Shader);
	if (RenderHandler::renderENV)  RenderHandler::tempCM->cubemapToUUIDShader("cmMainHandle", SSR_Shader);
	else Skybox::SkyboxCubemap->cubemapToUUIDShader("cmMainHandle", SSR_Shader);
	
	//shader.
	RenderQuad::draw();
}
