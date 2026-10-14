#include "texture3D.h"
#include <sstream>
#include <vector>
#include<stb/stb_image.h>

void Texture3D::createTexture3D(const char* path, const char* texType, GLuint slot){
    Texture3D::path = std::string(path);
    type = std::string(texType);
    Texture3D::slot = slot;
    
    atlasSize = sizeFromPath(path);
    //std::cout << atlasSize <<std::endl;
    if (atlasSize <= 0){
        created = false;
        return;
    }
    
    
    int widthImg, heightImg, numColCh;
    stbi_set_flip_vertically_on_load(false);
    
    // load image
    unsigned char* bytes = stbi_load(path, &widthImg, &heightImg, &numColCh, 0);
    
    if (!bytes) {
        if (enablePrint) std::cerr << "Failed to load 3D texture: " << path << "\n" << std::endl;
        static unsigned char fallbackPixel[] = {
            255, 0, 255, 0, 0, 0, 255, 0, 255, 0, 0, 0,            // row 1
            0, 0, 0, 255, 0, 255, 0, 0, 0, 255, 0, 255,            // row 2
            255, 0, 255, 0, 0, 0, 255, 0, 255, 0, 0, 0,            // row 3
            0, 0, 0, 255, 0, 255, 0, 0, 0, 255, 0, 255,            // row 4
        };
        widthImg = 4; heightImg = 4; numColCh = 3;
        bytes = fallbackPixel; 
        atlasSize = 1; // set to 1 for fallback
        skipstbi = true;
        failed = true;
    }
    
    GLsizei texWidth = atlasSize;
    GLsizei texHeight = atlasSize;
    
    int slicesX = widthImg / atlasSize;
    int slicesY = heightImg / atlasSize;
    GLsizei texDepth = slicesX * slicesY;
    
    GLenum format = GL_RGB;
    if (numColCh == 4)format = GL_RGBA;
    else if (numColCh == 1) format = GL_RED;
    
        std::vector<unsigned char> unpackedBytes(texWidth * texHeight * texDepth * numColCh);
        unsigned char* dst = unpackedBytes.data();

        for (int slice = 0; slice < texDepth; ++slice){
            int gridX = slice % slicesX;
            int gridY = slice / slicesX;

            for (int row = 0; row < texHeight; ++row){
                int srcX = gridX * texWidth;
                int srcY = (gridY * texHeight) + row;
                int srcIndex = (srcY * widthImg + srcX) * numColCh;
                
                std::memcpy(dst, &bytes[srcIndex], texWidth * numColCh);
                dst += texWidth * numColCh;
            }
        }
    
    width = texWidth;
    height = texHeight;
    depth = texDepth;
    
    glGenTextures(1, &ID);
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_3D, ID);
    
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, filtering); // GL_NEAREST_MIPMAP_LINEAR
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, filtering);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    glTexImage3D(
    GL_TEXTURE_3D,
    0,
    format,
    texWidth,
    texHeight,
    texDepth,
    0,
    format,
    GL_UNSIGNED_BYTE,
    unpackedBytes.data()
    );
    glGenerateMipmap(GL_TEXTURE_3D);
    
    if (!skipstbi) stbi_image_free(bytes); // Deletes the image data as it is already in the OpenGL Texture object

    if (GLAD_GL_ARB_bindless_texture) {
        handle = glGetTextureHandleARB(ID);
        glMakeTextureHandleResidentARB(handle);
        handleImage = glGetImageHandleARB(ID, 0, GL_FALSE, 0, format);
        glMakeImageHandleResidentARB(handleImage, GL_READ_WRITE);
    }
    glBindTexture(GL_TEXTURE_3D, 0);
    
    created = true;
}

void Texture3D::createImage3D(int w, int h, int d, const char* texType, GLuint slot, GLenum format){
    type = std::string(texType);
    Texture3D::slot = slot;
    width = w; height = h; depth = d;

    glGenTextures(1, &ID);
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_3D, ID);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, filtering); 
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, filtering);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTexStorage3D(GL_TEXTURE_3D, 1, format, width, height, depth);
    //glGenerateMipmap(GL_TEXTURE_3D);
    
    if (GLAD_GL_ARB_bindless_texture) {
        handle = glGetTextureHandleARB(ID);
        glMakeTextureHandleResidentARB(handle);
        handleImage = glGetImageHandleARB(ID, 0, GL_FALSE, 0, format);
        glMakeImageHandleResidentARB(handleImage, GL_READ_WRITE);
    }
    
    glBindTexture(GL_TEXTURE_3D, 0);
    
    created = true;
}

void Texture3D::Bind(){
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_3D, ID);
}

void Texture3D::Unbind(){
    glBindTexture(GL_TEXTURE_3D, 0);
}

void Texture3D::Delete(){
    if (GLAD_GL_ARB_bindless_texture){
        glMakeTextureHandleNonResidentARB(handle);
        glMakeImageHandleNonResidentARB(handleImage);
    }
    glDeleteTextures(1, &ID);
}

int Texture3D::sizeFromPath(std::string path){
    
    size_t startPos = path.rfind('_');
    size_t endPos = path.rfind('.');
    
    if (startPos != std::string::npos && endPos != std::string::npos && startPos < endPos){
        std::string nstring = path.substr(startPos + 1, endPos - startPos -1);
        std::stringstream ss(nstring);
        int number = 0;
        ss >> number;
        return number;
    }
        
    return -1;
}
