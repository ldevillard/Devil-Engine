#include "system/editor/Editor.h"

#include <glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <maths/glm/gtx/matrix_decompose.hpp>

// imgui
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"

#include "component/Transform.h"
#include "data/AxisGrid.h"
#include "data/CubeMap.h"
#include "maths/Math.h"
#include "physics/Physics.h"
#include "render/Raytracer.h"
#include "system/editor/SceneManager.h"
#include "system/entity/EntityManager.h"
#include "system/Input.h"
#include "system/Time.h"
#include "utils/ImFileDialog.h"
#include "utils/ImGui_Utils.h"
#include "utils/serializer/Serializer.h"
#include "utils/Utils.h"



#pragma region Singleton Methods

// singleton override
void Editor::initialize()
{
	Singleton<Editor>::initialize();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	// enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	// enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	// enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_Utils::SetPurpleTheme();

	ImGui_ImplGlfw_InitForOpenGL(instance->window, true);
	ImGui_ImplOpenGL3_Init("#version 450");

	// init file browser
	ifd::FileDialog::Instance().Initialize();

	// load default scene
	SceneManager::Get().LoadScene("resources/scenes/NinjaScene.devil", "NinjaScene");

	setupDebugScreenQuad();
}

#pragma endregion

#pragma region Public Methods

void Editor::Initialize(GLFWwindow* win)
{
	Get();
	instance->window = win;

	// setup
	instance->editorCamera = new EditorCamera(glm::vec3(0.0f, 5.f, 30.0f));
	instance->sceneBuffer = new FrameBuffer(SCENE_WIDTH, SCENE_HEIGHT, MULTISAMPLES);
	instance->raytracingBuffer = new FrameBuffer(RAYTRACED_SCENE_WIDTH, RAYTRACED_SCENE_HEIGHT, MULTISAMPLES);
	instance->accumulationBuffer = new FrameBuffer(RAYTRACED_SCENE_WIDTH, RAYTRACED_SCENE_HEIGHT, MULTISAMPLES);
	instance->outlineBuffer[0] = new FrameBuffer(SCENE_WIDTH, SCENE_HEIGHT, MULTISAMPLES);
	instance->outlineBuffer[1] = new FrameBuffer(SCENE_WIDTH, SCENE_HEIGHT, MULTISAMPLES);
	instance->depthMapBuffer = new FrameBuffer(SCENE_WIDTH, SCENE_HEIGHT, MULTISAMPLES);
	instance->depthMap = new DepthBuffer();
	instance->inspector = Inspector();

	instance->initialize();
}

Editor::~Editor()
{
	// clean ImGui resources
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// clean
	delete editorCamera;
	delete sceneBuffer;
	delete raytracingBuffer;
	delete accumulationBuffer;
	delete outlineBuffer[0];
	delete outlineBuffer[1];
}

#pragma region Getters

const EditorSettings& Editor::GetSettings() const
{
	return parameters;
}

const EditorCamera* Editor::GetCamera() const
{
	return editorCamera;
}

FrameBuffer* Editor::GetSceneBuffer() const
{
	return sceneBuffer;
}

FrameBuffer* Editor::GetRaytracingBuffer() const
{
	return raytracingBuffer;
}

FrameBuffer* Editor::GetAccumulationBuffer() const
{
	return accumulationBuffer;
}

FrameBuffer* Editor::GetOutlineBuffer(int idx) const
{
	// add assert to idx
	return outlineBuffer[idx];
}

const Entity* Editor::GetSelectedEntity() const
{
	return selectedEntity;
}

#pragma endregion

#pragma region Update

void Editor::Update(float deltaTime)
{
	if (parameters.OrbitMode)
		editorCamera->RotateAround(glm::vec3(0, 0, 0), 0.25f * deltaTime);

	editorCamera->ProcessMatrices();
}

#pragma endregion

#pragma region Rendering

// TODO: set this function in the camera class
void Editor::RenderCamera(Shader* shader)
{
	shader->Use();
	shader->SetVec3("viewPos", editorCamera->Position);
	shader->SetBool("wireframe", parameters.Wireframe);

	const glm::mat4& projection = editorCamera->GetProjectionMatrix(CameraProjectionType::SCENE);
	const glm::mat4& view = editorCamera->GetViewMatrix();
	shader->SetMat4("projection", projection);
	shader->SetMat4("view", view);
}

