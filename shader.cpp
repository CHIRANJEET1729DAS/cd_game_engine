#include "shader.hpp"

std::string readShaderSource(const char* path)
{
    std::ifstream file;
    file.open(path);
    if (!file.is_open())
    {
        std::cerr << "ERROR::SHADER::FILE_NOT_FOUND: " << path << std::endl;
        return "";
    }
    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

unsigned int compileShader(unsigned int type , const char* shaderSource)
{
    int success;
    char infoLog[512];
    unsigned int id = glCreateShader(type);
    glShaderSource(id , 1 , &shaderSource , NULL);
    glCompileShader(id);
    glGetShaderiv(id , GL_COMPILE_STATUS , &success);
    if (!success)
    {
        glGetShaderInfoLog(id , 512 , NULL , infoLog);
        std::cout<<"ERROR::SHADER::COMPILATION_FAILED\n"<<infoLog<<std::endl;
    }
    return id;
}

void manageShader(unsigned int &ID, unsigned int vertex, unsigned int fragment)
{
    int linksuccess;
    char infoLog[512];
    
    ID = glCreateProgram();
    glAttachShader(ID,vertex);
    glAttachShader(ID,fragment);
    glLinkProgram(ID);

    glGetProgramiv(ID , GL_LINK_STATUS , &linksuccess);
    if (!linksuccess)
    {
        glGetProgramInfoLog(ID , 512 , NULL , infoLog);
        std::cout<<"ERROR::SHADER::PROGRAM::LINKING_FAILED\n"<<infoLog<<std::endl;
    }
    
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    std::string vShaderSource = readShaderSource(vertexPath);
    std::string fShaderSource = readShaderSource(fragmentPath);

    unsigned int vertex = compileShader(GL_VERTEX_SHADER , vShaderSource.c_str());
    unsigned int fragment = compileShader(GL_FRAGMENT_SHADER , fShaderSource.c_str());

    manageShader(ID,vertex,fragment);
}

void Shader::use()
{
    glUseProgram(ID);
}

void Shader::setBool(const std::string& name , bool value) const
{
    glUniform1i(glGetUniformLocation(ID , name.c_str()) , (int)value);
}

void Shader::setInt(const std::string& name , int value) const
{
    glUniform1i(glGetUniformLocation(ID , name.c_str()) , value);
}

void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const
{
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}
