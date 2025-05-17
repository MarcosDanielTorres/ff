struct Bone
{
    unsigned int mBoneId = 0;
    std::string mNodeName; // a bone is a node
    glm::mat4 mOffsetMatrix = glm::mat4(1.0f);
};

struct OGLVertex {
  glm::vec3 position = glm::vec3(0.0f);
  glm::vec4 color = glm::vec4(1.0f);
  glm::vec3 normal = glm::vec3(0.0f);
  glm::vec2 uv = glm::vec2(0.0f);
  glm::uvec4 boneNumber = glm::uvec4(0);
  glm::vec4 boneWeight = glm::vec4(0.0f);
};

struct OGLMesh {
  std::vector<OGLVertex> vertices{};
  std::vector<uint32_t> indices{};
  std::unordered_map<aiTextureType, std::string> textures{};
  bool usesPBRColors = false;
};

struct Texture
{
    GLuint mTexture = 0;
    int mTexWidth = 0;
    int mTexHeight = 0;
    int mNumberOfChannels = 0;
    std::string mTextureName;

    bool loadTexture(OpenGL* opengl, std::string textureFilename, bool flipImage = 1) 
    {
        mTextureName = textureFilename;

        stbi_set_flip_vertically_on_load(flipImage);
        /* always load as RGBA */
        unsigned char *textureData = stbi_load(textureFilename.c_str(), &mTexWidth, &mTexHeight, &mNumberOfChannels, STBI_rgb_alpha);

        if (!textureData) {
            // Logger::log(1, "%s error: could not load file '%s'\n", __FUNCTION__, mTextureName.c_str());
            stbi_image_free(textureData);
            return false;
        }

        glGenTextures(1, &mTexture);
        glBindTexture(GL_TEXTURE_2D, mTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, mTexWidth, mTexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);

        opengl->glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(textureData);

        //Logger::log(1, "%s: texture '%s' loaded (%dx%d, %d channels)\n", __FUNCTION__, mTextureName.c_str(), mTexWidth, mTexHeight, mNumberOfChannels);
        return true;
    }

    bool loadTexture(OpenGL *opengl, std::string textureName, aiTexel* textureData, int width, int height, bool flipImage = true)
    {
        if (!textureData) {
            // Logger::log(1, "%s error: could not load texture '%s'\n", __FUNCTION__, textureName.c_str());
            return false;
        }

        // Logger::log(1, "%s: texture file '%s' has width %i and height %i\n", __FUNCTION__, textureName.c_str(), width, height);

        glGenTextures(1, &mTexture);
        glBindTexture(GL_TEXTURE_2D, mTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        /* allow to flip the image, similar to file loaded from disk */
        stbi_set_flip_vertically_on_load(flipImage);

        /* we use stbi to detect the in-memory format, but always request RGBA */
        unsigned char *data = nullptr;
        if (height == 0)   {
            data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(textureData), width, &mTexWidth, &mTexHeight, &mNumberOfChannels, STBI_rgb_alpha);
        }
        else   {
            data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(textureData), width * height, &mTexWidth, &mTexHeight, &mNumberOfChannels, STBI_rgb_alpha);
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, mTexWidth, mTexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);

        opengl->glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Logger::log(1, "%s: texture '%s' loaded (%dx%d, %d channels)\n", __FUNCTION__, textureName.c_str(), mTexWidth, mTexHeight, mNumberOfChannels);
        return true;
    }

    void bind() 
    {
        glBindTexture(GL_TEXTURE_2D, mTexture);
    }

    void unbind() 
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

struct Mesh
{
    std::string mMeshName;
    unsigned int mTriangleCount = 0;
    unsigned int mVertexCount = 0;

    glm::vec4 mBaseColor = glm::vec4(1.0f);

    OGLMesh mMesh{};
    //std::unordered_map<std::string, std::shared_ptr<Texture>> mTextures{};
    std::unordered_map<std::string, Texture*> mTextures{};

