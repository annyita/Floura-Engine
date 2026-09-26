#include "ImGuiWindow.h"
#include <Core/File/File.h>
#include <Sound/SoundRunner.h>
#include <Render/pipeline/prebuilt_pipelines/depreciated/raytracer.h>
#include <Scene/scene.h>
#include <Gameplay/Player.h>
#include <Render/window/WindowHandler.h>
#include <glm/gtx/euler_angles.hpp>
#include "ImGuiInclude.h"
#include "FE_ImGui.h"
#include <Scene/LightingHandler.h>
#include "utils/logConsole.h"
#include <utils/FE_math.h>
#include "ImGuiInclude/EcsInspector.h"
#include "Render/pipeline/prebuilt_pipelines/depreciated/denoise.h"
#include <Systems/util/relationshipManager.h>

#include "Render/Handler/RenderHandler.h"
#include  "Render/pipeline/prebuilt_pipelines/dbgPass.h"
#include "Render/Handler/UniformManager.h"
#include  "Render/pipeline/prebuilt_pipelines/swrt.h"
//#include <Instance.h>


bool FEImGuiWindow::imGuiEnabled = true;
bool FEImGuiWindow::showViewportIcons = true;
bool FEImGuiWindow::imGuiPanels[] = { true, true, true, true, true, true, true, true, true, true, true, true, true }; // ImGui Panels

bool FEImGuiWindow::isWireframe = false;

std::string FEImGuiWindow::SelectedObjectType;
int FEImGuiWindow::SelectedObjectIndex;

std::vector<std::string> FEImGuiWindow::ContentObjects;
std::vector<std::string> FEImGuiWindow::ContentObjectNames;
std::vector<std::string> FEImGuiWindow::ContentObjectPaths;
std::vector<std::string> FEImGuiWindow::ContentObjectTypes;

std::vector<std::string> FEImGuiWindow::MaterialObjectPaths;
std::vector<const char*> FEImGuiWindow::MaterialObjecNames;
int FEImGuiWindow::MaterialSelectedIndex = 0;

Texture FEImGuiWindow::logoIcon; // Icon for logo in ImGui
Texture FEImGuiWindow::FolderIcon; // Icon for folder in ImGui
Texture FEImGuiWindow::SaveIcon; // Icon for Save in ImGui
Texture FEImGuiWindow::ModelIcon; // Icon for Object in ImGui
Texture FEImGuiWindow::BillBoardIcon; // Icon for BillBoard in ImGui 
Texture FEImGuiWindow::colliderIcon; // Icon for Collider in ImGui 
Texture FEImGuiWindow::pointLightIcon; // Icon for pointLightIcon in ImGui
Texture FEImGuiWindow::spotLightIcon; // Icon for spotLightIcon in ImGui 
Texture FEImGuiWindow::arrowIcon; // Icon for arrow in ImGui 
Texture FEImGuiWindow::crossIcon; // Icon for cross in ImGui 
Texture FEImGuiWindow::plusIcon; // Icon for plus in ImGui
Texture FEImGuiWindow::cameraIcon; // Icon for camera in ImGui
Texture FEImGuiWindow::skyboxIcon; // Icon for skybox in ImGui
Texture FEImGuiWindow::directLight; // Icon for directLight in ImGui
Texture FEImGuiWindow::materialIcon; // Icon for material in ImGui
Texture FEImGuiWindow::environmentIcon; // Icon for material in ImGui
Texture FEImGuiWindow::moveIcon; // Icon for move in ImGui
Texture FEImGuiWindow::scaleIcon; // Icon for scale in ImGui
Texture FEImGuiWindow::rotateIcon; // Icon for rotate in ImGui
Texture FEImGuiWindow::wirefameIcon; // Icon for wireframe in ImGui
Texture FEImGuiWindow::iIcon; // Icon for Icon in ImGui
Texture FEImGuiWindow::SoundIcon; // Icon for Sound in ImGui
Texture FEImGuiWindow::stopIcon; // Icon for stop in ImGui
Texture FEImGuiWindow::playIcon; // Icon for play in ImGui
Texture FEImGuiWindow::pauseIcon; // Icon for pause in ImGui
Texture FEImGuiWindow::gizmoIcon; // Icon for pause in ImGui
Texture FEImGuiWindow::emptyIcon;
Texture FEImGuiWindow::volumeAREAIcon;

// collideicon.png

// Temporary buffer for path editing
static char pathBuffer[256]; // Ensure the size is appropriate

void FEImGuiWindow::init() {
	logoIcon.createTexture("Assets/Icons/Icon.png", "UI", 0);
	FolderIcon.createTexture("Assets/Icons/bindericon.png", "UI", 0);
	SaveIcon.createTexture("Assets/Icons/saveicon.png", "UI", 0);
	ModelIcon.createTexture("Assets/Icons/meshicon.png", "UI", 0);
	BillBoardIcon.createTexture("Assets/Icons/pictureIcon.png", "UI", 0);
	colliderIcon.createTexture("Assets/Icons/collideicon.png", "UI", 0);
	pointLightIcon.createTexture("Assets/Icons/point.png", "UI", 0);
	spotLightIcon.createTexture("Assets/Icons/spot.png", "UI", 0);
	arrowIcon.createTexture("Assets/Icons/arrow.png", "UI", 0);
	crossIcon.createTexture("Assets/Icons/cross.png", "UI", 0);
	plusIcon.createTexture("Assets/Icons/plus.png", "UI", 0);
	cameraIcon.createTexture("Assets/Icons/cameraIcon.png", "UI", 0);
	skyboxIcon.createTexture("Assets/Icons/skyboxIcon.png", "UI", 0);
	directLight.createTexture("Assets/Icons/directLight.png", "UI", 0);
	materialIcon.createTexture("Assets/Icons/materialIcon.png", "UI", 0);
	environmentIcon.createTexture("Assets/Icons/Environment.png", "UI", 0);
	moveIcon.createTexture("Assets/Icons/move.png", "UI", 0);
	scaleIcon.createTexture("Assets/Icons/scale.png", "UI", 0);
	rotateIcon.createTexture("Assets/Icons/rotate.png", "UI", 0);
	wirefameIcon.createTexture("Assets/Icons/wireframeIcon.png", "UI", 0);
	iIcon.createTexture("Assets/Icons/iIcon.png", "UI", 0);
	SoundIcon.createTexture("assets/Icons/soundIcon.png", "UI", 0);
	stopIcon.createTexture("assets/Icons/stop.png", "UI", 0);
	playIcon.createTexture("assets/Icons/play.png", "UI", 0);
	pauseIcon.createTexture("assets/Icons/pause.png", "UI", 0);
	gizmoIcon.createTexture("assets/Icons/gizmo.png", "UI", 0);
	emptyIcon.createTexture("assets/Icons/empty.png", "UI", 0);
	volumeAREAIcon.createTexture("assets/Icons/volumeLogo.png", "UI", 0);
}

