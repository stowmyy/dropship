#define IMGUI_DEFINE_MATH_OPERATORS // https://github.com/ocornut/imgui/issues/2832

#include "AppManager.h"

extern ImFont* font_subtitle;


AppManager::AppManager() : downloadState({ true, .9f }), appVersion("v0.0")
{

}

void AppManager::RenderInline()
{
	static const auto list = ImGui::GetWindowDrawList();
	static const auto &style = ImGui::GetStyle();

	static const auto width = ImGui::GetContentRegionAvail().x;


	// version
	{
		static const auto color_text = ImGui::ColorConvertFloat4ToU32({ .8, .8, .8, style.Alpha });
		list->AddText(font_subtitle, 14, ImGui::GetCursorScreenPos(), color_text, this->appVersion.c_str());
		if (this->downloadState.downloading)
		{
			static const auto text = "DOWNLOADING NEW APP";
			static const auto width_text = font_subtitle->CalcTextSizeA(14, FLT_MAX, 0.0f, text);
			list->AddText(font_subtitle, 14, ImGui::GetCursorScreenPos() + ImVec2(ImGui::GetContentRegionAvail().x - width_text.x, 0), color_text, text);
		}
		ImGui::Dummy({ 0, 4 });
	}

	if (this->downloadState.downloading)
	{
		//static const auto color_background = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 1 });
		static const auto color_progress_background = ImColor::HSV(fmod( - 0.02f, 1.0f) / 14.0f, 0.2f, 1.0f, 0.4f * style.Alpha);
		static const auto color_progress = ImColor::HSV(fmod( - 0.02f, 1.0f) / 14.0f, 0.4f, 1.0f, style.Alpha);

		//list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color_background, 5);
		ImGui::Dummy({ 0, 0 });
	
		//const auto pos = ImVec2(0, 0) + ImGui::GetWindowPos() + style.WindowPadding;
		const auto pos = ImGui::GetCursorScreenPos();
		const auto width_progress = width * this->downloadState.progress;

		list->AddRectFilled(pos, pos + ImVec2(width, 9), color_progress_background, 5);
		list->AddRectFilled(pos, pos + ImVec2(width_progress, 9), color_progress, 5);
		ImGui::Dummy({ width, 9 });
	}





	// ImGui::Text("DOWNLOADING APP UPDATE");

	// ImGui::SeparatorText("downloading app update");

	/*const auto pos = ImVec2(0, 0) + ImGui::GetWindowPos() + style.WindowPadding;

	static const auto color_text = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);

	list->AddRectFilled(pos, pos + ImVec2(width, 40), color_background, 5);

	list->AddRectFilled(pos, pos + ImVec2(width, 9), color_progress_background, 5);
	list->AddRectFilled(pos, pos + ImVec2(width_progress, 9), color_progress, 5);

	list->AddText(pos + ImVec2(0, style.ItemSpacing.y), color_text, "DOWNLOADING APP UPDATE");*/

    #ifdef _DEBUG
        {
            ImGui::Begin("debug");
            if (ImGui::CollapsingHeader("app_manager.h", ImGuiTreeNodeFlags_None))
            {
				ImGui::Checkbox("downloading", &(this->downloadState.downloading));

				ImGui::SliderFloat("progress", (&this->downloadState.progress), 0.0f, 1.0f);
            }


            ImGui::End();
        }
    #endif

}