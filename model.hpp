#ifndef MODEL_HPP
#define MODEL_HPP

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <shader.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <libraries/assimp/contrib/stb/stb_image.h>
#include <mesh.hpp>
#include <string>
#include <vector>
#include <map>
using namespace std;

class AssimpGLMHelpers
{
public:
    static inline glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
    {
        glm::mat4 to;
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return to;
    }

    static inline glm::vec3 GetGLMVec(const aiVector3D& vec) 
    { 
        return glm::vec3(vec.x, vec.y, vec.z); 
    }

    static inline glm::quat GetGLMQuat(const aiQuaternion& pOrientation)
    {
        return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y, pOrientation.z);
    }
};

struct BoneInfo
{
    int id;
    glm::mat4 offset;
};

vector<Vertex> fillVertices(aiMesh* mesh)
{
    vector<Vertex> vertices;
    bool hasTexCoords = mesh->mTextureCoords[0] != nullptr;
    cout << "DEBUG: Mesh " << mesh->mName.C_Str() << " has UVs: " << (hasTexCoords ? "YES" : "NO") << endl;
    for(unsigned int i=0; i<mesh->mNumVertices; i++)
    {
        Vertex vertex;
        vertex.SetBoneDataDefault();
        glm::vec3 vector;
        // positions
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.position = vector;
        // normals
        if (mesh->HasNormals())
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.normal = vector;
        }
        // texture coordinates
        if(mesh->mTextureCoords[0])
        {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x; 
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.texCoords = vec;
            // tangents
            if (mesh->HasTangentsAndBitangents())
            {
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.tangent = vector;
                // bitangents
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.bitangent = vector;
            }
        }
        else
            vertex.texCoords = glm::vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }
    return vertices;
}

vector<unsigned int>fillIndices(aiMesh* mesh)
{
    vector<unsigned int> indices;
    for(unsigned int i=0;i<mesh->mNumFaces;i++)
    {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j=0;j<face.mNumIndices;j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }
    return indices;
};


class Model
{
public:
    std::map<string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;

    Model(string const &path)
    {
        loadModel(path);
    }
    void Draw(Shader& shader)
    {
        for(unsigned int i=0;i<meshes.size();i++)
        {
            meshes[i].Draw(shader);
        }        
    }

    auto& GetBoneInfoMap() { return m_BoneInfoMap; }
    int& GetBoneCount() { return m_BoneCounter; }

    void UpdateAnimation(float currentTime, const aiScene* scene, vector<glm::mat4>& transforms)
    {
        if (scene->mNumAnimations > 0)
        {
            aiAnimation* animation = scene->mAnimations[0];
            float ticksPerSecond = animation->mTicksPerSecond != 0 ? animation->mTicksPerSecond : 25.0f;
            float timeInTicks = currentTime * ticksPerSecond;
            float animationTime = fmod(timeInTicks, (float)animation->mDuration);

            CalculateBoneTransform(scene->mRootNode, animation, animationTime, glm::mat4(1.0f), transforms);
        }
    }

    void CalculateBoneTransform(const aiNode* node, const aiAnimation* animation, float animationTime, glm::mat4 parentTransform, vector<glm::mat4>& transforms)
    {
        string nodeName(node->mName.data);
        glm::mat4 nodeTransform = AssimpGLMHelpers::ConvertMatrixToGLMFormat(node->mTransformation);

        const aiNodeAnim* nodeAnim = FindNodeAnim(animation, nodeName);

        if (nodeAnim)
        {
            glm::mat4 translation = InterpolateTranslation(animationTime, nodeAnim);
            glm::mat4 rotation = InterpolateRotation(animationTime, nodeAnim);
            glm::mat4 scale = InterpolateScaling(animationTime, nodeAnim);
            nodeTransform = translation * rotation * scale;
        }

        glm::mat4 globalTransformation = parentTransform * nodeTransform;

        if (m_BoneInfoMap.find(nodeName) != m_BoneInfoMap.end())
        {
            int index = m_BoneInfoMap[nodeName].id;
            glm::mat4 offset = m_BoneInfoMap[nodeName].offset;
            transforms[index] = globalTransformation * offset;
        }

        for (int i = 0; i < node->mNumChildren; i++)
            CalculateBoneTransform(node->mChildren[i], animation, animationTime, globalTransformation, transforms);
    }

