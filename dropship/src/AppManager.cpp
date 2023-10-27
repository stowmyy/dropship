#define IMGUI_DEFINE_MATH_OPERATORS // https://github.com/ocornut/imgui/issues/2832

#include "AppManager.h"


extern OPTIONS options;

extern ImFont* font_subtitle;

static const std::string message = "STORMY.GG/DROPSHIP";

// download uri
// https://github.com/stowmyy/dropship-test/releases/latest/download/dropship.exe


AppManager::AppManager() : downloadState({ false, false, 0.0f, message, "winton"})
{
	// checkForUpdate();

}

void AppManager::RenderInline()
{
	static const auto list = ImGui::GetWindowDrawList();
	static const auto &style = ImGui::GetStyle();

	static const auto width = ImGui::GetContentRegionAvail().x;

	if (ImGui::GetFrameCount() == 9)
	{

		std::thread([&]()
		{
			// loses utf-8
			// TODO this will be bad if their path is utf 8
			char szPath [MAX_PATH];
			GetModuleFileName(NULL, szPath, MAX_PATH);
			std::string _this_path(szPath); // gets path of current .exe
			std::string _this_hash = sw::sha512::file(_this_path.c_str());
			transform(_this_hash.begin(), _this_hash.end(), _this_hash.begin(), ::tolower);
			this->downloadState.appVersion = _this_hash.substr(0, 9);

			if (options.auto_update)
			{
				std::string version = "";
				this->downloadState.active = true;

				// TODO: move .old to %temp%
				std::filesystem::path _tmp_path = std::filesystem::path(_this_path + ".old"); // gets path of old exe

				if (std::filesystem::exists(_tmp_path)) {
					std::filesystem::remove(_tmp_path);
				}

				std::filesystem::path _downloaded_path;

				this->downloadState.progress = 0.009f;
				// this->downloadState.downloading = true;
				this->downloadState.status = "CHECKING VERSION";
				download_file("https://github.com/stowmyy/dropship-test/releases/latest/download/version.txt", "version.txt", &(this->downloadState.progress), &version);
				// this->downloadState.downloading = false;

				if (!version.empty())
				{	

					transform(version.begin(), version.end(), version.begin(), ::tolower);

					printf("\n\nf\n\n");

					if (version != _this_hash)
					{
						printf("versions do not match.\n");

						this->downloadState.progress = 0.009f;
						this->downloadState.downloading = true;
						this->downloadState.status = "DOWNLOADING NEW VERSION";
						download_file("https://github.com/stowmyy/dropship-test/releases/latest/download/dropship.exe", "dropship.exe", &(this->downloadState.progress), NULL, &_downloaded_path);
						this->downloadState.downloading = false;

						std::filesystem::rename(std::filesystem::path(_this_path), _tmp_path);
						std::filesystem::copy(_downloaded_path, std::filesystem::path(_this_path));

						// system(std::format("taskkill /IM \"dropship.exe\" /F && start {0}", _this_path.c_str()).c_str());
						//std::exit(42);

						this->downloadState.active = false;
						this->downloadState.status = "NEW VERSION AVAILABLE";



						// todo
						// start new program that waits for this to close and runs it again

					}
					else
					{
						printf("versions match.\n");

						this->downloadState.active = false;
						//this->downloadState.status = "NEWEST VERSION";
						this->downloadState.status = message;

					}

					// std::filesystem::hash_value();

					std::wcout << szPath << std::endl;
				}

			}

		}).detach();
	}


	// version
	{
		static const auto color_text = ImGui::ColorConvertFloat4ToU32({ .8, .8, .8, style.Alpha });
		list->AddText(font_subtitle, 14, ImGui::GetCursorScreenPos(), color_text, this->downloadState.appVersion.c_str());
		//if (this->downloadState.active)
		{
			const auto width_text = font_subtitle->CalcTextSizeA(14, FLT_MAX, 0.0f, this->downloadState.status.c_str());
			list->AddText(font_subtitle, 14, ImGui::GetCursorScreenPos() + ImVec2(ImGui::GetContentRegionAvail().x - width_text.x, 0), color_text, this->downloadState.status.c_str());
		}
		ImGui::Dummy({ 0, 4 });
	}

	if (this->downloadState.active)
	{
		//static const auto color_background = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 1 });
		static const auto color_progress_downloading_background = ImColor::HSV(0 / 14.0f, 0.2f, 1.0f, 0.4f * style.Alpha);
		static const auto color_progress_downloading = ImColor::HSV(0 / 14.0f, 0.4f, 1.0f, style.Alpha);

		static const auto color_progress_background = ImColor::HSV(0.0f, 0.0f, 0.9f, style.Alpha);
		// static const auto color_progress = ImColor::HSV(fmod( - 0.02f, 1.0f) / 14.0f, 0.0f, 1.0f, style.Alpha);


		if (this->downloadState.downloading)
		{
			//list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color_background, 5);
			ImGui::Dummy({ 0, 0 });
	
			//const auto pos = ImVec2(0, 0) + ImGui::GetWindowPos() + style.WindowPadding;
			const auto pos = ImGui::GetCursorScreenPos();
			const auto width_progress = width * this->downloadState.progress;


			list->AddRectFilled(pos, pos + ImVec2(width, 9), color_progress_downloading_background, 5);
			list->AddRectFilled(pos, pos + ImVec2(width_progress, 9), color_progress_downloading, 5);

			ImGui::Dummy({ width, 9 });
		}
		else
		{
			// list->AddRectFilled(pos, pos + ImVec2(width, 9), color_progress_background, 5);
		}
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