void Editor::RenderEditor()
{
	unsigned int w = SCENE_WIDTH;
	unsigned int h = SCENE_HEIGHT;

	// rendering ImGui
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::SetNextWindowSize(ImVec2(static_cast<float>(w), static_cast<float>(h)));

	// setup docking space
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 pos(viewport->Pos.x, viewport->Pos.y + TOP_BAR_HEIGHT);
	ImVec2 size(viewport->Size.x, viewport->Size.y - TOP_BAR_HEIGHT);
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	ImGui::Begin("DockSpace", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking);
	ImGuiID dockspace_id = ImGui::GetID("DockSpace");
	ImGui::DockSpace(dockspace_id);
	ImGui::End();

	// clean the screen
	glClear(GL_COLOR_BUFFER_BIT);

	// show loading screen if we are loading entities
	if (EntityManager::Get().IsLoadingEntities())
	{
		SceneManager::Get().ShowLoadingScreen(EntityManager::Get().GetLoadingProgress());
	}
	else 
	{
		// render the scene and the UI
		renderTopBar();
		renderSettings();
		renderHierarchy();
		renderInspector();
		renderRayTracer();
		renderScene(w, h);
		renderShadowMap();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Editor::RenderShadowMap(Shader* shader, Shader* quadShader)
{
	const Light* mainLight = EntityManager::Get().GetMainLight();

	if (mainLight == nullptr) return;

	glm::vec3 lightPos = mainLight->transform->Position;
	glm::vec3 lightDir = glm::normalize(mainLight->GetDirection());
	glm::vec3 right = glm::normalize(glm::cross(lightDir, glm::vec3(0, 1, 0)));
	glm::vec3 up = glm::cross(right, lightDir);

	float nearPlane = 1.0f, farPlane = 50.f;

	glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, nearPlane, farPlane);
	glm::mat4 lightView = glm::lookAt(lightPos, lightPos + lightDir, up);
	lightSpaceMatrix = lightProjection * lightView;
	
	// render scene from light's point of view
	shader->Use();
	shader->SetMat4("lightSpaceMatrix", lightSpaceMatrix);
	
	depthMap->Bind();
	
	EntityManager::Get().DrawAllMeshes(shader);
	
	depthMap->Unbind();
	depthMapBuffer->Bind();

	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	quadShader->Use();
	quadShader->SetFloat("nearPlane", nearPlane);
	quadShader->SetFloat("farPlane", farPlane);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMap->GetDepthTexture());

	glBindVertexArray(debugScreenQuad.VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	depthMapBuffer->Unbind();
	depthMapBuffer->Blit();
}

void Editor::RenderFrame(Shader* shader, CubeMap* cubemap, AxisGrid* grid)
{
	const glm::mat4& cameraView = editorCamera->GetViewMatrix();
	const glm::mat4& cameraProjection = editorCamera->GetProjectionMatrix(CameraProjectionType::SCENE);

	GetSceneBuffer()->Bind(); // bind to framebuffer
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	Editor::Get().RenderCamera(shader);

	// shadow
	shader->SetMat4("lightSpaceMatrix", lightSpaceMatrix);
	glActiveTexture(GL_TEXTURE1);
	glUniform1i(glGetUniformLocation(shader->ID, "shadowMap"), 0);
	glBindTexture(GL_TEXTURE_2D, depthMap->GetDepthTexture());

	EntityManager::Get().ComputeEntities();

	if (parameters.Skybox) 
		cubemap->Draw(cameraView, cameraProjection);
	
	if (parameters.Gizmo && parameters.Grid)
		grid->Draw(editorCamera->Position, cameraView, cameraProjection);

	bool success = EntityManager::Get().ComputeSelectedEntity();

	// unbind framebuffer
	Editor::Get().GetSceneBuffer()->Unbind();

	// set the multisampled texture to the rendered texture only if there's no selected entity because outliner modifying de raw texutre directly
	if (!success)
		Editor::Get().GetSceneBuffer()->Blit();

	glActiveTexture(GL_TEXTURE0);
}

#pragma endregion

#pragma region Callbacks

void Editor::MouseCallback(double xposIn, double yposIn)
{
	if (!Input::GetMouseButton(GLFW_MOUSE_BUTTON_RIGHT))
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		firstMouse = true;
		return;
	}

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	// smooth
	const float sensitivity = .1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	editorCamera->ProcessMouseMovement(xoffset, yoffset);
}

