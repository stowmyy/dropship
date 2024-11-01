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

		private:
			static std::set<std::string> _queryFirewallForPossibleExePaths(std::string rule_name /* = "Overwatch Application" */);

	};

}

