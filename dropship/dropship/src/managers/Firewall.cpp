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

void Firewall::tryWriteSettingsToFirewall(std::string data, std::string block) {

	println("storage: {} / 1024", data.length());

	util::win_firewall::forFirewallRulesInGroup(this->__group_name, [&data, &block](const CComPtr<INetFwRule>& FwRule, const CComPtr<INetFwRules>& rules) {

		CComBSTR description (data.c_str());
		if (SUCCEEDED(FwRule->put_Description(description)))
		{
			printf("description write successful\n");
		}


		/*
			. if no blocks, unblock and set scope to ""
			. if blocks, concat and block

		*/

		if (block.empty()) {
			println("nothing blocked, unblocking..");

			if (FAILED(FwRule->put_Enabled(VARIANT_FALSE)))
			{
				printf("unblock failed\n");
			}
		}
		else {

			CComBSTR blocked_addresses (block.c_str());

			if (FAILED(FwRule->put_RemoteAddresses(blocked_addresses)))
			{
				printf("block addresses failed\n");
			}
			else {
				/* !important make sure remote addresses are not blank. */
				/* !important otherwise this blocks all internet traffic permanently */
				if (FAILED(FwRule->put_Enabled(VARIANT_TRUE)))
				{
					printf("block failed\n");
				}
			}
		}


	});
}

std::optional<std::string> Firewall::tryFetchSettingsFromFirewall() {

	std::optional<std::string> loaded_settings = std::nullopt;

	util::win_firewall::forFirewallRulesInGroup(this->__group_name, [&loaded_settings](const CComPtr<INetFwRule>& FwRule, const CComPtr<INetFwRules>& rules) {

		//USES_CONVERSION;
		CComBSTR description;
		if (SUCCEEDED(FwRule->get_Description(&description)) && description)
		{

			/*const auto ws = std::wstring(description, SysStringLen(description));
			std::string s(ws.begin(), ws.end());*/

			// web std::string sTarget = OLE2A(bstrSource);

			CW2A s (description);

			loaded_settings = std::make_optional<std::string>(s);

			println("patch chars: {}/1024", description.ByteLength());
		}	
	});


	return loaded_settings;
}


/*
	.. currently, ensure a single rule exists
	.. in future, may want to ensure a single out and single in rule exist
*/
void Firewall::_validateRules() {

#ifdef _DEBUG
	util::timer::Timer timer("_validateRules");
#endif

	int c = 0;

	/* legacy */
	{	
		/* delete all stormy.gg/dropship blocks */
		util::win_firewall::forFirewallRulesInGroup(this->__group_name_legacy, [&c](const CComPtr<INetFwRule>& FwRule, const CComPtr<INetFwRules>& FwRules) {
			CComBSTR ruleName;
			FwRule->get_Name(&ruleName);
			FwRules->Remove(ruleName);
		});
	}

	util::win_firewall::forFirewallRulesInGroup(this->__group_name, [&c](const CComPtr<INetFwRule>& FwRule, const CComPtr<INetFwRules>& rules) {
		c ++;
	});

	if (c != 1) {
		// delete all rules
		util::win_firewall::forFirewallRulesInGroup(this->__group_name, [&c](const CComPtr<INetFwRule>& FwRule, const CComPtr<INetFwRules>& FwRules) {
			CComBSTR ruleName;
			if (FAILED(FwRule->get_Name(&ruleName)) && ruleName)
			{
				printf("failed to get rule name\n");
			}
			if (FAILED(FwRules->Remove(ruleName)))
			{
				printf("failed to delete rule\n");
			};
		});


		// add single rule
		util::win_firewall::firewallRulesPredicate([this](const CComPtr<INetFwRules>& FwRules)
		{
			CComBSTR rule_name ("stormy/dropship");
			CComBSTR group_name (this->__group_name);
			//CComBSTR remote_addresses ("");
			NET_FW_RULE_DIRECTION_ dir = NET_FW_RULE_DIR_OUT;
			NET_FW_PROFILE_TYPE2_ profile = NET_FW_PROFILE2_ALL;

			CComPtr<INetFwRule> pFwRule;
			if (FAILED(CoCreateInstance(__uuidof(NetFwRule), nullptr, CLSCTX_INPROC_SERVER, __uuidof(INetFwRule), (void**)&pFwRule)))
			{
				printf("CoCreateInstance for Firewall Rule failed \n");
			}

			// Populate the Firewall Rule object
			pFwRule->put_Name(rule_name);
			//pFwRule->put_Description(bstrRuleDescription);
			//pFwRule->put_ApplicationName(bstrRuleApplication);
			pFwRule->put_Protocol(NET_FW_IP_PROTOCOL_ANY);
			//pFwRule->put_RemoteAddresses(remote_addresses);
			pFwRule->put_Direction(dir);
			pFwRule->put_Grouping(group_name);
			pFwRule->put_Profiles(profile);
			pFwRule->put_Action(NET_FW_ACTION_BLOCK);
			pFwRule->put_Enabled(VARIANT_FALSE);

			// Add the Firewall Rule
			if (FAILED(FwRules->Add(pFwRule)))
			{
				printf("Firewall Rule Add failed: \n");
			}

			else printf("Firewall Rule Added\n");
		});
	}
}