void FEImGuiWindow::initImGui(GLFWwindow* window) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
	//pow(0.35f, 1.0f / 2.2f), pow(input, 1.0f / 2.2f), pow(input, 1.0f / 2.2f), pow(input, 1.0f / 2.2f);

	glm::vec3 colourF        = glm::vec3(0.36f, 0.72f, 0.92f);
	glm::vec3 backgoundF     = glm::vec3(0.25f, 0.20f, 0.25f);  

	glm::vec3 colour_hoveredF = glm::vec3(0.56f, 0.82f, 0.98f);
	glm::vec3 colour_activeF  = glm::vec3(0.26f, 0.62f, 0.82f);

	glm::vec3 bg_frameF      = glm::vec3(0.12f, 0.08f, 0.11f); 
	glm::vec3 bg_popupF      = glm::vec3(0.15f, 0.10f, 0.13f); 
	glm::vec3 bg_borderF     = glm::vec3(0.96f, 0.96f, 1.00f) * glm::vec3(0.5f); 


	ImGuiStyle& Style = ImGui::GetStyle();
	ImVec4 colour = ImVec4(colourF.x, colourF.y, colourF.z, 1.0f);
	ImVec4 backgound = ImVec4(backgoundF.x, backgoundF.y, backgoundF.z, 1.0f);

	ImVec4 colour_hovered = ImVec4(colour_hoveredF.x, colour_hoveredF.y, colour_hoveredF.z, 1.0f);
	ImVec4 colour_active = ImVec4(colour_activeF.x, colour_activeF.y, colour_activeF.z, 1.0f);

	ImVec4 bg_frame = ImVec4(bg_frameF.x, bg_frameF.y, bg_frameF.z, 1.0f);
	ImVec4 bg_popup = ImVec4(bg_popupF.x, bg_popupF.y, bg_popupF.z, 1.0f);
	ImVec4 bg_border = ImVec4(bg_borderF.x, bg_borderF.y, bg_borderF.z, 1.0f);

	float dim_factor = 0.35f;
	ImVec4 colour_dimmed = ImLerp(backgound, colour, dim_factor);

	glm::vec3 colourG = (glm::vec3(0.88f, 0.88f, 0.88f));
	// General Colors
	Style.Colors[ImGuiCol_Text] = ImVec4(colourG.x, colourG.y, colourG.z, 1.0f);
	Style.Colors[ImGuiCol_WindowBg] = backgound;
	Style.Colors[ImGuiCol_ChildBg] = backgound;
	Style.Colors[ImGuiCol_PopupBg] = bg_popup;

	// Borders
	Style.Colors[ImGuiCol_Border] = bg_border;
	Style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

	// Frames
	Style.Colors[ImGuiCol_FrameBg] = bg_frame;
	Style.Colors[ImGuiCol_FrameBgHovered] = colour_hovered;
	Style.Colors[ImGuiCol_FrameBgActive] = colour_active;

	// Buttons
	Style.Colors[ImGuiCol_Button] = colour;
	Style.Colors[ImGuiCol_ButtonHovered] = colour_hovered;
	Style.Colors[ImGuiCol_ButtonActive] = colour_active;

	// Headers
	Style.Colors[ImGuiCol_Header] = colour;
	Style.Colors[ImGuiCol_HeaderHovered] = colour_hovered;
	Style.Colors[ImGuiCol_HeaderActive] = colour_active;

	// Tabs
	Style.Colors[ImGuiCol_Tab] = colour_dimmed;
	Style.Colors[ImGuiCol_TabActive] = colour;
	Style.Colors[ImGuiCol_TabHovered] = colour_hovered;
	Style.Colors[ImGuiCol_TabUnfocused] = colour_dimmed;
	Style.Colors[ImGuiCol_TabUnfocusedActive] = colour;

	// Title Bars
	Style.Colors[ImGuiCol_TitleBg] = colour_active;
	Style.Colors[ImGuiCol_TitleBgActive] = colour_hovered;
	Style.Colors[ImGuiCol_TitleBgCollapsed] = colour_dimmed;

	// Docking
	Style.Colors[ImGuiCol_DockingEmptyBg] = bg_frame;
	Style.Colors[ImGuiCol_DockingPreview] = colour_hovered;

	// Resize Grip
	Style.Colors[ImGuiCol_ResizeGrip] = colour;
	Style.Colors[ImGuiCol_ResizeGripHovered] = colour_hovered;
	Style.Colors[ImGuiCol_ResizeGripActive] = colour_hovered;

	// Scrollbars
	Style.Colors[ImGuiCol_ScrollbarBg] = bg_frame;
	Style.Colors[ImGuiCol_ScrollbarGrab] = colour_dimmed;
	Style.Colors[ImGuiCol_ScrollbarGrabHovered] = colour;
	Style.Colors[ImGuiCol_ScrollbarGrabActive] = colour_hovered;

	// Sliders
	Style.Colors[ImGuiCol_SliderGrab] = colour;
	Style.Colors[ImGuiCol_SliderGrabActive] = colour_active;

	// Checkbox
	Style.Colors[ImGuiCol_CheckMark] = colour_hovered;

	// Menus
	Style.Colors[ImGuiCol_MenuBarBg] = bg_border;

	// Drag and drop
	Style.Colors[ImGuiCol_DragDropTarget] = colour_hovered;

	// Misc Separators
	Style.Colors[ImGuiCol_Separator] = colour_dimmed;
	Style.Colors[ImGuiCol_SeparatorHovered] = colour_hovered;
	Style.Colors[ImGuiCol_SeparatorActive] = colour_hovered;

	Style.FrameRounding = 8.0f;
	Style.WindowRounding = 6.0f;
	Style.ScrollbarRounding = 8.0f;
	Style.GrabRounding = 8.0f;

	Style.WindowPadding = ImVec2(8, 8);
	Style.FramePadding = ImVec2(6, 3);
	Style.ItemSpacing = ImVec2(7, 5);
}


void FEImGuiWindow::loadContentObjects(std::string path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cout << "Failed to open file: " << path << std::endl;
		return;
	}
	json ContentObjectFileData;
	try {
		file >> ContentObjectFileData;
	}
	catch (const nlohmann::json::parse_error& e) {
		std::cout << "JSON Parse Error loading ContentObject data: " << e.what() << std::endl;
		std::cout << "Error byte position: " << e.byte << std::endl; // Specific to nlohmann::json
	}
	catch (const std::ios_base::failure& e) {
		std::cout << "File I/O Error loading ContentObject data: " << e.what() << std::endl;
	}
	catch (const std::exception& e) {
		std::cout << "An unexpected error occurred loading ContentObject data: " << e.what() << std::endl;
	}
	file.close();
	int iteration = 0;
	for (const auto& item : ContentObjectFileData) {
		std::string contentObject = item.at("ObjectType").get<std::string>();
		std::string contentObjectType = item.at("Type").get<std::string>();
		std::string contentObjectPath = item.at("Path").get<std::string>();
		std::string contentObjectName = item.at("Name").get<std::string>();


		if (contentObject != "") {
			ContentObjects.push_back(contentObject); // Add a new content object to the list
			ContentObjectTypes.push_back(contentObjectType); // Add a new content object type to the list
			ContentObjectPaths.push_back(contentObjectPath); // Add a new content object path to the list
			ContentObjectNames.push_back(contentObjectName); // Add a new content object name to the list	
		}

		iteration++;
	}
	FEImGuiWindow::MaterialIndexUpdate();

	std::cout << "Loaded Scene ContentObject from: " << path << std::endl;

}

void FEImGuiWindow::saveContentObjects(std::string path) {
	try {
		json ContentObjectData = json::array();  // New JSON array to hold model data
		// Serialize each modelObject into JSON

		FEImGuiWindow::MaterialIndexUpdate();

		for (size_t i = 0; i < ContentObjects.size(); ++i) {
			json ContentObjectJson;
			ContentObjectJson["ObjectType"] = ContentObjects[i];
			ContentObjectJson["Type"] = ContentObjectTypes[i];
			ContentObjectJson["Path"] = ContentObjectPaths[i];
			ContentObjectJson["Name"] = ContentObjectNames[i];
			if (ContentObjects[i] != "") {
				ContentObjectData.push_back(ContentObjectJson);	
			}

		}
		// Write to file
		std::ofstream outFile(path, std::ios::out);
		if (!outFile.is_open()) {
			std::cout << "Failed to write to " << path << std::endl;
			return;
		}
		outFile << ContentObjectData.dump(4);  // Pretty-print with indentation
		outFile.close();
		std::cout << "Successfully updated " << path << std::endl;
	}
	catch (const std::exception& e) {
		std::cout << "Exception: " << e.what() << std::endl;
	}
}

// 0 positon, 1 scale, 2 rotate
int SelectedTransform = 0; 

glm::vec3 igPosition(glm::vec3 input)
{
	glm::mat4 T = glm::translate(glm::mat4(1.0f), input);

	ImGuizmo::Manipulate(glm::value_ptr(Scene::maincamera.view), glm::value_ptr(Scene::maincamera.projection),
		ImGuizmo::TRANSLATE, ImGuizmo::LOCAL,
		glm::value_ptr(T));

	if (ImGuizmo::IsUsing()) {
		return glm::vec3(T[3][0], T[3][1], T[3][2]);
	}
	return input;
}

glm::mat4 useGuizmo(glm::vec3 inputPosition, glm::vec3 inputRotation, glm::vec3 inputScale, int SelectedTransform)
{
	glm::mat4 mat = FE_Math::composeMatrix(inputPosition, inputScale, inputRotation);

	if (SelectedTransform == 0)
	{
		ImGuizmo::Manipulate(glm::value_ptr(Scene::maincamera.view), glm::value_ptr(Scene::maincamera.projectionAlwaysUnjittered),
			ImGuizmo::TRANSLATE, ImGuizmo::WORLD,
			glm::value_ptr(mat));
	}
	else if (SelectedTransform == 1)
	{
		ImGuizmo::Manipulate(glm::value_ptr(Scene::maincamera.view), glm::value_ptr(Scene::maincamera.projectionAlwaysUnjittered),
			ImGuizmo::SCALE, ImGuizmo::WORLD,
			glm::value_ptr(mat));
	}
	else if (SelectedTransform == 2)
	{
		ImGuizmo::Manipulate(glm::value_ptr(Scene::maincamera.view), glm::value_ptr(Scene::maincamera.projectionAlwaysUnjittered),
			ImGuizmo::ROTATE, ImGuizmo::WORLD,
			glm::value_ptr(mat));
	}

	return mat;
}


