#include "utils/ImGui_Utils.h"

// imgui
#include "imgui.h"
#include "imgui_internal.h"

namespace ImGui_Utils
{
	void SetPurpleTheme()
	{
		auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

		// headers
		colors[ImGuiCol_Header] = ImVec4{ 0.4f, 0.2f, 0.4f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.6f, 0.3f, 0.6f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.3f, 0.15f, 0.3f, 1.0f };

		// buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.4f, 0.2f, 0.4f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.6f, 0.3f, 0.6f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.3f, 0.15f, 0.3f, 1.0f };

		// checkboxes
		colors[ImGuiCol_CheckMark] = ImVec4{ 0.8f, 0.5f, 0.8f, 1.0f };

		// frame background
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.4f, 0.2f, 0.4f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.6f, 0.3f, 0.6f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.3f, 0.15f, 0.3f, 1.0f };

		// tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.3f, 0.15f, 0.3f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.75f, 0.38f, 0.75f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.56f, 0.28f, 0.56f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.3f, 0.15f, 0.3f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.4f, 0.2f, 0.4f, 1.0f };

		// titles
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.3f, 0.15f, 0.3f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.3f, 0.15f, 0.3f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.3f, 0.15f, 0.3f, 1.0f };
	}

	void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth) 
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.3f, 0.15f, 0.3f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
			values.z = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	void DrawFloatControl(const std::string& label, float& value, float resetValue, float columnWidth, const std::string& format)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushItemWidth(-1);

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.6f, 0.3f, 0.6f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.8f, 0.4f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 1.0f, 0.2f, 0.35f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("R", buttonSize))
			value = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Float", &value, 0.1f, 0.0f, 0.0f, format.c_str());

		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	void DrawIntControl(const std::string& label, int& value, int resetValue, float columnWidth)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushItemWidth(-1);

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.6f, 0.3f, 0.6f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.8f, 0.4f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 1.0f, 0.2f, 0.35f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("R", buttonSize))
			value = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragInt("##Int", &value, 1);

		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	void DrawBoolControl(const std::string& label, bool& value, float columnWidth)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushItemWidth(-1);

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		ImGui::Checkbox("##Checkbox", &value);

		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	void DrawComboBoxControl(const std::string& label, int& selectedItem, const std::vector<const char*>& options, float columnWidth)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);

		// draw label in the first column
		ImGui::Text(label.c_str());

		// save start position of the first column
		float posX = ImGui::GetCursorPosX();
		ImGui::NextColumn();

		ImGui::PushItemWidth(-1);

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		if (ImGui::BeginCombo("", options[selectedItem], ImGuiComboFlags_None))
		{
			for (size_t i = 0; i < options.size(); ++i)
			{
				bool isSelected = (selectedItem == static_cast<int>(i));
				if (ImGui::Selectable(options[i], isSelected))
				{
					selectedItem = static_cast<int>(i);
				}
				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::PopItemWidth();

		// restore the start position of the second column
		ImGui::SetCursorPosX(posX);

		ImGui::Columns(1);

		ImGui::PopID();
	}

	void DrawColorControl(const std::string& label, glm::vec3& color, float columnWidth)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushItemWidth(-1);

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 100.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ color.r, color.g, color.b, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ color.r + 0.1f, color.g + 0.1f, color.b + 0.1f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ color.r - 0.1f, color.g - 0.1f, color.b - 0.1f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("", buttonSize))
		{
			ImGui::OpenPopup("ColorPicker");
		}
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		if (ImGui::BeginPopup("ColorPicker"))
		{
			ImGui::ColorPicker3("##ColorPicker", &color.r);
			ImGui::EndPopup();
		}

		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	bool DrawButtonControl(const std::string& label, const std::string& buttonLabel, float columnWidth)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];
		bool clicked = false;

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushItemWidth(-1);

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		if (ImGui::Button(buttonLabel.c_str()))
			clicked = true;

		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();

		return clicked;
	}

	void SliderFloat(const std::string& label, float& value, float min, float max, const std::string& format, float columnWidth)
	{
		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushItemWidth(-1);
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.8f, 0.4f, 0.8f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.0f, 0.2f, 0.35f, 1.0f));
		ImGui::SliderFloat("##SliderFloat", &value, min, max, format.c_str());
		ImGui::PopStyleColor(2);
		ImGui::PopItemWidth();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	void SliderInt(const std::string& label, int& value, int min, int max, const std::string& format, float columnWidth)
	{
		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushItemWidth(-1);
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.8f, 0.4f, 0.8f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(1.0f, 0.2f, 0.35f, 1.0f));
		ImGui::SliderInt("##SliderInt", &value, min, max, format.c_str());
		ImGui::PopStyleColor(2);
		ImGui::PopItemWidth();

		ImGui::Columns(1);

		ImGui::PopID();
	}
}