    std::vector<Bone*> mBoneList{};


};

struct Node
{
    std::string nodeName;
    // Node *first;
    // Node *last;
    std::vector<Node*> mChildNodes{};

    Node *mParentNode;

    glm::vec3 mTranslation = glm::vec3(0.0f);
    glm::quat mRotation = glm::identity<glm::quat>();
    glm::vec3 mScaling = glm::vec3(1.0f);

    glm::mat4 mTranslationMatrix = glm::mat4(1.0f);
    glm::mat4 mRotationMatrix = glm::mat4(1.0f);
    glm::mat4 mScalingMatrix = glm::mat4(1.0f);

    glm::mat4 mParentNodeMatrix = glm::mat4(1.0f);
    glm::mat4 mLocalTRSMatrix = glm::mat4(1.0f);

    /* extra matrix to move model instances  around */
    glm::mat4 mRootTransformMatrix = glm::mat4(1.0f);
};

struct VertexIndexBuffer
{
    GLuint mVAO = 0;
    GLuint mVertexVBO = 0;
    GLuint mIndexVBO = 0;

    void init(OpenGL *opengl) 
    {
        opengl->glGenVertexArrays(1, &mVAO);
        opengl->glGenBuffers(1, &mVertexVBO);
        opengl->glGenBuffers(1, &mIndexVBO);

        opengl->glBindVertexArray(mVAO);

        opengl->glBindBuffer(GL_ARRAY_BUFFER, mVertexVBO);

        opengl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(OGLVertex), (void*) offsetof(OGLVertex, position));
        opengl->glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(OGLVertex), (void*) offsetof(OGLVertex, color));
        opengl->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(OGLVertex), (void*) offsetof(OGLVertex, normal));
        opengl->glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(OGLVertex), (void*) offsetof(OGLVertex, uv));
        opengl->glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT,   sizeof(OGLVertex), (void*) offsetof(OGLVertex, boneNumber));
        opengl->glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(OGLVertex), (void*) offsetof(OGLVertex, boneWeight));

        opengl->glEnableVertexAttribArray(0);
        opengl->glEnableVertexAttribArray(1);
        opengl->glEnableVertexAttribArray(2);
        opengl->glEnableVertexAttribArray(3);
        opengl->glEnableVertexAttribArray(4);
        opengl->glEnableVertexAttribArray(5);

        opengl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexVBO);
        /* do NOT unbind index buffer here!*/

        opengl->glBindBuffer(GL_ARRAY_BUFFER, 0);
        opengl->glBindVertexArray(0);

        //Logger::log(1, "%s: VAO and VBOs initialized\n", __FUNCTION__);
    }

    void cleanup(OpenGL *opengl) 
    {
        opengl->glDeleteBuffers(1, &mIndexVBO);
        opengl->glDeleteBuffers(1, &mVertexVBO);
        opengl->glDeleteVertexArrays(1, &mVAO);
    }

    void uploadData(OpenGL *opengl, std::vector<OGLVertex> vertexData, std::vector<uint32_t> indices) 
    {
        if (vertexData.empty() || indices.empty()) {
            //Logger::log(1, "%s error: invalid data to upload (vertices: %i, indices: %i)\n", __FUNCTION__, vertexData.size(), indices.size());
            return;
        }

        opengl->glBindBuffer(GL_ARRAY_BUFFER, mVertexVBO);
        opengl->glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(OGLVertex), &vertexData.at(0), GL_DYNAMIC_DRAW);
        opengl->glBindBuffer(GL_ARRAY_BUFFER, 0);

        opengl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexVBO);
        opengl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices.at(0), GL_DYNAMIC_DRAW);
        opengl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void bind(OpenGL *opengl) 
    {
        opengl->glBindVertexArray(mVAO);
    }

    void unbind(OpenGL *opengl) 
    {
        opengl->glBindVertexArray(0);
    }

    #if 0

    void draw(OpenGL *opengl, GLuint mode, unsigned int start, unsigned int num) 
    {
        opengl->glDrawArrays(mode, start, num);
    }

    void bindAndDraw(OpenGL *opengl, GLuint mode, unsigned int start, unsigned int num) 
    {
        bind();
        draw(mode, start, num);
        unbind();
    }

    void drawIndirect(OpenGL *opengl, GLuint mode, unsigned int num) 
    {
        opengl->glDrawElements(mode, num, GL_UNSIGNED_INT, 0);
    }

    void bindAndDrawIndirect(OpenGL *opengl, GLuint mode, unsigned int num) 
    {
        bind();
        drawIndirect(opengl, mode, num);
        unbind();
    }
    #endif

    void drawIndirectInstanced(OpenGL *opengl, GLuint mode, unsigned int num, int instanceCount) {
        opengl->glDrawElementsInstanced(mode, num, GL_UNSIGNED_INT, 0, instanceCount);
    }
    void bindAndDrawIndirectInstanced(OpenGL *opengl, GLuint mode, unsigned int num, int instanceCount) 
    {
        bind(opengl);
        drawIndirectInstanced(opengl, mode, num, instanceCount);
        unbind(opengl);
    }
     
};

