#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <iostream>
// fileysystem, sstream, fstream breaks the macro 'internal'

#include "bindings/opengl_bindings.cpp"
// thirdparty

/* TODO primero ver de sacar lo que esta en shader, capaz es el causante del internal
    y no glm. Seria ideal asi no tengo que renombrar todo ahora, y de paso tengo
    que dejar de usar la poronga de string igual asi que...

    primero migrar glfw, despues shader, despues chequear el internal
*/
//#define internal_linkage static
#define internal static
#define global_variable static
#define local_persist static
#define RAW_INPUT 1

// TODO get the core, i should only fix the assert macro! (just call it Assert)
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;
typedef uint32_t b32;
typedef u32 b32;

#define kb(value) (value * 1024)
#define mb(value) (1024 * kb(value))
#define gb(value) (1024 * mb(value))

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

// assimp
#include "AssimpLoader.h"

// TODO fix assert collision with windows.h or something else!

#define gui_assert(expr, fmt, ...) do {                                       \
    if (!(expr)) {                                                            \
        char _msg_buf[1024];                                                  \
                snprintf(_msg_buf, sizeof(_msg_buf),                                        \
            "Assertion failed!\n\n"                                                 \
            "File: %s\n"                                                            \
            "Line: %d\n"                                                            \
            "Condition: %s\n\n"                                                     \
            fmt,                                                                    \
            __FILE__, __LINE__, #expr, __VA_ARGS__);                                \
        MessageBoxA(0, _msg_buf, "Assertion Failed", MB_OK | MB_ICONERROR);   \
        __debugbreak();                                                       \
    }                                                                         \
} while (0)



//#include "base/base_core.h"
#include "base/base_arena.h"
#include "base/base_string.h"
#include "os/os_core.h"
#include "base/base_arena.cpp"
#include "base/base_string.cpp"
#include "os/os_core.cpp"
#include "input/input.h"
typedef Input GameInput;
#include "camera.h"

#define AIM_PROFILER
#include "aim_timer.h"
#include "aim_profiler.h"


#include "camera.cpp"
#include "aim_timer.cpp"
#include "aim_profiler.cpp"

#include "components.h"
using namespace aim::Components;

// TODO fix
//#include "logger/logger.h"
//#include "logger/logger.cpp"

glm::mat4 m_globalInverseTransform;
glm::mat4 todas_las_putas_transforms{};
glm::mat4 assault_rifle_transform{};

// should be local to the model
glm::mat4 manny_armature;
glm::mat4 mag_bone_transform;
std::vector<glm::mat4> manny_transforms;

// this `manny_world_transform` is the position of the manny in the world
glm::mat4 manny_world_transform{};

#define OpenGLSetFunction(name) (opengl->name) = OpenGLGetFunction(name);
#define OpenGLDeclareMemberFunction(name) OpenGLType_##name name

struct OpenGL
{
    i32 vsynch;
    OpenGLDeclareMemberFunction(wglCreateContextAttribsARB);
    OpenGLDeclareMemberFunction(wglGetExtensionsStringEXT);
    OpenGLDeclareMemberFunction(wglSwapIntervalEXT);
    OpenGLDeclareMemberFunction(wglGetSwapIntervalEXT);
    OpenGLDeclareMemberFunction(glGenVertexArrays);
    OpenGLDeclareMemberFunction(glBindVertexArray);
    OpenGLDeclareMemberFunction(glDeleteVertexArrays);

	OpenGLDeclareMemberFunction(glGenBuffers);
	OpenGLDeclareMemberFunction(glBindBuffer);
	OpenGLDeclareMemberFunction(glBufferData);
	OpenGLDeclareMemberFunction(glVertexAttribPointer);
	OpenGLDeclareMemberFunction(glVertexAttribIPointer);
	OpenGLDeclareMemberFunction(glEnableVertexAttribArray);

    OpenGLDeclareMemberFunction(glCreateShader);
    OpenGLDeclareMemberFunction(glCompileShader);
    OpenGLDeclareMemberFunction(glShaderSource);
    OpenGLDeclareMemberFunction(glCreateProgram);
    OpenGLDeclareMemberFunction(glAttachShader);
    OpenGLDeclareMemberFunction(glLinkProgram);
    OpenGLDeclareMemberFunction(glDeleteShader);
    OpenGLDeclareMemberFunction(glUseProgram);
    OpenGLDeclareMemberFunction(glGetShaderiv);
    OpenGLDeclareMemberFunction(glGetShaderInfoLog);
    OpenGLDeclareMemberFunction(glGetProgramiv);
    OpenGLDeclareMemberFunction(glGetProgramInfoLog);

    OpenGLDeclareMemberFunction(glUniform1i);
    OpenGLDeclareMemberFunction(glUniform1f);
    OpenGLDeclareMemberFunction(glUniform2fv);
    OpenGLDeclareMemberFunction(glUniform2f);
    OpenGLDeclareMemberFunction(glUniform3fv);
    OpenGLDeclareMemberFunction(glUniform3f);
    OpenGLDeclareMemberFunction(glUniform4fv);
    OpenGLDeclareMemberFunction(glUniform4f);
    OpenGLDeclareMemberFunction(glUniformMatrix2fv);
    OpenGLDeclareMemberFunction(glUniformMatrix3fv);
    OpenGLDeclareMemberFunction(glUniformMatrix4fv);
    OpenGLDeclareMemberFunction(glGetUniformLocation);
};

static Arena g_transient_arena;

#include "shader.h"

///////// TODO cleanup ////////////
// global state
// window size
global_variable u32 SRC_WIDTH = 1280;
global_variable u32 SRC_HEIGHT = 720;

// TODO: Removed these
// used during mouse callback
global_variable float lastX = SRC_WIDTH / 2.0f;
global_variable float lastY = SRC_HEIGHT / 2.0f;
global_variable bool firstMouse = true;

// lighting
glm::vec3 light_pos(1.2f, 1.0f, 2.0f);
glm::vec3 light_dir(-0.2f, -1.0f, -0.3f);
glm::vec3 light_scale(0.2f);

glm::vec3 light_ambient(0.2f, 0.2f, 0.2f);
glm::vec3 light_diffuse(0.5f, 0.5f, 0.5f); // darken diffuse light a bit
glm::vec3 light_specular(1.0f, 1.0f, 1.0f);

// rifle
glm::vec3 rifle_pos(-2.2f, 0.0f, 0.0f);
glm::vec3 rifle_rot(0.0f, 0.0f, 0.0f);
float rifle_scale(0.5f);

// manny
glm::vec3 manny_pos(-2.2f, 0.0f, 0.0f);
glm::vec3 manny_rot(0.0f, 0.0f, 0.0f);
float manny_scale(0.07f);

// model
glm::vec3 floor_pos(0.0f, 0.0f, -2.2f);
glm::vec3 floor_scale(30.0f, 1.0f, 30.0f);
glm::vec3 model_bounding_box(floor_scale * 1.0f); 

// model materials
glm::vec3 model_material_ambient(1.0f, 0.5f, 0.31f);
glm::vec3 model_material_diffuse(1.0f, 0.5f, 0.31f);
glm::vec3 model_material_specular(0.5f, 0.5f, 0.5f);
float model_material_shininess(32.0f);

static bool gui_mode = false;
static bool fps_mode = false;

Camera free_camera(FREE_CAMERA, glm::vec3(0.0f, 5.0f, 19.0f));
Camera fps_camera(FPS_CAMERA, glm::vec3(0.0f, 8.0f, 3.0f));

Camera& curr_camera = free_camera;
///////// TODO cleanup ////////////

struct MeshBox {
	Transform3D transform;
	MeshBox(Transform3D transform) {
		this->transform = transform;
	}
};


struct PointLight {
	Transform3D transform;

	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;

	float constant;
	float linear;
	float quadratic;
};

struct SpotLight {
	Transform3D transform;
	glm::vec3 direction;

	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;

	float constant;
	float linear;
	float quadratic;

	float cutOff;
	float outerCutOff;
};

struct DirectionalLight {
	glm::vec3 direction;

	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};

std::vector<Skeleton> skeletons;


struct SceneGraphNode {
	std::vector<AssimpNode*> assimp_nodes;
	//std::vector<AssimpAnimation> animations;
	std::string scene_name;

};

struct LoadedCollider {
	std::vector<AssimpVertex> vertices;
	std::vector<uint32_t> indices;
};

void processAssimpNode(OpenGL* opengl, aiNode* node, AssimpNode* parent, const aiScene* scene, SceneGraphNode& scene_graph_node);

struct SceneGraph {
	std::vector<SceneGraphNode> nodes;
	std::vector<LoadedCollider> collider_nodes;
	std::unordered_map<std::string, int> name_to_node_index;
	std::unordered_map<std::string, int> name_to_collider_node_index;

	int loadAssimp(OpenGL *opengl, AssimpNode* assimp_model, std::string path) {
		Assimp::Importer import;
		const aiScene * scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			//AIM_FATAL("ERROR::ASSIMP::%s\n", import.GetErrorString());
			printf("ERROR::ASSIMP::%s\n", import.GetErrorString());
			abort();
		}
		SceneGraphNode scene_graph_node;
		scene_graph_node.scene_name = scene->GetShortFilename(path.c_str());

        // TODO(anim): see why this is useful and what happens when its not the identity. In this case it seems that the model is setup in a way that
        // this is always the case
		m_globalInverseTransform = glm::inverse(AssimpGLMHelpers::ConvertMatrixToGLMFormat(scene->mRootNode->mTransformation));
		m_globalInverseTransform = glm::mat4(1.0f);


		processAssimpNode(opengl, scene->mRootNode, nullptr, scene, scene_graph_node);

