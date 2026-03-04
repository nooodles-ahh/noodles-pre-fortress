#include "cbase.h"
#include "clientsteamcontext.h"
#include "betatest_check.h"

#ifdef BETATEST_CHECK
// tfw you don't mask it at compile time and you find the id with a hex editor
#define MASK 0xDDD2423AFCA634AF
uint64 groupid = 103582791464173470 ^ MASK;

// TODO make this a list later because this is just stupid
uint64 Sandvich_Thief = 76561198851124770 ^ MASK; //Sandvich Thief

extern CSteamAPIContext* steamapicontext;
bool IsBetaTester()
{
	if (IsInGroup())
		return true;
	return HasntSpentMoney();
}

bool IsInGroup()
{
	int c = steamapicontext->SteamFriends()->GetClanCount();
	for (int i = 0; i < c; i++)
	{
		if (steamapicontext->SteamFriends()->GetClanByIndex( i ).ConvertToUint64() == (groupid ^ MASK))
			return true;
	}
	return false;
}

bool HasntSpentMoney()
{
	if (steamapicontext->SteamUser()->GetSteamID().ConvertToUint64() == (Sandvich_Thief ^ MASK))
		return true;
	return false;
}
#endif