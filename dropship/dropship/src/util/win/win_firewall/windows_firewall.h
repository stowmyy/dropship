#pragma once


#include <netfw.h> // INetFwPolicy2

//#include <windows.h>
//#include <stdio.h>
//#include <winsock.h> // for getprotocolbyname


#define IP_PROTOCOL_ICMP4  1
#define IP_PROTOCOL_ICMP6  58

// TODO remove
//#include <shlwapi.h> // for SHLoadIndirectString

/*

    !! QUARANTINE ZONE !!

    there is a lot of win32 stuff in this one file.
      • the official windows examples have massive memory leaks and are bad.
      • thus one uses ccomptr to avoid memory leaks: https://github.com/microsoft/Windows-classic-samples/blob/44d192fd7ec6f2422b7d023891c5f805ada2c811/Samples/Win7Samples/security/windowsfirewall/enumeratefirewallrules/EnumerateFirewallRules.cpp

*/

namespace util::win_firewall {

    void forFirewallRulesInGroup(std::string _target_groupName, std::function<void(const CComPtr<INetFwRule>&, const CComPtr<INetFwRules>&)> predicate);

    void forFirewallRulesWithName(std::string _target_ruleName, std::function<void(const CComPtr<INetFwRule>&, const CComPtr<INetFwRules>&)> predicate);

    void firewallRulesPredicate(std::function<void(const CComPtr<INetFwRules>&)> predicate);
}