void FEImGuiWindow::Update() {
	//Tell Imgui a new frame is about to begin
//
	//glDisable(GL_FRAMEBUFFER_SRGB);
	ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
	ImGuizmo::BeginFrame();
	

	//glEnable(GL_FRAMEBUFFER_SRGB);
	menuwindow();

	// Rendering panel
	if (FEImGuiWindow::imGuiPanels[1]) RenderWindow();
	if (FEImGuiWindow::imGuiPanels[3]) viewport();
	if (FEImGuiWindow::imGuiPanels[6]) PerformanceProfiler();
	if (FEImGuiWindow::imGuiPanels[7]) TextEditor();
	if (FEImGuiWindow::imGuiPanels[4]) HierarchyList();
	if (FEImGuiWindow::imGuiPanels[9]) EcsInspector::InspectorWindow();
	if (FEImGuiWindow::imGuiPanels[10]) SceneFolderWindow();
	if (FEImGuiWindow::imGuiPanels[11]) ConsoleWindow();
	//
	// imguizmo selection
	//
	if (FEImGuiWindow::SelectedObjectType != "") // needs guizmo enabled var // passes if object is selected
	{
		if (FEImGuiWindow::SelectedObjectType == "Model" || FEImGuiWindow::SelectedObjectType == "Billboard" || FEImGuiWindow::SelectedObjectType == "Empty") { // this needs to change to enum at some point

			glm::mat4 mat = useGuizmo(Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->fetchPosition(), Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->fetchRotation(), Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->fetchScale(), SelectedTransform);

			if (ImGuizmo::IsUsing()) {
				if (ImGuizmo::IsUsing()) {
					glm::vec3 position, rotation_unstable, scale;
					ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(mat),
						glm::value_ptr(position),
						glm::value_ptr(rotation_unstable),
						glm::value_ptr(scale));

					glm::quat newRotationQuat = glm::quat_cast(mat);

					if (SelectedTransform == 0) Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->setPosition(position);
					else if (SelectedTransform == 1) Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->setScale(scale);
					else if (SelectedTransform == 2) Scene::entityObjects[FEImGuiWindow::SelectedObjectIndex]->setRotation(rotation_unstable);	
				}
			}
		}
		else if (FEImGuiWindow::SelectedObjectType == "Light") LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].position = igPosition(LightingHandler::Lights[FEImGuiWindow::SelectedObjectIndex].position);
		else if (FEImGuiWindow::SelectedObjectType == "Sound") Scene::SoundObjects[FEImGuiWindow::SelectedObjectIndex].position = igPosition(Scene::SoundObjects[FEImGuiWindow::SelectedObjectIndex].position);
	}

	//scene
	ImGui::Render(); // Renders the ImGUI elements
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
static const char* rendererItems[]{ "None","Deferred", "Forward", "SWRT"};
void FEImGuiWindow::menuwindow()
{
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);

	// Important flags for the main dockspace window
	ImGuiWindowFlags window_flags =
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus;

	// Begin the main docking window
	ImGui::Begin("Docking Window", nullptr, window_flags);

	if (ImGui::BeginMenuBar())
	{
		ImGui::Image(
			(ImTextureID)(intptr_t)FEImGuiWindow::logoIcon.ID,
			ImVec2(20, 20)
		);

		if (ImGui::BeginMenu("File"))
		{
			ImGui::MenuItem("New Scene");
			if (ImGui::MenuItem("Open Scene")) {
				IGFD::FileDialogConfig config;
				config.path = ".";
				ImGuiFileDialog::Instance()->OpenDialog("LoadScene", "Choose Scene", ".*", config);
			}

			ImGui::MenuItem("Save Scene As");
			if (ImGui::MenuItem("Save Scene")) Scene::saveScene(Scene::sceneName);
			if (ImGui::MenuItem("Reload Scene")) Scene::loadScene(Scene::sceneName);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View")){
			ImGui::Combo("Renderer", &RenderClass::currentRendererInd, rendererItems, IM_ARRAYSIZE(rendererItems));
			ImGui::Checkbox("Wireframe", &FEImGuiWindow::isWireframe);
			ImGui::Checkbox("showBoxCollider", &Collision::showBoxCollider);
			ImGui::Checkbox("showViewportIcons", &FEImGuiWindow::showViewportIcons);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Reload Shaders?")) RenderClass::initGlobalShaders();
			ImGui::Spacing();
			ImGui::Text("Volume");
			ImGui::SliderFloat("Global Volume", &SoundRunner::GlobalVolume, 0, 1);
			ImGui::SliderFloat("Music Volume", &SoundRunner::MusicVolume, 0, 1);
			ImGui::SliderFloat("Environment Volume", &SoundRunner::environmentVolume, 0, 1);
			ImGui::SliderFloat("Entity Volume", &SoundRunner::entityVolume, 0, 1);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Panels")) {
			//panel
			FEImGuiWindow::PanelsWindow();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Debug")) {
			ImGui::Checkbox("overlayDebug", &dbgPass::overlayDebug);
			ImGui::Checkbox("doPlayerBoxCollision: ", &Player::CollideWithCamera);
			ImGui::EndMenu();
		}
		ImGui::SameLine(ImGui::GetWindowWidth() - 100.0f);
		if (ImGui::ImageButton("##SaveIcon", (ImTextureID)FEImGuiWindow::SaveIcon.ID, ImVec2(10, 10))) {
			Scene::saveScene(Scene::sceneName);
		}
		if (ImGui::ImageButton("##arrowIcon", (ImTextureID)FEImGuiWindow::arrowIcon.ID, ImVec2(10, 10))) {
			Scene::loadScene(Scene::sceneName);
		}
		if (ImGui::ImageButton("##crossIcon", (ImTextureID)FEImGuiWindow::crossIcon.ID, ImVec2(10, 10))) {
			Scene::Delete();
		}

		ImGui::EndMenuBar();
	}

	// display
	if (ImGuiFileDialog::Instance()->Display("LoadScene")) {
		if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
			std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
			std::replace(filePath.begin(), filePath.end(), '\\', '/');
			Scene::sceneName = filePath;
			std::cout << filePath << std::endl;
			Scene::loadScene(filePath);
		}
		ImGuiFileDialog::Instance()->Close();
	}

	ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

	ImGui::End();
}