// make sure the viewport matches the new window dimensions; note that width and 
// height will be significantly larger than specified on retina displays.
void Editor::FramebufferSizeCallback(int width, int height)
{
	glViewport(0, 0, width, height);

	SCENE_WIDTH = width;
	SCENE_HEIGHT = height;

	RAYTRACED_SCENE_WIDTH = width;
	RAYTRACED_SCENE_HEIGHT = height;

	sceneBuffer->RescaleFrameBuffer(width, height, MULTISAMPLES);
	raytracingBuffer->RescaleFrameBuffer(width, height, MULTISAMPLES);
	accumulationBuffer->RescaleFrameBuffer(width, height, MULTISAMPLES);
	outlineBuffer[0]->RescaleFrameBuffer(width, height, MULTISAMPLES);
	outlineBuffer[1]->RescaleFrameBuffer(width, height, MULTISAMPLES);
	depthMapBuffer->RescaleFrameBuffer(width, height, MULTISAMPLES);
}

void Editor::ScrollCallback(double xoffset, double yoffset)
{
	editorCamera->ProcessMouseScroll(static_cast<float>(yoffset));
}

#pragma endregion

void Editor::SelectEntity(Entity* entity)
{
	selectedEntity = entity;
}

#pragma endregion

#pragma region Private Methods

// maybe make an input manager with KeyPressed, KeyReleased
void Editor::processInputs()
{
	// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
	float deltaTime = Time::DeltaTime;

	// close the editor by press the escape key
	/*if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);*/

	if (Input::GetKey(GLFW_KEY_W))
		editorCamera->ProcessKeyboard(FORWARD, deltaTime);
	if (Input::GetKey(GLFW_KEY_S))
		editorCamera->ProcessKeyboard(BACKWARD, deltaTime);
	if (Input::GetKey(GLFW_KEY_A))
		editorCamera->ProcessKeyboard(LEFT, deltaTime);
	if (Input::GetKey(GLFW_KEY_D))
		editorCamera->ProcessKeyboard(RIGHT, deltaTime);
	if (Input::GetKey(GLFW_KEY_Q))
		editorCamera->ProcessKeyboard(DOWN, deltaTime);
	if (Input::GetKey(GLFW_KEY_E))
		editorCamera->ProcessKeyboard(UP, deltaTime);

	if (Input::GetKey(GLFW_KEY_LEFT_SHIFT)) // shift key
		editorCamera->SetSpeedFactor(2);
	else
		editorCamera->SetSpeedFactor(1);

	// gizmos shortcuts
	if (Input::GetKeyDown(GLFW_KEY_1))
		gizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
	if (Input::GetKeyDown(GLFW_KEY_2))
		gizmoOperation = ImGuizmo::OPERATION::ROTATE;
	if (Input::GetKeyDown(GLFW_KEY_3))
		gizmoOperation = ImGuizmo::OPERATION::SCALE;

	// mouse picker
	if (Input::GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT) && !ImGuizmo::IsOver())
	{
		// need to check if the mouse is inside the scene window
		ImVec2 sceneWindowPos = ImGui::GetWindowPos();
		ImVec2 sceneWindowSize = ImGui::GetWindowSize();
		ImVec2 sceneWindowMin = ImVec2(sceneWindowPos.x, sceneWindowPos.y);
		ImVec2 sceneWindowMax = ImVec2(sceneWindowPos.x + sceneWindowSize.x, sceneWindowPos.y + sceneWindowSize.y);

		// compute mouse position relative to the imgui window
		ImVec2 mPos = ImGui::GetMousePos();
		ImVec2 mousePosScene = ImVec2(mPos.x - sceneWindowPos.x, mPos.y - sceneWindowPos.y);

		// check if the mouse is inside the scene window
		if (!(mPos.x >= sceneWindowMin.x && mPos.x <= sceneWindowMax.x &&
			mPos.y >= sceneWindowMin.y && mPos.y <= sceneWindowMax.y))
			return;

		glm::vec3 worldPos = Math::ScreenToWorldPoint(glm::vec2(mousePosScene.x, mousePosScene.y)
			, editorCamera->GetViewMatrix()
			, editorCamera->GetProjectionMatrix(CameraProjectionType::SCENE)
			, glm::vec4(0, 0, sceneWindowSize.x, sceneWindowSize.y));

		// raycast
		glm::vec3 direction = glm::normalize(worldPos - editorCamera->Position);
		Ray ray = Ray(editorCamera->Position, direction);
		RaycastHit hit;
		Physics::EditorRaycast(ray, hit);

		if (hit.editorCollider != nullptr)
		{
			SelectEntity(hit.editorCollider->entity);
		}
		else
		{
			SelectEntity(nullptr);
		}
	}
}

