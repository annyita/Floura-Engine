#ifndef TEXTURE_CLASS_H
#define TEXTURE_CLASS_H

#include <glad/gl.h>
#include<stb/stb_image.h>

#include "Render/Shader/shaderClass.h"

class Texture
{
public:
	GLuint ID; // texture ID
	GLuint64 handle; /// for bindless textures
	std::string type;
	GLuint unit;
	std::string path;
	bool flipVertical = false;
	bool linearFilter = false;
	
	bool suppressCreation = false;
	bool created = false;
	
	// ehh
	glm::vec4 sColour;
	bool nIsTexture = false;
	
	glm::vec4 colour = glm::vec4(0.0);
	bool isFactor = false;
	float powerFactor = 0.0f;
	
	void createFromCapture();
	
	void createColour(glm::vec4 colour, const char* texType, GLuint slot);
	void createTexture(const char* image, const char* texType, GLuint slot);

	void reload(GLuint slot);

	// Assigns a texture unit to a texture
	void texUnit(Shader& shader, const char* uniform, GLuint unit);

	// send handle to shader
	void handleToShader(Shader& shader, const char* uniform);

	// Binds a texture
	void Bind();
	// Unbinds a texture
	void Unbind();
	// Deletes a texture
	void Delete();
	
	glm::vec4 rgbaSample(glm::vec2 uv, bool flip);

	bool skipstbi = false;
private:

};
#endif

//texture header is not the problem