void FEImGuiWindow::SystemInfomation() {
	if (ImGui::TreeNode("System information")) {
		GLint temp = 0;
		ImGui::Text("OpenGL Version: %s", glGetString(GL_VERSION)); // Display OpenGL version
		ImGui::Text("Renderer: %s", glGetString(GL_RENDERER));  // Display GPU renderer
		ImGui::Text("Vendor: %s", glGetString(GL_VENDOR));  // Display GPU vendor
		ImGui::Text("Shading lang verr: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &temp);
		ImGui::Text("Max Colour Attachments: %s", std::to_string(temp));
		glGetIntegerv(GL_MAX_DRAW_BUFFERS, &temp);
		ImGui::Text("Max Draw Buffers: %s", std::to_string(temp));
		ImGui::Text("glfw version. %s", glfwGetVersionString());
		

		ImGui::Text((std::string("Viewport Size: ") + std::to_string(static_cast<int>(renderTarget::ViewPortWidth)) + "*" + std::to_string(static_cast<int>(renderTarget::ViewPortHeight))).c_str());
		
		ImGui::TreePop();// Ends The ImGui Window
	}

}

void FEImGuiWindow::RenderWindow() {
	ImGui::Begin("Rendering"); // ImGUI window creation
	ImGui::Combo("Renderer", &RenderClass::currentRendererInd, rendererItems, IM_ARRAYSIZE(rendererItems));
	if (ImGui::SmallButton("Reload Model Shaders?")) for (size_t i = 0; i < ShaderHandler::shaderObjects.size(); i++)ShaderHandler::reloadShader(i);
	if (ImGui::SmallButton("Reload Global Shaders")) RenderClass::initGlobalShaders();
	if (ImGui::SmallButton("Compile Shader Uniforms")) RenderClass::compileShaderUniforms();
	ImGui::Checkbox("compileOnDirty", &UniformManager::compileOnDirty);
	if (ImGui::TreeNode("window")) {
		ImGui::Checkbox("Vsync", &windowHandler::doVsync); // Set the value of doVsync (bool)
		// Screen
		ImGui::DragInt("Width", &renderTarget::tempWidth);
		ImGui::DragInt("Height", &renderTarget::tempHeight); // screen slider

		if (ImGui::SmallButton("Apply Changes?")) { // apply button
			glViewport(0, 0, renderTarget::tempWidth, renderTarget::tempHeight); // real internal res
			glfwSetWindowSize(windowHandler::window, renderTarget::tempWidth, renderTarget::tempHeight);
			windowHandler::setVSync(windowHandler::doVsync); // Set Vsync to value of doVsync (bool)
			renderTarget::updateFrameBufferResolution(renderTarget::tempWidth, renderTarget::tempHeight); // Update frame buffer resolution
		}
		if (ImGui::SmallButton("Toggle Fullscreen (WARNING WILL TOGGLE HDR OFF)")){
			windowHandler::toggleFullscreen(windowHandler::window, windowHandler::isFullscreen, windowHandler::width, windowHandler::height); //needs to be fixed //GLFWwindow* &window, GLFWmonitor* &monitor, int windowedWidth, int windowedHeight
		} //Toggle Fullscreen

		ImGui::TreePop();// Ends The ImGui Window
	}
	
	if (ImGui::TreeNode("ScreenSpace")) {
		ImGui::Checkbox("doSSR", &RenderClass::doSSR);
		ImGui::Checkbox("doContactShadows", &RenderClass::doContactShadows);
		ImGui::DragInt("indirectSamples", &ProbeHandler::indirectSamples);
		
		ImGui::TreePop();// Ends The ImGui Window
	}
	
	if (ImGui::TreeNode("General")) {
		ImGui::Checkbox("doBinaryAlpha", &RenderClass::doBinaryAlpha);
		ImGui::Checkbox("animateBinaryAlpha", &RenderClass::animateBinaryAlpha);
		
		ImGui::TreePop();// Ends The ImGui Window
	}
	//RenderClass::doTAA
	if (ImGui::TreeNode("Anti Aliasing & Post")) {
		ImGui::Checkbox("doTAA", &RenderClass::doTAA);
		ImGui::DragFloat("Jitter Factor", &RenderClass::jitterFactor);
		ImGui::DragFloat("post sharpness", &renderTarget::sharpness);
		
		ImGui::TreePop();
	}
	if (RenderClass::currentRendererInd == RenderClass::SWRT){
		if (ImGui::TreeNode("Raytracer")) {
			ImGui::DragFloat("Resolution Scale Factor", &FlouraSWRT::resScaleFactor);
			
			if (ImGui::TreeNode("ray world")){
				// figures users might abuse this slider too much so i limited it, but i like extremes so i left it high
				ImGui::DragInt("Auto MDF Res (force)", &FlouraSWRT::autoMDFres, 1.0, 1, 8192);
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("denoising")){
				ImGui::Checkbox("doDenoiseSplitDBGView", &FlouraSWRT::doDenoiseSplitDBGView);
				ImGui::Spacing();
				ImGui::Checkbox("doTemporalAccumulation", &FlouraSWRT::doTemporalAccumulation);
				ImGui::DragFloat("temporalAccumulationBlendFactor",&FlouraSWRT:: temporalAccumulationBlendFactor);
				ImGui::SliderInt("svgf passes", &FlouraSWRT::passNum, 1, 5);
				ImGui::Checkbox("doSVGF", &FlouraSWRT::doSVGF);
					
				ImGui::TreePop();
			}
			ImGui::TreePop();
		}
	}

		ImGui::End();
}

void FEImGuiWindow::PanelsWindow() {
	ImGui::Text("Settings (Press escape to use mouse)");
	if (ImGui::SmallButton("load")) { FileClass::loadSettings(); } // load settings button
	if (ImGui::SmallButton("save (just settings)")) { FileClass::saveSettings(); } // save settings button
	ImGui::Checkbox("Rendering", &FEImGuiWindow::imGuiPanels[1]);
	ImGui::Checkbox("ViewPort", &FEImGuiWindow::imGuiPanels[3]);
	ImGui::Checkbox("Scene Hierarchy", &FEImGuiWindow::imGuiPanels[4]);
	ImGui::Checkbox("Performance Profiler", &FEImGuiWindow::imGuiPanels[6]);
	ImGui::Checkbox("Text Editor", &FEImGuiWindow::imGuiPanels[7]);
	ImGui::Checkbox("Inspector", &FEImGuiWindow::imGuiPanels[9]);
	ImGui::Checkbox("Scene Folder", &FEImGuiWindow::imGuiPanels[10]);
	ImGui::Checkbox("Console", &FEImGuiWindow::imGuiPanels[11]);
}

void FEImGuiWindow::PerformanceProfiler() {
	ImGui::Begin("Performance Profiler"); // ImGUI window creation

	SystemInfomation();

	ImGui::Spacing();
	
	if (ImGui::TreeNode("Thread Latency")) {

		ImGui::Text(("Main Thread: " + std::to_string(TimeUtil::mtTimer.deltatime * 1000.0f) + " ms").c_str() ); 
		ImGui::Text(("Physics Thread: " + std::to_string(TimeUtil::ptTimer.deltatime * 1000.0f) + " ms").c_str() ); 
		ImGui::Text(("Game Thread: " + std::to_string(TimeUtil::deltatime * 1000.0f) + " ms").c_str() ); 
		ImGui::Text(("Worker Thread: " + std::to_string(TimeUtil::wtTimer.deltatime * 1000.0f) + " ms").c_str() ); 
		ImGui::TreePop();// Ends The ImGui Window
	}
	
	ImGui::Spacing();

	static float framerateValues[60] = { 0 };
	static int frValues_offset = 0;
	framerateValues[frValues_offset] = static_cast<float>(TimeUtil::frameRate);
	frValues_offset = (frValues_offset + 1) % IM_ARRAYSIZE(framerateValues);

	//Frame time graph
	static float frameTimeValues[90] = { 0 }; //stores 90 snapshots of frametime

	static int ftValues_offset = 0;
	frameTimeValues[ftValues_offset] = TimeUtil::deltatime * 1000.0f; // Convert to milliseconds
	ftValues_offset = (ftValues_offset + 1) % IM_ARRAYSIZE(frameTimeValues);
	std::string frametimes = "LAT: " + std::to_string(frameTimeValues[ftValues_offset] = TimeUtil::deltatime * 1000.0f) + " ms";

	ImGui::BeginGroup();
	ImGui::TextColored(ImVec4(1, 0, 0, 1), ("fps: " + std::to_string(static_cast<int>(TimeUtil::frameRate1hz))).c_str());
	//std::string stringFPS = "FPS: " + std::to_string(deltaTimeStr.frameRate1IHZ) + frametimes;
	ImGui::PlotLines("##FPS", framerateValues, (IM_ARRAYSIZE(framerateValues)), frValues_offset, nullptr, 0.0f, TimeUtil::frameRate * 1.5f, ImVec2(180, 50));
	ImGui::EndGroup();

	ImGui::SameLine();

	ImGui::BeginGroup();
	ImGui::TextColored(ImVec4(0, 1, 0, 1), frametimes.c_str());
	ImGui::PlotLines("##MS", frameTimeValues, IM_ARRAYSIZE(frameTimeValues), ftValues_offset, nullptr, 0.0f, 50.0f, ImVec2(180, 50));
	ImGui::EndGroup();

	ImGui::Spacing();
	
	ImGui::TextColored(ImVec4(0,1,0,1), ("Render: " + std::to_string(TimeUtil::deltatime * 1000.0f) + " ms").c_str());

	ImGui::End();
}

void FEImGuiWindow::TextEditor() {
	ImGui::Begin("Text Editor"); // ImGUI window creation

	float header_height = 25.0f;
	ImGui::BeginChild("HeaderBar", ImVec2(0, header_height), false, ImGuiWindowFlags_NoScrollbar);

	// Input for the path to the text file
	if (ImGui::InputText("##Path", pathBuffer, sizeof(pathBuffer))) {
		FileClass::currentPath = pathBuffer;
	}

	ImGui::SameLine();

	strncpy(pathBuffer, FileClass::currentPath.c_str(), sizeof(pathBuffer) - 1);
	pathBuffer[sizeof(pathBuffer) - 1] = '\0';

	if (ImGui::ImageButton("##FolderIcon", (ImTextureID)FEImGuiWindow::FolderIcon.ID, ImVec2(15, 15))) {
		FEImGui::spawnFileWindow("TextEditor", "Choose File", ".*", ".");
	}
	// display

	if (FEImGui::renderFileWindow("TextEditor")) {
		if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
			std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();

			FileClass::currentPath = filePathName; // Update current path

			FileClass::loadContents();
		}

		// close
		ImGuiFileDialog::Instance()->Close();
	}

	ImGui::SameLine(ImGui::GetWindowWidth() - 95.0f);

	// Save and Load buttons
	if (ImGui::ImageButton("##SaveIcon", (ImTextureID)FEImGuiWindow::SaveIcon.ID, ImVec2(15, 15))) {
		FileClass::saveContents(); // Save contents of text editor
	}
	ImGui::SameLine();
	if (ImGui::ImageButton("##arrowIcon", (ImTextureID)FEImGuiWindow::arrowIcon.ID, ImVec2(15, 15))) {
		FileClass::loadContents(); // Load contents of text editor
	}
	if (ImGui::SameLine(), ImGui::ImageButton("##crossIcon", (ImTextureID)FEImGuiWindow::crossIcon.ID, ImVec2(15, 15))) {
		FileClass::Contents = ""; // Clear contents of text editor
	}

	ImGui::EndChild();

	ImGui::Separator();
	

	// text box area
	ImGui::BeginChild("TextBox", ImVec2(0, 0), false);

	std::vector<char> contentBuffer(FileClass::Contents.size() + 256);
	strncpy(contentBuffer.data(), FileClass::Contents.c_str(), contentBuffer.size());
	contentBuffer[contentBuffer.size() - 1] = '\0';

	ImVec2 textSize = ImGui::CalcTextSize(FileClass::Contents.c_str());
	ImVec2 boxSize(std::max(textSize.x + 20, ImGui::GetWindowWidth()), std::max(textSize.y + 50, ImGui::GetWindowHeight()));

	// Main Text Editor Input
	if (ImGui::InputTextMultiline("Text Box", contentBuffer.data(), contentBuffer.size(), boxSize)) {
		FileClass::Contents = std::string(contentBuffer.data());
	}

	ImGui::EndChild();

	ImGui::End();
}

