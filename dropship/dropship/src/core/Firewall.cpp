#include "pch.h"

#include "Firewall.h"

Firewall::Firewall()
{
	// Initialize COM.
	this->_coInitilizeSuccess = SUCCEEDED(CoInitialize(0));

	if (!(this->_coInitilizeSuccess)) {
		throw std::runtime_error("windows firewall: CoInitialize failed");
	}

	// TODO ensure rule exists
	this->_validateRules();

	this->_queryNetworkStatus();

	// TODO legacy


	/*

	legacy: always remove stormy.gg/dropship
	new group name: stormy/dropship

	legacy: always remove stormy.gg/dropship
	new group name: stormy/dropship

	legacy: always remove stormy.gg/dropship
	new group name: stormy/dropship

	legacy: always remove stormy.gg/dropship
	new group name: stormy/dropship

	*/

}

Firewall::~Firewall() {
	// Uninitialize COM.
	if (this->_coInitilizeSuccess) {
		CoUninitialize();
	}
}

void Firewall::_queryNetworkStatus() {

#ifdef _DEBUG
	//util::timer::Timer timer ("_queryNetworkStatus");
#endif

	//this->_network_information = std::make_optional<util::win_network::NetworkInformation>(util::win_network::queryNetwork());
	this->_network_information = util::win_network::queryNetwork();

}

void Firewall::tryWriteSettingsToFirewall(std::string data, std::string block, std::set<std::filesystem::path> tunneling_paths) {
	// Delete all existing rules in the dropship group
	util::win_firewall::removeAllRulesInGroup(this->__group_name);

	// Always create rules to persist the Description (settings data).
	// If there's nothing to block, create disabled rules.
	BSTR description_bstr = SysAllocStringByteLen(data.data(), (UINT)data.length());

	bool has_block = !block.empty();
	CComBSTR blocked_addresses(block.c_str());

	int rule_index = 0;
	for (auto& path : tunneling_paths) {
		CComBSTR rule_name(rule_index == 0 ? L"stormy/dropship" :
			(std::wstring(L"stormy/dropship ") + std::to_wstring(rule_index + 1)).c_str());
		CComBSTR group_name(this->__group_name.c_str());

		CComPtr<INetFwRule> pFwRule;
		if (FAILED(CoCreateInstance(__uuidof(NetFwRule), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), (void**)&pFwRule)))
		{
			printf("CoCreateInstance for Firewall Rule failed\n");
			continue;
		}

		pFwRule->put_Name(rule_name);
		if (description_bstr) pFwRule->put_Description(description_bstr);
		CComBSTR application_name(path.wstring().c_str());
		pFwRule->put_ApplicationName(application_name);
		pFwRule->put_Protocol(NET_FW_IP_PROTOCOL_ANY);
		if (has_block) pFwRule->put_RemoteAddresses(blocked_addresses);
		pFwRule->put_Direction(NET_FW_RULE_DIR_OUT);
		pFwRule->put_Grouping(group_name);
		pFwRule->put_Profiles(NET_FW_PROFILE2_ALL);
		pFwRule->put_Action(NET_FW_ACTION_BLOCK);
		pFwRule->put_Enabled(has_block ? VARIANT_TRUE : VARIANT_FALSE);

		util::win_firewall::firewallRulesPredicate([&pFwRule](const CComPtr<INetFwRules>& FwRules) {
			if (FAILED(FwRules->Add(pFwRule))) {
				printf("Firewall Rule Add failed\n");
			}
		});

		rule_index++;
	}

	// If no per-app paths, create a single global rule
	if (tunneling_paths.empty()) {
		CComBSTR rule_name(L"stormy/dropship");
		CComBSTR group_name(this->__group_name.c_str());

		CComPtr<INetFwRule> pFwRule;
		if (FAILED(CoCreateInstance(__uuidof(NetFwRule), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), (void**)&pFwRule)))
		{
			printf("CoCreateInstance for Firewall Rule failed\n");
		}
		else {
			pFwRule->put_Name(rule_name);
			if (description_bstr) pFwRule->put_Description(description_bstr);
			pFwRule->put_Protocol(NET_FW_IP_PROTOCOL_ANY);
			if (has_block) pFwRule->put_RemoteAddresses(blocked_addresses);
			pFwRule->put_Direction(NET_FW_RULE_DIR_OUT);
			pFwRule->put_Grouping(group_name);
			pFwRule->put_Profiles(NET_FW_PROFILE2_ALL);
			pFwRule->put_Action(NET_FW_ACTION_BLOCK);
			pFwRule->put_Enabled(has_block ? VARIANT_TRUE : VARIANT_FALSE);

			util::win_firewall::firewallRulesPredicate([&pFwRule](const CComPtr<INetFwRules>& FwRules) {
				if (FAILED(FwRules->Add(pFwRule))) {
					printf("Firewall Rule Add failed\n");
				}
			});
		}
	}

	if (description_bstr) SysFreeString(description_bstr);
}

	std::optional<std::string> Firewall::tryFetchSettingsFromFirewall() {

	std::optional<std::string> loaded_settings = std::nullopt;

	util::win_firewall::forFirewallRulesInGroup(this->__group_name, [&loaded_settings](const CComPtr<INetFwRule>& FwRule, const CComPtr<INetFwRules>& rules) {

		CComBSTR description;
		if (SUCCEEDED(FwRule->get_Description(&description)) && description)
		{
			UINT byte_len = SysStringByteLen(description);
			if (byte_len > 0)
			{
				std::string s(reinterpret_cast<const char*>(static_cast<BSTR>(description)), byte_len);
				loaded_settings = std::make_optional<std::string>(s);
			}
		}	
	});


	return loaded_settings;
}


/*
	.. ensure a single rule exists
	.. in future, may want to ensure a single out and single in rule exist
*/
void Firewall::_validateRules() {

#ifdef _DEBUG
	util::timer::Timer timer("_validateRules");
#endif

	/* legacy - remove old group safely */
	util::win_firewall::removeAllRulesInGroup(this->__group_name_legacy);

	/* ensure at least one rule exists in the group (for settings storage) */
	{
		int c = 0;
		util::win_firewall::forFirewallRulesInGroup(this->__group_name, [&c](const CComPtr<INetFwRule>&, const CComPtr<INetFwRules>&) {
			c++;
		});

		if (c == 0) {
			util::win_firewall::firewallRulesPredicate([this](const CComPtr<INetFwRules>& FwRules)
			{
				CComBSTR rule_name("stormy/dropship");
				CComBSTR group_name(this->__group_name.c_str());
				NET_FW_RULE_DIRECTION_ dir = NET_FW_RULE_DIR_OUT;
				NET_FW_PROFILE_TYPE2_ profile = NET_FW_PROFILE2_ALL;

				CComPtr<INetFwRule> pFwRule;
				if (FAILED(CoCreateInstance(__uuidof(NetFwRule), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), (void**)&pFwRule)))
				{
					printf("CoCreateInstance for Firewall Rule failed\n");
				}
				else {
					pFwRule->put_Name(rule_name);
					pFwRule->put_Protocol(NET_FW_IP_PROTOCOL_ANY);
					pFwRule->put_Direction(dir);
					pFwRule->put_Grouping(group_name);
					pFwRule->put_Profiles(profile);
					pFwRule->put_Action(NET_FW_ACTION_BLOCK);
					pFwRule->put_Enabled(VARIANT_FALSE);

					if (FAILED(FwRules->Add(pFwRule)))
						printf("Firewall Rule Add failed\n");
					else
						printf("Firewall Rule Added\n");
				}
			});
		}
	}
}