    const aiNodeAnim* FindNodeAnim(const aiAnimation* animation, string nodeName)
    {
        for (int i = 0; i < animation->mNumChannels; i++)
        {
            const aiNodeAnim* nodeAnim = animation->mChannels[i];
            if (string(nodeAnim->mNodeName.data) == nodeName)
                return nodeAnim;
        }
        return nullptr;
    }

    glm::mat4 InterpolateTranslation(float animationTime, const aiNodeAnim* nodeAnim)
    {
        if (nodeAnim->mNumPositionKeys == 1)
            return glm::translate(glm::mat4(1.0f), AssimpGLMHelpers::GetGLMVec(nodeAnim->mPositionKeys[0].mValue));

        int p0Index = GetPositionIndex(animationTime, nodeAnim);
        int p1Index = p0Index + 1;
        float lerpFactor = GetLerpFactor(animationTime, nodeAnim->mPositionKeys[p0Index].mTime, nodeAnim->mPositionKeys[p1Index].mTime);
        glm::vec3 finalTranslation = glm::mix(AssimpGLMHelpers::GetGLMVec(nodeAnim->mPositionKeys[p0Index].mValue), AssimpGLMHelpers::GetGLMVec(nodeAnim->mPositionKeys[p1Index].mValue), lerpFactor);
        return glm::translate(glm::mat4(1.0f), finalTranslation);
    }

    glm::mat4 InterpolateRotation(float animationTime, const aiNodeAnim* nodeAnim)
    {
        if (nodeAnim->mNumRotationKeys == 1)
        {
            auto rotation = AssimpGLMHelpers::GetGLMQuat(nodeAnim->mRotationKeys[0].mValue);
            return glm::mat4_cast(rotation);
        }

        int r0Index = GetRotationIndex(animationTime, nodeAnim);
        int r1Index = r0Index + 1;
        float lerpFactor = GetLerpFactor(animationTime, nodeAnim->mRotationKeys[r0Index].mTime, nodeAnim->mRotationKeys[r1Index].mTime);
        glm::quat finalRotation = glm::slerp(AssimpGLMHelpers::GetGLMQuat(nodeAnim->mRotationKeys[r0Index].mValue), AssimpGLMHelpers::GetGLMQuat(nodeAnim->mRotationKeys[r1Index].mValue), lerpFactor);
        return glm::mat4_cast(finalRotation);
    }

    glm::mat4 InterpolateScaling(float animationTime, const aiNodeAnim* nodeAnim)
    {
        if (nodeAnim->mNumScalingKeys == 1)
            return glm::scale(glm::mat4(1.0f), AssimpGLMHelpers::GetGLMVec(nodeAnim->mScalingKeys[0].mValue));

        int s0Index = GetScalingIndex(animationTime, nodeAnim);
        int s1Index = s0Index + 1;
        float lerpFactor = GetLerpFactor(animationTime, nodeAnim->mScalingKeys[s0Index].mTime, nodeAnim->mScalingKeys[s1Index].mTime);
        glm::vec3 finalScale = glm::mix(AssimpGLMHelpers::GetGLMVec(nodeAnim->mScalingKeys[s0Index].mValue), AssimpGLMHelpers::GetGLMVec(nodeAnim->mScalingKeys[s1Index].mValue), lerpFactor);
        return glm::scale(glm::mat4(1.0f), finalScale);
    }

    float GetLerpFactor(float animationTime, float lastTimeStamp, float nextTimeStamp)
    {
        float midWayLength = animationTime - lastTimeStamp;
        float framesDiff = nextTimeStamp - lastTimeStamp;
        return midWayLength / framesDiff;
    }

