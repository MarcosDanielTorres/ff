#include <stdint.h>
#include <vector>
#include <stdio.h>

#define global_variable static
#define internal static
#define local_persist static

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

// thirdparty
// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#if 0
// assimp
#include "AssimpLoader.h"

// jolt
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
// vehicle physics
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/VehicleCollisionTester.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
// debug renderer
#include <Jolt/Renderer/DebugRenderer.h>
#include "physics/jolt_debug_renderer.h"
#include "physics/physics_system.h"

#include "physics/jolt_debug_renderer.cpp"
#include "physics/physics_system.cpp"
#endif


// TODO fix assert collision with windows.h or something else!


#include "input/input.h"
#include "camera.h"
//TODO: get the timer
//#include "aim_timer.h"


#include "camera.cpp"
//#include "aim_timer.cpp"

#include "components.h"
using namespace aim::Components;

// TODO fix
//#include "logger/logger.h"
//#include "logger/logger.cpp"
#include "shader.h"

///////// TODO cleanup ////////////
// global state
// game input
global_variable GameInput* game_input{};

// window size
global_variable u32 SRC_WIDTH = 1280;
global_variable u32 SRC_HEIGHT = 720;

// TODO: Removed these
// used during mouse callback
global_variable float lastX = SRC_WIDTH / 2.0f;
global_variable float lastY = SRC_HEIGHT / 2.0f;
global_variable bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

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
	//PhysicsBody body;

	MeshBox(Transform3D transform) {
		this->transform = transform;
	}

	// this is not the best api, its only used once when there is no body. Only done in creation
	// This api is more like a builder style api, its probably better to have it inside the constructor and that's it or use a builder for this shapes. which could be good.
	//void set_shape(JPH::Ref<JPH::Shape> shape) {
	//	this->body.shape = shape;
	//}

	//void update_body_shape(PhysicsSystem& physics_system) {
	//	JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(this->transform.scale.x / 2.0, this->transform.scale.y / 2.0, this->transform.scale.z / 2.0));
	//	floor_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.
	//	JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
	//	JPH::Ref<JPH::Shape> floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()
	//	this->body.shape = floor_shape;
	//	physics_system.get_body_interface().SetShape(this->body.physics_body_id, this->body.shape, false, JPH::EActivation::DontActivate);

	//	glm::vec3 my_pos = this->transform.pos;
	//	JPH::Vec3 new_pos = JPH::Vec3(my_pos.x, my_pos.y, my_pos.z);
	//	physics_system.get_body_interface().SetPosition(this->body.physics_body_id, new_pos, JPH::EActivation::DontActivate);

	//	physics_system.get_body_interface().SetRotation(
	//		this->body.physics_body_id,
	//		JPH::Quat(this->transform.rot.x, this->transform.rot.y, this->transform.rot.z, this->transform.rot.w),
	//		JPH::EActivation::DontActivate
	//	);
	//}
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


int main() {
    //glfwInit();
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //GLFWwindow* window = glfwCreateWindow(SRC_WIDTH, SRC_HEIGHT, "Some GLFW Window!", NULL, NULL);

	//glfwMakeContextCurrent(window);
 	//glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	//glfwSetKeyCallback(window, keyboard_callback);
	//glfwSetCursorPosCallback(window, mouse_callback);
	//glfwSetScrollCallback(window, scroll_callback);

	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
	//	return -1;
	//}

    // init game input
    GameInput temp_game_input{};
    game_input = &temp_game_input;

	
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
    Shader skinning_shader("skel_shader-2.vs.glsl", "6.multiple_lights.fs.glsl");
	float vertices[] = {
		// Back face
		-0.5f, -0.5f, -0.5f,	0.0f,  0.0f, -1.0f,			0.0f, 0.0f, // Bottom-left
		 0.5f,  0.5f, -0.5f,	0.0f,  0.0f, -1.0f,			1.0f, 1.0f, // top-right
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
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &VBO);


	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(cubeVAO);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// texture attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

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

    while (!glfwWindowShouldClose(window)) 
	{
        float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            curr_camera.process_keyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            curr_camera.process_keyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            curr_camera.process_keyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            curr_camera.process_keyboard(RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
			curr_camera.process_keyboard(UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            curr_camera.process_keyboard(DOWN, deltaTime);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(1.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glm::mat4 view = curr_camera.GetViewMatrix();

        skinning_shader.use();
		skinning_shader.setMat4("nodeMatrix", glm::mat4(1.0f));
        glm::vec3 model_color = glm::vec3(1.0f, 0.5f, 0.31f);
		skinning_shader.setVec3("objectColor", model_color.r, model_color.g, model_color.b);
		skinning_shader.setInt("material.diffuse", 0);
		skinning_shader.setInt("material.specular", 1);
		skinning_shader.setFloat("material.shininess", model_material_shininess);

        skinning_shader.setVec3("viewPos", curr_camera.position);
        skinning_shader.setVec3("spotLight.position", curr_camera.position);
        skinning_shader.setVec3("spotLight.direction", curr_camera.forward);

		// directional_light
		skinning_shader.setVec3("dirLight.direction", directional_light.direction);
		skinning_shader.setVec3("dirLight.ambient", directional_light.ambient);
		skinning_shader.setVec3("dirLight.diffuse", directional_light.diffuse);
		skinning_shader.setVec3("dirLight.specular", directional_light.specular);

		// point_lights
		int n = sizeof(point_lights) / sizeof(point_lights[0]);
		for (int i = 0; i < n; i++) {
			std::string prefix = "pointLights[" + std::to_string(i) + "]";

			skinning_shader.setVec3(prefix + ".position", point_lights[i].transform.pos);
			skinning_shader.setVec3(prefix + ".ambient", point_lights[i].ambient);
			skinning_shader.setVec3(prefix + ".diffuse", point_lights[i].diffuse);
			skinning_shader.setVec3(prefix + ".specular", point_lights[i].specular);
			skinning_shader.setFloat(prefix + ".constant", point_lights[i].constant);
			skinning_shader.setFloat(prefix + ".linear", point_lights[i].linear);
			skinning_shader.setFloat(prefix + ".quadratic", point_lights[i].quadratic);
		}

		// spot_light
		skinning_shader.setVec3("spotLight.ambient", spot_light.ambient);
		skinning_shader.setVec3("spotLight.diffuse", spot_light.diffuse);
		skinning_shader.setVec3("spotLight.specular", spot_light.specular);
		skinning_shader.setFloat("spotLight.constant", spot_light.constant);
		skinning_shader.setFloat("spotLight.linear", spot_light.linear);
		skinning_shader.setFloat("spotLight.quadratic", spot_light.quadratic);
		skinning_shader.setFloat("spotLight.cutOff", spot_light.cutOff);
		skinning_shader.setFloat("spotLight.outerCutOff", spot_light.outerCutOff);
		skinning_shader.setMat4("projection", projection);
		skinning_shader.setMat4("view", view);
        
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

		skinning_shader.setMat4("model", model_mat);
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
            skinning_shader.setMat4("model", model);

            //skinning_shader.setMat4("nodeMatrix", glm::mat4(1.0f));

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        if(is_key_just_pressed(game_input, KEY_A)){
            printf("KEY_A is just pressed\n");
        }

        if(is_key_pressed(game_input, KEY_D)){
            printf("KEY_A is just pressed\n");
        }
        game_input->keyboard_state.prev_state = game_input->keyboard_state.curr_state;
        glfwPollEvents();
        glfwSwapBuffers(window);
    }
}