		this->nodes.push_back(scene_graph_node);
		int node_index{};
		if (this->name_to_node_index.find(scene_graph_node.scene_name) == this->name_to_node_index.end()) {
			node_index = this->name_to_node_index.size() - 1;
			this->name_to_node_index[scene_graph_node.scene_name] = node_index;
		}
		else {
			abort();
			node_index = this->name_to_node_index[scene_graph_node.scene_name];
		}
		return node_index;
	}
};


#pragma region bones_container
/* Container for bone data */

struct KeyPosition
{
	glm::vec3 position;
	float timeStamp;
};

struct KeyRotation
{
	glm::quat orientation;
	float timeStamp;
};

struct KeyScale
{
	glm::vec3 scale;
	float timeStamp;
};

class Bone
{
public:
	Bone(const std::string& name, int ID, const aiNodeAnim* channel)
		:
		m_Name(name),
		m_ID(ID),
		m_LocalTransform(1.0f)
	{
		m_NumPositions = channel->mNumPositionKeys;

		for (int positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex)
		{
			aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
			float timeStamp = channel->mPositionKeys[positionIndex].mTime;
			KeyPosition data;
			data.position = AssimpGLMHelpers::GetGLMVec(aiPosition);
			data.timeStamp = timeStamp;
			m_Positions.push_back(data);
		}

		m_NumRotations = channel->mNumRotationKeys;
		for (int rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex)
		{
			aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
			float timeStamp = channel->mRotationKeys[rotationIndex].mTime;
			KeyRotation data;
			data.orientation = AssimpGLMHelpers::GetGLMQuat(aiOrientation);
			data.timeStamp = timeStamp;
			m_Rotations.push_back(data);
		}

		m_NumScalings = channel->mNumScalingKeys;
		for (int keyIndex = 0; keyIndex < m_NumScalings; ++keyIndex)
		{
			aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
			float timeStamp = channel->mScalingKeys[keyIndex].mTime;
			KeyScale data;
			data.scale = AssimpGLMHelpers::GetGLMVec(scale);
			data.timeStamp = timeStamp;
			m_Scales.push_back(data);
		}
	}

	void Update(float animationTime)
	{
		glm::mat4 translation = InterpolatePosition(animationTime);
		glm::mat4 rotation = InterpolateRotation(animationTime);
		glm::mat4 scale = InterpolateScaling(animationTime);
		m_LocalTransform = translation * rotation * scale;
	}
	glm::mat4 GetLocalTransform() { return m_LocalTransform; }
	std::string GetBoneName() const { return m_Name; }
	int GetBoneID() { return m_ID; }



	int GetPositionIndex(float animationTime)
	{
		for (int index = 0; index < m_NumPositions - 1; ++index)
		{
			if (animationTime < m_Positions[index + 1].timeStamp)
				return index;
		}
        return 1;
		assert(0);
	}

	int GetRotationIndex(float animationTime)
	{
		for (int index = 0; index < m_NumRotations - 1; ++index)
		{
			if (animationTime < m_Rotations[index + 1].timeStamp)
				return index;
		}
        return 1;
		assert(0);
	}

	int GetScaleIndex(float animationTime)
	{
		for (int index = 0; index < m_NumScalings - 1; ++index)
		{
			if (animationTime < m_Scales[index + 1].timeStamp)
				return index;
		}
        return 1;
		assert(0);
	}


private:

	float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
	{
		float scaleFactor = 0.0f;
		float midWayLength = animationTime - lastTimeStamp;
		float framesDiff = nextTimeStamp - lastTimeStamp;
		scaleFactor = midWayLength / framesDiff;
		return scaleFactor;
	}

	glm::mat4 InterpolatePosition(float animationTime)
	{
		if (1 == m_NumPositions)
			return glm::translate(glm::mat4(1.0f), m_Positions[0].position);

		int p0Index = GetPositionIndex(animationTime);
		int p1Index = p0Index + 1;
		float scaleFactor = GetScaleFactor(m_Positions[p0Index].timeStamp,
			m_Positions[p1Index].timeStamp, animationTime);
		glm::vec3 finalPosition = glm::mix(m_Positions[p0Index].position, m_Positions[p1Index].position
			, scaleFactor);
		return glm::translate(glm::mat4(1.0f), finalPosition);
	}

	glm::mat4 InterpolateRotation(float animationTime)
	{
		if (1 == m_NumRotations)
		{
			auto rotation = glm::normalize(m_Rotations[0].orientation);
			return glm::toMat4(rotation);
		}

		int p0Index = GetRotationIndex(animationTime);
		int p1Index = p0Index + 1;
		float scaleFactor = GetScaleFactor(m_Rotations[p0Index].timeStamp,
			m_Rotations[p1Index].timeStamp, animationTime);
		glm::quat finalRotation = glm::slerp(m_Rotations[p0Index].orientation, m_Rotations[p1Index].orientation
			, scaleFactor);
		finalRotation = glm::normalize(finalRotation);
		return glm::toMat4(finalRotation);

	}

	glm::mat4 InterpolateScaling(float animationTime)
	{
		if (1 == m_NumScalings)
			return glm::scale(glm::mat4(1.0f), m_Scales[0].scale);

		int p0Index = GetScaleIndex(animationTime);
		int p1Index = p0Index + 1;
		float scaleFactor = GetScaleFactor(m_Scales[p0Index].timeStamp,
			m_Scales[p1Index].timeStamp, animationTime);
		glm::vec3 finalScale = glm::mix(m_Scales[p0Index].scale, m_Scales[p1Index].scale
			, scaleFactor);
		return glm::scale(glm::mat4(1.0f), finalScale);
	}

	std::vector<KeyPosition> m_Positions;
	std::vector<KeyRotation> m_Rotations;
	std::vector<KeyScale> m_Scales;
	int m_NumPositions;
	int m_NumRotations;
	int m_NumScalings;

	glm::mat4 m_LocalTransform;
	std::string m_Name;
	int m_ID;
};

internal b32 animationActive = true;
Bone* root_node_bone = nullptr;
Bone* root = nullptr;
Bone* ik_hand_root = nullptr;
Bone* ik_hand_gun_bone = nullptr;

struct Entity {
	Transform3D transform;

	// TODO: Check this because I've seen enums used...
	bool has_animations;

	// TODO: Check this because I've seen enums used...
	bool has_model;
	uint32_t models_count;
	AssimpNode** models;
};


#pragma endregion bones_container

#pragma region assimp_animation

struct AssimpNodeData
{
	glm::mat4 transformation;
	std::string name;
	int childrenCount;
	std::vector<AssimpNodeData> children;
};

class Animation
{
public:
	Animation() = default;

	Animation(const std::string& animationPath, int skeleton_index)
	{
		Assimp::Importer importer;
		m_skeleton_index = skeleton_index;
		const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);
		assert(scene && scene->mRootNode);
		auto animation = scene->mAnimations[0];
		m_Duration = animation->mDuration;
		m_TicksPerSecond = animation->mTicksPerSecond;
		//m_TicksPerSecond = 1000;

		// esto podria apuntar al nodo en la lista de nodes. Esto va a pasar por
		// RootNode -> Armature -> ...
		ReadHeirarchyData(m_RootNode, scene->mRootNode);

		//m_globalInverseTransform = glm::inverse(AssimpGLMHelpers::ConvertMatrixToGLMFormat(scene->mRootNode->mTransformation));
		// esto se puede quedar
		ReadMissingBones(animation);

		std::cout << "Animation name is: " << scene->mAnimations[0]->mName.C_Str() << std::endl;
	}

	~Animation()
	{
	}

	// this is my version
	Bone* FindBone(const std::string& name)
	{
		auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
			[&](const Bone& Bone)
			{
				return Bone.GetBoneName() == name;
			}
		);
		if (iter == m_Bones.end()) return nullptr;
		else return &(*iter);
	}

	//Bone* FindBone(const std::string& name)
	//{
	//	auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
	//		[&](const Bone& Bone)
	//		{
	//			return Bone.GetBoneName() == name;
	//		}
	//	);
	//	if (iter == m_Bones.end()) return nullptr;
	//	else return &(*iter);
	//}


	inline float GetTicksPerSecond() { return m_TicksPerSecond; }

	inline float GetDuration() { return m_Duration; }

	inline const AssimpNodeData& GetRootNode() { return m_RootNode; }

	//inline const std::map<std::string, AssimpBoneInfo>& GetBoneIDMap()
	//{
	//	return m_BoneInfoMap;
	//}

private:
	void ReadMissingBones(const aiAnimation* animation)
	{
		int size = animation->mNumChannels;
		//reading channels(bones engaged in an animation and their keyframes)
		for (int i = 0; i < size; i++)
		{
			auto channel = animation->mChannels[i];
			std::string boneName = channel->mNodeName.data;

			if (skeletons[m_skeleton_index].m_BoneInfoMap.find(boneName) == skeletons[m_skeleton_index].m_BoneInfoMap.end())
			{
				skeletons[m_skeleton_index].m_BoneInfoMap[boneName].id = skeletons[m_skeleton_index].m_BoneCounter;
				skeletons[m_skeleton_index].m_BoneCounter++;
			}
			m_Bones.push_back(Bone(channel->mNodeName.data,
				skeletons[m_skeleton_index].m_BoneInfoMap[channel->mNodeName.data].id, channel));
		}
	}

	// esto lee toda la jerarquia de la animacion, no solo el skeleton
	void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src)
	{
		assert(src);

		dest.name = src->mName.data;
		dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
		dest.childrenCount = src->mNumChildren;

		//if (m_BoneInfoMap.find(dest.name) != m_BoneInfoMap.end()) {
		//	std::cout << dest.name << std::endl;
		//	std::cout << m_BoneInfoMap[dest.name].id << std::endl;
		//}
		for (int i = 0; i < src->mNumChildren; i++)
		{
			AssimpNodeData newData;
			ReadHeirarchyData(newData, src->mChildren[i]);
			dest.children.push_back(newData);
		}
	}

	float m_Duration;
	int m_TicksPerSecond;
