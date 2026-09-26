#include "Cubemap.h"
#include <utils/logConsole.h>
#include <Render/Shader/renderTarget.h>

Cubemap::Cubemap() {
}

void Cubemap::loadCubeMap(std::string path){
	//glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	Cubemap::path = path;
	// loading

	std::ifstream SkyboxJson(path);
	if (SkyboxJson.is_open()) {
		json SkyboxJsonData;
		SkyboxJson >> SkyboxJsonData;
		SkyboxJson.close();

		std::string nPath = SkyboxJsonData[0]["Path"].get<std::string>() + "/";

		//LogConsole::print("Skybox Path: " + nPath);

		for (int i = 0; i < 6; i++)
		{
			facesCubemap[i] = nPath + SkyboxJsonData[0]["Faces"][i].get<std::string>();
			//LogConsole::print("Skybox Face: " + facesCubemap[i]);
		}
	}
	else {
		std::cerr << "Failed to open Skybox.json" << std::endl;
	}

	// creation
	glDeleteTextures(1, &ID);

	// Creates the cubemap texture object
	glGenTextures(1, &ID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, ID);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	// These are very important to prevent seams
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_SEAMLESS, GL_TRUE);
	//res
	 
	int width, height, numColCh;

	// Cycles through all the textures and attaches them to the cubemap object
	for (unsigned int i = 0; i < 6; i++)
	{
		unsigned char* data = stbi_load(facesCubemap[i].c_str(), &width, &height, &numColCh, 0);
		if (data)
		{
			stbi_set_flip_vertically_on_load(false);
			
			format = GL_RGB;
			if (numColCh == 4)format = GL_RGBA;
			//else if (numColCh == 3) format = GL_RGB;
			else if (numColCh == 1) format = GL_RED;
			
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0,
				format,
				width,
				height,
				0,
				format,
				GL_UNSIGNED_BYTE,
				data
			);

			stbi_image_free(data);
		}
		else{
			LogConsole::print("Failed to load texture: " + facesCubemap[i]);
			stbi_image_free(data);
		}


	}

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	if (GLAD_GL_ARB_bindless_texture) {
		handle = glGetTextureHandleARB(ID);
		glMakeTextureHandleResidentARB(handle);
	}

	res = glm::vec2(width, height);


}

void Cubemap::bind(int unit)
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_CUBE_MAP, ID);
}

void Cubemap::unbind()
{
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void Cubemap::cubemapToShader(Shader& shader, int unit)
{
	shader.Activate();
	shader.setInt("skybox", unit);
}

void Cubemap::cubemapToUUIDShader(const char* uniform, Shader & shader){
	shader.Activate();
	shader.setHandleui64ARB(uniform, handle); // guess im keeping this weird capital underscore naming convension
}

void Cubemap::resizeCubeMap(glm::vec2 resolution)
{
	// skip if new res is same as old
	if (res == resolution) return;


	if (GLAD_GL_ARB_bindless_texture && handle != 0) {
		glMakeTextureHandleNonResidentARB(handle);
	}

	if (ID == 0 || true) glGenTextures(1, &ID);

	glBindTexture(GL_TEXTURE_CUBE_MAP, ID);

	for (unsigned int i = 0; i < 6; i++)
	{
		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0,
			format, // different internal formats will cause issues with this but ehhh
			(int)resolution.x,
			(int)resolution.y,
			0,
			format,
			GL_UNSIGNED_BYTE,
			NULL
		);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_SEAMLESS, GL_TRUE);

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	if (GLAD_GL_ARB_bindless_texture) {
		handle = glGetTextureHandleARB(ID);
		glMakeTextureHandleResidentARB(handle);
	}

	res = resolution;
}


Cubemap::~Cubemap()
{
	// delete the handle first
	if (GLAD_GL_ARB_bindless_texture) {
		glMakeTextureHandleNonResidentARB(handle);
	}
	glDeleteTextures(1, &ID);	
}