#include "shaderClass.h"
#include <utils/logConsole.h>

std::string get_file_contents(const char* filename)
{
    std::ifstream in(filename, std::ios::binary);
    if (in)
    {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();

        // Check for BOM and remove it if present
        const std::string BOM = "\xEF\xBB\xBF";
        if (contents.compare(0, BOM.size(), BOM) == 0) {
            contents.erase(0, BOM.size());
        }
        
        return contents;
    }
    throw std::runtime_error("Failed to open file: " + std::string(filename));
}

void Shader::LoadShader(const char* vertexFile, const char* fragmentFile)
{

        //create shaders
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    
    if (!isSpirv){
    
        std::string vertexCode; 
        std::string fragmentCode;
        const char* vertexSource; 
        const char* fragmentSource;
    
        if (takePath){
            vertexCode = get_file_contents(vertexFile); 
            fragmentCode = get_file_contents(fragmentFile);
            vertexSource = vertexCode.c_str();
            fragmentSource = fragmentCode.c_str();
        }
        else{
            vertexSource = vertexFile;
            fragmentSource = fragmentFile;
        }
    
            //feed vert shader data at line 5
            glShaderSource(vertexShader, 1, &vertexSource, NULL);
            //compile into machine code
            glCompileShader(vertexShader);
            //error checking
            compileErrors(vertexShader, "VERTEX");
    
            //feed vert shader data at line 5
            glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
            //compile into machine code
            glCompileShader(fragmentShader);
            //error checking
            compileErrors(fragmentShader, "FRAGMENT");
        }
        else{ // spirv
            std::string vertexCode;std::string fragmentCode;
            const void* vertexBinaryPtr = nullptr;
            const void* fragmentBinaryPtr = nullptr;
            size_t vertexSize = 0;
            size_t fragmentSize = 0;
            
            if (takePath){
                vertexCode = get_file_contents(vertexFile);
                fragmentCode = get_file_contents(fragmentFile);
                vertexBinaryPtr = vertexCode.data();
                fragmentBinaryPtr = fragmentCode.data();
                vertexSize = vertexCode.size();
                fragmentSize = fragmentCode.size();
            }
            glShaderBinary(1, &vertexShader, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, vertexBinaryPtr, (GLsizei)vertexSize);
            glShaderBinary(1, &fragmentShader, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, fragmentBinaryPtr, (GLsizei)fragmentSize);
            
            glSpecializeShader(vertexShader, "main", 0, nullptr, nullptr);
            glSpecializeShader(fragmentShader, "main", 0, nullptr, nullptr);
            compileErrors(vertexShader, "VERTEX");
            compileErrors(fragmentShader, "FRAGMENT");
        }
            //}
            //wrap then into shader 
            //create shader program
            ID = glCreateProgram(); //ep2
            //attach vert shader and frag shader
            glAttachShader(ID, vertexShader); //ep2
            glAttachShader(ID, fragmentShader);//ep2
            //wrap
            glLinkProgram(ID);//ep2
            //error checking
            compileErrors(ID, "PROGRAM");

        //delete shaders because its already in the program
        glDeleteShader(vertexShader);//ep2
        glDeleteShader(fragmentShader);//ep2
        //LogConsole::print(("Vert: ", vertexFile, "Frag: ", fragmentFile));

   
}