void FEImGuiWindow::viewport() {
	ImGui::Begin("ViewPort");
	bool placeholderBool;

	float window_width = ImGui::GetContentRegionAvail().x;
	float window_height = ImGui::GetContentRegionAvail().y;
	
	renderTarget::attemptFrameBufferResize(static_cast<unsigned int>(window_width), static_cast<unsigned int>(window_height)); // Update frame buffer resolution
	
	ImGui::Image((ImTextureID)(uintptr_t)renderTarget::Ftexture, ImVec2(window_width, window_height), ImVec2(0, 1), ImVec2(1, 0));

	//prevEnableLinearScaling
	//ScreenUtils::UpdateViewportResize();
	
	ImVec2 viewportPos = ImGui::GetItemRectMin();    // top-left corner relative to window
	ImVec2 viewportSize = ImGui::GetItemRectSize(); // size of the image

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

	float padding = 10.0f;
	ImVec2 iconPos = ImVec2(viewportPos.x + padding, viewportPos.y + padding);

	ImGui::SetCursorScreenPos(iconPos);

	ImGui::BeginGroup();
	if (ImGui::ImageButton("##moveIcon", (ImTextureID)FEImGuiWindow::moveIcon.ID, ImVec2(30, 30))) {
		SelectedTransform = 0;
	}
	if (ImGui::ImageButton("##scaleIcon", (ImTextureID)FEImGuiWindow::scaleIcon.ID, ImVec2(30, 30))) {
		SelectedTransform = 1;
	}
	if (ImGui::ImageButton("##rotateIcon", (ImTextureID)FEImGuiWindow::rotateIcon.ID, ImVec2(30, 30))) {
		SelectedTransform = 2;
	}
	ImGui::EndGroup();

	// stopIcon

	if (Player::playstate != 0) ImGui::SameLine((ImGui::GetWindowWidth() * 0.5) - 40);
	else ImGui::SameLine((ImGui::GetWindowWidth() * 0.5) - 20);

	ImGui::BeginGroup();
	if (Player::playstate != 0) { if (ImGui::ImageButton("##stopIcon", (ImTextureID)FEImGuiWindow::stopIcon.ID, ImVec2(20, 20))) { Player::stopState(); } ImGui::SameLine(); }
	if (Player::playstate != 1) { if (ImGui::ImageButton("##playIcon", (ImTextureID)FEImGuiWindow::playIcon.ID, ImVec2(20, 20))) { Player::playState(); } }
	if (Player::playstate == 1) { if (ImGui::ImageButton("##pauseIcon", (ImTextureID)FEImGuiWindow::pauseIcon.ID, ImVec2(20, 20))) { Player::pauseState(); } }
	ImGui::EndGroup();

	ImGui::SameLine(ImGui::GetWindowWidth() - (60.0f)); // ImGui::GetWindowWidth() - 
	if (ImGui::ImageButton("##wireframeIcon", (ImTextureID)FEImGuiWindow::wirefameIcon.ID, ImVec2(30, 30))) {
		FEImGuiWindow::isWireframe = !FEImGuiWindow::isWireframe;
	}
	ImGui::SameLine(ImGui::GetWindowWidth() - (107.0f)); // ImGui::GetWindowWidth() - 
	if (ImGui::ImageButton("##iIcon", (ImTextureID)FEImGuiWindow::iIcon.ID, ImVec2(30, 30))) {
		FEImGuiWindow::showViewportIcons = !FEImGuiWindow::showViewportIcons;
	}


	ImGui::SameLine(ImGui::GetWindowWidth() - (ImGui::GetWindowWidth() - 10.0f));
	ImGui::BeginGroup();
	ImGui::Dummy(ImVec2(0, ImGui::GetWindowHeight() - 135.0f));
	//ImGui::TextColored(ImVec4(1, 0, 0, 1), "OpenGL Version: %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
	ImGui::TextColored(ImVec4(1, 0, 0, 1), "GPU: %s", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
	ImGui::TextColored(ImVec4(1, 0, 0, 1), (std::string("RES: ") + std::to_string(static_cast<int>(renderTarget::ViewPortWidth)) + "*" + std::to_string(static_cast<int>(renderTarget::ViewPortHeight))).c_str());
	ImGui::TextColored(ImVec4(1, 0, 0, 1), ("FPS: " + std::to_string(static_cast<int>(TimeUtil::frameRate1hz))).c_str());
	ImGui::TextColored(ImVec4(1, 0, 0, 1), ("MS: " + std::to_string(TimeUtil::deltatime * 1000.0f) ).c_str());

	ImGui::EndGroup();
	ImGui::End();
}

float icon_size = 30.0f;

float header_height = 30.0f;

void FEImGuiWindow::HierarchyElement(int parentIndex)
{
	
	if (Scene::entityObjects[parentIndex]->type == entity::ENT_MODEL_TYPE) // check if model entity
	{
		ImGui::BeginGroup();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
		ImGui::ImageButton(
			("##openButton" + std::to_string(parentIndex)).c_str(),
			(ImTextureID)(intptr_t)FEImGuiWindow::ModelIcon.ID,
			ImVec2(icon_size, icon_size)
		);
		if (ImGui::IsItemClicked()) {
			FEImGuiWindow::SelectedObjectType = "Model";
			FEImGuiWindow::SelectedObjectIndex = static_cast<int>(parentIndex);
		}
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		//if ()
		// here as a alternative drop down 
		if (Scene::entityObjects[parentIndex]->component.relationship.childUUID.empty())
		{
			if (ImGui::MenuItem((Scene::entityObjects[parentIndex]->name + "##" + std::to_string(parentIndex)).c_str())) {
				FEImGuiWindow::SelectedObjectType = "Model";
				FEImGuiWindow::SelectedObjectIndex = static_cast<int>(parentIndex);
			}
		}
		else
		{
			if (ImGui::TreeNode((Scene::entityObjects[parentIndex]->name + "##DD" + std::to_string(parentIndex)).c_str())) {

				for (size_t i = 0; i < Scene::entityObjects[parentIndex]->component.relationship.childUUID.size(); i++)
				{
					int entityIndex = RelationshipManager::indexFromUUIDEntity(Scene::entityObjects[parentIndex]->component.relationship.childUUID[i]);
					HierarchyElement(entityIndex);
				}

				ImGui::TreePop();
			}
		}
		ImGui::EndGroup();
	}
	// billboard
	else if (Scene::entityObjects[parentIndex]->type == entity::ENT_BILLBOARD_TYPE) // check if billboard entity
	{
		ImGui::BeginGroup();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
		ImGui::ImageButton(
			("##openButton" + std::to_string(parentIndex)).c_str(),
			(ImTextureID)(intptr_t)FEImGuiWindow::BillBoardIcon.ID,
			ImVec2(icon_size, icon_size)
		);
		if (ImGui::IsItemClicked()) {
			FEImGuiWindow::SelectedObjectType = "Billboard";
			FEImGuiWindow::SelectedObjectIndex = static_cast<int>(parentIndex);
		}
		ImGui::PopStyleColor(3);


		ImGui::SameLine();
		if (Scene::entityObjects[parentIndex]->component.relationship.childUUID.empty())
		{
			if (ImGui::MenuItem((Scene::entityObjects[parentIndex]->name + "##" + std::to_string(parentIndex)).c_str())) {
				FEImGuiWindow::SelectedObjectType = "Billboard";
				FEImGuiWindow::SelectedObjectIndex = static_cast<int>(parentIndex);
			}
		}
		else
		{
			if (ImGui::TreeNode((Scene::entityObjects[parentIndex]->name + "##DD" + std::to_string(parentIndex)).c_str())) {
				for (size_t i = 0; i < Scene::entityObjects[parentIndex]->component.relationship.childUUID.size(); i++)
				{
					int entityIndex = RelationshipManager::indexFromUUIDEntity(Scene::entityObjects[parentIndex]->component.relationship.childUUID[i]);
					HierarchyElement(entityIndex);
				}
				ImGui::TreePop();
			}
		}

		ImGui::EndGroup();
	}
	
	
	else if (Scene::entityObjects[parentIndex]->type == entity::ENT_EMPTY_TYPE) // check if billboard entity
	{
		ImGui::BeginGroup();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
		ImGui::ImageButton(
			("##openButton" + std::to_string(parentIndex)).c_str(),
			(ImTextureID)(intptr_t)FEImGuiWindow::emptyIcon.ID,
			ImVec2(icon_size, icon_size)
		);
		if (ImGui::IsItemClicked()) {
			FEImGuiWindow::SelectedObjectType = "Empty";
			FEImGuiWindow::SelectedObjectIndex = static_cast<int>(parentIndex);
		}
		ImGui::PopStyleColor(3);


		ImGui::SameLine();
		if (Scene::entityObjects[parentIndex]->component.relationship.childUUID.empty())
		{
			if (ImGui::MenuItem((Scene::entityObjects[parentIndex]->name + "##" + std::to_string(parentIndex)).c_str())) {
				FEImGuiWindow::SelectedObjectType = "Empty";
				FEImGuiWindow::SelectedObjectIndex = static_cast<int>(parentIndex);
			}
		}
		else
		{
			if (ImGui::TreeNode((Scene::entityObjects[parentIndex]->name + "##DD" + std::to_string(parentIndex)).c_str())) {
				for (size_t i = 0; i < Scene::entityObjects[parentIndex]->component.relationship.childUUID.size(); i++)
				{
					int entityIndex = RelationshipManager::indexFromUUIDEntity(Scene::entityObjects[parentIndex]->component.relationship.childUUID[i]);
					HierarchyElement(entityIndex);
				}
				ImGui::TreePop();
			}
		}

		ImGui::EndGroup();
	}
	
	
}

bool addWindowBool = false;

void FEImGuiWindow::create() {

	if (ImGui::ImageButton("##plusIcon", (ImTextureID)FEImGuiWindow::plusIcon.ID, ImVec2(20, 20))) {
		addWindowBool = true;
	}
	if (addWindowBool) {
		FEImGuiWindow::addWindow("hierarchy", addWindowBool);
	}


}
static bool open_state = false;

void FEImGuiWindow::HierarchyList() { // have size of icons increase with window size 
	ImGui::Begin("Scene hierarchy"); // ImGUI window creation
	ImGui::BeginChild("AddBar", ImVec2(0, header_height), false, ImGuiWindowFlags_NoScrollbar);

	FEImGuiWindow::create();
	
	ImGui::SameLine();
	
	ImGui::Checkbox("spawn Near Camera", &Scene::spawnNearCamera);
	
	ImGui::EndChild();
	ImGui::Separator();
	ImGui::BeginChild("Objects", ImVec2(0, 0), false);
	
	ImGui::Text("Constant Objects:");
	//cameraIcon
	ImGui::Image(
		(ImTextureID)(intptr_t)FEImGuiWindow::cameraIcon.ID,
		ImVec2(icon_size, icon_size)
	);
	ImGui::SameLine();
	if (ImGui::MenuItem("Camera")) {
		FEImGuiWindow::SelectedObjectType = "Camera";
		FEImGuiWindow::SelectedObjectIndex = 0;
	}
	ImGui::Image(
		(ImTextureID)(intptr_t)FEImGuiWindow::directLight.ID,
		ImVec2(icon_size, icon_size),
		ImVec2(0, 0),
		ImVec2(1, 1),
		ImVec4(LightingHandler::directLightCol.r, LightingHandler::directLightCol.g, LightingHandler::directLightCol.b, 1.0f), // tint
		ImVec4(0, 0, 0, 0) // no border
	);
	ImGui::SameLine();
	if (ImGui::MenuItem("DirectLight")) {
		FEImGuiWindow::SelectedObjectType = "DirectLight";
		FEImGuiWindow::SelectedObjectIndex = 0;
	}
	ImGui::Image(
		(ImTextureID)(intptr_t)FEImGuiWindow::skyboxIcon.ID,
		ImVec2(icon_size, icon_size),
		ImVec2(0, 0),
		ImVec2(1, 1),
		ImVec4(RenderClass::skyRGBA.r, RenderClass::skyRGBA.g, RenderClass::skyRGBA.b, 1.0f), // tint
		ImVec4(0, 0, 0, 0) // no border
	);
	ImGui::SameLine();
	if (ImGui::MenuItem("Skybox")) {
		FEImGuiWindow::SelectedObjectType = "Skybox";
		FEImGuiWindow::SelectedObjectIndex = 0;
	}

	ImGui::Image(
		(ImTextureID)(intptr_t)FEImGuiWindow::environmentIcon.ID,
		ImVec2(icon_size, icon_size)
	);
	ImGui::SameLine();
	if (ImGui::MenuItem("Environment")) {
		FEImGuiWindow::SelectedObjectType = "Environment";
		FEImGuiWindow::SelectedObjectIndex = 0;
	}
	ImGui::NewLine();
	ImGui::Text(("Entities: " + std::to_string(Scene::entityObjects.size())).c_str());
	ImGui::Text("Objects:");
	
	// entity model, bb, empty
	for (size_t i = 0; i < Scene::entityObjects.size(); i++)
	{
		if (!Scene::entityObjects[i]->component.relationship.hasParent) HierarchyElement(i);
	}
	
		for (size_t i = 0; i < Scene::SoundObjects.size(); i++) {
			ImGui::BeginGroup();

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::ImageButton(
				("##openButtonS" + std::to_string(i)).c_str(),
				(ImTextureID)(intptr_t)FEImGuiWindow::SoundIcon.ID,
				ImVec2(icon_size, icon_size)
			);
			if (ImGui::IsItemClicked()) {
				FEImGuiWindow::SelectedObjectType = "Sound";
				FEImGuiWindow::SelectedObjectIndex = static_cast<int>(i);
			}
			ImGui::PopStyleColor(3);
			// removed the isplayng check that changed tint, its useless

			ImGui::SameLine();
			if (ImGui::MenuItem((Scene::SoundObjects[i].name + "##" + std::to_string(i)).c_str())) { 
				FEImGuiWindow::SelectedObjectType = "Sound";
				FEImGuiWindow::SelectedObjectIndex = static_cast<int>(i);
			}
			ImGui::EndGroup();
		}
		for (size_t i = 0; i < LightingHandler::Lights.size(); i++) {
			ImGui::BeginGroup();
			if (LightingHandler::Lights[i].type == 0) { // spot light

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
				ImGui::ImageButton(
					("##lightbutton" + std::to_string(i)).c_str(),
					(ImTextureID)(intptr_t)FEImGuiWindow::spotLightIcon.ID,
					ImVec2(icon_size, icon_size),
					ImVec2(0, 0), ImVec2(1, 1),
					ImVec4(LightingHandler::Lights[i].colour.x,
						LightingHandler::Lights[i].colour.y,
						LightingHandler::Lights[i].colour.z, 0.0f)
				);
				if (ImGui::IsItemClicked()) {
					FEImGuiWindow::SelectedObjectType = "Light";
					FEImGuiWindow::SelectedObjectIndex = static_cast<int>(i);
				}
				ImGui::PopStyleColor(3);
			}
			else if (LightingHandler::Lights[i].type == 1) { // point light
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.f, 0.f, 0.f, 0.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.f, 0.f, 0.f, 0.f));
				ImGui::ImageButton(
					("##lightbutton" + std::to_string(i)).c_str(),
					(ImTextureID)(intptr_t)FEImGuiWindow::pointLightIcon.ID,
					ImVec2(icon_size, icon_size),
					ImVec2(0, 0), ImVec2(1, 1),
					ImVec4(LightingHandler::Lights[i].colour.x,
						LightingHandler::Lights[i].colour.y,
						LightingHandler::Lights[i].colour.z, 0.0f)
				);
				if (ImGui::IsItemClicked()) {
					FEImGuiWindow::SelectedObjectType = "Light";
					FEImGuiWindow::SelectedObjectIndex = static_cast<int>(i);
				}
				ImGui::PopStyleColor(3);
			}
			ImGui::SameLine();
			if (ImGui::MenuItem(("Light X:" + std::to_string(static_cast<int>(LightingHandler::Lights[i].position.x))
				+ " Y:" + std::to_string(static_cast<int>(LightingHandler::Lights[i].position.y))
				+ " Z:" + std::to_string(static_cast<int>(LightingHandler::Lights[i].position.z))
				+ ("##" + std::to_string(i) ) ).c_str() ) ) 
			{
				FEImGuiWindow::SelectedObjectType = "Light";
				FEImGuiWindow::SelectedObjectIndex = static_cast<int>(i);
			}
			ImGui::EndGroup();
		}

	ImGui::EndChild();

	ImGui::End();
}


