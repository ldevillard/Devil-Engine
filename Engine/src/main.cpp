#include <windows.h>

// std
#include <iostream>

// libs
#include <utils/glad/glad.h>
#include <glfw3.h>
#include <render/stb_image.h>

// glm
#include <maths/glm/glm.hpp>
#include <maths/glm/gtc/matrix_transform.hpp>

// engine
#include "component/Model.h"
#include "data/AxisGrid.h"
#include "data/CubeMap.h"
#include "render/ComputeShader.h"
#include "render/Raytracer.h"
#include "render/Shader.h"
#include "system/editor/Editor.h"
#include "system/entity/EntityManager.h"
#include "system/editor/Gizmo.h"
#include "system/editor/Outliner.h"
#include "system/editor/SceneManager.h"
#include "system/Input.h"
#include "system/Time.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

int main()
{
	// glfw: initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// glfw window creation
	GLFWwindow* window = glfwCreateWindow(SCENE_WIDTH, SCENE_HEIGHT, "Engine", NULL, NULL);
	if (window == NULL)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	Input::Initialize(window);

	// icon
	int iconWidth, iconHeight, iconChannels;
	stbi_uc* iconPixels = stbi_load("resources/DevilEngine.png", &iconWidth, &iconHeight, &iconChannels, 4);

	GLFWimage images[1] = {};
	images[0].width = iconWidth;
	images[0].height = iconHeight;
	images[0].pixels = iconPixels;

	if (iconPixels)
	{
		glfwSetWindowIcon(window, 1, images);
		stbi_image_free(iconPixels);
	}
	else
	{
		std::cerr << "Failed to load icon" << std::endl;
	}

	// glad: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glfwSwapInterval(1); // enable vsync

	// configure global opengl state
	glEnable(GL_DEPTH_TEST); // enable depth testing
	glEnable(GL_STENCIL_TEST); // enable stencil testing
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); // configure the stencil buffer to replace the value of the stencil buffer if the depth test fails
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// build and compile shader programs
	Shader shader("shaders/VertexShader.glsl", "shaders/FragmentShader.glsl");
	Shader gizmoShader("shaders/gizmo/GizmoVertexShader.glsl", "shaders/gizmo/GizmoFragmentShader.glsl");
	Shader outlineShader("shaders/outline/OutlineVertexShader.glsl", "shaders/outline/OutlineFragmentShader.glsl");
	Shader outlineDilateShader("shaders/outline/OutlineQuadVertexShader.glsl", "shaders/outline/OutlineDilatingFragmentShader.glsl");
	Shader cubeMapShader("shaders/skybox/SkyboxVertexShader.glsl", "shaders/skybox/SkyboxFragmentShader.glsl");
	Shader axisGridShader("shaders/grid/AxisGridVertexShader.glsl", "shaders/grid/AxisGridFragmentShader.glsl");
	Shader raytracingShader("shaders/raytracing/RaytracerVertexShader.glsl", "shaders/raytracing/RaytracerFragmentShader.glsl");
	Shader shadowMapShader("shaders/depth/ShadowMapVertexShader.glsl", "shaders/depth/ShadowMapFragmentShader.glsl");
	Shader depthQuadShader("shaders/depth/DepthQuadVertexShader.glsl", "shaders/depth/DepthQuadFragmentShader.glsl");

	ComputeShader accumulateShader("shaders/compute/AccumulateComputeShader.glsl", glm::uvec2(RAYTRACED_SCENE_WIDTH, RAYTRACED_SCENE_HEIGHT));
	ComputeShader outlineBlitShader("shaders/compute/BlitTexturesComputeShader.glsl", glm::uvec2(SCENE_WIDTH, SCENE_HEIGHT));
	
	const std::vector<std::string> faces = 
	{
		"resources/skybox/right.png",
		"resources/skybox/left.png",
		"resources/skybox/top.png",
		"resources/skybox/bottom.png",
		"resources/skybox/front.png",
		"resources/skybox/back.png",
	};
	CubeMap cubemap(faces, &cubeMapShader);
	AxisGrid grid(&axisGridShader);

	// initialize systems
	Outliner::Initialize(&outlineShader, &outlineDilateShader, &outlineBlitShader);
	Raytracer::Initialize(&raytracingShader, &accumulateShader);
	Gizmo::InitGizmos(&gizmoShader);
	EntityManager::Initialize(&shader);
	Model::LoadPrimitives();
	SceneManager::Initialize();
	// initialize editor
	Editor::Initialize(window);
	// initialize timers
	Time::Update();
	Time::ResetFixedTime();

	// render loop
	while (!glfwWindowShouldClose(window))
	{
		Editor::Get().Update(Time::DeltaTime);

		EditorSettings settings = Editor::Get().GetSettings();

		if (settings.Wireframe)
		{
			glDisable(GL_CULL_FACE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else
		{
			glEnable(GL_CULL_FACE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		// we don't want to render the scene if we are loading entities
		if (!EntityManager::Get().IsLoadingEntities())
		{
			if (settings.isPlaying)
			{
				const unsigned int fixedSteps = Time::ConsumeFixedSteps(Time::DeltaTime);
				for (unsigned int i = 0; i < fixedSteps; ++i)
				{
					EntityManager::Get().FixedUpdateEntities(Time::FixedDeltaTime);
				}

				EntityManager::Get().UpdateEntities(Time::DeltaTime);
			}
			else
			{
				Time::ResetFixedTime();
			}

			// 3D rendering
			Editor::Get().RenderShadowMap(&shadowMapShader, &depthQuadShader);
			Editor::Get().RenderFrame(&shader, &cubemap, &grid);

			// raytracing
			if (settings.Raytracing) 
			{
				Raytracer::Get().Draw(cubemap);
			}
		}
		else
		{
			Time::ResetFixedTime();
		}

		// editor interface rendering
		Editor::Get().RenderEditor();

		// update time
		Time::Update();

		// swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// cleanup
	Gizmo::ReleaseGizmos();

	// terminate, clearing all previously allocated GLFW resources.
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	Editor::Get().MouseCallback(xposIn, yposIn);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	Editor::Get().ScrollCallback(xoffset, yoffset);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	if (width == 0 || height == 0)
		return;

	SCR_WIDTH = width;
	SCR_HEIGHT = height;

	Editor& editor = Editor::Get();
	editor.FramebufferSizeCallback(width, height);
	editor.RenderEditor();
}