void Editor::renderTopBar()
{
	// TODO: correct format for the save and load filedialog
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Scene"))
			{
				resetEntitySelection();
				SceneManager::Get().LoadScene("resources/scenes/Blank.devil", "Blank");
			}
			if (ImGui::MenuItem("Save Scene"))
			{
				SceneManager::Get().ShowSaveSceneDialog();
			}
			if (ImGui::MenuItem("Load Scene"))
			{
				SceneManager::Get().ShowLoadSceneDialog();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Tools"))
		{
			if (ImGui::MenuItem("Logger"))
			{

			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	if (ifd::FileDialog::Instance().IsDone("SaveSceneDialog"))
	{
		if (ifd::FileDialog::Instance().HasResult()) 
		{
			std::string filepath = ifd::FileDialog::Instance().GetResult().string();
			std::string filename = ifd::FileDialog::Instance().GetResult().stem().string();

			Serializer::SaveSceneToFile(filepath, filename);
		}
		ifd::FileDialog::Instance().Close();
	}

	if (ifd::FileDialog::Instance().IsDone("LoadSceneDialog"))
	{
		if (ifd::FileDialog::Instance().HasResult())
		{
			std::string filepath = ifd::FileDialog::Instance().GetResult().string();
			std::string filename = ifd::FileDialog::Instance().GetResult().stem().string();

			resetEntitySelection();
			SceneManager::Get().LoadScene(filepath, filename);
		}
		ifd::FileDialog::Instance().Close();
	}
}

void Editor::renderScene(unsigned int width, unsigned int height)
{
	ImGui::Begin("Scene", nullptr);

	ImGui::BeginChild("GameRender");

	ImVec2 availSize = ImGui::GetContentRegionAvail();
	SCENE_WIDTH = static_cast<unsigned int>(availSize.x);
	SCENE_HEIGHT = static_cast<unsigned int>(availSize.y);

	ImGui::Image(
		reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(GetSceneBuffer()->GetFrameTexture())),
		availSize,
		ImVec2(0, 1),
		ImVec2(1, 0)
	);

	renderToolbar();

	processInputs();

	if (selectedEntity != nullptr)
		transformGizmo(width, height);

	ImGui::EndChild();
	ImGui::End();
}

void Editor::renderToolbar()
{
	// toolbar positioned at the top left corner of the scene window
	ImGui::SetCursorPos(ImVec2(5.0f, 5.0f));
	// color setup
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.7f));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));

	ImGui::BeginChild("ToolbarOverlay", ImVec2(60.0f, 40.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysUseWindowPadding);

	renderPlayButton();

	ImGui::EndChild();

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor();
}

void Editor::renderPlayButton()
{
	if (parameters.isPlaying)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.9f, 0.4f, 1.0f));
	}

	if (ImGui::Button(parameters.isPlaying ? "STOP" : "PLAY", ImVec2(50, 30)))
	{
		parameters.isPlaying = !parameters.isPlaying;
	}

	ImGui::PopStyleColor(3);
}

void Editor::renderShadowMap()
{
	if (!parameters.ShadowMap) return;

	ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(FLT_MAX, FLT_MAX), [](ImGuiSizeCallbackData* data) {
		// force square dimensions
		float newSize = std::min(data->DesiredSize.x, data->DesiredSize.y);
		data->DesiredSize = ImVec2(newSize, newSize);
		});

	ImGui::Begin("ShadowMap", nullptr);
	{
		ImGui::BeginChild("Render");

		ImVec2 availableSize = ImGui::GetContentRegionAvail();
		float squareSize = std::min(availableSize.x, availableSize.y);

		ImGui::Image(
			reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(depthMapBuffer->GetFrameTexture())),
			ImVec2(squareSize, squareSize),
			ImVec2(0, 1),
			ImVec2(1, 0)
		);
	}
	ImGui::EndChild();
	ImGui::End();
}

