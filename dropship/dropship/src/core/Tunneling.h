#pragma once

#include "core/Settings.h"
#include "core/Firewall.h"

#include "util/win/win_filesystem/file_picker.h";

namespace core::tunneling
{

	class Tunneling
	{

		public:
			Tunneling();
			void render();
			void openExplainer();

		private:
			static std::set<std::string> _queryFirewallForPossibleExePaths(std::string rule_name /* = "Overwatch Application" */);
			bool _open_path_picker_next_frame = false;
			bool _open_explainer_next_frame = false;


	};

}

