#include "Texture.h"
#include <utils/logConsole.h>
#include "utils/FE_math.h"
#include <utils/imageResize.h>

//#define STB_IMAGE_RESIZE_IMPLEMENTATION
//#include "stb/stb_image_resize.h"

void Texture::createFromCapture(){
    if (!suppressCreation) return;
    
    suppressCreation = false;
    
    if (!nIsTexture)
    {
        createColour(sColour, type.c_str(), unit);
        return;
    }
    createTexture(path.c_str(), type.c_str(), unit);
}

void Texture::createColour(glm::vec4 colour, const char* texType, GLuint slot){
    path = "null";
    //LogConsole::print("Texture loading started");
    // Assigns the type of the texture to the texture object
    type = texType;
    
    unit = slot;
    //std::cout << type << " U " << slot << std::endl;

    if (suppressCreation){
        sColour = colour;
        return;
    }
    
    
    // Flips the image so it appears right side up
    //stbi_set_flip_vertically_on_load(true);
    // Reads the image from a file and stores it in bytes
    unsigned char* bytes = 0;

    unsigned char r = FE_Math::floatToByteRGB(colour.x);
    unsigned char g = FE_Math::floatToByteRGB(colour.y);
    unsigned char b = FE_Math::floatToByteRGB(colour.z);
    unsigned char a = FE_Math::floatToByteRGB(colour.w);
    
    unsigned char fallbackPixel[] = { r , g, b , a};
    bytes = fallbackPixel;

    glGenTextures(1, &ID);
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, ID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        1,
        1,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        bytes
    );

    if (GLAD_GL_ARB_bindless_texture) {
        handle = glGetTextureHandleARB(ID);
        glMakeTextureHandleResidentARB(handle);
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    //LogConsole::print("Texture loading finished");
    created = true;
}