void Editor::renderRayTracer()
{
	ImGui::Begin("RayTracer", nullptr);
	{
		ImGui::BeginChild("Render");

		float width = ImGui::GetContentRegionAvail().x;
		float height = ImGui::GetContentRegionAvail().y;

		RAYTRACED_SCENE_WIDTH = static_cast<unsigned int>(width);
		RAYTRACED_SCENE_HEIGHT = static_cast<unsigned int>(height);

		FrameBuffer* buffer = parameters.Accumulate ? accumulationBuffer : raytracingBuffer;

		ImGui::Image(
			reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(buffer->GetFrameTexture())),
			ImGui::GetContentRegionAvail(),
			ImVec2(0, 1),
			ImVec2(1, 0)
		);
	}
	ImGui::EndChild();
	ImGui::End();
}

void Editor::renderInspector()
{
	ImGui::Begin("Inspector");
	if (selectedEntity != nullptr)
	{
		inspector.Inspect(selectedEntity);
	}
	ImGui::End();
}

void Editor::renderHierarchy()
{
	ImGui::Begin("Hierarchy");
	{
		EntityManager& manager = EntityManager::Get();
		for (Entity* entity : manager.GetEntities())
		{
			if (ImGui::Selectable(entity->Name.c_str(), selectedEntity == entity))
			{
				SelectEntity(entity);
			}
			if (ImGui::GetID(entity->Name.c_str()) == ImGui::GetHoveredID())
			{
				hoveredEntity = entity;
			}
		}

		// handle right click menu
		showHierarchyContextMenu();
		showEntityContextMenu();
	}
	ImGui::End();
}

void Editor::renderSettings()
{
	// update triangle count
	parameters.TrianglesNumber = EntityManager::Get().GetNumberOfTriangles();

	ImGui::Begin("Editor");
	ImGui::Text("FPS: %.1f", Time::FrameRate());
	ImGui::Text("Frame time : %.1f ms", Time::DeltaTime * 1000);
	ImGui::Text("Triangles: %d", parameters.TrianglesNumber);
	
	ImGui::Separator();
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Gizmos"))
	{
		ImGui_Utils::DrawBoolControl("All", parameters.Gizmo, 100.f);
		if (parameters.Gizmo)
		{
			ImGui_Utils::DrawBoolControl("Grid", parameters.Grid, 100.f);
			ImGui_Utils::DrawBoolControl("Skybox", parameters.Skybox, 100.f);
			ImGui_Utils::DrawBoolControl("Bounding Box", parameters.BoundingBoxGizmo, 100.f);
			ImGui_Utils::DrawBoolControl("BVH", parameters.BVHGizmo, 100.f);
			if (parameters.BVHGizmo)
			{
				ImGui_Utils::SliderInt("Visual Depth", BVH::VISUAL_MAX_DEPTH, 0, BVH::GetMaxDepth(), "%d", 100.f);
			}
		}

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Transform"))
		{
			int transformControl = static_cast<int>(gizmoOperation); // ImGuizmo::OPERATION::TRANSLATE
			ImGui_Utils::DrawComboBoxControl("Mode", transformControl, gizmoOperations, 100.f);
			gizmoOperation = static_cast<ImGuizmo::OPERATION>(transformControl);
			
			int spaceControl = static_cast<int>(gizmoSpace); // ImGuizmo::MODE::LOCAL
			ImGui_Utils::DrawComboBoxControl("Space", spaceControl, gizmoSpaces, 100.f);
			gizmoSpace = static_cast<ImGuizmo::MODE>(spaceControl);

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
	ImGui::Separator();

	if (ImGui::TreeNode("Settings and Actions"))
	{
		ImGui_Utils::DrawBoolControl("Wireframe", parameters.Wireframe, 100.f);
		ImGui_Utils::DrawBoolControl("ShadowMap", parameters.ShadowMap, 100.f);
		ImGui_Utils::DrawBoolControl("OrbitMode", parameters.OrbitMode, 100.f);
		ImGui_Utils::DrawFloatControl("Camera Speed", editorCamera->MovementSpeed, 5.f, 100.f);
		if (ImGui_Utils::DrawButtonControl("Light View", "APPLY", 100.0f))
			setCameraToLightView();
		ImGui::TreePop();
	}
	ImGui::Separator();

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("RayTracing"))
	{
		ImGui_Utils::DrawBoolControl("Enabled", parameters.Raytracing, 100.f);
		ImGui_Utils::DrawBoolControl("BVH", parameters.BVH, 100.f);
		{
			ImGui_Utils::DrawBoolControl("Accumulate", parameters.Accumulate, 100.f);
			if (parameters.Accumulate)
			{
				// progress bar purpose is to show the velocity of the accumulation (based on 10 frames computations)
				ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.6f, 0.3f, 0.6f, 1.0f));
				ImGui::ProgressBar(float(Raytracer::Get().GetFrameCount() % 10) / 10.f, ImVec2(-1, 0), "");
				ImGui::PopStyleColor();
				ImGui::Text("Samples: %d", Raytracer::Get().GetFrameCount());
				ImGui::NewLine();
			}
			ImGui_Utils::DrawIntControl("Max Bounces", parameters.MaxBounces, 1, 100.f);
			ImGui_Utils::DrawIntControl("Rays Per Pixel", parameters.RaysPerPixel, 1, 100.f);
			ImGui_Utils::SliderFloat("Diverge Strength", parameters.DivergeStrength, 0.0f, 10.0f, "%.3f", 135.f);
		}
		ImGui::TreePop();
	}
	ImGui::NewLine();

	ImGui::End();
}

