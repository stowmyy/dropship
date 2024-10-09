#include "pch.h"

#include "Update.h"

extern std::unique_ptr<Settings> g_settings;

//extern OPTIONS options;
//
//extern AppStore appStore;
//extern AppStore __default__appStore;

extern ImFont* font_subtitle;


// static const std::string message = "STORMY.GG/DROPSHIP";

// download uri
// https://github.com/stowmyy/dropship/releases/latest/download/dropship.exe

std::wstring _get_exe_path() {
	/*TCHAR _path[MAX_PATH] = { 0 };
	::GetModuleFileName(NULL, _path, MAX_PATH);
	return std::string(_path);*/

	wchar_t _path[MAX_PATH];
	::GetModuleFileNameW(NULL, _path, MAX_PATH);

	return std::wstring(_path);
}

std::string _get_file_sha512(std::wstring _path) {
	/*std::string _hash = sw::sha512::file(_path.c_str());
	std::transform(_hash.begin(), _hash.end(), _hash.begin(), ::tolower);
	return _hash;*/

	std::ifstream fs;
	fs.open(_path.c_str(), (std::ios::in | std::ios::binary));
	sw::detail::basic_sha512<char>::str_t s = sw::sha512::calculate(fs);
	fs.close();

	std::string _hash = s;
	std::transform(_hash.begin(), _hash.end(), _hash.begin(), ::tolower);
	return _hash;
}


// TODO ZoeResult CalculateFileSHA256(const utf8string& file_path, Options* opt, utf8string& str_hash);
// TODO ZoeResult CalculateFileSHA256(const utf8string& file_path, Options* opt, utf8string& str_hash);
// TODO ZoeResult CalculateFileSHA256(const utf8string& file_path, Options* opt, utf8string& str_hash);
// TODO ZoeResult CalculateFileSHA256(const utf8string& file_path, Options* opt, utf8string& str_hash);
void Updater::getMeta() {
	/* get .exe metadata */
	const auto path = _get_exe_path();
	const auto hash = _get_file_sha512(path);

	//std::println("path: {}\nhash: {}", path, hash);

	this->__exe_path = path;
	this->__exe_hash = hash;
}

/* downloads the new version hash from github */
std::string _fetch_update_sha512() {
	std::string update_hash = util::win_download::_download_txt(__UPDATE_VERSION_URI);
	return update_hash;
}

bool Updater::checkForUpdate() {
	std::string update_hash = _fetch_update_sha512();
	std::transform(update_hash.begin(), update_hash.end(), update_hash.begin(), ::tolower);
	trim(update_hash);

	std::println("update hash: {}", update_hash);

	if (update_hash.length() != this->__exe_hash.length()) {
		throw std::runtime_error("bad version");
	}

	return (this->__exe_hash != update_hash);
}

void Updater::update() {

	this->update_future = std::async(std::launch::async, [&] {
		try {

			this->progress = 0.0f;
			this->downloading = true;
			this->version = "downloading new version";

			std::filesystem::path _downloaded_path;
			util::win_download::download_file(__UPDATE_EXE_URI, "dropship.exe", &(this->progress), NULL, &_downloaded_path);
			this->downloading = false;
			this->version = "updating application";

			const auto tmp = std::filesystem::path(this->__exe_path + L".old");
			const auto dst = std::filesystem::path(this->__exe_path);

			// std::filesystem::temp_directory_path().append("dropship").append("dropship.exe);


			{
				std::filesystem::rename(dst, tmp);
				//std::filesystem::copy(_downloaded_path, dst);
				std::filesystem::rename(_downloaded_path, dst);
				static char buffer[512];
				strcpy_s(buffer, dst.string().c_str());


				/* CreateProcess API initialization */
				STARTUPINFOA siStartupInfo;
				PROCESS_INFORMATION piProcessInfo;
				memset(&siStartupInfo, 0, sizeof(siStartupInfo));
				memset(&piProcessInfo, 0, sizeof(piProcessInfo));
				siStartupInfo.cb = sizeof(siStartupInfo);

				::CreateProcessA(buffer, // application name/path
					NULL, // command line (optional)
					NULL, // no process attributes (default)
					NULL, // default security attributes
					false,
					CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_CONSOLE,
					NULL, // default env
					NULL, // default working dir
					&siStartupInfo,
					&piProcessInfo);

				::TerminateProcess(GetCurrentProcess(), 0);
				::ExitProcess(0); // exit this process

				// this does not return.
			}
		}
		catch (const std::exception& e)
		{
			this->version = std::format("UPDATE FAILED ({})", e.what());
			this->update_available = false;
		}
		catch (...)
		{
			this->version = "UPDATE FAILED (unknown error)";
			this->update_available = false;
		}
	});
}