/* transposes the matrix from Assimp to GL format */
glm::mat4 convertAiToGLM(aiMatrix4x4 inMat) {
  return {
    inMat.a1, inMat.b1, inMat.c1, inMat.d1,
    inMat.a2, inMat.b2, inMat.c2, inMat.d2,
    inMat.a3, inMat.b3, inMat.c3, inMat.d3,
    inMat.a4, inMat.b4, inMat.c4, inMat.d4
  };
}


struct Model
{
    u32 vertex_count;
    u32 triangle_count;
    Node *mRootNode;
    /* a map to find the node by name */
    std::unordered_map<std::string, Node*> mNodeMap{};
    /* and a 'flat' map to keep the order of insertation  */
    std::vector<Node*> mNodeList{};

    std::vector<Bone*> mBoneList{};
    std::unordered_map<std::string, glm::mat4> mBoneOffsetMatrices{};


    std::vector<OGLMesh> mModelMeshes;
    std::vector<VertexIndexBuffer> mVertexBuffers{};

    glm::mat4 mRootTransformMatrix = glm::mat4(1.0f);

    // map textures to external or internal texture names
    std::unordered_map<std::string, Texture *> mTextures{};
    Texture *mPlaceholderTexture = nullptr;
    Texture *mWhiteTexture = nullptr;


};

Node *createNode(std::string nodeName)
{
    Node* node = new Node();
    node->nodeName = nodeName;
    node->mParentNode = {};
    node->mChildNodes = {};

    node->mTranslation = glm::vec3(0.0f);
    node->mRotation = glm::identity<glm::quat>();
    node->mScaling = glm::vec3(1.0f);

    node->mTranslationMatrix = glm::mat4(1.0f);
    node->mRotationMatrix = glm::mat4(1.0f);
    node->mScalingMatrix = glm::mat4(1.0f);

    node->mParentNodeMatrix = glm::mat4(1.0f);
    node->mLocalTRSMatrix = glm::mat4(1.0f);

    /* extra matrix to move model instances  around */
    node->mRootTransformMatrix = glm::mat4(1.0f);

    return node;
}