static const char* hierarchyItems[]{ "Models","BillBoards","Sound", "Light", "Empty"};
static int hierarchySelectedItem = 0; // Index of the selected item in the hierarchy combo box

static const char* contentItems[]{ "Models","BillBoards", "Sound", "Material", "Skybox"};
static int contentSelecteditem = 0;

char name[32] = "Name";
std::string Path = "Assets/";
bool type = false;

int InputTextCallback(ImGuiInputTextCallbackData* data) {
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
		// Resize string when needed
		std::string* str = static_cast<std::string*>(data->UserData);
		str->resize(data->BufTextLen);
		data->Buf = str->data();
	}
	return 0;
}

void pathInput() {
	ImGui::InputText("##PathInput", Path.data(), Path.capacity() + 1,
		ImGuiInputTextFlags_CallbackResize,
		InputTextCallback, static_cast<void*>(&Path));
	ImGui::SameLine();

	if (ImGui::ImageButton("##FolderIcon", (ImTextureID)FEImGuiWindow::FolderIcon.ID, ImVec2(15, 15))) {
		FEImGui::spawnFileWindow("ChooseAddPath", "Add Object", ".*", ".");
	}
	ImGui::SameLine();
	ImGui::Text("Path");
	// display
	if (ImGuiFileDialog::Instance()->Display("ChooseAddPath")) {
		if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK

			std::string fullPath = ImGuiFileDialog::Instance()->GetFilePathName();
			std::filesystem::path selectedPath(fullPath);
			std::filesystem::path currentDir = std::filesystem::current_path();

			// Make relative path
			std::filesystem::path relativePath = std::filesystem::relative(selectedPath, currentDir);

			// Normalize slashes to forward slashes
			std::string pathStr = relativePath.generic_string(); // gives "Assets/Models/fps/s"

			Path = pathStr;

		}

		// close
		ImGuiFileDialog::Instance()->Close();
	}

	
}
void nameInput() {
	ImGui::InputText("Name Input", name, IM_ARRAYSIZE(name));
}

