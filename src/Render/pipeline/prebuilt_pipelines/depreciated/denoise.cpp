#include "denoise.h"
#include <Render/Shader/renderTarget.h>
#include "Render/Object/RenderQuad.h"
#include <Render/pipeline/prebuilt_pipelines/geometryPass.h>
#include <utils/FE_math.h>
#include <Render/pipeline/prebuilt_pipelines/depreciated/raytracer.h>
GLuint denoiser::denoiseTexture;
bool denoiser::doDenoise = true;
int denoiser::minRadius = 2;
Shader denoiser::denoiserQuadShader;
Shader denoiser::denoiseCompute;


unsigned int denoiser::CurrentWidth;
unsigned int denoiser::CurrentHeight;

void denoiser::init() {
}

void denoiser::initcomputeShader(unsigned int width, unsigned int height) {
	denoiserQuadShader.LoadShader("Assets/Shaders/Db/RenderQuad.vert", "Assets/Shaders/compute/DenoiseRenderQuad.frag");

	glCreateTextures(GL_TEXTURE_2D, 1, &denoiseTexture); // 6
	glTextureParameteri(denoiseTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(denoiseTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(denoiseTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(denoiseTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(denoiseTexture, 1, GL_RGBA32F, width, height);
	glBindImageTexture(1, denoiseTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	CurrentWidth = width;
	CurrentHeight = height;

	denoiseCompute.LoadComputeShader("Assets/Shaders/compute/denoise.comp");

	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		std::cerr << "OpenGL error: " << err << std::endl;
	}

}

bool isresized = false;

void denoiser::resizeTexture(unsigned int width, unsigned int height) {
    glDeleteTextures(1, &denoiseTexture); // Delete old texture

	glCreateTextures(GL_TEXTURE_2D, 1, &denoiseTexture);
	glTextureParameteri(denoiseTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(denoiseTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(denoiseTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(denoiseTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(denoiseTexture, 1, GL_RGBA32F, width, height);
	glBindImageTexture(1, denoiseTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	CurrentWidth = width;
	CurrentHeight = height;

	isresized = true;
}

void denoiser::RenderToQuad() {
	glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::FBO);
	glDisable(GL_CULL_FACE);

	denoiserQuadShader.Activate();

	//glBindImageTexture(0, raytracer::raytracedOutput, 0, GL_FALSE, 7, GL_WRITE_ONLY, GL_RGBA32F);
	glBindTextureUnit(0, denoiser::denoiseTexture);

	RenderQuad::draw();

	glEnable(GL_CULL_FACE);
	glBindFramebuffer(GL_FRAMEBUFFER, renderTarget::FBO);
}

void denoiser::render() {

	denoiseCompute.Activate();
	denoiseCompute.setBool("doDenoise", doDenoise);
	denoiseCompute.setInt("minRadius", minRadius);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, GeometryPass::gNormal);
	denoiseCompute.setInt("gNormal", 2);

	// albedo spec
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, GeometryPass::gAlbedo);
	denoiseCompute.setInt("gAlbedoSpec", 3);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, GeometryPass::depthTexture);
	denoiseCompute.setInt("depthMap", 5);

	denoiseCompute.ActivateCompute((CurrentWidth + 7) / 8, (CurrentHeight + 3) / 4, 1);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	RenderToQuad();

	glDisable(GL_BLEND);

}