bool processMesh(OpenGL *opengl, Mesh *myMesh, aiMesh* mesh, const aiScene* scene, std::string assetDirectory)
{
    myMesh->mMeshName = mesh->mName.C_Str();

  myMesh->mTriangleCount = mesh->mNumFaces;
  myMesh->mVertexCount = mesh->mNumVertices;

  //Logger::log(1, "%s: -- mesh '%s' has %i faces (%i vertices)\n", __FUNCTION__, myMesh->mMeshName.c_str(), mTriangleCount, mVertexCount);
  for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i) {
    if (mesh->HasVertexColors(i)) {
      //Logger::log(1, "%s: --- mesh has vertex colors in set %i\n", __FUNCTION__, i);
    }
  }
  if (mesh->HasNormals()) {
    //Logger::log(1, "%s: --- mesh has normals\n", __FUNCTION__);
  }
  for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
    if (mesh->HasTextureCoords(i)) {
      //Logger::log(1, "%s: --- mesh has texture cooords in set %i\n", __FUNCTION__, i);
    }
  }

  aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
  if (material) {
    aiString materialName = material->GetName();
    //Logger::log(1, "%s: - material found, name '%s'\n", __FUNCTION__, materialName.C_Str());

    if (mesh->mMaterialIndex >= 0) {
      // scan only for diifuse and scalar textures for a start
      std::vector<aiTextureType> supportedTexTypes = { aiTextureType_DIFFUSE, aiTextureType_SPECULAR };
      for (const auto& texType : supportedTexTypes) {
        unsigned int textureCount = material->GetTextureCount(texType);
        if (textureCount > 0) {
          //Logger::log(1, "%s: -- material '%s' has %i images of type %i\n", __FUNCTION__, materialName.C_Str(), textureCount, texType);
          for (unsigned int i = 0; i < textureCount; ++i) {
            aiString textureName;
            material->GetTexture(texType, i, &textureName);
            //Logger::log(1, "%s: --- image %i has name '%s'\n", __FUNCTION__, i, textureName.C_Str());

            std::string texName = textureName.C_Str();
            myMesh->mMesh.textures.insert({texType, texName});

            // do not try to load internal textures
            if (!texName.empty() && texName.find("*") != 0) {
              Texture *newTex = new Texture();
              std::string texNameWithPath = assetDirectory + '/' + texName;
              if (!newTex->loadTexture(opengl, texNameWithPath)) {
                // Logger::log(1, "%s error: could not load texture file '%s', skipping\n", __FUNCTION__, texNameWithPath.c_str());
                continue;
              }

              myMesh->mTextures.insert({texName, newTex});
            }
          }
        }
      }
    }

    aiColor4D baseColor(0.0f, 0.0f, 0.0f, 1.0f);
    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor) == aiReturn_SUCCESS && myMesh->mTextures.empty()) {
        myMesh->mBaseColor = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
        myMesh->mMesh.usesPBRColors = true;
    }
  }

  for (unsigned int i = 0; i < myMesh->mVertexCount; ++i) {
    OGLVertex vertex;
    vertex.position.x = mesh->mVertices[i].x;
    vertex.position.y = mesh->mVertices[i].y;
    vertex.position.z = mesh->mVertices[i].z;


    if (mesh->HasVertexColors(0)) {
      vertex.color.r = mesh->mColors[0][i].r;
      vertex.color.g = mesh->mColors[0][i].g;
      vertex.color.b = mesh->mColors[0][i].b;
      vertex.color.a = mesh->mColors[0][i].a;
    } else {
      if (myMesh->mMesh.usesPBRColors) {
        vertex.color = myMesh->mBaseColor;
      } else {
        vertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
      }
    }

    if (mesh->HasNormals()) {
      vertex.normal.x = mesh->mNormals[i].x;
      vertex.normal.y = mesh->mNormals[i].y;
      vertex.normal.z = mesh->mNormals[i].z;
    } else {
      vertex.normal = glm::vec3(0.0f);
    }

    if (mesh->HasTextureCoords(0)) {
      vertex.uv.x = mesh->mTextureCoords[0][i].x;
      vertex.uv.y = mesh->mTextureCoords[0][i].y;
    } else {
      vertex.uv = glm::vec2(0.0f);
    }

    myMesh->mMesh.vertices.emplace_back(vertex);
  }

  for (unsigned int i = 0; i < myMesh->mTriangleCount; ++i) {
    aiFace face = mesh->mFaces[i];
    myMesh->mMesh.indices.push_back(face.mIndices[0]);
    myMesh->mMesh.indices.push_back(face.mIndices[1]);
    myMesh->mMesh.indices.push_back(face.mIndices[2]);
  }


  #if 1
  if (mesh->HasBones()) {
    unsigned int numBones = mesh->mNumBones;
    //Logger::log(1, "%s: -- mesh has information about %i bones\n", __FUNCTION__, numBones);
    for (unsigned int boneId = 0; boneId < numBones; ++boneId) {
      std::string boneName = mesh->mBones[boneId]->mName.C_Str();
      unsigned int numWeights = mesh->mBones[boneId]->mNumWeights;
      //Logger::log(1, "%s: --- bone nr. %i has name %s, contains %i weights\n", __FUNCTION__, boneId, boneName.c_str(), numWeights);

      Bone *newBone = new Bone(boneId, boneName, convertAiToGLM(mesh->mBones[boneId]->mOffsetMatrix));
      myMesh->mBoneList.push_back(newBone);

      for (unsigned int weight = 0; weight < numWeights; ++weight) {
        unsigned int vertexId = mesh->mBones[boneId]->mWeights[weight].mVertexId;
        float vertexWeight = mesh->mBones[boneId]->mWeights[weight].mWeight;

        glm::uvec4 currentIds = myMesh->mMesh.vertices.at(vertexId).boneNumber;
        glm::vec4 currentWeights = myMesh->mMesh.vertices.at(vertexId).boneWeight;

        /* insert weight and bone id into first free slot (weight => 0.0f) */
        for (unsigned int i = 0; i < 4; ++i) {
          if (currentWeights[i] == 0.0f) {
            currentIds[i] = boneId;
            currentWeights[i] = vertexWeight;

            /* skip to next weight */
            break;
          }
        }

        myMesh->mMesh.vertices.at(vertexId).boneNumber = currentIds;
        myMesh->mMesh.vertices.at(vertexId).boneWeight = currentWeights;

      }
    }
  }
  #endif

  return true;
}