void Shader::LoadShaderGeom(const char* vertexFile, const char* fragmentFile, const char* geometryFile)
{
    
    //create shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
    
    if (!isSpirv){ 
    
        std::string vertexCode;std::string fragmentCode;std::string geometryCode;
        const char* vertexSource;const char* fragmentSource;const char* geometrySource;
        
        if (takePath){
            vertexCode = get_file_contents(vertexFile);
            fragmentCode = get_file_contents(fragmentFile);
            geometryCode = get_file_contents(geometryFile);

            vertexSource = vertexCode.c_str();
            fragmentSource = fragmentCode.c_str();
            geometrySource = geometryCode.c_str();
        }
        else{
            vertexSource = vertexFile;
            fragmentSource = fragmentFile;
            geometrySource = geometryFile;
        }
    
    
        //feed vert shader data at line 5
        glShaderSource(vertexShader, 1, &vertexSource, NULL);
        //compile into machine code
        glCompileShader(vertexShader);
        //error checking
        compileErrors(vertexShader, "VERTEX");

        //feed vert shader data at line 5
        glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
        //compile into machine code
        glCompileShader(fragmentShader);
        //error checking
        compileErrors(fragmentShader, "FRAGMENT");
        
        //feed vert shader data at line 5
        glShaderSource(geometryShader, 1, &geometrySource, NULL);
        //compile into machine code
        glCompileShader(geometryShader);
        //error checking
        compileErrors(geometryShader, "GEOMETRY");
    }
    else// spirv
    {
        std::string vertexCode;std::string fragmentCode;std::string geometryCode;
        const void* vertexBinaryPtr = nullptr;
        const void* fragmentBinaryPtr = nullptr;
        const void* geometryBinaryPtr = nullptr;
        size_t vertexSize = 0;
        size_t fragmentSize = 0;
        size_t geometrySize = 0;
        
        if (takePath){
            vertexCode = get_file_contents(vertexFile);
            fragmentCode = get_file_contents(fragmentFile);
            geometryCode = get_file_contents(geometryFile);
            vertexBinaryPtr = vertexCode.data();
            fragmentBinaryPtr = fragmentCode.data();
            geometryBinaryPtr = geometryCode.data();
            vertexSize = vertexCode.size();
            fragmentSize = fragmentCode.size();
            geometrySize = geometryCode.size();
        }
        
        glShaderBinary(1, &vertexShader, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, vertexBinaryPtr, (GLsizei)vertexSize);
        glShaderBinary(1, &fragmentShader, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, fragmentBinaryPtr, (GLsizei)fragmentSize);
        glShaderBinary(1, &geometryShader, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, geometryBinaryPtr, (GLsizei)geometrySize);
            
        glSpecializeShader(vertexShader, "main", 0, nullptr, nullptr);
        glSpecializeShader(fragmentShader, "main", 0, nullptr, nullptr);
        glSpecializeShader(geometryShader, "main", 0, nullptr, nullptr);
        compileErrors(vertexShader, "VERTEX");
        compileErrors(fragmentShader, "FRAGMENT");
        compileErrors(geometryShader, "GEOMETRY");
        
    }
    //wrap then into shader 
    //create shader program
    ID = glCreateProgram(); //ep2
    //attach vert shader and frag shader
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
	glAttachShader(ID, geometryShader);
    //wrap
    glLinkProgram(ID);//ep2
    //error checking
    compileErrors(ID, "PROGRAM");

    //delete shaders because its already in the program
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
	glDeleteShader(geometryShader);
    //LogConsole::print(("Vert: ", vertexFile, "Frag: ", fragmentFile, "Geom", geometryFile));}

}

void Shader::LoadComputeShader(const char* computeFile){
    
    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
    
    if (!isSpirv){
    
        std::string computeCode;
        const char* computeSource;
        
        if (takePath){
            computeCode = get_file_contents(computeFile);
            computeSource = computeCode.c_str();
        }
        else{computeSource = computeFile;}
        
        
        glShaderSource(computeShader, 1, &computeSource, NULL);
        glCompileShader(computeShader);
        compileErrors(computeShader, "COMPUTE");
    }
    else // spirv
    {
        std::string computeCode;
        const void* computeBinaryPtr = nullptr;
        size_t computeSize = 0;
        
        if (takePath){
            computeCode = get_file_contents(computeFile);
            computeBinaryPtr = computeCode.data();
            computeSize = computeCode.size();
        }
        glShaderBinary(1, &computeShader, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, computeBinaryPtr, (GLsizei)computeSize);
            
        glSpecializeShader(computeShader, "main", 0, nullptr, nullptr);
        compileErrors(computeShader, "COMPUTE");
    }

    ID = glCreateProgram();
    glAttachShader(ID, computeShader);
    glLinkProgram(ID);
    compileErrors(ID, "PROGRAM");

    glDeleteShader(computeShader);
    /*
    int work_grp_cnt[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
    std::cout << "Max work groups per compute shader" <<
    	" x:" << work_grp_cnt[0] <<
    	" y:" << work_grp_cnt[1] <<
    	" z:" << work_grp_cnt[2] << "\n";

    int work_grp_size[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
    std::cout << "Max work group sizes" <<
    	" x:" << work_grp_size[0] <<
    	" y:" << work_grp_size[1] <<
    	" z:" << work_grp_size[2] << "\n";

    int work_grp_inv;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
    std::cout << "Max invocations count per work group: " << work_grp_inv << "\n";

    if (init::LogALL || init::LogSystems) {
        std::cout << "Compute: " << computeFile << std::endl;
    }
    */
}

void Shader::Activate(){
    glUseProgram(ID);
}
// just making a seperate function for compute shaders to skip a if check
void Shader::ActivateCompute(int x, int y, int z) {
	glUseProgram(ID);
	// Dispatch the compute shader with the specified work group size
	glDispatchCompute(x, y, z);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void Shader::Delete(){
    glDeleteProgram(ID);
}

void Shader::compileErrors(unsigned int shader, const char* type) {
    GLint hasCompiled;
    char infoLog[1024];
    if (strcmp(type, "PROGRAM") != 0) {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &hasCompiled);
        if (hasCompiled == GL_FALSE) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "SHADER_COMPILATION_ERROR for: " << type << "\n" << infoLog << std::endl;
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &hasCompiled);
        if (hasCompiled == GL_FALSE) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "SHADER_LINKING_ERROR for: " << type << "\n" << infoLog << std::endl;
        }
    }
}