void FEImGuiWindow::addWindow(std::string typeString, bool &isOpen) {
	ImGui::Begin("Add");
	ImGui::Dummy(ImVec2((ImGui::GetWindowWidth() - 50.0f), 1));
	ImGui::SameLine();
	if (ImGui::ImageButton(("##crossIcon" + typeString).c_str(), (ImTextureID)FEImGuiWindow::crossIcon.ID, ImVec2(10, 10))) {
		isOpen = false;
	}
	if (typeString == "hierarchy") {
		ImGui::Combo("ObjectType", &hierarchySelectedItem, hierarchyItems, IM_ARRAYSIZE(hierarchyItems));

		ImGui::Spacing();
		if (hierarchySelectedItem == 0) {
			//ImGui::Text("Model");
			pathInput();
			nameInput();

			if (ImGui::ImageButton("##plusIcon", (ImTextureID)FEImGuiWindow::plusIcon.ID, ImVec2(10, 10))) {
				
					//if (spawnNearCamera) newEntity->setPosition(maincamera.Position - ( FE_Math::getForwardFromViewMatrix(maincamera.cameraMatrix) * 5.0f ));
					//Scene::maincamera.Position - ( FE_Math::getForwardFromViewMatrix(Scene::maincamera.cameraMatrix) * 5.0f 
				if (Scene::spawnNearCamera) Scene::AddEntityObject(entity::ENT_MODEL_TYPE, name, Path, 
					Scene::maincamera.Position - ( FE_Math::getForwardFromViewMatrix(Scene::maincamera.cameraMatrix * 5.0f) ) , glm::vec3(1.0f), glm::vec3(0.0f) );
				else Scene::AddEntityObject(entity::ENT_MODEL_TYPE, name, Path, glm::vec3(0.0f), glm::vec3(1.0f), glm::vec3(0.0f));
			}

		}
		else if (hierarchySelectedItem == 1) {
			//ImGui::Text("BillBoard");
			pathInput();
			nameInput();
			if (ImGui::ImageButton("##plusIcon", (ImTextureID)FEImGuiWindow::plusIcon.ID, ImVec2(10, 10))) {
				if (Scene::spawnNearCamera) Scene::AddEntityObject(entity::ENT_BILLBOARD_TYPE, name, Path, 
					Scene::maincamera.Position - ( FE_Math::getForwardFromViewMatrix(Scene::maincamera.cameraMatrix * 5.0f) ) , glm::vec3(1.0f), glm::vec3(0.0f) );
				else Scene::AddEntityObject(entity::ENT_BILLBOARD_TYPE, name, Path, glm::vec3(0.0f), glm::vec3(1.0f), glm::vec3(0.0f));
			}
		}
		else if (hierarchySelectedItem == 2) {
			pathInput();
			nameInput();
			if (ImGui::ImageButton("##plusIcon", (ImTextureID)FEImGuiWindow::plusIcon.ID, ImVec2(10, 10))) {
				Scene::AddSceneSoundObject(name, Path);
			}
		}
		else if (hierarchySelectedItem == 3) {
			if (ImGui::ImageButton("##plusIcon", (ImTextureID)FEImGuiWindow::plusIcon.ID, ImVec2(10, 10))) {
				LightingHandler::createLight();
			}
		}
		else if (hierarchySelectedItem == 4) {
			
			nameInput();
			
			if (ImGui::ImageButton("##plusIcon", (ImTextureID)FEImGuiWindow::plusIcon.ID, ImVec2(10, 10))) {
				if (Scene::spawnNearCamera) Scene::AddEntityObject(entity::ENT_EMPTY_TYPE, name, "", 
		Scene::maincamera.Position - ( FE_Math::getForwardFromViewMatrix(Scene::maincamera.cameraMatrix * 5.0f) ) , glm::vec3(1.0f), glm::vec3(0.0f) );
				else Scene::AddEntityObject(entity::ENT_EMPTY_TYPE, name, "", glm::vec3(0.0f), glm::vec3(1.0f), glm::vec3(0.0f));
			}
		}
		
		}
	if (typeString == "content") {
		ImGui::Combo("ObjectType", &contentSelecteditem, contentItems, IM_ARRAYSIZE(contentItems));
		ImGui::Spacing();
		if (contentSelecteditem == 0) {
			//ImGui::Text("Model");
			pathInput();
			nameInput();
			if (ImGui::ImageButton("##plusIcon", (ImTextureID)FEImGuiWindow::plusIcon.ID, ImVec2(10, 10))) {
				ContentObjects.push_back("Model"); // Add a new content object to the list
				ContentObjectTypes.push_back("NULL"); // Add a new content object type to the list
				ContentObjectPaths.push_back(Path); // Add a new content object path to the list
				ContentObjectNames.push_back(name);
			}
		}
		else if (contentSelecteditem == 1) {
			//ImGui::Text("BillBoard");
			pathInput();
			nameInput();
			if (ImGui::ImageButton("##plusIcon", (ImTextureID)FEImGuiWindow::plusIcon.ID, ImVec2(10, 10))) {

				ContentObjects.push_back("BillBoard"); // Add a new content object to the list
				ContentObjectTypes.push_back("NULL"); // Add a new content object type to the list

				ContentObjectPaths.push_back(Path); // Add a new content object path to the list
				ContentObjectNames.push_back(name);
			}
		}
		// Sound
		else if (contentSelecteditem == 2) {
			pathInput();
			nameInput();
			if (ImGui::ImageButton("##plusIcon", (ImTextureID)FEImGuiWindow::plusIcon.ID, ImVec2(10, 10))) {
				ContentObjects.push_back("Sound"); // Add a new content object to the list
				ContentObjectTypes.push_back("NULL"); // Add a new content object type to the list
				ContentObjectPaths.push_back(Path); // Add a new content object path to the list
				ContentObjectNames.push_back(name);
			}
		}
		// material
		else if (contentSelecteditem == 3) {
			pathInput();
			nameInput();
			if (ImGui::ImageButton("##plusIcon", (ImTextureID)FEImGuiWindow::plusIcon.ID, ImVec2(10, 10))) {

				ContentObjects.push_back("Material"); // Add a new content object to the list
				ContentObjectTypes.push_back("NULL"); // Add a new content object type to the list
				ContentObjectPaths.push_back(Path); // Add a new content object path to the list
				ContentObjectNames.push_back(name);

				FEImGuiWindow::MaterialIndexUpdate();
			}
		}
		// skybox
		else if (contentSelecteditem == 4) {
			pathInput();
			nameInput();
			if (ImGui::ImageButton("##plusIcon", (ImTextureID)FEImGuiWindow::plusIcon.ID, ImVec2(10, 10))) {

				ContentObjects.push_back("Skybox"); // Add a new content object to the list
				ContentObjectTypes.push_back("NULL"); // Add a new content object type to the list
				ContentObjectPaths.push_back(Path); // Add a new content object path to the list
				ContentObjectNames.push_back(name);
			}
		}
	}
	ImGui::End();
}

void FEImGuiWindow::MaterialIndexUpdate()
{
	//clear index first
	if (MaterialObjectPaths.size() > 0){
		MaterialObjectPaths.clear();
		MaterialObjecNames.clear();
		MaterialSelectedIndex = 0;
	}

	// update index of Materials
	for (size_t i = 0; i < ContentObjects.size(); i++)
	{
		if (ContentObjects[i] == "Material")
		{
			//std::string path = ContentObjectPaths[i];
			//std::string name = ContentObjectNames[i];

			//std::cout << " Path:" << path << " Name:" << name << std::endl;
			MaterialObjectPaths.push_back(ContentObjectPaths[i]);
			MaterialObjecNames.push_back((ContentObjectNames[i]).c_str());
		}
	}
}

bool addContentBool = false;