void processNode(OpenGL *opengl, Model *model, Node* node, aiNode* aNode, const aiScene* scene, std::string assetDirectory) {
  std::string nodeName = aNode->mName.C_Str();
  //Logger::log(1, "%s: node name: '%s'\n", __FUNCTION__, nodeName.c_str());

  unsigned int numMeshes = aNode->mNumMeshes;
  if (numMeshes > 0) {
    //Logger::log(1, "%s: - node has %i meshes\n", __FUNCTION__, numMeshes);
    for (unsigned int i = 0; i < numMeshes; ++i) {
      aiMesh* modelMesh = scene->mMeshes[aNode->mMeshes[i]];

      Mesh mesh;
      processMesh(opengl, &mesh, modelMesh, scene, assetDirectory);

      model->mModelMeshes.emplace_back(mesh.mMesh);
      model->mTextures.merge(mesh.mTextures);

      /* avoid inserting duplicate bone Ids - meshes can reference the same bones */
      std::vector<Bone*> flatBones = mesh.mBoneList;
      for (const auto& bone : flatBones) 
      {
        const auto iter = std::find_if(model->mBoneList.begin(), model->mBoneList.end(), [bone](Bone* otherBone) { return bone->mBoneId == otherBone->mBoneId; });
        if (iter == model->mBoneList.end()) 
        {
          model->mBoneList.emplace_back(bone);
        }
      }
    }
  }

  model->mNodeMap.insert({nodeName, node});
  model->mNodeList.emplace_back(node);

  unsigned int numChildren = aNode->mNumChildren;
  //Logger::log(1, "%s: - node has %i children \n", __FUNCTION__, numChildren);

  for (unsigned int i = 0; i < numChildren; ++i) {
    std::string childName = aNode->mChildren[i]->mName.C_Str();

    //Node childNode = node->addChild(childName);
    Node *childNode = createNode(childName);
    childNode->mParentNode = node;
    node->mChildNodes.push_back(childNode);
    /* should be: node->last = createNode(childName)
    node->last->next = createNode(childName);
    node->last = node->last->next;
    */

    processNode(opengl, model, childNode, aNode->mChildren[i], scene, assetDirectory);
  }
}