public:
	// todo deal with this. Prolly erase it
	std::vector<Bone> m_Bones;
	AssimpNodeData m_RootNode;
public:
	int m_skeleton_index;
};
#pragma endregion assimp_animation

#pragma region assimp_animator

class Animator
{
public:
	Animator(Animation* animation)
	{
		m_CurrentTime = 0.0;
		m_CurrentAnimation = animation;

		m_FinalBoneMatrices.reserve(200);

		for (int i = 0; i < 200; i++)
			m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
	}

	void UpdateAnimation(float dt)
	{
		m_DeltaTime = dt;
		if (m_CurrentAnimation)
		{
			m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
			m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
			CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
		}
	}

	void PlayAnimation(Animation* pAnimation)
	{
		m_CurrentAnimation = pAnimation;
		m_CurrentTime = 0.0f;
	}

	void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform)
	{
		std::string nodeName = node->name;
		// todo check this throughly
		// it could matter that node->transformation is useful if i have something like this:
		/*
			Node
			  MeshNode
			  MeshNode
				Meshnode
				  Bone
					Bone1
					Bone2
					Bone3
					  BoneChild1...
		*/

		glm::mat4 nodeTransform;
		glm::mat4 globalTransformation;

		Bone* Bone = m_CurrentAnimation->FindBone(nodeName);

		// armature has a transform
		// me parece que es el que mueve la armature de lugar inicialmente. probar comentando esto si es el Bone->GetBoneName() == "Armature" 
		// entonces identity// Si era eso. 

		if (Bone) {
			Bone->Update(m_CurrentTime);
			nodeTransform = Bone->GetLocalTransform();
			globalTransformation = parentTransform * nodeTransform;

			int index = skeletons[m_CurrentAnimation->m_skeleton_index].m_BoneInfoMap[nodeName].id;
			glm::mat4 offset = skeletons[m_CurrentAnimation->m_skeleton_index].m_BoneInfoMap[nodeName].offset;
			m_FinalBoneMatrices[index] = m_globalInverseTransform * globalTransformation * offset;
		}
		else {
			nodeTransform = node->transformation;
			globalTransformation = parentTransform * nodeTransform;
		}

		for (int i = 0; i < node->childrenCount; i++)
			CalculateBoneTransform(&node->children[i], globalTransformation);
	}

	std::vector<glm::mat4> GetFinalBoneMatrices()
	{
		return m_FinalBoneMatrices;
	}

private:
	float m_CurrentTime;
	float m_DeltaTime;
public:
	std::vector<glm::mat4> m_FinalBoneMatrices;
	Animation* m_CurrentAnimation;

};
#pragma endregion assimp_animator
void processAssimpNode(OpenGL* opengl, aiNode* node, AssimpNode* parent, const aiScene* scene, SceneGraphNode& scene_graph_node) {
	AssimpNode* new_node = new AssimpNode{};
	new_node->parent = parent;
	new_node->name = std::string(node->mName.C_Str());
	if (new_node->name == "RootNode") {
		new_node->name = scene_graph_node.scene_name;
	}
	new_node->transform = AssimpGLMHelpers::ConvertMatrixToGLMFormat(node->mTransformation);

	for (int i = 0; i < node->mNumChildren; i++) {
		processAssimpNode(opengl, node->mChildren[i], new_node, scene, scene_graph_node);
	}

	std::cout << "Node name: " << new_node->name << std::endl;

	if (node->mNumMeshes > 0) {
		AssimpMesh* new_mesh = new AssimpMesh{};
		for (int i = 0; i < node->mNumMeshes; i++) {
			// Si o si tienen que estar los dos meshes.
			// El problema de esto es que se repite el skeleton por algun motivo. Como puedo saber si el skeleton ya existe?
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			AssimpPrimitive* new_primitive = new AssimpPrimitive{};
			bool has_skin = mesh->HasBones();

			std::cout << "Mesh name: " << std::string(mesh->mName.C_Str()) << std::endl;
			BoneData* vertex_id_to_bone_id = nullptr;
			if (has_skin) {
				Skeleton new_skeleton = Skeleton{};
				vertex_id_to_bone_id = new BoneData[mesh->mNumVertices]{};
				//AIM_DEBUG("Processing Bones:\n");
				printf("Processing Bones:\n");
				for (size_t i = 0; i < mesh->mNumBones; i++) {
					std::string boneName = mesh->mBones[i]->mName.C_Str();
					int boneId = -1;
					if (new_skeleton.m_BoneInfoMap.find(boneName) == new_skeleton.m_BoneInfoMap.end()) {
						AssimpBoneInfo boneInfo;
						boneInfo.id = new_skeleton.m_BoneCounter;
						boneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[i]->mOffsetMatrix);
						new_skeleton.m_BoneInfoMap[boneName] = boneInfo;
						boneId = new_skeleton.m_BoneCounter;
						new_skeleton.m_BoneCounter++;
					}
					else {
						boneId = new_skeleton.m_BoneInfoMap[boneName].id;
						//abort();
						std::cout << "hola";
					}
					assert(boneId != -1);
					for (size_t j = 0; j < mesh->mBones[i]->mNumWeights; j++) {
						uint32_t vertex_id = mesh->mBones[i]->mWeights[j].mVertexId;
						float weight_value = mesh->mBones[i]->mWeights[j].mWeight;

						if (vertex_id_to_bone_id[vertex_id].count < MAX_NUM_BONES_PER_VERTEX) {
							vertex_id_to_bone_id[vertex_id].joints[vertex_id_to_bone_id[vertex_id].count] = boneId;
							vertex_id_to_bone_id[vertex_id].weights[vertex_id_to_bone_id[vertex_id].count] = weight_value;
							vertex_id_to_bone_id[vertex_id].count++;
						}
					}
				}
				skeletons.push_back(new_skeleton);
			}
			printf("Processing Mesh: %s", mesh->mName.C_Str());

			printf("Processing Vertices:");
			printf("\tVertices count: % d", mesh->mNumVertices);
			//AIM_DEBUG("Processing Mesh: %s", mesh->mName.C_Str());

			//AIM_DEBUG("Processing Vertices:");
			//AIM_DEBUG("\tVertices count: % d", mesh->mNumVertices);

			std::vector<AssimpVertex> assimp_vertices;
			assimp_vertices.reserve(mesh->mNumVertices);
			for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			{
				AssimpVertex vertex;
				glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
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
				vertex.aTexCoords = glm::vec2(0.0f, 0.0f);
				if (has_skin) {
					//vertex.joints = glm::make_vec4(vertex_id_to_bone_id[i].joints.data());
					//vertex.weights = glm::make_vec4(vertex_id_to_bone_id[i].weights.data());
					vertex.joints[0] = vertex_id_to_bone_id[i].joints[0];
					vertex.joints[1] = vertex_id_to_bone_id[i].joints[1];
					vertex.joints[2] = vertex_id_to_bone_id[i].joints[2];
					vertex.joints[3] = vertex_id_to_bone_id[i].joints[3];

					vertex.weights[0] = vertex_id_to_bone_id[i].weights[0];
					vertex.weights[1] = vertex_id_to_bone_id[i].weights[1];
					vertex.weights[2] = vertex_id_to_bone_id[i].weights[2];
					vertex.weights[3] = vertex_id_to_bone_id[i].weights[3];
				}
				else {
					//vertex.joints = glm::vec4(0.0f);

					vertex.joints[0] = 0;
					vertex.joints[1] = 0;
					vertex.joints[2] = 0;
					vertex.joints[3] = 0;

					//vertex.weights = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
					vertex.weights[0] = 1.0f;
					vertex.weights[1] = 0.0f;
					vertex.weights[2] = 0.0f;
					vertex.weights[3] = 0.0f;

				}
				// this is probably not needed

				assimp_vertices.push_back(vertex);
			}


			//AIM_DEBUG("Processing Indices...\n");
			printf("Processing Indices...\n");
			std::vector<uint32_t> assimp_indices;
			for (unsigned int i = 0; i < mesh->mNumFaces; i++)
			{
				aiFace face = mesh->mFaces[i];
				for (unsigned int j = 0; j < face.mNumIndices; j++) {
					if (face.mNumIndices != 3) abort();
					assimp_indices.push_back(face.mIndices[j]);
				}
			}

			new_primitive->index_count = assimp_indices.size();
			//AIM_DEBUG("There are %d indices\n", new_primitive->index_count);
			printf("There are %d indices\n", new_primitive->index_count);


			opengl->glGenVertexArrays(1, &new_primitive->vao);
			opengl->glBindVertexArray(new_primitive->vao);

			opengl->glGenBuffers(1, &new_primitive->vbo);
			opengl->glBindBuffer(GL_ARRAY_BUFFER, new_primitive->vbo);
			opengl->glBufferData(GL_ARRAY_BUFFER, assimp_vertices.size() * sizeof(AssimpVertex), assimp_vertices.data(), GL_STATIC_DRAW);

			opengl->glGenBuffers(1, &new_primitive->ebo);
			opengl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, new_primitive->ebo);
			opengl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, assimp_indices.size() * sizeof(uint32_t), assimp_indices.data(), GL_STATIC_DRAW);

			opengl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AssimpVertex), (void*)offsetof(AssimpVertex, position));
			opengl->glEnableVertexAttribArray(0);
			opengl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AssimpVertex), (void*)offsetof(AssimpVertex, normal));
			opengl->glEnableVertexAttribArray(1);
			opengl->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(AssimpVertex), (void*)offsetof(AssimpVertex, aTexCoords));
			opengl->glEnableVertexAttribArray(2);

			if (has_skin) {
				opengl->glVertexAttribIPointer(3, 4, GL_UNSIGNED_INT, sizeof(AssimpVertex), (void*)offsetof(AssimpVertex, joints));
				//glVertexAttribPointer(3, 4, GL_INT, GL_FALSE, sizeof(AssimpVertex), (void*)offsetof(AssimpVertex, joints));
				opengl->glEnableVertexAttribArray(3);
				opengl->glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(AssimpVertex), (void*)offsetof(AssimpVertex, weights));
				opengl->glEnableVertexAttribArray(4);

				delete[] vertex_id_to_bone_id;
			}
			opengl->glBindVertexArray(0);

			//for (size_t i = 0; i < assimp_vertices.size(); i++) {
				//std::cout << "Vertex: " << i << std::endl;
				//std::cout << "\t joints: [" << assimp_vertices[i].joints[0] << ", " << assimp_vertices[i].joints[1] << ", " << assimp_vertices[i].joints[2] << ", " << assimp_vertices[i].joints[3] << "]";
				//std::cout << "\t weights: [" << assimp_vertices[i].weights[0] << ", " << assimp_vertices[i].weights[1] << ", " << assimp_vertices[i].weights[2] << ", " << assimp_vertices[i].weights[3] << "]";
			//}

			new_mesh->meshes.push_back(new_primitive);
		}
		new_node->mesh = new_mesh;
	}
	if (parent) {
		parent->children.push_back(new_node);
		std::string parent_name = new_node->parent ? new_node->parent->name : std::string("NULL");
		std::cout << "NODE NAME: " << new_node->name << "  NODE PARENT: " << parent_name << std::endl;
	}
	else {
		scene_graph_node.assimp_nodes.push_back(new_node);
	}
	if (new_node->name == "ROOT") {
		std::cout << "IM ROOT" << std::endl;
		for (size_t i = 0; i < new_node->children.size(); i++) {
			std::cout << "CHILDREN " << i << "  CHILDREN NAME: " << new_node->children[i]->name << std::endl;

		}
	}
}