void Editor::transformGizmo(unsigned int width, unsigned int height)
{
	ImGuizmo::BeginFrame();
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, static_cast<float>(width), static_cast<float>(height));

	const glm::mat4& projection = editorCamera->GetProjectionMatrix(CameraProjectionType::SCENE);
	const glm::mat4& view = editorCamera->GetViewMatrix();
	glm::mat4 model = selectedEntity->transform->GetTransformMatrix();

	// snapping
	bool snap = Input::GetKey(GLFW_KEY_LEFT_CONTROL);
	float snapValue = 1.f; // snap to 1m for translation and scale

	if (gizmoOperation == ImGuizmo::OPERATION::ROTATE)
		snapValue = 22.5f; // snap to 22.5 degrees for rotation

	float snapValues[3] = { snapValue, snapValue, snapValue };

	ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), 
		gizmoOperation, gizmoSpace,
		glm::value_ptr(model),
		nullptr, snap ? snapValues : nullptr);

	if (ImGuizmo::IsUsing())
	{
		// decompose the model matrix
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		
		glm::decompose(model, scale, rotation, translation, skew, perspective);

		if (scale.x > 0 && scale.y > 0 && scale.z > 0)
		{
			glm::vec3 deltaRotation = glm::eulerAngles(rotation) - glm::radians(selectedEntity->transform->Rotation);

			selectedEntity->transform->SetPosition(translation);
			selectedEntity->transform->SetRotation(selectedEntity->transform->Rotation + glm::degrees(deltaRotation));
			selectedEntity->transform->SetScale(scale);
		}
	}
}

