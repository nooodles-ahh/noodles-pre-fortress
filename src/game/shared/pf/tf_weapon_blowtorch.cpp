#include "cbase.h"
#include "tf_weapon_blowtorch.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#endif
#ifdef GAME_DLL
#include "tf_player.h"
#endif

IMPLEMENT_NETWORKCLASS_ALIASED(TFBlowtorch, DT_TFBlowtorch)

BEGIN_NETWORK_TABLE(CTFBlowtorch, DT_TFBlowtorch)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFBlowtorch)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_blowtorch, CTFBlowtorch);
PRECACHE_WEAPON_REGISTER(tf_weapon_blowtorch);

CTFBlowtorch::CTFBlowtorch()
{

}

#ifdef GAME_DLL
bool CTFBlowtorch::Deploy(void)		//make the weapon dev only until it's complete
{
	return true;
}
#endif