void FEImGuiWindow::SceneFolderWindow()
{
	ImGui::Begin("Scene Folder"); // ImGUI window creation

	float header_height = 30.0f;
	ImGui::BeginChild("HeaderBar", ImVec2(0, header_height), false, ImGuiWindowFlags_NoScrollbar);

	if (ImGui::ImageButton("##plusIcon", (ImTextureID)FEImGuiWindow::plusIcon.ID, ImVec2(20, 20))) {
		addContentBool = true;
	}
	ImGui::EndChild();

	if (addContentBool) {
		FEImGuiWindow::addWindow("content", addContentBool);
	}

	ImGui::Separator();

	ImGui::BeginChild("ContentArea", ImVec2(0, 0), false);

	//if (FEImGuiWindow::ContentObjects.size() > 0) {
		//ImGui::Dummy(ImVec2(0, ImGui::GetWindowHeight() - 30.0f));
	//}
		for (size_t i = 0; i < FEImGuiWindow::ContentObjects.size(); i++)
		{
			if (FEImGuiWindow::ContentObjects[i] != "") {
				ImVec2 cursor_screen_pos = ImGui::GetCursorScreenPos(); // cursor pos at start of group
				std::string name = FEImGuiWindow::ContentObjectNames[i];
				ImVec2 textSize = ImGui::CalcTextSize(name.c_str());

				if (textSize.x > 85.0f) { // max width
					while (ImGui::CalcTextSize((name + "...").c_str()).x > 85.0f && !name.empty()) {
						name.pop_back();
					}
					name += "...";
				}

				ImGui::BeginGroup();
				if (FEImGuiWindow::ContentObjects[i] == "Model") { // ShadowMap
					if (ImGui::ImageButton(("##ObjectIcon" + std::to_string(i)).c_str(), (ImTextureID)FEImGuiWindow::ModelIcon.ID, ImVec2(100, 100))) {
						
						if (Scene::spawnNearCamera) Scene::AddEntityObject(entity::ENT_MODEL_TYPE, FEImGuiWindow::ContentObjectNames[i], FEImGuiWindow::ContentObjectPaths[i], 
	Scene::maincamera.Position - ( FE_Math::getForwardFromViewMatrix(Scene::maincamera.cameraMatrix * 5.0f) ) , glm::vec3(1.0f), glm::vec3(0.0f) );
						else Scene::AddEntityObject(entity::ENT_MODEL_TYPE, FEImGuiWindow::ContentObjectNames[i], FEImGuiWindow::ContentObjectPaths[i], glm::vec3(0.0f), glm::vec3(1.0f), glm::vec3(0.0f));
						
					}
					if (ImGui::ImageButton(("##crossIcon" + std::to_string(i)).c_str(), (ImTextureID)FEImGuiWindow::crossIcon.ID, ImVec2(10, 10))) {
						FEImGuiWindow::ContentObjects.erase(FEImGuiWindow::ContentObjects.begin() + i);
						FEImGuiWindow::ContentObjectNames.erase(FEImGuiWindow::ContentObjectNames.begin() + i);
						FEImGuiWindow::ContentObjectPaths.erase(FEImGuiWindow::ContentObjectPaths.begin() + i);
						FEImGuiWindow::ContentObjectTypes.erase(FEImGuiWindow::ContentObjectTypes.begin() + i);
					}
					ImGui::SameLine();
					ImGui::Text("%s", name.c_str());
				}
				if (FEImGuiWindow::ContentObjects[i] == "BillBoard") {
					if (ImGui::ImageButton(("##BillBoardIcon" + std::to_string(i)).c_str(), (ImTextureID)FEImGuiWindow::BillBoardIcon.ID, ImVec2(100, 100))) {
						
						if (Scene::spawnNearCamera) Scene::AddEntityObject(entity::ENT_BILLBOARD_TYPE, FEImGuiWindow::ContentObjectNames[i], FEImGuiWindow::ContentObjectPaths[i], 
							Scene::maincamera.Position - ( FE_Math::getForwardFromViewMatrix(Scene::maincamera.cameraMatrix * 5.0f) ) , glm::vec3(1.0f), glm::vec3(0.0f) );
						else Scene::AddEntityObject(entity::ENT_BILLBOARD_TYPE, FEImGuiWindow::ContentObjectNames[i], FEImGuiWindow::ContentObjectPaths[i], glm::vec3(0.0f), glm::vec3(1.0f), glm::vec3(0.0f));
						
					}
					if (ImGui::ImageButton(("##crossIcon" + std::to_string(i)).c_str(), (ImTextureID)FEImGuiWindow::crossIcon.ID, ImVec2(10, 10))) {
						FEImGuiWindow::ContentObjects.erase(FEImGuiWindow::ContentObjects.begin() + i);
						FEImGuiWindow::ContentObjectNames.erase(FEImGuiWindow::ContentObjectNames.begin() + i);
						FEImGuiWindow::ContentObjectPaths.erase(FEImGuiWindow::ContentObjectPaths.begin() + i);
						FEImGuiWindow::ContentObjectTypes.erase(FEImGuiWindow::ContentObjectTypes.begin() + i);
					}
					ImGui::SameLine();
					ImGui::Text("%s", name.c_str());
				}
				if (FEImGuiWindow::ContentObjects[i] == "Material") {
					if (ImGui::ImageButton(("##materialIcon" + std::to_string(i)).c_str(), (ImTextureID)FEImGuiWindow::materialIcon.ID, ImVec2(100, 100))) {
						
						FEImGuiWindow::SelectedObjectType = "Material";
						FEImGuiWindow::SelectedObjectIndex = i;
						// when clicked on should summon material editor
					}

					if (ImGui::ImageButton(("##crossIcon" + std::to_string(i)).c_str(), (ImTextureID)FEImGuiWindow::crossIcon.ID, ImVec2(10, 10))) {
						FEImGuiWindow::ContentObjects.erase(FEImGuiWindow::ContentObjects.begin() + i);
						FEImGuiWindow::ContentObjectNames.erase(FEImGuiWindow::ContentObjectNames.begin() + i);
						FEImGuiWindow::ContentObjectPaths.erase(FEImGuiWindow::ContentObjectPaths.begin() + i);
						FEImGuiWindow::ContentObjectTypes.erase(FEImGuiWindow::ContentObjectTypes.begin() + i);

						FEImGuiWindow::MaterialIndexUpdate();
					}
					ImGui::SameLine();
					ImGui::Text("%s", name.c_str());

				}
				if (FEImGuiWindow::ContentObjects[i] == "Skybox") {
					if (ImGui::ImageButton(("##skyboxIcon" + std::to_string(i)).c_str(), (ImTextureID)FEImGuiWindow::skyboxIcon.ID, ImVec2(100, 100))) {
						Skybox::DefaultSkyboxPath = FEImGuiWindow::ContentObjectPaths[i];
						Skybox::LoadSkyBoxTexture(FEImGuiWindow::ContentObjectPaths[i]);
					}

					if (ImGui::ImageButton(("##crossIcon" + std::to_string(i)).c_str(), (ImTextureID)FEImGuiWindow::crossIcon.ID, ImVec2(10, 10))) {
						FEImGuiWindow::ContentObjects.erase(FEImGuiWindow::ContentObjects.begin() + i);
						FEImGuiWindow::ContentObjectNames.erase(FEImGuiWindow::ContentObjectNames.begin() + i);
						FEImGuiWindow::ContentObjectPaths.erase(FEImGuiWindow::ContentObjectPaths.begin() + i);
						FEImGuiWindow::ContentObjectTypes.erase(FEImGuiWindow::ContentObjectTypes.begin() + i);
					}
					ImGui::SameLine();
					ImGui::Text("%s", name.c_str());

				}
				if (FEImGuiWindow::ContentObjects[i] == "Sound") {
					if (ImGui::ImageButton(("##SoundIcon" + std::to_string(i)).c_str(), (ImTextureID)FEImGuiWindow::SoundIcon.ID, ImVec2(100, 100))) {
						Scene::AddSceneSoundObject(FEImGuiWindow::ContentObjectNames[i], FEImGuiWindow::ContentObjectPaths[i]);
					}

					if (ImGui::ImageButton(("##crossIcon" + std::to_string(i)).c_str(), (ImTextureID)FEImGuiWindow::crossIcon.ID, ImVec2(10, 10))) {
						FEImGuiWindow::ContentObjects.erase(FEImGuiWindow::ContentObjects.begin() + i);
						FEImGuiWindow::ContentObjectNames.erase(FEImGuiWindow::ContentObjectNames.begin() + i);
						FEImGuiWindow::ContentObjectPaths.erase(FEImGuiWindow::ContentObjectPaths.begin() + i);
						FEImGuiWindow::ContentObjectTypes.erase(FEImGuiWindow::ContentObjectTypes.begin() + i);
					}
					ImGui::SameLine();
					ImGui::Text("%s", name.c_str());

				}
				ImGui::EndGroup();

				ImVec2 window_pos = ImGui::GetWindowPos();
				float window_width = ImGui::GetWindowWidth();

				// Predict next item width (icon + margin)
				float next_item_width = 125.0f;

				if (cursor_screen_pos.x + next_item_width < window_pos.x + window_width)
				{
					ImGui::SameLine();
				}
			}
		
		}

		ImGui::EndChild();

		ImGui::End();
}

void FEImGuiWindow::ConsoleWindow()
{
	ImGui::Begin("Console");

	if (ImGui::Button("Clear")) {
		LogConsole::logs.clear();
	}

	ImGui::Separator();

	ImGui::BeginChild("LogScrollingRegion", ImVec2(0, -25.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

	for (size_t i = 0; i < LogConsole::logs.size(); i++)
	{
		std::string log_line = "#" + std::to_string(i) + ": " + LogConsole::logs[i];
		ImGui::TextUnformatted(log_line.c_str());
	}

	ImGui::EndChild();

	ImGui::Separator();
	ImGui::Text("Text box should go here");

	ImGui::End();
}