// show context menu when right click on the hierarchy                            
// for the entity creation we generate a name to avoid duplicate name in the scene
void Editor::showHierarchyContextMenu()
{
	if (!ImGui::IsAnyItemHovered() || ImGui::IsPopupOpen("HierarchyContextMenu"))
	{
		if (ImGui::BeginPopupContextWindow("HierarchyContextMenu"))
		{
			if (ImGui::BeginMenu("Create"))
			{
				if (ImGui::MenuItem("Load Mesh"))
				{
					ifd::FileDialog::Instance().Open("LoadMeshDialog", "Load Mesh", "Mesh file (*.obj;*.fbx){.obj,.fbx},.*");
				}

				// TODO: make a method instead of redondant code
				if (ImGui::MenuItem("Sphere"))
				{
					const std::string name = EntityManager::Get().GenerateNewEntityName("Sphere");
					Entity* entity = EntityManager::Get().CreateEntity(name);
					entity->transform->SetPosition(editorCamera->Position + editorCamera->Front * 15.0f);
					entity->AddComponent<Model>(SpherePrimitive);
					SelectEntity(entity);
				}
				if (ImGui::MenuItem("Cube"))
				{
					const std::string name = EntityManager::Get().GenerateNewEntityName("Cube");
					Entity* entity = EntityManager::Get().CreateEntity(name);
					entity->transform->SetPosition(editorCamera->Position + editorCamera->Front * 15.0f);
					entity->AddComponent<Model>(CubePrimitive);
					SelectEntity(entity);
				}
				if (ImGui::MenuItem("Plane"))
				{
					const std::string name = EntityManager::Get().GenerateNewEntityName("Plane");
					Entity* entity = EntityManager::Get().CreateEntity(name);
					entity->transform->SetPosition(editorCamera->Position + editorCamera->Front * 15.0f);
					entity->AddComponent<Model>(PlanePrimitive);
					SelectEntity(entity);
				}
				if (ImGui::MenuItem("Cylinder"))
				{
					const std::string name = EntityManager::Get().GenerateNewEntityName("Cylinder");
					Entity* entity = EntityManager::Get().CreateEntity(name);
					entity->transform->SetPosition(editorCamera->Position + editorCamera->Front * 15.0f);
					entity->AddComponent<Model>(CylinderPrimitive);
					SelectEntity(entity);
				}
				if (ImGui::MenuItem("Torus"))
				{
					const std::string name = EntityManager::Get().GenerateNewEntityName("Torus");
					Entity* entity = EntityManager::Get().CreateEntity(name);
					entity->transform->SetPosition(editorCamera->Position + editorCamera->Front * 15.0f);
					entity->AddComponent<Model>(TorusPrimitive);
					SelectEntity(entity);
				}
				ImGui::EndMenu();
			}
			ImGui::EndPopup();
		}
	}

	if (ifd::FileDialog::Instance().IsDone("LoadMeshDialog"))
	{
		if (ifd::FileDialog::Instance().HasResult())
		{
			std::string filepath = ifd::FileDialog::Instance().GetResult().string();
			std::string filename = ifd::FileDialog::Instance().GetResult().stem().string();

			const std::string name = EntityManager::Get().GenerateNewEntityName(filename);
			Entity* entity = EntityManager::Get().CreateEntity(name);
			entity->transform->SetPosition(editorCamera->Position + editorCamera->Front * 15.0f);
			entity->AddComponent<Model>(Utils::GetSingleSlashPath(filepath));
			SelectEntity(entity);
		}
		ifd::FileDialog::Instance().Close();
	}
}

/* Show context menu when right click on an entity
/* We want to have the ability to delete/duplicate the entity */
void Editor::showEntityContextMenu()
{
	if (hoveredEntity == nullptr)
		return;

	if (!ImGui::IsAnyItemHovered() && !ImGui::IsPopupOpen("HierarchyContextEntity"))
	{
		hoveredEntity = nullptr;
		return;
	}
		
	if (ImGui::BeginPopupContextWindow("HierarchyContextEntity"))
	{
		if (ImGui::Selectable("Delete"))
		{
			Entity* entity = hoveredEntity;
			resetEntitySelection();
			EntityManager::Get().DestroyEntity(entity);
		}
		if (ImGui::Selectable("Duplicate"))
		{
			Entity* entity = hoveredEntity;
			resetEntitySelection();
			Entity* duplicatedEntity = EntityManager::Get().DuplicateEntity(entity);
			SelectEntity(duplicatedEntity);
		}
		ImGui::EndPopup();
	}
}

void Editor::resetEntitySelection()
{
	selectedEntity = nullptr;
	hoveredEntity = nullptr;
}

void Editor::setCameraToLightView()
{
	const Light* mainLight = EntityManager::Get().GetMainLight();

	if (mainLight == nullptr) return;

	editorCamera->SetPositionAndDirection(mainLight->transform->Position, mainLight->GetDirection());
}

void Editor::setupDebugScreenQuad()
{
	debugScreenQuad = ScreenQuad();

	glGenVertexArrays(1, &debugScreenQuad.VAO);
	glGenBuffers(1, &debugScreenQuad.VBO);
	glBindVertexArray(debugScreenQuad.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, debugScreenQuad.VBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(debugScreenQuad.vertices), &debugScreenQuad.vertices, GL_STATIC_DRAW);
	// position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
	// texture coord attribute
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glBindVertexArray(0);
}

#pragma endregion