void Texture::createTexture(const char* image, const char* texType, GLuint slot){
    path = image;
    //LogConsole::print("Texture loading started");
    // Assigns the type of the texture to the texture object
    type = texType;
    //std::cout << type << " U " << slot << std::endl;
    unit = slot;
    nIsTexture = true;
    if (suppressCreation) return;
    
    // Stores the width, height, and the number of color channels of the image
    int widthImg, heightImg, numColCh;
    // Flips the image so it appears right side up
    //stbi_set_flip_vertically_on_load(true);
    // Reads the image from a file and stores it in bytes
    stbi_set_flip_vertically_on_load(flipVertical);
    //unsigned char* bytes;
    unsigned char* bytes = stbi_load(image, &widthImg, &heightImg, &numColCh, 0);
    //std::cout << "\n" << image << " < image \n" << widthImg << " < widthImg \n" << heightImg << " < heightImg \n" << numColCh << " < numColCh \n" << std::endl;

    if (!bytes) {
        std::cerr << "\nFailed to load texture: " << image << "\n" << std::endl;
        static unsigned char fallbackPixel[] = {
            255, 0, 255, 0, 0, 0, 255, 0, 255, 0, 0, 0,            // row 1
            0, 0, 0, 255, 0, 255, 0, 0, 0, 255, 0, 255,            // row 2
            255, 0, 255, 0, 0, 0, 255, 0, 255, 0, 0, 0,            // row 3
            0, 0, 0, 255, 0, 255, 0, 0, 0, 255, 0, 255,            // row 4
        };
        widthImg = 4; heightImg = 4; numColCh = 3;
        bytes = fallbackPixel; 
        skipstbi = true;
    }
    
    
    // investigate cleaning up and adding proper error handling to this + freeing resources
    /*
    int nW = 128; int nH = 128;
    unsigned char* obytes = (unsigned char*)malloc(nW * nH * numColCh);
    FlouraImageResize::resize2D(bytes, widthImg, heightImg, numColCh, obytes, nW, nH);
    bytes = obytes; widthImg = nW; heightImg = nH;
    */
    GLenum format = GL_RGB;
    if (numColCh == 4)format = GL_RGBA;
    //else if (numColCh == 3) format = GL_RGB;
    else if (numColCh == 1) format = GL_RED;
    
    glGenTextures(1, &ID);
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, ID);
    
    if (linearFilter && !skipstbi){
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else{
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    glTexImage2D(
    GL_TEXTURE_2D,
    0,
    format,
    widthImg,
    heightImg,
    0,
    format,
    GL_UNSIGNED_BYTE,
    bytes
    );
    glGenerateMipmap(GL_TEXTURE_2D);

    if (!skipstbi) stbi_image_free(bytes); // Deletes the image data as it is already in the OpenGL Texture object

    if (GLAD_GL_ARB_bindless_texture) {
        handle = glGetTextureHandleARB(ID);
		glMakeTextureHandleResidentARB(handle);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    //LogConsole::print("Texture loading finished");
    created = true;
}

void Texture::reload(GLuint slot){
    // Assigns the texture to a Texture Unit
    glActiveTexture(GL_TEXTURE0 + slot);
    unit = slot;
    glBindTexture(GL_TEXTURE_2D, ID);
    
    if (linearFilter){
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else{
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    glGenerateMipmap(GL_TEXTURE_2D);

    // remake the handle
	if (GLAD_GL_ARB_bindless_texture) {
        handle = glGetTextureHandleARB(ID);
        glMakeTextureHandleResidentARB(handle);
    }
    // Unbinds the OpenGL Texture object so that it can't accidentally be modified
	glBindTexture(GL_TEXTURE_2D, 0);
    created = true;
}


void Texture::texUnit(Shader& shader, const char* uniform, GLuint unit)
{
    // Gets the location of the uniform
    GLuint texUni = glGetUniformLocation(shader.ID, uniform);
    if (texUni == -1)
    {
        // comes from mesh::Draw
       //std::cerr << "Texture.cpp Failed to find uniform: " << uniform << std::endl;
    }
    // Shader needs to be activated before changing the value of a uniform
    
    shader.Activate();
    // Sets the value of the uniform
	//std::cout << "Texture unit: " << unit << std::endl;
    glUniform1i(texUni, unit);
}

void Texture::handleToShader(Shader& shader, const char* uniform)
{
    shader.Activate();
	shader.setHandleui64ARB(uniform, handle);
}

void Texture::Bind(){
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, ID);
}

void Texture::Unbind(){
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Delete()
{
	// delete the handle first
    if (GLAD_GL_ARB_bindless_texture)
        glMakeTextureHandleNonResidentARB(handle);
    glDeleteTextures(1, &ID);
    //std::cout << "Texture deleted: " << ID << std::endl;
}

glm::vec4 Texture::rgbaSample(glm::vec2 uv, bool flip){
    glBindTexture(GL_TEXTURE_2D, ID);
                
    int width; int height;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
                
    if (width <= 0 || height <= 0) return glm::vec4(0.0f);
                
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
                
    std::vector<unsigned char> pixels(width * height * 4);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
                
    glm::vec2 repeatingUV = uv - glm::floor(uv);
                
    int pixelX = static_cast<int>(repeatingUV.x * static_cast<float>(width - 1));
    int pixelY = 0.0f;
    if (flip){ pixelY = static_cast<int>( (1.0f - repeatingUV.y) * static_cast<float>(height - 1));}// flip
    else pixelY = static_cast<int>( repeatingUV.y * static_cast<float>(height - 1));
                
    pixelX = glm::clamp(pixelX, 0, width - 1);
    pixelY = glm::clamp(pixelY, 0, height - 1);
                
    int ind = (pixelY * width + pixelX) * 4;
    
    return glm::vec4(
     pixels[ind + 0] / 255.0f,
     pixels[ind + 1] / 255.0f,
     pixels[ind + 2] / 255.0f,
     pixels[ind + 3] / 255.0f
     );   
}