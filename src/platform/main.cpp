#include <iostream>
#include "raylib.h"
#include "imgui.h"
#include <rlImGui.h>
#include <gameMain.h>
int main()
{
#pragma region productionBuild
#if PRODUCTION_BUILD == 1
	SetTraceLogLevel(LOG_NONE); // disable log when not in development/debug mode
#endif
#pragma endregion

#pragma region initialConfig
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	SetTargetFPS(240);
	SetExitKey(KEY_NULL); // Disable esc from closing the window
	InitWindow(800, 450, "Oilman");
#pragma endregion

#pragma region imguiMainConfig
	// ImGui Config
	rlImGuiSetup(true); // Required setup context. Won't work without this.
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 2;
	io.ConfigFlags = ImGuiConfigFlags_DockingEnable; // Allows us to snap ImGui windows together
	ImGui::StyleColorsLight();
#pragma endregion
	if (!initGame())
	{
		return 0;
	}
#pragma region mainWhileLoop
	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(BLACK);
		#pragma region imGuiWindow
		rlImGuiBegin(); // ImGui window

		// Allow for docking ImGui windows without covering with a gray screen
		// Allow for docking ImGui windows without covering with a gray screen
		ImGui::PushStyleColor(ImGuiCol_WindowBg, {});
		ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, {}); 
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode); // Allows docking on main window.
		ImGui::PopStyleColor(2);

		ImGui::Begin("Test");
		ImGui::Text("ImGui visible test");
		if (ImGui::Button("Button"))
		{
			std::cout << "Button pressed" << std::endl;
		}
		if (ImGui::Button("Button 2")) // Can't have 2 widgets with the same Id.
		{
			std::cout << "Second button pressed" << std::endl;
		}
		ImGui::Separator();
		ImGui::NewLine();
		static float a = 0;
		ImGui::SliderFloat("slider", &a, 0, 1);
		ImGui::End();
		#pragma endregion imGuiWindow
		
		if (!updateGame())
		{
			CloseWindow();
		}

		rlImGuiEnd();

		EndDrawing();
	}
#pragma endregion

	CloseWindow();
	closeGame();
#pragma region imgui
	rlImGuiShutdown();
#pragma endregion
	return 0;
}