    int GetPositionIndex(float animationTime, const aiNodeAnim* nodeAnim)
    {
        for (int index = 0; index < nodeAnim->mNumPositionKeys - 1; ++index)
        {
            if (animationTime < (float)nodeAnim->mPositionKeys[index + 1].mTime)
                return index;
        }
        return 0;
    }

    int GetRotationIndex(float animationTime, const aiNodeAnim* nodeAnim)
    {
        for (int index = 0; index < nodeAnim->mNumRotationKeys - 1; ++index)
        {
            if (animationTime < (float)nodeAnim->mRotationKeys[index + 1].mTime)
                return index;
        }
        return 0;
    }

    int GetScalingIndex(float animationTime, const aiNodeAnim* nodeAnim)
    {
        for (int index = 0; index < nodeAnim->mNumScalingKeys - 1; ++index)
        {
            if (animationTime < (float)nodeAnim->mScalingKeys[index + 1].mTime)
                return index;
        }
        return 0;
    }

private:
    vector<Mesh> meshes;
    string directory;

    void loadModel(const string& path)
    {
        cout << "DEBUG: Loading model from " << path << endl;
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
        {
            cout << "ERROR::ASSIMP::" << importer.GetErrorString() << endl;
            return;
        }
        directory = path.substr(0,path.find_last_of('/'));
        cout << "DEBUG: Model directory is " << directory << endl;
        processNode(scene->mRootNode,scene);
    }

