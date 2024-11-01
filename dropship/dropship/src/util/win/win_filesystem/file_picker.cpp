#include "pch.h"
#include "file_picker.h"

extern HWND g_hwnd;

namespace util::win_filesystem {

	std::optional<std::filesystem::path> prompt_file()
	{

		/*
		wchar_t _path[MAX_PATH];

		OPENFILENAMEW ofn = { 0 };

		ofn.lStructSize = sizeof(ofn);
		//if (g_hwnd != nullptr) ofn.hwndOwner = g_hwnd;
		ofn.lpstrFilter = L"Text Files\0Overwatch.exe\0All Files\0*\0\0";
		ofn.lpstrTitle = L"select the file pls";
		ofn.lpstrFile = _path;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
		ofn.lpstrDefExt = L"exe";

		if (::GetOpenFileName(&ofn)) {
			// use open.whatever to get data about the selected file

			return std::make_optional(std::wstring(_path));
		}
		else
		{
			printf("ho");
			return std::nullopt;
		}*/

		TCHAR path[MAX_PATH];

		OPENFILENAME ofn = {};
		ofn.lStructSize = sizeof(OPENFILENAME);
		if (g_hwnd != nullptr) ofn.hwndOwner = g_hwnd;
		//ofn.lpstrFilter = TEXT("実況ログ(*.jkl;*.xml)\0*.jkl;*.xml\0すべてのファイル\0*.*\0");
		//ofn.lpstrFilter = TEXT("Overwatch.exe\0Overwatch.exe\0all files\0*.*\0");
		ofn.lpstrFilter = TEXT("Overwatch.exe\0Overwatch.exe\0");
		//ofn.lpstrTitle = TEXT("ファイルを開く");
		ofn.lpstrTitle = TEXT("please find \"Overwatch.exe\"");
		ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
		ofn.lpstrFile = path;
		ofn.nMaxFile = _countof(path);
		path[0] = TEXT('\0');

		if (::GetOpenFileName(&ofn)) {
			return std::make_optional(std::wstring(path));
		}
		else
		{
			return std::nullopt;
		}

		return std::nullopt;
	}

}
