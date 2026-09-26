#ifndef FLOURA_IMAGE_RESIZE_CLASS_H	
#define FLOURA_IMAGE_RESIZE_CLASS_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
//#include <glad/gl.h>

class FlouraImageResize{
public:
    static void resize2D(unsigned char* &ibytes, int &iwidthImg, int &iheightImg, int &inumColCh, unsigned char* &obytes, int &owidthImg, int &oheightImg);
    
private:
};
#endif