Updater::Updater():
	description("@stormyy_ow")
{
	if (!g_settings) throw std::runtime_error("updater depends on g_settings");

	this->version_future = std::async(std::launch::async, [&] {
		try {
			/* get .exe path and hash */
			this->getMeta();
			this->version = this->__exe_hash.substr(0, 9);

			// TODO: delete this in a future version. it's only here for older versions.
			/* !deprecated */
			{
				std::filesystem::path _tmp_path = std::filesystem::path(this->__exe_path + L".old"); // gets path of old exe
				if (std::filesystem::exists(_tmp_path)) {
					std::filesystem::remove(_tmp_path);
				}
			}

			/* fetch new hash and compare */
			if (this->checkForUpdate()) {
				//this->version = "UPDATE AVAILABLE";
				this->update_available = true;

				std::println("auto update: {}", (*g_settings).getAppSettings().options.auto_update);

				// todo settings
				if ((*g_settings).getAppSettings().options.auto_update)
				{
		#ifndef _DEBUG
					this->update();
		#endif
				}
			}
		}
		catch (const std::exception& e)
		{
			this->version = std::format("VERSION FAILED ({})", e.what());
			this->update_available = false;
		}
		catch (...)
		{
			this->version = "VERSION FAILED (unknown error)";
			this->update_available = false;
		}
	});
}

Updater::~Updater() {
	if (this->version_future.valid()) this->version_future.get();
	if (this->update_future.valid()) this->update_future.get();
}

void Updater::render()
{

	if (!ImGui::IsWindowAppearing()) {
		if (downloading && !ImGui::IsPopupOpen("updating")) {
			ImGui::OpenPopup("updating");
		}
		else if (!update_available && ImGui::IsPopupOpen("updating")) {
			//ImGui::CloseCurrentPopup();
			ImGui::ClosePopupsOverWindow(ImGui::FindWindowByName("dashboard"), false);
		}
	}

	ImGui::CloseCurrentPopup();
	static const auto list = ImGui::GetWindowDrawList();
	static const auto &style = ImGui::GetStyle();

	static const auto width = ImGui::GetContentRegionAvail().x;


	// version
	{
		static const auto color_text = ImGui::ColorConvertFloat4ToU32({ .8f, .8f, .8f, style.Alpha });
		list->AddText(font_subtitle, 14, ImGui::GetCursorScreenPos(), color_text, this->version.c_str());
		//if (this->downloadState.active)
		{
			const auto width_text = font_subtitle->CalcTextSizeA(14, FLT_MAX, 0.0f, this->description.c_str());
			list->AddText(font_subtitle, 14, ImGui::GetCursorScreenPos() + ImVec2(ImGui::GetContentRegionAvail().x - width_text.x, 0), color_text, this->description.c_str());
		}
		ImGui::Dummy({ 0, 4 });
	}

	//if (this->display)
	{
		//static const auto color_background = ImGui::ColorConvertFloat4ToU32({ 1, 1, 1, 1 });
		static const auto color_progress_downloading_background = ImColor::HSV(0 / 14.0f, 0.2f, 1.0f, 0.4f * style.Alpha);
		static const auto color_progress_downloading = ImColor::HSV(0 / 14.0f, 0.4f, 1.0f, style.Alpha);

		static const auto color_progress_background = ImColor::HSV(0.0f, 0.0f, 0.9f, style.Alpha);
		// static const auto color_progress = ImColor::HSV(fmod( - 0.02f, 1.0f) / 14.0f, 0.0f, 1.0f, style.Alpha);


		if (this->downloading)
		{
			//list->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), color_background, 5);
			ImGui::Dummy({ 0, 0 });
	
			//const auto pos = ImVec2(0, 0) + ImGui::GetWindowPos() + style.WindowPadding;
			const auto pos = ImGui::GetCursorScreenPos();
			const auto width_progress = (((width * this->progress) > (9.0f)) ? (width * this->progress) : (9.0f));


			list->AddRectFilled(pos, pos + ImVec2(width, 9), color_progress_downloading_background, 5);
			list->AddRectFilled(pos, pos + ImVec2(width_progress, 9), color_progress_downloading, 5);

			ImGui::Dummy({ width, 9 });
		}
		else
		{
			// list->AddRectFilled(pos, pos + ImVec2(width, 9), color_progress_background, 5);
		}
	}

	ImGui::Spacing();

	{
		if (this->update_available && !(*g_settings).getAppSettings().options.auto_update) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0.09f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0.3f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0.2f));
			{
				if (ImGui::Button("Update available", { ImGui::GetContentRegionAvail().x, 0 })) {
					this->update();
				}
				ImGui::SetItemTooltip("Click to download an update");
			}
			ImGui::PopStyleColor(3);
		}

		if (ImGui::BeginPopupModal("updating", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground))
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
			{
				ImGui::Text("updating");
			}
			ImGui::PopStyleColor();
			ImGui::EndPopup();
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
            if (ImGui::CollapsingHeader("update.h", ImGuiTreeNodeFlags_None))
            {
				ImGui::Checkbox("downloading", &(this->downloading));

				ImGui::SliderFloat("progress", (&this->progress), 0.0f, 1.0f);
            }


            ImGui::End();
        }
    #endif

}