void render_player(OpenGL *opengl, Entity* player, Shader shader) {
	for (uint32_t idx = 0; idx < player->models_count; idx++) {
		AssimpNode* player_model = player->models[idx];
		std::vector<AssimpPrimitive*> meshes = player_model->mesh->meshes;
		std::string name = player_model->name;
		auto node_transform = player_model->transform;
		for (const auto& mesh : meshes) {
			opengl->glBindVertexArray(mesh->vao);

			shader_use(shader);
			shader_set_mat4(shader, "model", glm::mat4(1.0f));

			if (name == "SM_AssaultRifle_Magazine") {
				opengl->glUniform1i(opengl->glGetUniformLocation(shader.id, "jointCount"), 0);
                // NOTE(anim): The position of the magazine is given by the transform of the mag bone + transform of grip bone + the transform of the assault rifle
                // and it seems the grip is the identity in this case, as its centered around the origin in blender.
				opengl->glUniformMatrix4fv(opengl->glGetUniformLocation(shader.id, "nodeMatrix"), 1, GL_FALSE, &(assault_rifle_transform * mag_bone_transform)[0][0]);
			}

			if (name == "SM_AssaultRifle_Casing") {
				glm::mat4 base_model_mat =
					glm::translate(glm::mat4(1.0f), glm::vec3(30.0f, 3.0f, 0.0f)) *
					glm::scale(glm::mat4(1.0f), glm::vec3(0.0125f));

			    shader_set_mat4(shader, "model", base_model_mat);
			}

			if (name == "SK_AssaultRifle") {
				opengl->glUniform1i(opengl->glGetUniformLocation(shader.id, "jointCount"), 0);
                // NOTE(anim) the rifle shouldn't move by itself (by having a model matrix for example). What moves the rifle is the changes in the bones 
                // Well maybe it may move by itself if you drop it or change it, but not in this design
				assault_rifle_transform = manny_world_transform * todas_las_putas_transforms;
				opengl->glUniformMatrix4fv(opengl->glGetUniformLocation(shader.id, "nodeMatrix"), 1, GL_FALSE, &assault_rifle_transform[0][0]);
			}

			if (name == "SK_Manny_Arms") {
				// TODO: this should be embeded inside node->transform. So:
				// if (node->name == "SK_Manny_Arms") node->transform = glm::mat4_cast(correction_rot) * node->transform;
				glm::quat qx = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				glm::quat qy = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				glm::quat qz = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				glm::quat correction_rot = qy * qx * qz; // Specify order of rotations here

				glm::quat qx1 = glm::angleAxis(glm::radians(manny_rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
				glm::quat qy1 = glm::angleAxis(glm::radians(manny_rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
				glm::quat qz1 = glm::angleAxis(glm::radians(manny_rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
				glm::quat rot1 = qy1 * qx1 * qz1; // Specify order of rotations here

				glm::mat4 model = glm::translate(glm::mat4(1.0f), manny_pos)
					* glm::mat4_cast(rot1)
					* glm::scale(glm::mat4(1.0f), glm::vec3(manny_scale));

				//skinning_shader->setMat4("model", model);
				shader_set_mat4(shader, "model", model);
				manny_world_transform = model * glm::mat4_cast(correction_rot) * node_transform;
				for (int i = 0; i < manny_transforms.size(); ++i)
				{
                    std::string mat_name = "jointMatrices[" + std::to_string(i) + "]";
					shader_set_mat4(shader, mat_name.c_str(), manny_transforms[i]);
				}
				opengl->glUniform1i(opengl->glGetUniformLocation(shader.id, "jointCount"), 1);
				opengl->glUniformMatrix4fv(opengl->glGetUniformLocation(shader.id, "nodeMatrix"), 1, GL_FALSE, glm::value_ptr(glm::mat4_cast(correction_rot) * node_transform));
			}


			// TODO ESTO DE CAMBIAR EL TRANSFORM DE ACA NO SIRVE, TIENE QUE EDITARSE A NIVEL PADRE DEL OBJETO. PORQUE PASA QUE PARA OBJETOS
			// COMO EL PORSCHE, NO SE COMO SE LLAMA EL PADRE

			glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
			opengl->glUniform1i(opengl->glGetUniformLocation(shader.id, "jointCount"), 0);
			opengl->glBindVertexArray(0);
			//break;
			// NOTE: by brekaing here i can see that the SK_Manny_Arms is composed of effectively two meshes and
			// this is not just two meshes duplicated as i initially thought. This theory only applied to SK_MannyArms as
			// the other models only have one mesh

			// INVESTIGATE:
			// But... do i really need the two meshes for the skinning? Or I only need one of these meshes?
			// In other words: the skinning information is present of both meshes or in only one? Or maybe I need both
			// meshes's skinning information

			// TODO: What I should do is: I can use ONLY the skinning info of the first mesh, but render both
		}

		// IMPORTANT los huesos que se cargan en las animaciones no estan en el scene graph. eso es importante


		// Armature == SKEL_AssaultRifle == SK_AssaultRifle (Mesh)
		// tengo que ver como es que se updatea el hueso con el mesh. esto funciona en las animaciones si
		// Si muevo el grip todo se tiene que mover, entonces si aplico... TODO: mover el mesh y el grip lo mismo, el grip moverlo con un glm::translate etc
	}
}



#if 0
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	// Note: width and height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
	// im not updating the perspective camera correctly using the new properties of the window. It only works because the aspect ratio remains the same in my
	// normal usecase
}

void keyboard_callback(GLFWwindow* window, int key, int scan_code, int action, int mods) {
	u8 pressed = ((action == GLFW_PRESS || action == GLFW_REPEAT) == 1);
	game_input->keyboard_state.curr_state.keys[Keys(key)] = pressed;
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;


	curr_camera.process_mouse_movement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	curr_camera.process_mouse_scroll(static_cast<float>(yoffset));
}
#endif
global_variable OS_Window global_w32_window;
global_variable u32 os_modifiers;
global_variable GameInput global_input;
void win32_process_pending_msgs() {
    MSG Message;
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&Message);
        switch(Message.message)
        {
            case WM_QUIT:
            {
                global_w32_window.is_running = false;
            } break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                WPARAM wparam = Message.wParam;
                LPARAM lparam = Message.lParam;

                u32 key = (u32)wparam;
                u32 alt_key_is_pressed = (lparam & (1 << 29)) != 0;
                u32 was_pressed = (lparam & (1 << 30)) != 0;
                u32 is_pressed = (lparam & (1 << 31)) == 0;
                global_input.curr_keyboard_state.keys[key] = u8(is_pressed);
                global_input.prev_keyboard_state.keys[key] = u8(was_pressed);
                if (GetKeyState(VK_SHIFT) & (1 << 15)) {
                    os_modifiers |= OS_Modifiers_Shift;
                }
                // NOTE reason behin this is that every key will be an event and each event knows what are the modifiers pressed when they key was processed
                // It the key is a modifier as well then the modifiers of this key wont containt itself. Meaning if i press Shift and i consult the modifiers pressed
                // for this key, shift will not be set
                if (GetKeyState(VK_CONTROL) & (1 << 15)) {
                    os_modifiers |= OS_Modifiers_Ctrl;
                }
                if (GetKeyState(VK_MENU) & (1 << 15)) {
                    os_modifiers |= OS_Modifiers_Alt;
                }
                if (key == Keys_Shift && os_modifiers & OS_Modifiers_Shift) {
                    os_modifiers &= ~OS_Modifiers_Shift;
                }
                if (key == Keys_Control && os_modifiers & OS_Modifiers_Ctrl) {
                    os_modifiers &= ~OS_Modifiers_Ctrl;
                }
                if (key == Keys_Alt && os_modifiers & OS_Modifiers_Alt) {
                    os_modifiers &= ~OS_Modifiers_Alt;
                }
            } break;

            case WM_MOUSEMOVE:
            {
                i32 xPos = (Message.lParam & 0x0000FFFF); 
                i32 yPos = ((Message.lParam & 0xFFFF0000) >> 16); 
                if(xPos >= 0 && xPos < SRC_WIDTH && yPos >= 0 && yPos < SRC_HEIGHT)
                {
                }
                // comente esto
                //SetCapture(global_w32_window.handle);

                i32 xxPos = LOWORD(Message.lParam);
                i32 yyPos = HIWORD(Message.lParam);
                char buf[100];
                sprintf(buf,  "MOUSE MOVE: x: %d, y: %d\n", xPos, yPos);
                //printf(buf);

                assert((xxPos == xPos && yyPos == yPos));
                global_input.curr_mouse_state.x = xPos;
                global_input.curr_mouse_state.y = yPos;
                
                #if !defined(RAW_INPUT)
                global_input.dx = global_input.curr_mouse_state.x - global_input.prev_mouse_state.x;
                global_input.dy = -(global_input.curr_mouse_state.y - global_input.prev_mouse_state.y);
                #endif

            }
            break;
            #if RAW_INPUT
            case WM_INPUT: {
                UINT size;
                GetRawInputData((HRAWINPUT)Message.lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));

                gui_assert(size < 1000, "GetRawInputData surpassed 1000 bytes");
                u8 bytes[1000];
                RAWINPUT* raw = (RAWINPUT*)bytes;
                GetRawInputData((HRAWINPUT)Message.lParam, RID_INPUT, raw, &size, sizeof(RAWINPUTHEADER));
                if (!(raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
                {
                    if (raw->header.dwType == RIM_TYPEMOUSE) 
                    {
                        LONG dx = raw->data.mouse.lLastX;
                        LONG dy = raw->data.mouse.lLastY;
                        global_input.dx = f32(dx);
                        global_input.dy = f32(-dy);
                    }
                }else
                {
                    gui_assert(1 < 0, "MOUSE_MOVE_ABSOLUTE");
                }
                POINT center = { LONG(SRC_WIDTH)/2, LONG(SRC_HEIGHT)/2 };
                ClientToScreen(global_w32_window.handle, &center);
                SetCursorPos(center.x, center.y);
                //printf("RECENTERING!!!\n");

            } break;
            #endif
            case WM_LBUTTONUP:
            {
                global_input.curr_mouse_state.button[MouseButtons_LeftClick] = 0;

            } break;
            case WM_MBUTTONUP:
            {
                global_input.curr_mouse_state.button[MouseButtons_MiddleClick] = 0;
            } break;
            case WM_RBUTTONUP:
            {
                global_input.curr_mouse_state.button[MouseButtons_RightClick] = 0;
            } break;

            case WM_LBUTTONDOWN:
            {
                global_input.curr_mouse_state.button[MouseButtons_LeftClick] = 1;

            } break;
            case WM_MBUTTONDOWN:
            {
                global_input.curr_mouse_state.button[MouseButtons_MiddleClick] = 1;
            } break;
            case WM_RBUTTONDOWN:
            {
                global_input.curr_mouse_state.button[MouseButtons_RightClick] = 1;
            } break;
            default:
            {
                DispatchMessageA(&Message);
            } break;
        }
    }
    #if RAW_INPUT
    if (global_w32_window.focused)
    {
        if (global_input.prev_mouse_state.x != SRC_WIDTH / 2 ||
            global_input.prev_mouse_state.y != SRC_HEIGHT / 2)
        {
            POINT pos = {i32(SRC_WIDTH / 2), i32(SRC_HEIGHT / 2)};
            ClientToScreen(global_w32_window.handle, &pos);
            SetCursorPos(pos.x, pos.y);
            //printf("RECENTERING!!!\n");
        }
    }
    #endif
}

LRESULT CALLBACK win32_main_callback(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    switch(Message)
    {
        case WM_DESTROY:
        {
            global_w32_window.is_running = false;
        } break;
        case WM_SIZE:
        {
            // os get window dimensions
        RECT r;
        GetClientRect(global_w32_window.handle, &r);
        u32 width = r.right - r.left;
        u32 height = r.bottom - r.top;
        if (width != SRC_WIDTH || height != SRC_HEIGHT)
        {
            //SRC_WIDTH = width;
            //SRC_HEIGHT = height;
            glViewport(0, 0, width, height);
        }
        }break;
        #if RAW_INPUT
        case WM_KILLFOCUS:
        {
            global_w32_window.focused = false;
            //while( ShowCursor(TRUE) < 0 );
            printf("KILL FOCUS\n");
            RAWINPUTDEVICE rid = { 0x01, 0x02, RIDEV_REMOVE, NULL };

            if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
            {
                assert('wtf');
            }
            ClipCursor(NULL);
            ReleaseCapture();
            ShowCursor(1);
        }break;
        case WM_SETFOCUS:
        {
            global_w32_window.focused = true;
            ShowCursor(0);
            //while( ShowCursor(FALSE) >= 0 );
            //POINT center = { LONG(SRC_WIDTH)/2, LONG(SRC_HEIGHT)/2 };
            //ClientToScreen(global_w32_window.handle, &center);
            //SetCursorPos(center.x, center.y);

            printf("SET FOCUS\n");
            RAWINPUTDEVICE rid = {
                .usUsagePage = 0x01,                   // generic desktop controls
                .usUsage     = 0x02,                   // mouse
                //.dwFlags     = RIDEV_INPUTSINK       // receive even when not focused
                //            | RIDEV_NOLEGACY,      // no WM_MOUSEMOVE fallback
                .dwFlags = 0,
                .hwndTarget  = global_w32_window.handle,
            };

            if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
                // failure!
            }
            //RECT clipRect;
            //GetClientRect(Window, &clipRect);
            //ClientToScreen(Window, (POINT*) &clipRect.left);
            //ClientToScreen(Window, (POINT*) &clipRect.right);
            //ClipCursor(&clipRect);

            //RECT ja;
            //GetWindowRect(Window, &ja);
            //ClipCursor(&ja);
        }break;
        #endif
        //case WM_PAINT: {
        //    PAINTSTRUCT Paint;
        //    HDC DeviceContext = BeginPaint(Window, &Paint);
        //    OS_Window_Dimension Dimension = os_win32_get_window_dimension(Window);
        //    os_win32_display_buffer(DeviceContext, &global_pixel_buffer,
        //                               Dimension.width, Dimension.height);
        //    EndPaint(Window, &Paint);
        //}break;
        default:
        {
            result = DefWindowProcA(Window, Message, wParam, lParam);
        } break;
    }
    return result;
}

int manny_count = 0;
int first_manny_index = 0;
void find_player_model_aux(AssimpNode* node, AssimpNode** player_models, uint32_t** player_models_cnt) {
	if (node->mesh) {
		std::string name = node->name;
		if (name == "SM_AssaultRifle_Magazine" ||
			name == "SM_AssaultRifle_Casing" ||
			name == "SK_AssaultRifle" || (name == "SK_Manny_Arms"))
		{

			player_models[(**player_models_cnt)++] = node;
		}


#if 0
		for (const auto& mesh : node->mesh->meshes) {
			if (name == "SM_AssaultRifle_Magazine" ||
				name == "SM_AssaultRifle_Casing" ||
				name == "SK_AssaultRifle" || (name == "SK_Manny_Arms"))
			{
				if (name == "SK_Manny_Arms" && manny_count != 0) {
					player_models[first_manny_index] = node;
				}
				else {
					player_models[(**player_models_cnt)++] = node;
				}
			}

			if (name == "SK_Manny_Arms") {
				if (manny_count == 0) {
					int copy = (**player_models_cnt);
					first_manny_index = --copy;
				}
				manny_count++;
			}
		}
#endif
	}
	for (auto& child : node->children) {
		find_player_model_aux(child, player_models, player_models_cnt);
	}
}


AssimpNode** find_player_models(SceneGraph scene_graph, uint32_t* player_models_cnt) {
	AssimpNode** player_models = new AssimpNode * [10];
	for (auto& scene : scene_graph.nodes) {
		for (auto node : scene.assimp_nodes) {
			find_player_model_aux(node, player_models, &player_models_cnt);
		}
	}
	return player_models;
}


int main() {
	aim_profiler_begin();
    global_w32_window = os_win32_open_window("opengl", SRC_WIDTH, SRC_HEIGHT, win32_main_callback, 1);
	
	arena_init(&g_transient_arena, 1024 * 1024 * 2);

    //os_win32_toggle_fullscreen(global_w32_window.handle, &global_w32_window.window_placement);

    #if RAW_INPUT
    RAWINPUTDEVICE rid = {
        .usUsagePage = 0x01,                   // generic desktop controls
        .usUsage     = 0x02,                   // mouse
        //.dwFlags     = RIDEV_INPUTSINK       // receive even when not focused
        //            | RIDEV_NOLEGACY,      // no WM_MOUSEMOVE fallback
        .dwFlags = 0,
        .hwndTarget  = global_w32_window.handle,
    };

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        // failure!
    }
    // clamp the cursor to your client rect if you want absolute confinement:
    #endif                        

    ShowCursor(0);

    OpenGL* opengl = new OpenGL();
    opengl->vsynch = -1;

    HDC hdc = GetDC(global_w32_window.handle);
    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixel_format = ChoosePixelFormat(hdc, &pfd);
    if(pixel_format) {
        SetPixelFormat(hdc, pixel_format, &pfd);
    }else{
        __debugbreak();
    }

    HGLRC tempRC = wglCreateContext(hdc);
    if(wglMakeCurrent(hdc, tempRC))
    {
        // NOTE It seems that in order to get anything from `wglGetProcAddress`, `wglMakeCurrent` must have been called!
        //wglCreateContextAttribsARB = OpenGLGetFunction(wglCreateContextAttribsARB);
        OpenGLSetFunction(wglCreateContextAttribsARB)

        i32 attrib_list[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_FLAGS_ARB, 0,
            WGL_CONTEXT_PROFILE_MASK_ARB,
            WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0
        };
        HGLRC hglrc = opengl->wglCreateContextAttribsARB(hdc, 0, attrib_list);
        wglMakeCurrent(0, 0);
        wglDeleteContext(tempRC);
        wglMakeCurrent(hdc, hglrc);
    }
    OpenGLSetFunction(wglGetExtensionsStringEXT);
    {
        const char* extensions = opengl->wglGetExtensionsStringEXT();
        b32 swap_control_supported = false;
		const char* at = extensions;
        while(*at) 
        {
            while(*at == ' ') {
                at++;
            }

            const char* start = at;
            while(*at && *at != ' ') {
                at++; 
            }
            if (!*at) {
                break;
            }
            size_t count = at - start;
            Str8 extension = str8((char*)start, count);
            b32 res1 = str8_equals(extension, str8_lit("WGL_EXT_framebuffer_sRGB"));
            b32 res2 = str8_equals(extension, str8_lit("WGL_ARB_framebuffer_sRGB"));
            if(str8_equals(extension, str8_lit("WGL_EXT_swap_control")))
            {
                swap_control_supported = true;
            }

        }
        if(swap_control_supported) {
            OpenGLSetFunction(wglSwapIntervalEXT);
            OpenGLSetFunction(wglGetSwapIntervalEXT);
            if(opengl->wglSwapIntervalEXT(1)) 
            {
                opengl->vsynch = opengl->wglGetSwapIntervalEXT();
            }
        }
    }

    OpenGLSetFunction(glGenVertexArrays);
    OpenGLSetFunction(glBindVertexArray);
    OpenGLSetFunction(glDeleteVertexArrays);

	OpenGLSetFunction(glGenBuffers);
	OpenGLSetFunction(glBindBuffer);
	OpenGLSetFunction(glBufferData);
	OpenGLSetFunction(glVertexAttribPointer);
	OpenGLSetFunction(glVertexAttribIPointer);
	OpenGLSetFunction(glEnableVertexAttribArray);

    OpenGLSetFunction(glCreateShader);
    OpenGLSetFunction(glCompileShader);
    OpenGLSetFunction(glShaderSource);
    OpenGLSetFunction(glCreateProgram);
    OpenGLSetFunction(glAttachShader);
    OpenGLSetFunction(glLinkProgram);
    OpenGLSetFunction(glDeleteShader);
    OpenGLSetFunction(glUseProgram);
    OpenGLSetFunction(glGetShaderiv);
    OpenGLSetFunction(glGetShaderInfoLog);
    OpenGLSetFunction(glGetProgramiv);
    OpenGLSetFunction(glGetProgramInfoLog);

    OpenGLSetFunction(glUniform1i);
    OpenGLSetFunction(glUniform1f);
    OpenGLSetFunction(glUniform2fv);
    OpenGLSetFunction(glUniform2f);
    OpenGLSetFunction(glUniform3fv);
    OpenGLSetFunction(glUniform3f);
    OpenGLSetFunction(glUniform4fv);
    OpenGLSetFunction(glUniform4f);
    OpenGLSetFunction(glUniformMatrix2fv);
    OpenGLSetFunction(glUniformMatrix3fv);
    OpenGLSetFunction(glUniformMatrix4fv);
    OpenGLSetFunction(glGetUniformLocation);

	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// physics-init
	#if 0
    JPH::RegisterDefaultAllocator();
	PhysicsSystem physics_system{};
	JPH::BodyCreationSettings sphere_settings(new JPH::SphereShape(1.5f), JPH::RVec3(0.0_r, 15.0_r, 0.0_r), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
	sphere_settings.mMassPropertiesOverride = JPH::MassProperties{ .mMass = 40.0f };
	Transform3D sphere_transform = Transform3D(glm::vec3(0.0, 15.0, 0.0));
	JPH::BodyID my_sphere = physics_system.create_body(&sphere_transform, new JPH::SphereShape(1.5f), false);
	physics_system.get_body_interface().GetShape(my_sphere);
	physics_system.get_body_interface().SetLinearVelocity(my_sphere, JPH::Vec3(0.0f, -2.0f, 0.0f));
	physics_system.get_body_interface().SetRestitution(my_sphere, 0.5f);

	JPH::RefConst<JPH::Shape> mStandingShape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * 1.0 + 1.0, 0), JPH::Quat::sIdentity(), new JPH::CapsuleShape(0.5f * 1.0, 1.0)).Create().Get();

	JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
	settings->mMaxStrength = 5000.0f;
	settings->mMass = 70.0f;
	settings->mShape = mStandingShape;
	JPH::Ref<JPH::CharacterVirtual> mCharacter = new JPH::CharacterVirtual(settings, JPH::Vec3::sZero(), JPH::Quat::sIdentity(), 0, &physics_system.inner_physics_system);
	#endif

    // rendering
    glm::mat4 projection;
    projection = glm::perspective(glm::radians(curr_camera.zoom), (float)SRC_WIDTH / (float)SRC_HEIGHT, 0.1f, 10000.0f);

    // shaders
    Shader skinning_shader{};
    shader_init(&skinning_shader, opengl, str8("skel_shader-2.vs.glsl"), str8("6.multiple_lights.fs.glsl"));

	float vertices[] = {
		// Back face
		-0.5f, -0.5f, -0.5f,	0.0f,  0.0f, -1.0f,			0.0f, 0.0f, // Bottom-left
		 0.5f,  0.5f, -0.5f,	0.0f,  0.0f, -1.0f,			1.0f, 1.0f, // top-rightopengl, 
		 0.5f, -0.5f, -0.5f,	0.0f,  0.0f, -1.0f,			1.0f, 0.0f, // bottom-right         
		 0.5f,  0.5f, -0.5f,	0.0f,  0.0f, -1.0f,			1.0f, 1.0f, // top-right
		-0.5f, -0.5f, -0.5f,	0.0f,  0.0f, -1.0f,			0.0f, 0.0f, // bottom-left
		-0.5f,  0.5f, -0.5f,	0.0f,  0.0f, -1.0f,			0.0f, 1.0f, // top-left
		// Front face        
		-0.5f, -0.5f,  0.5f,	0.0f,  0.0f,  1.0f,			0.0f, 0.0f, // bottom-left
		 0.5f, -0.5f,  0.5f,	0.0f,  0.0f,  1.0f,			1.0f, 0.0f, // bottom-right
		 0.5f,  0.5f,  0.5f,	0.0f,  0.0f,  1.0f,			1.0f, 1.0f, // top-right
		 0.5f,  0.5f,  0.5f,	0.0f,  0.0f,  1.0f,			1.0f, 1.0f, // top-right
		-0.5f,  0.5f,  0.5f,	0.0f,  0.0f,  1.0f,			0.0f, 1.0f, // top-left
		-0.5f, -0.5f,  0.5f,	0.0f,  0.0f,  1.0f,			0.0f, 0.0f, // bottom-left
		// Left face         
		-0.5f,  0.5f,  0.5f,	-1.0f,  0.0f,  0.0f,			1.0f, 0.0f, // top-right
		-0.5f,  0.5f, -0.5f,	-1.0f,  0.0f,  0.0f,			1.0f, 1.0f, // top-left
		-0.5f, -0.5f, -0.5f,	-1.0f,  0.0f,  0.0f,			0.0f, 1.0f, // bottom-left
		-0.5f, -0.5f, -0.5f,	-1.0f,  0.0f,  0.0f,			0.0f, 1.0f, // bottom-left
		-0.5f, -0.5f,  0.5f,	-1.0f,  0.0f,  0.0f,			0.0f, 0.0f, // bottom-right
		-0.5f,  0.5f,  0.5f,	-1.0f,  0.0f,  0.0f,			1.0f, 0.0f, // top-right
		// Right face        
		 0.5f,  0.5f,  0.5f,	1.0f,  0.0f,  0.0f,			1.0f, 0.0f, // top-left
		 0.5f, -0.5f, -0.5f,	1.0f,  0.0f,  0.0f,			0.0f, 1.0f, // bottom-right
		 0.5f,  0.5f, -0.5f,	1.0f,  0.0f,  0.0f,			1.0f, 1.0f, // top-right         
		 0.5f, -0.5f, -0.5f,	1.0f,  0.0f,  0.0f,			0.0f, 1.0f, // bottom-right
		 0.5f,  0.5f,  0.5f,	1.0f,  0.0f,  0.0f,			1.0f, 0.0f, // top-left
		 0.5f, -0.5f,  0.5f,	1.0f,  0.0f,  0.0f,			0.0f, 0.0f, // bottom-left     
		 // Bottom face       
		 -0.5f, -0.5f, -0.5f,	0.0f, -1.0f,  0.0f,			0.0f, 1.0f, // top-right
		  0.5f, -0.5f, -0.5f,	0.0f, -1.0f,  0.0f,			1.0f, 1.0f, // top-left
		  0.5f, -0.5f,  0.5f,	0.0f, -1.0f,  0.0f,			1.0f, 0.0f, // bottom-left
		  0.5f, -0.5f,  0.5f,	0.0f, -1.0f,  0.0f,			1.0f, 0.0f, // bottom-left
		 -0.5f, -0.5f,  0.5f,	0.0f, -1.0f,  0.0f,			0.0f, 0.0f, // bottom-right
		 -0.5f, -0.5f, -0.5f,	0.0f, -1.0f,  0.0f,			0.0f, 1.0f, // top-right
		 // Top face          
		 -0.5f,  0.5f, -0.5f,	0.0f,  1.0f,  0.0f,			0.0f, 1.0f, // top-left
		  0.5f,  0.5f,  0.5f,	0.0f,  1.0f,  0.0f,			1.0f, 0.0f, // bottom-right
		  0.5f,  0.5f, -0.5f,	0.0f,  1.0f,  0.0f,			1.0f, 1.0f, // top-right     
		  0.5f,  0.5f,  0.5f,	0.0f,  1.0f,  0.0f,			1.0f, 0.0f, // bottom-right
		 -0.5f,  0.5f, -0.5f,	0.0f,  1.0f,  0.0f,			0.0f, 1.0f, // top-left
		 -0.5f,  0.5f,  0.5f,	0.0f,  1.0f,  0.0f,			0.0f, 0.0f  // bottom-left        
	};
    unsigned int VBO, cubeVAO;
	opengl->glGenVertexArrays(1, &cubeVAO);
	opengl->glGenBuffers(1, &VBO);

	opengl->glBindBuffer(GL_ARRAY_BUFFER, VBO);
	opengl->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	opengl->glBindVertexArray(cubeVAO);

	// position attribute
	opengl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	opengl->glEnableVertexAttribArray(0);

	// normal attribute
	opengl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	opengl->glEnableVertexAttribArray(1);

	// texture attribute
	opengl->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	opengl->glEnableVertexAttribArray(2);

	MeshBox floor_meshbox = MeshBox(Transform3D(glm::vec3(floor_pos.x, floor_pos.y, floor_pos.z), glm::vec3(floor_scale.x, floor_scale.y, floor_scale.z)));
    std::vector<MeshBox> boxes = 
	{
		///////
		MeshBox(Transform3D(glm::vec3(-3.0f,  1.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(-2.0f,  1.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  1.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  1.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f, 1.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(2.0f, 1.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(3.0f, 1.0f, -5.0f))),

		MeshBox(Transform3D(glm::vec3(-3.0f,  2.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(-2.0f,  2.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  2.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  2.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f, 2.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(2.0f, 2.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(3.0f, 2.0f, -5.0f))),

		MeshBox(Transform3D(glm::vec3(-3.0f,  3.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(-2.0f,  3.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  3.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  3.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f, 3.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(2.0f, 3.0f, -5.0f))),
		MeshBox(Transform3D(glm::vec3(3.0f, 3.0f, -5.0f))),

		///////
		MeshBox(Transform3D(glm::vec3(0.0f,  13.0f,  0.0f))),
		MeshBox(Transform3D(glm::vec3(5.0f,  6.0f, -10.0f))),
		MeshBox(Transform3D(glm::vec3(-1.5f, 5.2f, -2.5f))),
		MeshBox(Transform3D(glm::vec3(-9.8f, 5.0f, -12.3f))),
		MeshBox(Transform3D(glm::vec3(2.4f, 3.4f, -3.5f))),
		MeshBox(Transform3D(glm::vec3(-8.7f,  6.0f, -7.5f))),
		MeshBox(Transform3D(glm::vec3(3.3f, 5.0f, -2.5f))),
		MeshBox(Transform3D(glm::vec3(1.5f,  5.0f, -2.5f))),
		MeshBox(Transform3D(glm::vec3(8.5f,  8.2f, -1.5f))),
		MeshBox(Transform3D(glm::vec3(-8.3f,  4.0f, -1.5f))),


		// a pile of boxes
		MeshBox(Transform3D(glm::vec3(0.0f,  20.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  22.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  24.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f,  26.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  27.0f, 0.0f))),


		MeshBox(Transform3D(glm::vec3(0.0f,  30.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  31.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  32.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f,  33.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  34.0f, 0.0f))),

		MeshBox(Transform3D(glm::vec3(0.0f,  30.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  31.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  32.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f,  33.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  34.0f, 0.0f))),

		MeshBox(Transform3D(glm::vec3(0.0f,  30.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  31.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  32.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f,  33.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  34.0f, 0.0f))),

		MeshBox(Transform3D(glm::vec3(0.0f,  30.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  31.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  32.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f,  33.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  34.0f, 0.0f))),

		MeshBox(Transform3D(glm::vec3(0.0f,  30.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  31.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(-1.0f,  32.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(1.0f,  33.0f, 0.0f))),
		MeshBox(Transform3D(glm::vec3(0.0f,  34.0f, 0.0f))),
	};

    PointLight point_lights[] = 
	{
		PointLight
		{
			.transform = Transform3D(glm::vec3(0.7f,  0.2f,  2.0f)),
			.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
			.diffuse = glm::vec3(0.8f, 0.8f, 0.8f),
			.specular = glm::vec3(1.0f, 1.0f, 1.0f),
			.constant = 1.0f,
			.linear = 0.09f,
			.quadratic = 0.032f,
		},
		PointLight
		{
			.transform = Transform3D(glm::vec3(2.3f,  -3.3f,  -4.0f)),
			.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
			.diffuse = glm::vec3(0.8f, 0.8f, 0.8f),
			.specular = glm::vec3(1.0f, 1.0f, 1.0f),
			.constant = 1.0f,
			.linear = 0.09f,
			.quadratic = 0.032f,
		},
		PointLight
		{
			.transform = Transform3D(glm::vec3(-4.0f,  2.0f,  -12.0f)),
			.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
			.diffuse = glm::vec3(0.8f, 0.8f, 0.8f),
			.specular = glm::vec3(1.0f, 1.0f, 1.0f),
			.constant = 1.0f,
			.linear = 0.09f,
			.quadratic = 0.032f,
		},
		PointLight
		{
			.transform = Transform3D(glm::vec3(0.0f,  0.0f,  -3.0f)),
			.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
			.diffuse = glm::vec3(0.8f, 0.8f, 0.8f),
			.specular = glm::vec3(1.0f, 1.0f, 1.0f),
			.constant = 1.0f,
			.linear = 0.09f,
			.quadratic = 0.032f,
		},
	};

	DirectionalLight directional_light
	{
		.direction = glm::vec3(-0.2f, -1.0f, -0.3f),
		.ambient = glm::vec3(0.05f, 0.05f, 0.05f),
		.diffuse = glm::vec3(0.4f, 0.4f, 0.4f),
		.specular = glm::vec3(0.5f, 0.5f, 0.5f)
	};

	SpotLight spot_light
	{
		.ambient = glm::vec3(0.0f, 0.0f, 0.0f),
		.diffuse = glm::vec3(1.0f, 1.0f, 1.0f),
		.specular = glm::vec3(1.0f, 1.0f, 1.0f),
		.constant = 1.0f,
		.linear = 0.09f,
		.quadratic = 0.032f,
		.cutOff = glm::cos(glm::radians(12.5f)),
		.outerCutOff = glm::cos(glm::radians(15.0f)),
	};

    shader_use(skinning_shader);
    glm::vec3 model_color = glm::vec3(1.0f, 0.5f, 0.31f);
    shader_set_vec3(skinning_shader, "objectColor", model_color.r, model_color.g, model_color.b);
    shader_set_int(skinning_shader, "material.diffuse", 0);
    shader_set_int(skinning_shader, "material.specular", 1);
    shader_set_float(skinning_shader, "material.shininess", model_material_shininess);


    // directional_light
    shader_set_vec3(skinning_shader, "dirLight.direction", directional_light.direction);
    shader_set_vec3(skinning_shader, "dirLight.ambient", directional_light.ambient);
    shader_set_vec3(skinning_shader, "dirLight.diffuse", directional_light.diffuse);
    shader_set_vec3(skinning_shader, "dirLight.specular", directional_light.specular);

    // point_lights
    int n = sizeof(point_lights) / sizeof(point_lights[0]);
    TempArena per_frame = temp_begin(&g_transient_arena);
    for (int i = 0; i < n; i++) {
        aim_profiler_time_block("pointLights creation");
        {
            /*
                3 alternativas:
                - Hacerlo como raddbg y tener temp blocks en shader_set_* (mas lenta)
                - Pasar la arena a shader_set_* y no tener que hacer el temp_begin y end todo el tiempo (intermedia)
                - Usar cstrings y listo con sprintf (la mas rapida)

                Aparte... pero ver si hay que hacer esto todos los frames y si se tiene que hacer es mejor guardar
                el nombre en una variable y listo, antes de tener que andar haciendo estos calculos pelotudos todo el tiempo.
            */
            #if 1
                Str8 prefix = str8_fmt(per_frame.arena, "pointLights[%d]", i);
                shader_set_vec3(skinning_shader, str8_concat(per_frame.arena, prefix, str8(".position")), point_lights[i].transform.pos);
                shader_set_vec3(skinning_shader, str8_concat(per_frame.arena, prefix, str8(".ambient")), point_lights[i].ambient);
                shader_set_vec3(skinning_shader, str8_concat(per_frame.arena, prefix, str8(".diffuse")), point_lights[i].diffuse);
                shader_set_vec3(skinning_shader, str8_concat(per_frame.arena, prefix, str8(".specular")), point_lights[i].specular);
                shader_set_float(skinning_shader, str8_concat(per_frame.arena, prefix, str8(".constant")), point_lights[i].constant);
                shader_set_float(skinning_shader, str8_concat(per_frame.arena, prefix, str8(".linear")), point_lights[i].linear);
                shader_set_float(skinning_shader, str8_concat(per_frame.arena, prefix, str8(".quadratic")), point_lights[i].quadratic);
            #else
                // NOTE using _sprintf_s here works because it appends a '\0' after processing. If not I would have to advance at by 
                // the total written, place a '\0' and come back
                // 
                // Remark: "The _snprintf_s function formats and stores count or fewer characters in buffer and appends a terminating NULL."
                // Source: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/snprintf-s-snprintf-s-l-snwprintf-s-snwprintf-s-l?view=msvc-170#remarks
                char prefix[300];
                char *end = prefix + sizeof(prefix);
                char *at = prefix;
                at += _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), "pointLights[%d]", i);

                _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), ".position");
                shader_set_vec3(skinning_shader, prefix, point_lights[i].transform.pos);

                _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), ".ambient");
                shader_set_vec3(skinning_shader, prefix, point_lights[i].ambient);

                _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), ".diffuse");
                shader_set_vec3(skinning_shader, prefix, point_lights[i].diffuse);

                _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), ".specular");
                shader_set_vec3(skinning_shader, prefix, point_lights[i].specular);

                _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), ".constant");
                shader_set_float(skinning_shader, prefix, point_lights[i].constant);

                _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), ".linear");
                shader_set_float(skinning_shader, prefix, point_lights[i].linear);

                _snprintf_s(at, (size_t)(end - at), (size_t)(end - at), ".quadratic");
                shader_set_float(skinning_shader, prefix, point_lights[i].quadratic);
            #endif
        }
    }
    temp_end(per_frame);

    // spot_light
    shader_set_vec3(skinning_shader, "spotLight.ambient", spot_light.ambient);
    shader_set_vec3(skinning_shader, "spotLight.diffuse", spot_light.diffuse);
    shader_set_vec3(skinning_shader, "spotLight.specular", spot_light.specular);
    shader_set_float(skinning_shader, "spotLight.constant", spot_light.constant);
    shader_set_float(skinning_shader, "spotLight.linear", spot_light.linear);
    shader_set_float(skinning_shader, "spotLight.quadratic", spot_light.quadratic);
    shader_set_float(skinning_shader, "spotLight.cutOff", spot_light.cutOff);
    shader_set_float(skinning_shader, "spotLight.outerCutOff", spot_light.outerCutOff);
    shader_set_mat4(skinning_shader, "projection", projection);

    //while (!glfwWindowShouldClose(window)) 
    LONGLONG frequency = aim_timer_get_os_freq();
    LONGLONG last_frame = 0;
    global_w32_window.is_running = true;
    glViewport(0, 0, SRC_WIDTH, SRC_HEIGHT);


	AssimpNode assault_rifle, assault_rifle_magazine, assault_rifle_casing;
    SceneGraph scene_graph{};

	////////////////////// NEW /////////////////////////
	scene_graph.loadAssimp(opengl, &assault_rifle, "D:/c-c++ projects/aim/aim-v5/engine/assets/models/Unreal/SK_FP_Manny_Simple.fbx");
	std::cout << "Finished loading Manny" << std::endl;
	scene_graph.loadAssimp(opengl, &assault_rifle, "D:/c-c++ projects/aim/aim-v5/engine/assets/models/Unreal/SK_AssaultRifle.fbx");
	scene_graph.loadAssimp(opengl, &assault_rifle, "D:/c-c++ projects/aim/aim-v5/engine/assets/models/Unreal/SM_AssaultRifle_Magazine.fbx");
	scene_graph.loadAssimp(opengl, &assault_rifle, "D:/c-c++ projects/aim/aim-v5/engine/assets/models/Unreal/SM_AssaultRifle_Casing.fbx");


	Animation danceAnimation("D:/c-c++ projects/aim/aim-v5/engine/assets/models/Unreal/Animations/A_FP_AssaultRifle_Reload.fbx", 0);
	Animation mag_anim("D:/c-c++ projects/aim/aim-v5/engine/assets/models/Unreal/Animations/A_FP_WEP_AssaultRifle_Reload.fbx", 2);
    Animator animator(&danceAnimation);
	Animator mag_animator(&mag_anim);

    Entity player{};
	// find all AssimpNodes corresponding to the player model
	player.has_animations = true;
	player.has_model = true;
	player.transform = Transform3D();
	player.models = find_player_models(scene_graph, &player.models_count);

    while (global_w32_window.is_running)
	{
        LONGLONG now = aim_timer_get_os_time();
        LONGLONG dt_long = now - last_frame;
        last_frame = now;
        f32 dt = aim_timer_ticks_to_sec(dt_long, frequency);
        global_input.dt = dt;

        win32_process_pending_msgs();
        if (input_is_key_pressed(&global_input, Keys_W))
            curr_camera.process_keyboard(FORWARD, dt);
        if (input_is_key_pressed(&global_input, Keys_S))
            curr_camera.process_keyboard(BACKWARD, dt);
        if (input_is_key_pressed(&global_input, Keys_A))
            curr_camera.process_keyboard(LEFT, dt);
        if (input_is_key_pressed(&global_input, Keys_D))
            curr_camera.process_keyboard(RIGHT, dt);
        if (input_is_key_pressed(&global_input, Keys_Space))
			curr_camera.process_keyboard(UP, dt);
        if (input_is_key_pressed(&global_input, Keys_Control))
            curr_camera.process_keyboard(DOWN, dt);

        
        //if(global_input.curr_mouse_state.x - )
        {

            f32 xx = global_input.dx;
            f32 yy = global_input.dy;
            curr_camera.process_mouse_movement(xx, yy);
        }
        //global_input.curr_mouse_state.y = yPos;

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glm::mat4 view = curr_camera.GetViewMatrix();

	    opengl->glBindVertexArray(cubeVAO);
        shader_use(skinning_shader);

         shader_set_mat4(skinning_shader, "nodeMatrix", glm::mat4(1.0f));
		shader_set_mat4(skinning_shader, "view", view);
        shader_set_vec3(skinning_shader, "viewPos", curr_camera.position);
        shader_set_vec3(skinning_shader, "spotLight.position", curr_camera.position);
        shader_set_vec3(skinning_shader, "spotLight.direction", curr_camera.forward);

		// bind diffuse map
        #if 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, diffuseMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, specularMap);
		glBindVertexArray(cubeVAO);
        #endif

		// render floor, is just a plane
		glm::mat4 model_mat = glm::translate(glm::mat4(1.0f), floor_meshbox.transform.pos) *
			glm::mat4_cast(floor_meshbox.transform.rot) *
			glm::scale(glm::mat4(1.0f), floor_meshbox.transform.scale);

		shader_set_mat4(skinning_shader, "model", model_mat);
        // Esto va en OpenGL struct??
		glDrawArrays(GL_TRIANGLES, 0, 36);
        for (unsigned int i = 0; i < boxes.size(); i++)
        {
            // calculate the model matrix for each object and pass it to shader before drawing
            //glm::mat4 model = glm::mat4(1.0f);
            //model = glm::translate(model, boxes[i].transform.pos);
            //float angle = 20.0f * i;
            //model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f)); // take rotation out for testing

            // with rotations
            glm::mat4 model = glm::translate(glm::mat4(1.0f), boxes[i].transform.pos) *
                glm::mat4_cast(boxes[i].transform.rot) *
                glm::scale(glm::mat4(1.0f), boxes[i].transform.scale);
            shader_set_mat4(skinning_shader, "model", model);

            //shader_set_mat4("nodeMatrix", glm::mat4(1.0f));

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Esto es atroz porque se usan igual estas mierdas
        if (animationActive) 
        {
            animator.UpdateAnimation(dt);
            mag_animator.UpdateAnimation(dt);
            for (auto& bone : animator.m_CurrentAnimation->m_Bones) 
            {
                /* NOTE(anim): This bone corresponds to: SK_FP_Manny_Simple.fbx
                Armature has bones ->
                    root
                        pelvis              (78)
                        ik_foot_root        (2)
                        ik_hand_root        (3)
                            ik_hand_gun
                                ik_hand_l
                                ik_hand_r
                        interaction         (0)
                        center_of_mass      (0)
                */
                if (bone.GetBoneName() == "Armature") {
                    manny_armature = bone.GetLocalTransform();
                }

                /* NOTE(anim): This bone corresponds to: SK_FP_Manny_Simple.fbx
                    As shown above, is the first bone in the Armature node
                */
                if (bone.GetBoneName() == "root") {
                    root = &bone;
                }
                /* NOTE(anim): This bone corresponds to: SK_FP_Manny_Simple.fbx
                    As shown above, they are from the hand
                */
                if (bone.GetBoneName() == "ik_hand_root") {
                    ik_hand_root = &bone;
                }
                if (bone.GetBoneName() == "ik_hand_gun") {
                    ik_hand_gun_bone = &bone;
                }
            }

            for (auto& bone : mag_animator.m_CurrentAnimation->m_Bones) 
            {
                /* NOTE(anim): This bone corresponds to: SK_AssaultRifle.fbx, this is the only model related to the gun that has bones
                    The other two: SM_AssaultRifle_Magazine.fbx and SM_AssaultRifle_Casing.fbx are just static meshes

                    This bone right here is used so that the SM_AssaultRifle_Magazine transform can be set to be equal to the SK_AssaultRifle's bone, animating the mag
                */
                if (bone.GetBoneName() == "Magazine") {
                    mag_bone_transform = bone.GetLocalTransform();
                }
            }
        }
		todas_las_putas_transforms = manny_armature * root->GetLocalTransform() * ik_hand_root->GetLocalTransform() * ik_hand_gun_bone->GetLocalTransform();
		
		manny_transforms = animator.GetFinalBoneMatrices();
        render_player(opengl, &player, skinning_shader);

        opengl->glUseProgram(0);

        input_update(&global_input);
        SwapBuffers(hdc);
    }
	aim_profiler_end();
	aim_profiler_print();
}