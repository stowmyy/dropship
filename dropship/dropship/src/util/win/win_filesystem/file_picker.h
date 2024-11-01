#pragma once

#include <commdlg.h> // dialog boxes

namespace util::win_filesystem {

	// std::expected if error
	std::optional<std::filesystem::path> prompt_file();

}