internal void
drawInstanced(Model *model, OpenGL *opengl, u32 instanceCount)
{
    // TODO por que cuando tenia comentadas las texturas defaulteaba a las otras texturas? en algun lado se cargaron, donde?
    // This was happening because I forgot to unbind the texture I use for UI and also I got confused because the woman and the ui
    // was using similar colors, or the same, but not quite so I thought it was using the woman's texture but it was not, it was using the cube.
    // So the fix was to unbind the texture in end_ui_frame
    for (u32 i = 0; i < model->mModelMeshes.size(); ++i) 
    {
        OGLMesh& mesh = model->mModelMeshes.at(i);
        // find diffuse texture by name

        Texture *diffuseTex = nullptr;
        auto diffuseTexName = mesh.textures.find(aiTextureType_DIFFUSE);
        if (diffuseTexName != mesh.textures.end()) {
        auto diffuseTexture = model->mTextures.find(diffuseTexName->second);
        if (diffuseTexture != model->mTextures.end()) {
            diffuseTex = diffuseTexture->second;
        }
        }

        opengl->glActiveTexture(GL_TEXTURE0);

        if (diffuseTex) {
          diffuseTex->bind();
        } else {
          if (mesh.usesPBRColors) {
            model->mWhiteTexture->bind();
          } else {
            model->mPlaceholderTexture->bind();
          }
        }

        //model->mVertexBuffers.at(i).bindAndDrawIndirectInstanced(opengl, GL_TRIANGLES, mesh.indices.size(), instanceCount);

        VertexIndexBuffer model_buffer = model->mVertexBuffers.at(i);
        model_buffer.bind(opengl);
        #if 0
            glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
        #else
            opengl->glDrawElementsInstanced(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0, 1);
        #endif
        model_buffer.unbind(opengl);

        if (diffuseTex) {
          diffuseTex->unbind();
        } else {
          if (mesh.usesPBRColors) {
            model->mWhiteTexture->unbind();
          } else {
            model->mPlaceholderTexture->unbind();
          }
        }
    }
}

