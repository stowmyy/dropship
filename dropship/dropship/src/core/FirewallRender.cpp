#include "pch.h"

#include "Firewall.h"



void Firewall::render() {
	
	/*if (this->_network_information) {

	}

	ImGui::Text("waiting for network information");*/


	bool close__win_net_fw_popup = false;


	// if done during IsWindowAppearing() it's visually offset.  band-aid here.
	/*if (!ImGui::IsWindowAppearing() && this->_network_information_updated_frametime == 0.)
		if (!this->_network_information.connected_networks_are_enabled_in_firewall)
			if (!ImGui::IsPopupOpen(this->__win_net_fw_popup_name)) ImGui::OpenPopup(this->__win_net_fw_popup_name);*/


	if (!this->__win_net_fw_popup_ignored) {

		if (ImGui::GetTime() > this->_network_information_updated_frametime + this->__network_query_delay_s) {
			this->_queryNetworkStatus();
			this->_network_information_updated_frametime = ImGui::GetTime();

			/* connected to a network that's not covered in firewall */
			if (!this->_network_information.connected_networks_are_enabled_in_firewall) {
				if (!ImGui::IsPopupOpen(this->__win_net_fw_popup_name.c_str()))
				{
					ImGui::OpenPopup(this->__win_net_fw_popup_name.c_str());
				}
			}
			else {
				close__win_net_fw_popup = true;
			}
		}

		bool not_quit = true;
		if (ImGui::BeginPopupModal(this->__win_net_fw_popup_name.c_str(), &not_quit, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) {

			if (close__win_net_fw_popup) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::Text("To block servers, dropship needs Windows \nFirewall to be enabled");

			/* vpn note */
			if (this->_network_information.connected_networks_count > 1)
			{
				//ImGui::SeparatorText("note");
				ImGui::BulletText("Multiple networks detected. If you are\nusing a VPN, turning it off may fix\nthis problem");
			}

			static const NET_FW_PROFILE_TYPE2_ profiles [3] = { NET_FW_PROFILE2_DOMAIN, NET_FW_PROFILE2_PUBLIC, NET_FW_PROFILE2_PRIVATE };
			static const char* names [3] = { "domain", "public", "private" };

			/* network notes */
			for (int i = 0; i < 3; i ++)
			{
				auto& p = profiles[i];
				auto& n = names[i];

				if ((this->_network_information.network_profiles_connected & p) && !(this->_network_information.firewall_profiles_enabled & p))
				{
					ImGui::BulletText("This device is connected to a %s\nnetwork, but %s networks are not\nenabled in windows firewall", n, n);
				}
			}

			ImGui::Spacing();

			//ImGui::Text("This will turn on Windows Firewall");

			ImGui::Spacing();

			/* network actions */
			for (int i = 0; i < 3; i++)
			{
				auto& p = profiles[i];
				auto& n = names[i];

				if ((this->_network_information.network_profiles_connected & p) && !(this->_network_information.firewall_profiles_enabled & p))
				{
					if (ImGui::Button(std::format("Turn on for {} networks", n).c_str(), {ImGui::GetContentRegionAvail().x, 0})) {
						util::win_net_fw::enableFirewallForNetworkProfile(p);
					}
				}
			}

			/*ImGui::BeginDisabled();
			if (ImGui::Button("turn on firewall for private networks", { ImGui::GetContentRegionAvail().x, 0 })) {
				not_quit = false;
			}
			ImGui::EndDisabled();*/

			ImGui::PushStyleColor(ImGuiCol_Text, { 0, 0, 0, 0.4f });
			ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0.0f });
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0, 0, 0, 0.04f });
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0, 0, 0, 0.09f });
			{
				if (ImGui::Button("Network settings", { ImGui::GetContentRegionAvail().x, 0 })) {

					// option 01
					//system("%systemroot%\\system32\\control.exe /name Microsoft.WindowsFirewall");

					// option 02
					system("start windowsdefender://network");

					ImGui::OpenPopup(this->__win_net_fw_manual_popup_name.c_str());
				}
			}
			ImGui::PopStyleColor(4);


			/* manual fix modal */
			if (ImGui::BeginPopupModal(this->__win_net_fw_manual_popup_name.c_str(), NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {

				ImGui::Text("Tap the [Turn on] button next to the network\nthat says (active)");

				if (ImGui::Button("done", { ImGui::GetContentRegionAvail().x, 0 })) {
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::EndPopup();
		}

		if (!not_quit)
		{
			this->__win_net_fw_popup_ignored = true;
		}
	}
}
