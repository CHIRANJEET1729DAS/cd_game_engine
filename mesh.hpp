#ifndef MESH_HPP
#define MESH_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <shader.hpp>

#include <string>
#include <vector>
using namespace std;

#define MAX_BONE_INFLUENCE 4

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];

    void SetBoneDataDefault()
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            m_BoneIDs[i] = -1;
            m_Weights[i] = 0.0f;
        }
    }
};


void bindbuffer(unsigned int &VAO,unsigned int &VBO,unsigned int &EBO,vector<Vertex>vertices,vector<unsigned int>indices)
{
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    glGenBuffers(1,&EBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),&vertices[0],GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size()*sizeof(unsigned int),&indices[0],GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,texCoords));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,tangent));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,bitangent));

    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));

    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));
    
    glBindVertexArray(0);    
}

struct Texture
{
    unsigned int id;
    string type;
    string path;
};

class Mesh
{
public:
 Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
 {
    this->vertices = vertices;
    this->textures = textures;
    this->indices = indices;

    setupMesh();
 }
 void Draw(Shader& shader)
 {
    unsigned int diffuse{1};
    unsigned int normal{1};
    unsigned int specular{1};
    unsigned int height{1};

    shader.setBool("hasTexture", textures.size() > 0);
    for(unsigned int i=0; i<textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        string number;
        string name = textures[i].type;
        if(name=="texture_diffuse")
        {
            number = std::to_string(diffuse++);
        }
        else if(name=="texture_specular")
        {
            number = std::to_string(specular++);
        }
        else if(name=="texture_normal")
        {
            number = std::to_string(normal++);
        }
        else if(name=="texture_height")
        {
            number = std::to_string(height++);
        }
        shader.setInt((name + number).c_str(), i);
        glBindTexture(GL_TEXTURE_2D, textures[i].id);
    }
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES,indices.size(),GL_UNSIGNED_INT,0);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
 }
private:
    unsigned int VAO,VBO,EBO;
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;
    void setupMesh()
    {
        bindbuffer(VAO,VBO,EBO,vertices,indices);
    }
};

#endif