internal Model *
load_model(OpenGL *opengl, const char* model_filepath)
{
        Model *model = new Model(); //placeholder_state->model;
        // w1 models
        Assimp::Importer importer;

        const aiScene *scene = importer.ReadFile(model_filepath, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_ValidateDataStructure);
        AssertGui(scene && scene->mRootNode, "Error loading model, probably not found!");

        model->vertex_count = 0;
        model->triangle_count = 0;
        model->mRootNode = 0;
        
        u32 meshes_count = scene->mNumMeshes;
        for (u32 mesh_idx = 0; mesh_idx < meshes_count; mesh_idx++)
        {
            u32 vertices_count = scene->mMeshes[mesh_idx]->mNumVertices;
            u32 faces_count = scene->mMeshes[mesh_idx]->mNumFaces;

            model->vertex_count += vertices_count;
            model->triangle_count += faces_count;
        }

        if(scene->HasTextures())
        {
            u32 num_textures = scene->mNumTextures;
            for(u32 t = 0; t < num_textures; t++)
            {
                std::string texName = scene->mTextures[t]->mFilename.C_Str();
                i32 height = scene->mTextures[t]->mHeight;
                i32 width = scene->mTextures[t]->mWidth;
                aiTexel *data = scene->mTextures[t]->pcData;

                Texture *newTex = new Texture();
                if(!newTex->loadTexture(opengl, texName, data, width, height))
                {
                    //return false;
                }

                std::string internalTexName = "*" + std::to_string(t);
                model->mTextures.insert({internalTexName, newTex});
            }
        }
        // ... yunk
        // ... yunk
        /* add a white texture in case there is no diffuse tex but colors */
        model->mWhiteTexture = new Texture();
        std::string whiteTexName = "src/samples/render_example/textures/white.png";
        if (!model->mWhiteTexture->loadTexture(opengl, whiteTexName)) {
            //Logger::log(1, "%s error: could not load white default texture '%s'\n", __FUNCTION__, whiteTexName.c_str());
            // Por que tendria que retornar? zii
            //return false;
        }

        /* add a placeholder texture in case there is no diffuse tex */
        model->mPlaceholderTexture = new Texture();
        std::string placeholderTexName = "src/samples/render_example/textures/missing_tex.png";
        if (!model->mPlaceholderTexture->loadTexture(opengl, placeholderTexName)) {
            //Logger::log(1, "%s error: could not load placeholder texture '%s'\n", __FUNCTION__, placeholderTexName.c_str());
            // Por que tendria que retornar? zii
            //return false;
        }

        aiNode* rootNode = scene->mRootNode;
        std::string rootNodeName = rootNode->mName.C_Str();
        model->mRootNode = createNode(rootNodeName);
        //Logger::log(1, "%s: root node name: '%s'\n", __FUNCTION__, rootNodeName.c_str());

        // TODO is this the texture directory?
        std::string assetDirectory = "E:/Mastering-Cpp-Game-Animation-Programming/chapter01/01_opengl_assimp/assets/woman";
        processNode(opengl, model, model->mRootNode, rootNode, scene, assetDirectory);

        //Logger::log(1, "%s: ... processing nodes finished...\n", __FUNCTION__);

        for (const auto& node : model->mNodeList) {
            std::string nodeName = node->nodeName;
            int x = 1;
            const auto boneIter = std::find_if(model->mBoneList.begin(), model->mBoneList.end(), [nodeName](Bone* bone) { return bone->mNodeName == nodeName; }); // a bone is a node!
            if (boneIter != model->mBoneList.end()) 
            {
                model->mBoneOffsetMatrices.insert({nodeName, model->mBoneList.at(std::distance(model->mBoneList.begin(), boneIter))->mOffsetMatrix});
            }
        }

        /* create vertex buffers for the meshes */
        for (const auto& mesh : model->mModelMeshes) {
            VertexIndexBuffer buffer;
            buffer.init(opengl);
            buffer.uploadData(opengl, mesh.vertices, mesh.indices);
            model->mVertexBuffers.emplace_back(buffer);
        }

        #if 0
        /* animations */
        unsigned int numAnims = scene->mNumAnimations;
        for (unsigned int i = 0; i < numAnims; ++i) {
            aiAnimation* animation = scene->mAnimations[i];

            Logger::log(1, "%s: -- animation clip %i has %i skeletal channels, %i mesh channels, and %i morph mesh channels\n",
            __FUNCTION__, i, animation->mNumChannels, animation->mNumMeshChannels, animation->mNumMorphMeshChannels);

            std::shared_ptr<AssimpAnimClip> animClip = std::make_shared<AssimpAnimClip>();
            animClip->addChannels(animation);
            if (animClip->getClipName().empty()) {
            animClip->setClipName(std::to_string(i));
            }
            mAnimClips.emplace_back(animClip);
        }

        mModelFilenamePath = modelFilename;
        mModelFilename = std::filesystem::path(modelFilename).filename().generic_string();
        #endif

        /* get root transformation matrix from model's root node */
        model->mRootTransformMatrix = convertAiToGLM(rootNode->mTransformation);

        //Logger::log(1, "%s: - model has a total of %i texture%s\n", __FUNCTION__, mTextures.size(), mTextures.size() == 1 ? "" : "s");
        //Logger::log(1, "%s: - model has a total of %i bone%s\n", __FUNCTION__, mBoneList.size(), mBoneList.size() == 1 ? "" : "s");
        //Logger::log(1, "%s: - model has a total of %i animation%s\n", __FUNCTION__, numAnims, numAnims == 1 ? "" : "s");

        //Logger::log(1, "%s: successfully loaded model '%s' (%s)\n", __FUNCTION__, modelFilename.c_str(), mModelFilename.c_str());
        return model;
    }