    void processNode(aiNode* node, const aiScene* scene)
    {
        for(unsigned int i=0;i<node->mNumMeshes;i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh,scene));
        }
        for(unsigned int i=0;i<node->mNumChildren;i++)
        {
            processNode(node->mChildren[i],scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        vector<Vertex> vertices = fillVertices(mesh);
        vector<unsigned int> indices = fillIndices(mesh);
        vector<Texture> textures;

        ExtractBoneWeightForVertices(vertices, mesh, scene);

        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        
        vector<Texture> diffuseMaps = loadMaterialTextures(material,aiTextureType_DIFFUSE,"texture_diffuse");
        vector<Texture> specularMaps = loadMaterialTextures(material,aiTextureType_SPECULAR,"texture_specular");
        vector<Texture> normalMaps = loadMaterialTextures(material,aiTextureType_HEIGHT,"texture_normal");      
        vector<Texture> heightMaps = loadMaterialTextures(material,aiTextureType_AMBIENT,"texture_height");


        textures.insert(textures.end(),diffuseMaps.begin(),diffuseMaps.end());
        textures.insert(textures.end(),normalMaps.begin(),normalMaps.end());
        textures.insert(textures.end(),heightMaps.begin(),heightMaps.end());
        textures.insert(textures.end(),specularMaps.begin(),specularMaps.end());
        
        return Mesh(vertices,indices,textures);
    }

    void SetVertexBoneData(Vertex& vertex, int boneID, float weight)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
        {
            if (vertex.m_BoneIDs[i] < 0)
            {
                vertex.m_Weights[i] = weight;
                vertex.m_BoneIDs[i] = boneID;
                break;
            }
        }
    }

    void ExtractBoneWeightForVertices(vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
    {
        for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
        {
            int boneID = -1;
            string boneName = mesh->mBones[boneIndex]->mName.C_Str();
            if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
            {
                BoneInfo newBoneInfo;
                newBoneInfo.id = m_BoneCounter;
                newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
                m_BoneInfoMap[boneName] = newBoneInfo;
                boneID = m_BoneCounter;
                m_BoneCounter++;
            }
            else
            {
                boneID = m_BoneInfoMap[boneName].id;
            }
            assert(boneID != -1);
            auto weights = mesh->mBones[boneIndex]->mWeights;
            int numWeights = mesh->mBones[boneIndex]->mNumWeights;

            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
            {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;
                assert(vertexId <= vertices.size());
                SetVertexBoneData(vertices[vertexId], boneID, weight);
            }
        }
    }
    vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        
        // SPECIAL CASE: WSKRS always uses this specific texture for diffuse
        if (directory.find("wskrs") != string::npos) {
            if (typeName == "texture_diffuse") {
                string forcedPath = directory + "/../textures/0de74da3bcee217e5bac706608edf79f.jpg";
                cout << "WSKRS Special: FORCING load of " << forcedPath << endl;
                Texture texture;
                texture.id = loadTexture(forcedPath);
                if (texture.id != 0) {
                    texture.type = typeName;
                    texture.path = "forced_wskrs_texture";
                    textures.push_back(texture);
                    return textures; 
                }
            }
        }

        // SPECIAL CASE: Star Cruiser Enigma
        if (directory.find("star-cruiser-x-enigma") != string::npos) {
            string texName = "";
            if (typeName == "texture_diffuse") texName = "model_baseColor.png";
            else if (typeName == "texture_emissive") texName = "model_emissive.png";

            if (texName != "") {
                string forcedPath = directory + "/textures/" + texName;
                cout << "ENIGMA Special: FORCING load of " << forcedPath << " for " << typeName << endl;
                Texture texture;
                texture.id = loadTexture(forcedPath);
                if (texture.id != 0) {
                    texture.type = typeName;
                    texture.path = "forced_enigma_" + typeName;
                    textures.push_back(texture);
                    return textures;
                }
            }
        }

        for(unsigned int i=0; i<mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            string texPath = string(str.C_Str());
            
            // Handle Windows/Linux path separators
            size_t lastBackslash = texPath.find_last_of('\\');
            size_t lastSlash = texPath.find_last_of('/');
            size_t lastSpec = (lastBackslash == string::npos) ? lastSlash : (lastSlash == string::npos ? lastBackslash : max(lastSlash, lastBackslash));
            
            string filename = (lastSpec == string::npos) ? texPath : texPath.substr(lastSpec + 1);
            string parentDir = "";
            if (lastSpec != string::npos) {
                size_t prevSpec = texPath.find_last_of("\\/", lastSpec - 1);
                parentDir = (prevSpec == string::npos) ? texPath.substr(0, lastSpec) : texPath.substr(prevSpec + 1, lastSpec - prevSpec - 1);
            }

            Texture texture;
            string texturesDir = directory + "/../textures/";
            
            // Heuristic for Storm Trooper model: maps "body/diffuse.png" to "diffuse_body.png"
            string mappedName = filename;
            if (filename == "diffuse.png" && parentDir != "") {
                mappedName = "diffuse_" + parentDir + ".png";
                if (parentDir == "helmet") mappedName = "diffuse_helmets.png";
            }

            string fullPath = directory + "/" + filename;
            string mappedPath = texturesDir + mappedName;
            
            // Try different paths and naming conventions
            texture.id = loadTexture(mappedPath);
            if (texture.id == 0) {

                // Force first texture for WSKRS model
                if (directory.find("wskrs") != string::npos) {
                    texture.id = loadTexture(directory + "/../textures/0de74da3bcee217e5bac706608edf79f.jpg");
                }
            }
            if (texture.id == 0) texture.id = loadTexture(texturesDir + filename);
            if (texture.id == 0) texture.id = loadTexture(fullPath);
            
            if (texture.id != 0) {
                cout << "SUCCESS: Loaded texture " << mappedPath << endl;
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
            } else {
                cout << "FAILED: Could not load texture from file " << filename << endl;
            }
        }
        return textures;
    }
    
    unsigned int loadTexture(string const& path)
    {
        int width, height, nrComponents;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0); 
        if(data)
        {
            cout << "STB SUCCESS: " << path << " (" << width << "x" << height << ", " << nrComponents << " channels)" << endl;
            unsigned int textureID;
            glGenTextures(1, &textureID);
            GLenum format;
            if(nrComponents == 1) format = GL_RED;
            else if(nrComponents == 3) format = GL_RGB;
            else if(nrComponents == 4) format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            return textureID;
        }
        else
        {
            cout << "STB FAILED: " << path << endl;
            stbi_image_free(data);
            return 0;
        }
    }

};

#endif