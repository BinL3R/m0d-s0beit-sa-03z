//updated from SA:MP 0.3x by FYP
//updated from SA:MP 0.3z by Pushok, iMaddy, povargek
/*

	PROJECT:		mod_sa
	LICENSE:		See LICENSE in the top level directory
	COPYRIGHT:		Copyright we_sux

	mod_sa is available from http://code.google.com/p/m0d-s0beit-sa/

	mod_sa is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	mod_sa is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with mod_sa.  If not, see <http://www.gnu.org/licenses/>.

*/
#include "main.h"

#define SAMP_DLL		"samp.dll"
#define SAMP_CMP		"51528D8C244401000051" 

//randomStuff
extern int						iViewingInfoPlayer;
int								g_iSpectateEnabled = 0, g_iSpectateLock = 0, g_iSpectatePlayerID = -1;
int								g_iCursorEnabled = 0;

// global samp pointers
int								iIsSAMPSupported = 0;
int								g_renderSAMP_initSAMPstructs;
stSAMP							*g_SAMP = NULL;
stPlayerPool					*g_Players = NULL;
stVehiclePool					*g_Vehicles = NULL;
stChatInfo						*g_Chat = NULL;
stInputInfo						*g_Input = NULL;
stKillInfo						*g_DeathList = NULL;
stDialog						*g_Dialog = NULL;

// global managed support variables
stTranslateGTASAMP_vehiclePool	translateGTASAMP_vehiclePool;
stTranslateGTASAMP_pedPool		translateGTASAMP_pedPool;

stStreamedOutPlayerInfo			g_stStreamedOutInfo;



//////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// FUNCTIONS //////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

// update SAMPGTA vehicle translation structure
void update_translateGTASAMP_vehiclePool ( void )
{
	traceLastFunc( "update_translateGTASAMP_vehiclePool()" );
	if ( !g_Vehicles )
		return;

	int iGTAID;
	for ( int i = 0; i <= SAMP_VEHICLE_MAX; i++ )
	{
		if ( g_Vehicles->iIsListed[i] != 1 )
			continue;
		if ( isBadPtr_writeAny(g_Vehicles->pSAMP_Vehicle[i], sizeof(stSAMPVehicle)) )
			continue;
		iGTAID = getVehicleGTAIDFromInterface( (DWORD *)g_Vehicles->pSAMP_Vehicle[i]->pGTA_Vehicle );
		if ( iGTAID <= SAMP_VEHICLE_MAX && iGTAID >= 0 )
		{
			translateGTASAMP_vehiclePool.iSAMPID[iGTAID] = i;
		}
	}
}

// update SAMPGTA ped translation structure
void update_translateGTASAMP_pedPool ( void )
{
	traceLastFunc( "update_translateGTASAMP_pedPool()" );
	if ( !g_Players )
		return;

	int iGTAID, i;
	for ( i = 0; i < SAMP_PLAYER_MAX; i++ )
	{
		if ( i == g_Players->sLocalPlayerID )
		{
			translateGTASAMP_pedPool.iSAMPID[0] = i;
			continue;
		}

		if ( isBadPtr_writeAny(g_Players->pRemotePlayer[i], sizeof(stRemotePlayer)) )
			continue;
		if ( isBadPtr_writeAny(g_Players->pRemotePlayer[i]->pPlayerData, sizeof(stRemotePlayerData)) )
			continue;
		if ( isBadPtr_writeAny(g_Players->pRemotePlayer[i]->pPlayerData->pSAMP_Actor, sizeof(stSAMPPed)) )
			continue;

		iGTAID = getPedGTAIDFromInterface( (DWORD *)g_Players->pRemotePlayer[i]->pPlayerData->pSAMP_Actor->pGTA_Ped );
		if ( iGTAID <= SAMP_PLAYER_MAX && iGTAID >= 0 )
		{
			translateGTASAMP_pedPool.iSAMPID[iGTAID] = i;
		}
	}
}

//ClientCommands

extern int	joining_server;
void cmd_change_server ( char *param )	//127.0.0.1 7777 Username Password
{
	traceLastFunc( "cmd_change_server()" );

	bool	success = false;

	char	IP[128], Nick[MAX_PLAYER_NAME], Password[128] = "", Port[128];
	int		iPort;

	int ipc = sscanf( param, "%s%s%s%s", IP, Port, Nick, Password );
	if ( ipc < 2 )
	{
		addMessageToChatWindow( "USAGE: /m0d_change_server <ip> <port> <Username> <Server Password>" );
		addMessageToChatWindow( "Variables that are set to \"NULL\" (capitalized) will be ignored." );
		addMessageToChatWindow( "If you set the Password to \"NULL\" it is set to <no server password>." );
		addMessageToChatWindow( "Username and password can also be left out completely." );
		return;
	}
	if ( stricmp( IP, "NULL" ) == NULL )
		strcpy( IP, g_SAMP->szIP );

	if ( stricmp( Port, "NULL" ) == NULL )
		iPort = g_SAMP->ulPort;
	else
		iPort = atoi( Port );

	if ( ipc > 2 )
	{
		if ( stricmp( Nick, "NULL" ) != NULL )
		{
			if ( strlen( Nick ) > ALLOWED_PLAYER_NAME_LENGTH )
				Nick[ALLOWED_PLAYER_NAME_LENGTH] = '\0';
			setLocalPlayerName( Nick );
		}
	}
	if ( ipc > 3 )
	{
		if ( stricmp( Password, "NULL" ) == NULL )
			strcpy( Password, "" );
	}

	changeServer( IP, iPort, Password );
}

void cmd_change_server_fav ( char *param )
{
	traceLastFunc( "cmd_change_server_fav()" );

	if ( strlen(param) == 0 )
	{
		addMessageToChatWindow( "/m0d_fav_server <server name/part of server name>" );
		addMessageToChatWindow( "In order to see the favorite server list type: /m0d_fav_server list" );
		return;
	}

	if ( strncmp(param, "list", 4) == 0 )
	{
		int count = 0;
		for ( int i = 0; i < INI_SERVERS_MAX; i++ )
		{
			if ( set.server[i].server_name == NULL )
				continue;

			count++;
			addMessageToChatWindow( "%s", set.server[i].server_name );
		}
		if ( count == 0 )
			addMessageToChatWindow( "No servers in favorite server list. Edit the ini file to add some." );
		return;
	}

	for ( int i = 0; i < INI_SERVERS_MAX; i++ )
	{
		if ( set.server[i].server_name == NULL || set.server[i].ip == NULL
			|| strlen(set.server[i].ip) < 7 || set.server[i].port == 0 )
			continue;

		if ( !findstrinstr((char *)set.server[i].server_name, param) )
			continue;

		if ( !set.use_current_name )
			setLocalPlayerName( set.server[i].nickname );

		changeServer( set.server[i].ip, set.server[i].port, set.server[i].password );

		return;
	}

	addMessageToChatWindow( "/m0d_fav_server <server name/part of server name>" );
	return;
}

void cmd_current_server ( char *param )
{
	addMessageToChatWindow( "Server Name: %s", g_SAMP->szHostname );
	addMessageToChatWindow( "Server Address: %s:%i", g_SAMP->szIP, g_SAMP->ulPort );
	addMessageToChatWindow( "Username: %s", getPlayerName(g_Players->sLocalPlayerID) );
}

// strtokstristr?
bool findstrinstr ( char *text, char *find )
{
	char	realtext[256];
	char	subtext[256];
	char	*result;
	char	*next;
	char	temp;
	int		i = 0;

	traceLastFunc( "findstrinstr()" );

	// can't find stuff that isn't there unless you are high
	if ( text == NULL || find == NULL )
		return false;

	// lower case text ( sizeof()-2 = 1 for array + 1 for termination after while() )
	while ( text[i] != NULL && i < (sizeof(realtext)-2) )
	{
		temp = text[i];
		if ( isupper(temp) )
			temp = tolower( temp );
		realtext[i] = temp;
		i++;
	}
	realtext[i] = 0;

	// replace unwanted characters/spaces with dots
	i = 0;
	while ( find[i] != NULL && i < (sizeof(subtext)-2) )
	{
		temp = find[i];
		if ( isupper(temp) )
			temp = tolower( temp );
		if ( !isalpha(temp) )
			temp = '.';
		subtext[i] = temp;
		i++;
	}
	subtext[i] = 0;

	// use i to count the successfully found text parts
	i = 0;

	// split and find every part of subtext/find in text
	result = &subtext[0];
	while ( *result != NULL )
	{
		next = strstr( result, "." );
		if ( next != NULL )
		{
			// more than one non-alphabetic character
			if ( next == result )
			{
				do
					next++;
				while ( *next == '.' );

				if ( *next == NULL )
					return (i != 0);
				result = next;
				next = strstr( result, "." );
				if ( next != NULL )
					*next = NULL;
			}
			else
				*next = NULL;
		}

		if ( strstr(realtext, result) == NULL )
			return false;

		if ( next == NULL )
			return true;

		i++;
		result = next + 1;
	}

	return false;
}

void cmd_tele_loc ( char *param )
{
	if ( strlen(param) == 0 )
	{
		addMessageToChatWindow( "USAGE: /m0d_tele_loc <location name>" );
		addMessageToChatWindow( "Use /m0d_tele_locations to show the location names." );
		addMessageToChatWindow( "The more specific you are on location name the better the result." );
		return;
	}

	for ( int i = 0; i < STATIC_TELEPORT_MAX; i++ )
	{
		if ( strlen(set.static_teleport_name[i]) == 0 || vect3_near_zero(set.static_teleport[i].pos) )
			continue;

		if ( !findstrinstr(set.static_teleport_name[i], param) )
			continue;

		cheat_state_text( "Teleported to: %s.", set.static_teleport_name[i] );
		cheat_teleport( set.static_teleport[i].pos, set.static_teleport[i].interior_id );
		return;
	}

	addMessageToChatWindow( "USAGE: /m0d_tele_loc <location name>" );
	addMessageToChatWindow( "Use /m0d_tele_locations to show the location names." );
	addMessageToChatWindow( "The more specific you are on location name the better the result." );
}

void cmd_tele_locations ()
{
	for ( int i = 0; i < STATIC_TELEPORT_MAX; i++ )
	{
		if ( strlen(set.static_teleport_name[i]) == 0 || vect3_near_zero(set.static_teleport[i].pos) )
			continue;
		addMessageToChatWindow( "%s", set.static_teleport_name[i] );
	}

	addMessageToChatWindow( "To teleport use the menu or: /m0d_tele_loc <location name>" );
}

void cmd_pickup ( char *params )
{
	if ( !strlen( params ) )
	{
		addToChatWindow("�������: /sendpic [����� ������]",-1);
		return;
	}
	g_RakClient->SendPickUp(atoi(params));
}
void cmd_save_objects (char *param) 
{   
   FILE    *flStolenObjects = NULL; 
   char    filename[512]; 
   snprintf( filename, sizeof(filename), "%s\\stolen_objects.txt", g_szWorkingDirectory); 

   flStolenObjects = fopen( filename, "a" ); 
   if (flStolenObjects == NULL)return; 

   DWORD baseObjAddr; 
   float rotMatrix[3]; 
   int objectscount = 0, radius = 0; 
   char comment[50]; 
   const struct actor_info *actor_self = actor_info_get( ACTOR_SELF, 0 ); 
   float dist[3] = { 0.0f, 0.0f, 0.0f }; 
   strcpy(comment, ""); 
   sscanf(param, "%d %[^\n]s", &radius, comment); 
   fprintf(flStolenObjects, "// ================= [SAMP OBJECTS STEALER BY MAZAHACKA] =================\n"); 
   if(strlen(comment))fprintf(flStolenObjects, "// %s\n", comment); 
   for (int i = 0; i < SAMP_OBJECTS_MAX; i++ ) 
   { 
    if ( g_SAMP->pPools->pPool_Object->iIsListed[i] != 1 ) 
     continue; 
    if ( g_SAMP->pPools->pPool_Object->object[i] == NULL ) 
     continue; 
    if ( g_SAMP->pPools->pPool_Object->object[i]->pGTAObject == NULL ) 
     continue; 

    float    pos[3]; 
    vect3_copy( &g_SAMP->pPools->pPool_Object->object[i]->pGTAObject->base.matrix[4 * 3], pos ); 
    if ( vect3_near_zero(pos) ) 
     continue; 
    vect3_vect3_sub( &g_SAMP->pPools->pPool_Object->object[i]->pGTAObject->base.matrix[4 * 3], &actor_self->base.matrix[4 * 3], dist ); 

    if(vect3_length(dist) > radius && radius)continue; 
    baseObjAddr = (DWORD)g_SAMP->pPools->pPool_Object->object[i]; 
    rotMatrix[0] = *(float *)(baseObjAddr + 0xAC); 
    rotMatrix[1] = *(float *)(baseObjAddr + 0xB0); 
    rotMatrix[2] = *(float *)(baseObjAddr + 0xB4); 
    fprintf(flStolenObjects, "CreateObject(%d, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f); // object (%d)\n", g_SAMP->pPools->pPool_Object->object[i]->pGTAObject->base.model_alt_id, pos[0], pos[1], pos[2], 
     rotMatrix[0], rotMatrix[1], rotMatrix[2], i); 
    objectscount++; 
   } 

   fclose(flStolenObjects); 

   addMessageToChatWindow("%d objects saved to stolen_objects.txt file.", objectscount); 
} 
void cmd_dialoghide ( char *params )
{
	g_Dialog->iDialogShowed ^= 1;
}
void cmd_nick ( char *params )
{
	if ( !strlen( params ) )
	{
		addToChatWindow("�������: /nick [���]",-1);
		return;
	}
	setLocalPlayerName(params);
	restartGame();
	g_SAMP->iGameState = GAMESTATE_WAIT_CONNECT;
}
void cmd_spawncar ( char *params )
{
	if ( !strlen( params ) )
	{
		addToChatWindow("�������: /spawncar [vehicleid]",-1);
		return;
	}
	int packetcarboom = RPC_VehicleDestroyed;
	int carid = atoi(params);
    BitStream bs;
    bs.Write(carid);
    g_RakClient->RPC(packetcarboom, &bs, HIGH_PRIORITY, RELIABLE_SEQUENCED, 0, 0);
}

void cmd_setclass ( char *params )
{
	if ( !strlen( params ) )
	{
		addMessageToChatWindow( "USAGE: /m0d_setclass <class id>" );
		return;
	}

	g_RakClient->RequestClass( atoi( params ) );
}

void cmd_fakekill ( char *params )
{
	int killer, reason, amount;
	if ( !strlen( params ) || sscanf( params, "%d%d%d", &killer, &reason, &amount ) < 3 )
	{
		addMessageToChatWindow( "USAGE: /m0d_fakekill <killer id> <reason> <amount>" );
		return;
	}
	if ( amount < 1 || killer < 0 || killer > SAMP_PLAYER_MAX )
		return;

	for ( int i = 0; i < amount; i++ ) 
		g_RakClient->SendDeath( killer, reason );
}

// new functions to check for bad pointers
int isBadPtr_SAMP_iVehicleID ( int iVehicleID )
{
	if ( g_Vehicles == NULL || iVehicleID == (uint16_t)-1)
		return 1;
	return !g_Vehicles->iIsListed[iVehicleID];

	// this hasn't been required yet
	//if (g_Vehicles->pSAMP_Vehicle[iVehicleID] == NULL) continue;
}

int isBadPtr_SAMP_iPlayerID ( int iPlayerID )
{
	if ( g_Players == NULL || iPlayerID < 0 || iPlayerID > SAMP_PLAYER_MAX)
		return 1;
	return !g_Players->iIsListed[iPlayerID];
}

void getSamp ()
{
	if ( set.basic_mode )
		return;

	uint32_t	samp_dll = getSampAddress();

	if ( samp_dll != NULL )
	{
		g_dwSAMP_Addr = ( uint32_t ) samp_dll;

		if ( g_dwSAMP_Addr != NULL )
		{
			if ( memcmp_safe((uint8_t *)g_dwSAMP_Addr + 0xBB75, hex_to_bin(SAMP_CMP), 10) )
			{
				strcpy(g_szSAMPVer, "SA:MP 0.3z-R2");
				Log( "%s was detected. g_dwSAMP_Addr: 0x%p", g_szSAMPVer, g_dwSAMP_Addr );

				// anticheat patch
				struct patch_set fuckAC =
    				{
      					"Anticheat patch", 0, 0,
      					{
							{ 1, (void *)(g_dwSAMP_Addr + 0x957D0), NULL, (uint8_t *)"\xC3", 0 }, // 0.3z
      					}
    				};
    				patcher_install( &fuckAC );
					
					if (set.useuserfont)
					{
						BYTE FontPatch[] =
						{
							0x67, 0x74, 0x61, 0x73, 0x61
						};
						memcpy_safe((void *)(g_dwSAMP_Addr + SAMP_FONT_OFFSET), FontPatch, 5); // user's font
					}

				iIsSAMPSupported = 1;
			}
			else
			{
				Log( "Unknown SA:MP version. Running in basic mode." );
				iIsSAMPSupported = 0;
				set.basic_mode = true;

				g_dwSAMP_Addr = NULL;
			}
		}
	}
	else
	{
		iIsSAMPSupported = 0;
		set.basic_mode = true;
		Log( "samp.dll not found. Running in basic mode." );
	}

	return;
}

uint32_t getSampAddress ()
{
	if ( set.run_mode == RUNMODE_SINGLEPLAYER )
		return 0x0;

	uint32_t	samp_dll;

	if ( set.run_mode == RUNMODE_SAMP )
	{
		if ( set.wine_compatibility )
		{
			HMODULE temp = LoadLibrary( SAMP_DLL );
			__asm mov samp_dll, eax
		}
		else
		{
			void	*temp = dll_baseptr_get( SAMP_DLL );
			__asm mov samp_dll, eax
		}
	}

	if ( samp_dll == NULL )
		return 0x0;

	return samp_dll;
}

struct stSAMP *stGetSampInfo ( void )
{
	if ( g_dwSAMP_Addr == NULL )
		return NULL;

	uint32_t	info_ptr;
	info_ptr = ( UINT_PTR ) * ( uint32_t * ) ( (uint8_t *) (void *)((uint8_t *)g_dwSAMP_Addr + SAMP_INFO_OFFSET) );
	if ( info_ptr == NULL )
		return NULL;

	return (struct stSAMP *)info_ptr;
}

struct stChatInfo *stGetSampChatInfo ( void )
{
	if ( g_dwSAMP_Addr == NULL )
		return NULL;

	uint32_t	chat_ptr;
	chat_ptr = ( UINT_PTR ) * ( uint32_t * ) ( (uint8_t *) (void *)((uint8_t *)g_dwSAMP_Addr + SAMP_CHAT_INFO_OFFSET) );
	if ( chat_ptr == NULL )
		return NULL;

	return (struct stChatInfo *)chat_ptr;
}

struct stInputInfo *stGetInputInfo ( void )
{
	if ( g_dwSAMP_Addr == NULL )
		return NULL;

	uint32_t	input_ptr;
	input_ptr = ( UINT_PTR ) * ( uint32_t * ) ( (uint8_t *) (void *)((uint8_t *)g_dwSAMP_Addr + SAMP_CHAT_INPUT_INFO_OFFSET) );
	if ( input_ptr == NULL )
		return NULL;

	return (struct stInputInfo *)input_ptr;
}

struct stKillInfo *stGetKillInfo ( void )
{
	if ( g_dwSAMP_Addr == NULL )
		return NULL;

	uint32_t	kill_ptr;
	kill_ptr = ( UINT_PTR ) * ( uint32_t * ) ( (uint8_t *) (void *)((uint8_t *)g_dwSAMP_Addr + SAMP_KILL_INFO_OFFSET) );
	if ( kill_ptr == NULL )
		return NULL;

	return (struct stKillInfo *)kill_ptr;
}

struct stDialog *stGetDialogInfo ( void ) 
{ 
  if ( g_dwSAMP_Addr == NULL ) 
   return NULL; 

  uint32_t    dialog_ptr; 
  dialog_ptr = ( UINT_PTR ) * ( uint32_t * ) ( (uint8_t *) (void *)((uint8_t *)g_dwSAMP_Addr + SAMP_DIALOG_INFO_OFFSET) ); 
  if ( dialog_ptr == NULL ) 
   return NULL; 

  return (struct stDialog *)dialog_ptr; 
}

D3DCOLOR samp_color_get ( int id, DWORD trans )
{
	if ( g_dwSAMP_Addr == NULL )
		return NULL;

	D3DCOLOR	*color_table;
	if ( id < 0 || id >= (SAMP_PLAYER_MAX + 3) )
		return D3DCOLOR_ARGB( 0xFF, 0x99, 0x99, 0x99 );

	switch ( id )
	{
	case ( SAMP_PLAYER_MAX ):
		return 0xFF888888;

	case ( SAMP_PLAYER_MAX + 1 ):
		return 0xFF0000AA;

	case ( SAMP_PLAYER_MAX + 2 ):
		return 0xFF63C0E2;
	}

	color_table = ( D3DCOLOR * ) ( (uint8_t *)g_dwSAMP_Addr + SAMP_COLOR_OFFSET );
	return ( color_table[id] >> 8 ) | trans;
}

void spectatePlayer(int iPlayerID)
{
	if ( iPlayerID == -1 )
	{
		GTAfunc_TogglePlayerControllable(0);
		GTAfunc_LockActor(0);
		pGameInterface->GetCamera()->RestoreWithJumpCut();

		g_iSpectateEnabled = 0;
		g_iSpectateLock = 0;
		g_iSpectatePlayerID = -1;
		return;
	}

	g_iSpectatePlayerID = iPlayerID;
	g_iSpectateLock = 0;
	g_iSpectateEnabled = 1;
}

void spectateHandle()
{
	if(g_iSpectateEnabled)
	{
		if(g_iSpectateLock) return;

		if(g_iSpectatePlayerID != -1)
		{
			if(g_Players->iIsListed[g_iSpectatePlayerID] != 0)
			{
				if(g_Players->pRemotePlayer[g_iSpectatePlayerID] != NULL)
				{
					int iState = getPlayerState(g_iSpectatePlayerID);

					if(iState == PLAYER_STATE_ONFOOT)
					{
						struct actor_info *pPlayer = getGTAPedFromSAMPPlayerID(g_iSpectatePlayerID);
						if(pPlayer == NULL) return;
						GTAfunc_CameraOnActor(pPlayer);
						g_iSpectateLock = 1;
					}
					else if(iState == PLAYER_STATE_DRIVER)
					{
						struct vehicle_info *pPlayerVehicleID = g_Players->pRemotePlayer[g_iSpectatePlayerID]->pPlayerData->pSAMP_Vehicle->pGTA_Vehicle;
						if(pPlayerVehicleID == NULL) return;
						GTAfunc_CameraOnVehicle(pPlayerVehicleID);
						g_iSpectateLock = 1;
					}
					else if(iState == PLAYER_STATE_PASSENGER)
					{
						struct vehicle_info *pPlayerVehicleID = g_Players->pRemotePlayer[g_iSpectatePlayerID]->pPlayerData->pSAMP_Vehicle->pGTA_Vehicle;
						if(pPlayerVehicleID == NULL) return;
						GTAfunc_CameraOnVehicle(pPlayerVehicleID);
						g_iSpectateLock = 1;
					}
				}
				else
				{
					cheat_state_text("Player is not streamed in");
					g_iSpectateEnabled = 0;
				}
			}
		}
	}
}

void sampMainCheat ()
{
	traceLastFunc( "sampMainCheat()" );

	// g_Vehicles & g_Players pointers need to be refreshed or nulled
	if ( isBadPtr_writeAny(g_SAMP->pPools->pPool_Vehicle, sizeof(stVehiclePool)) )
		g_Vehicles = NULL;
	else
		g_Vehicles = g_SAMP->pPools->pPool_Vehicle;

	if ( isBadPtr_writeAny(g_SAMP->pPools->pPool_Player, sizeof(stPlayerPool)) )
		g_Players = NULL;
	else
		g_Players = g_SAMP->pPools->pPool_Player;

	// update GTA to SAMP translation structures
	update_translateGTASAMP_vehiclePool();
	update_translateGTASAMP_pedPool();

	spectateHandle();

	// start chatbox logging
	if ( set.chatbox_logging )
	{
		static int	chatbox_init;
		if ( !chatbox_init )
		{
			SYSTEMTIME	time;
			GetLocalTime( &time );
			LogChatbox( false, "Session started at %02d/%02d/%02d", time.wDay, time.wMonth, time.wYear );
			chatbox_init = 1;
		}
	}

	if ( KEY_DOWN(set.secondary_key) )
	{
		if ( KEY_PRESSED(set.key_player_info_list) )
			cheat_state->player_info_list ^= 1;

		if ( KEY_PRESSED(set.key_rejoin) )
		{
			restartGame();
			disconnect( 500 );
			cheat_state_text( "Rejoining in %d seconds...", set.rejoin_delay / 1000 );
			cheat_state->_generic.rejoinTick = GetTickCount();
		}

		if ( KEY_PRESSED(set.key_respawn) )
			playerSpawn();
	}

	if ( KEY_DOWN(set.chat_secondary_key) )
	{
		int			i, key, spam;
		const char	*msg;
		for ( i = 0; i < INI_CHATMSGS_MAX; i++ )
		{
			struct chat_msg *msg_item = &set.chat[i];
			if ( msg_item->key == NULL )
				continue;
			if ( msg_item->msg == NULL )
				continue;
			if ( msg_item->key != key_being_pressed )
				continue;
			key = msg_item->key;
			msg = msg_item->msg;
			spam = msg_item->spam;
			if ( spam )
			{
				if ( msg )
					if ( KEY_DOWN(key) )
						say( "%s", msg );
			}
			else
			{
				if ( msg )
					if ( KEY_PRESSED(key) )
						say( "%s", msg );
			}
		}
	}

	static int	iSAMPHooksInstalled;
	if ( !iSAMPHooksInstalled )
	{
		installSAMPHooks();
		iSAMPHooksInstalled = 1;
	}

	if ( cheat_state->_generic.rejoinTick && cheat_state->_generic.rejoinTick < (GetTickCount() - set.rejoin_delay) )
	{
		g_SAMP->iGameState = GAMESTATE_WAIT_CONNECT;
		cheat_state->_generic.rejoinTick = 0;
	}

	if ( joining_server == 1 )
	{
		restartGame();
		disconnect( 500 );
		cheat_state_text( "Joining server in %d seconds...", set.rejoin_delay / 1000 );
		cheat_state->_generic.join_serverTick = GetTickCount();
		joining_server = 2;
	}

	if ( joining_server == 2
	 &&	 cheat_state->_generic.join_serverTick
	 &&	 cheat_state->_generic.join_serverTick < (GetTickCount() - set.rejoin_delay) )
	{
		g_SAMP->iGameState = GAMESTATE_WAIT_CONNECT;
		joining_server = 0;
		cheat_state->_generic.join_serverTick = 0;
	}
}

int getNthPlayerID ( int n )
{
	if ( g_Players == NULL )
		return -1;

	int thisplayer = 0;
	for ( int i = 0; i < SAMP_PLAYER_MAX; i++ )
	{
		if ( g_Players->iIsListed[i] != 1 )
			continue;
		if ( g_Players->sLocalPlayerID == i )
			continue;
		if ( thisplayer < n )
		{
			thisplayer++;
			continue;
		}

		return i;
	}

	//shouldnt happen
	return -1;
}

int getPlayerCount ( void )
{
	if ( g_Players == NULL )
		return NULL;

	int iCount = 0;
	int i;

	for ( i = 0; i < SAMP_PLAYER_MAX; i++ )
	{
		if ( g_Players->iIsListed[i] != 1 )
			continue;
		iCount++;
	}

	return iCount + 1;
}

#define FUNC_ADDRECALL 0x63060 //0.3z R2
void AddRecallBufer(char* text)
{
	uint32_t func = g_dwSAMP_Addr + FUNC_ADDRECALL;
	_asm
	{
	mov ecx, g_Input;
	push text
	call func;
	}
}

#define SAMP_FUNC_NAMECHANGE 0xA440 //0.3z R2 
int setLocalPlayerName ( const char *name )
{
	if ( g_Players == NULL || g_Players->pLocalPlayer == NULL )
		return 0;

	int strlen_name = strlen( name );
	if ( strlen_name == 0 || strlen_name > ALLOWED_PLAYER_NAME_LENGTH )
		return 0;

	traceLastFunc( "setLocalPlayerName()" );

	//strcpy(g_Players->szLocalPlayerName, name);
	//g_Players->iStrlen_LocalPlayerName = strlen_name;

	DWORD	vtbl_nameHandler = ((DWORD)&g_Players->pVTBL_txtHandler);
	DWORD	func = g_dwSAMP_Addr + SAMP_FUNC_NAMECHANGE;
	__asm push strlen_name
	__asm push name
	__asm mov ecx, vtbl_nameHandler
	__asm call func
	return 1;
}

int getVehicleCount ( void )
{
	if ( g_Vehicles == NULL )
		return NULL;

	int iCount = 0;
	int i;

	for ( i = 0; i < SAMP_VEHICLE_MAX; i++ )
	{
		if ( g_Vehicles->iIsListed[i] != 1 )
			continue;
		iCount++;
	}

	return iCount;
}

int getPlayerPos ( int iPlayerID, float fPos[3] )
{
	traceLastFunc( "getPlayerPos()" );

	struct actor_info	*pActor = NULL;
	struct vehicle_info *pVehicle = NULL;

	struct actor_info	*pSelfActor = actor_info_get( ACTOR_SELF, 0 );

	if ( g_Players == NULL )
		return 0;
	if ( g_Players->iIsListed[iPlayerID] != 1 )
		return 0;
	if ( g_Players->pRemotePlayer[iPlayerID] == NULL )
		return 0;
	if ( g_Players->pRemotePlayer[iPlayerID]->pPlayerData == NULL )
		return 0;

	if ( g_Players->pRemotePlayer[iPlayerID]->pPlayerData->pSAMP_Actor == NULL )
		return 0;	// not streamed
	else
	{
		pActor = g_Players->pRemotePlayer[iPlayerID]->pPlayerData->pSAMP_Actor->pGTA_Ped;

		if ( g_Players->pRemotePlayer[iPlayerID]->pPlayerData->pSAMP_Vehicle != NULL )
			pVehicle = g_Players->pRemotePlayer[iPlayerID]->pPlayerData->pSAMP_Vehicle->pGTA_Vehicle;

		if ( pVehicle != NULL && pActor->vehicle == pVehicle && pVehicle->passengers[0] == pActor )
		{
			// driver of a vehicle
			vect3_copy( &pActor->vehicle->base.matrix[4 * 3], fPos );

			//vect3_copy(g_Players->pRemotePlayer[iPlayerID]->fVehiclePosition, fPos);
		}
		else if ( pVehicle != NULL )
		{
			// passenger of a vehicle
			vect3_copy( &pActor->base.matrix[4 * 3], fPos );

			//vect3_copy(g_Players->pRemotePlayer[iPlayerID]->fActorPosition, fPos);
		}
		else
		{
			// on foot
			vect3_copy( &pActor->base.matrix[4 * 3], fPos );

			//vect3_copy(g_Players->pRemotePlayer[iPlayerID]->fActorPosition, fPos);
		}
	}

	if ( pSelfActor != NULL )
	{
		if ( vect3_dist(&pSelfActor->base.matrix[4 * 3], fPos) < 100.0f )
			vect3_copy( &pActor->base.matrix[4 * 3], fPos );
	}

	// detect zombies
	if ( vect3_near_zero(fPos) )
		vect3_copy( &pActor->base.matrix[4 * 3], fPos );

	return !vect3_near_zero( fPos );
}

const char *getPlayerName ( int iPlayerID )
{
	if ( g_Players == NULL || iPlayerID < 0 || iPlayerID > SAMP_PLAYER_MAX )
		return NULL;

	if ( iPlayerID == g_Players->sLocalPlayerID )
	{
		if ( g_Players->iStrlen_LocalPlayerName <= 0xF )
			return g_Players->szLocalPlayerName;
		return g_Players->pszLocalPlayerName;
	}

	if ( g_Players->pRemotePlayer[iPlayerID] == NULL )
		return NULL;

	if ( g_Players->pRemotePlayer[iPlayerID]->iStrlenName <= 0xF )
		return g_Players->pRemotePlayer[iPlayerID]->szPlayerName;

	return g_Players->pRemotePlayer[iPlayerID]->pszPlayerName;
}

int getPlayerState ( int iPlayerID )
{
	if ( g_Players == NULL || iPlayerID < 0 || iPlayerID > SAMP_PLAYER_MAX )
		return NULL;
	if ( iPlayerID == g_Players->sLocalPlayerID )
		return NULL;
	if ( g_Players->iIsListed[iPlayerID] != 1 )
		return NULL;
	if ( g_Players->pRemotePlayer[iPlayerID]->pPlayerData == NULL )
		return NULL;

	return g_Players->pRemotePlayer[iPlayerID]->pPlayerData->bytePlayerState;
}

int getPlayerVehicleGTAScriptingID ( int iPlayerID )
{
	if ( g_Players == NULL )
		return 0;

	// fix to always return our own vehicle always if that's what's being asked for
	if ( iPlayerID == ACTOR_SELF )
	{
		if(g_Players->pLocalPlayer->sCurrentVehicleID == (uint16_t)-1) return 0;

		stSAMPVehicle	*sampveh = g_Vehicles->pSAMP_Vehicle[g_Players->pLocalPlayer->sCurrentVehicleID];
		if ( sampveh )
		{
			return ScriptCarId( sampveh->pGTA_Vehicle );
			//return (int)( ((DWORD) sampveh->pGTA_Vehicle) - (DWORD) pool_vehicle->start ) / 2584;
		}
		else
			return 0;
	}

	// make sure remote player is legit
	if ( g_Players->pRemotePlayer[iPlayerID] == NULL || g_Players->pRemotePlayer[iPlayerID]->pPlayerData == NULL ||
		g_Players->pRemotePlayer[iPlayerID]->pPlayerData->pSAMP_Vehicle == NULL ||
		g_Players->pRemotePlayer[iPlayerID]->pPlayerData->pSAMP_Vehicle->pGTA_Vehicle == NULL)
		return 0;

	// make sure samp knows the vehicle exists
	if ( g_Vehicles->pSAMP_Vehicle[g_Players->pRemotePlayer[iPlayerID]->pPlayerData->sVehicleID] == NULL )
		return 0;

	// return the remote player's vehicle
	return ScriptCarId( g_Players->pRemotePlayer[iPlayerID]->pPlayerData->pSAMP_Vehicle->pGTA_Vehicle );
}

int getPlayerSAMPVehicleID(int iPlayerID)
{
	if(g_Players == NULL && g_Vehicles == NULL) return 0;
	if(g_Players->pRemotePlayer[iPlayerID] == NULL) return 0;
	if(g_Vehicles->pSAMP_Vehicle[g_Players->pRemotePlayer[iPlayerID]->pPlayerData->sVehicleID] == NULL) return 0;
	return g_Players->pRemotePlayer[iPlayerID]->pPlayerData->sVehicleID;
}

struct actor_info *getGTAPedFromSAMPPlayerID ( int iPlayerID )
{
	if ( g_Players == NULL || iPlayerID < 0 || iPlayerID > SAMP_PLAYER_MAX )
		return NULL;
	if ( iPlayerID == g_Players->sLocalPlayerID )
		return actor_info_get( ACTOR_SELF, 0 );
	if ( g_Players->iIsListed[iPlayerID] != 1 )
		return NULL;
	if ( g_Players->pRemotePlayer[iPlayerID] == NULL )
		return NULL;
	if ( g_Players->pRemotePlayer[iPlayerID]->pPlayerData == NULL )
		return NULL;
	if ( g_Players->pRemotePlayer[iPlayerID]->pPlayerData->pSAMP_Actor == NULL )
		return NULL;
	if ( g_Players->pRemotePlayer[iPlayerID]->pPlayerData->pSAMP_Actor->pGTA_Ped == NULL )
		return NULL;
		
	// return actor_info, null or otherwise
	return g_Players->pRemotePlayer[iPlayerID]->pPlayerData->pSAMP_Actor->pGTA_Ped;
}

struct vehicle_info *getGTAVehicleFromSAMPVehicleID ( int iVehicleID )
{
	if ( g_Vehicles == NULL || iVehicleID < 0 || iVehicleID >= SAMP_VEHICLE_MAX )
		return NULL;
	if ( iVehicleID == g_Players->pLocalPlayer->sCurrentVehicleID )
		return vehicle_info_get( VEHICLE_SELF, 0 );
	if ( g_Vehicles->iIsListed[iVehicleID] != 1 )
		return NULL;

	// return vehicle_info, null or otherwise
	return g_Vehicles->pGTA_Vehicle[iVehicleID];
}

int getSAMPPlayerIDFromGTAPed ( struct actor_info *pGTAPed )
{
	if ( g_Players == NULL )
		return 0;
	if ( actor_info_get(ACTOR_SELF, 0) == pGTAPed )
		return g_Players->sLocalPlayerID;

	int i;
	for ( i = 0; i < SAMP_PLAYER_MAX; i++ )
	{
		if ( g_Players->iIsListed[i] != 1 )
			continue;
		if ( g_Players->pRemotePlayer[i] == NULL )
			continue;
		if ( g_Players->pRemotePlayer[i]->pPlayerData == NULL )
			continue;
		if ( g_Players->pRemotePlayer[i]->pPlayerData->pSAMP_Actor == NULL )
			continue;
		if ( g_Players->pRemotePlayer[i]->pPlayerData->pSAMP_Actor->pGTA_Ped == NULL )
			continue;
		if ( g_Players->pRemotePlayer[i]->pPlayerData->pSAMP_Actor->pGTA_Ped == pGTAPed )
			return i;
	}

	return ACTOR_SELF;
}

int getSAMPVehicleIDFromGTAVehicle ( struct vehicle_info *pVehicle )
{
	if ( g_Vehicles == NULL )
		return NULL;
	if ( vehicle_info_get(VEHICLE_SELF, 0) == pVehicle && g_Players != NULL )
		return g_Players->pLocalPlayer->sCurrentVehicleID;

	int i, iReturn = 0;
	for ( i = 0; i < SAMP_VEHICLE_MAX; i++ )
	{
		if ( g_Vehicles->iIsListed[i] != 1 )
			continue;
		if ( g_Vehicles->pGTA_Vehicle[i] == pVehicle )
			return i;
	}

	return VEHICLE_SELF;
}

uint32_t getPedGTAScriptingIDFromPlayerID ( int iPlayerID )
{
	if ( g_Players == NULL )
		return NULL;

	if ( g_Players->iIsListed[iPlayerID] != 1 )
		return NULL;
	if ( g_Players->pRemotePlayer[iPlayerID] == NULL )
		return NULL;
	if ( g_Players->pRemotePlayer[iPlayerID]->pPlayerData == NULL )
		return NULL;
	if ( g_Players->pRemotePlayer[iPlayerID]->pPlayerData->pSAMP_Actor == NULL )
		return NULL;

	return g_Players->pRemotePlayer[iPlayerID]->pPlayerData->pSAMP_Actor->ulGTA_Ped_ID;
}

uint32_t getVehicleGTAScriptingIDFromVehicleID ( int iVehicleID )
{
	if ( g_Vehicles == NULL )
		return NULL;

	if ( g_Vehicles->iIsListed[iVehicleID] != 1 )
		return NULL;
	if ( g_Vehicles->pSAMP_Vehicle[iVehicleID] == NULL )
		return NULL;

	return g_Vehicles->pSAMP_Vehicle[iVehicleID]->ulGTA_Vehicle_ID;
}

struct m0dClientCMD
{
#pragma pack( 1 )
	char	cmd_name[30];

	//char cmd_description[128];
} m0d_cmd_list[( MAX_CLIENTCMDS - 22 )];
int m0d_cmd_num = 0;

void cmd_showCMDS ()
{
	int i = 0;
	for ( ; i < m0d_cmd_num; i++ )
	{
		addMessageToChatWindow( "%s", m0d_cmd_list[i].cmd_name );
	}
}

#define FUNC_ADDCLIENTCMD	0x63200 //0.3z R2 
void addClientCommand ( char *name, int function )
{
	if ( name == NULL || function == NULL || g_Input == NULL )
		return;

	if ( g_Input->iCMDCount == (MAX_CLIENTCMDS-1) )
	{
		Log( "Error: couldn't initialize '%s'. Maximum command amount reached.", name );
		return;
	}

	if ( strlen(name) > 30 )
	{
		Log( "Error: command name '%s' was too long.", name );
		return;
	}

	if ( m0d_cmd_num < (MAX_CLIENTCMDS - 22) )
	{
		strncpy_s( m0d_cmd_list[m0d_cmd_num].cmd_name, name, sizeof(m0d_cmd_list[m0d_cmd_num].cmd_name)-1 );
		m0d_cmd_num++;
	}
	else
		Log( "m0d_cmd_list[] too short." );

	uint32_t	data = g_dwSAMP_Addr + SAMP_CHAT_INPUT_INFO_OFFSET;
	uint32_t	func = g_dwSAMP_Addr + FUNC_ADDCLIENTCMD;
	__asm mov eax, data
	__asm mov ecx, [eax]
	__asm push function
	__asm push name
	__asm call func
}

bool	modcommands = false;
bool get_isModCommandsActive ()
{
	return modcommands;
}

void init_samp_chat_cmds ()
{
	if ( modcommands == true )
	{
		return;
	}
	else
	{
		cheat_state_text( "initiated modcommands" );
		modcommands = true;
	}

	addClientCommand( "m0d_show_cmds", (int)cmd_showCMDS );
	addClientCommand( "m0d_change_server", (int)cmd_change_server );
	addClientCommand( "m0d_fav_server", (int)cmd_change_server_fav );
	addClientCommand( "m0d_current_server", (int)cmd_current_server );
	addClientCommand( "m0d_tele_loc", (int)cmd_tele_loc );
	addClientCommand( "m0d_teleport_location", (int)cmd_tele_loc );
	addClientCommand( "m0d_tele_locations", (int)cmd_tele_locations );
	addClientCommand( "m0d_teleport_locations", (int)cmd_tele_locations );
	addClientCommand( "sendpic", (int)cmd_pickup );
	addClientCommand( "dialoghide", (int)cmd_dialoghide );
	addClientCommand( "sendclass", (int)cmd_setclass );
	addClientCommand( "spawncar", (int)cmd_spawncar );
	addClientCommand( "m0d_fakekill", (int)cmd_fakekill );
	addClientCommand("save_objects", (int)cmd_save_objects); 
	addClientCommand("nick", (int)cmd_nick); 
}

struct gui	*gui_samp_cheat_state_text = &set.guiset[1];
void addMessageToChatWindow ( const char *text, ... )
{
	if ( g_SAMP != NULL )
	{
		va_list ap;
		if ( text == NULL )
			return;

		char	tmp[512];
		memset( tmp, 0, 512 );

		va_start( ap, text );
		vsnprintf( tmp, sizeof(tmp)-1, text, ap );
		va_end( ap );

		addToChatWindow( tmp, D3DCOLOR_XRGB(gui_samp_cheat_state_text->red, gui_samp_cheat_state_text->green,
						 gui_samp_cheat_state_text->blue) );
	}
	else
	{
		va_list ap;
		if ( text == NULL )
			return;

		char	tmp[512];
		memset( tmp, 0, 512 );

		va_start( ap, text );
		vsnprintf( tmp, sizeof(tmp)-1, text, ap );
		va_end( ap );

		cheat_state_text( tmp, D3DCOLOR_ARGB(255, 0, 200, 200) );
	}
}

void addMessageToChatWindowSS ( const char *text, ... )
{
	if ( g_SAMP != NULL )
	{
		va_list ap;
		if ( text == NULL )
			return;

		char	tmp[512];
		memset( tmp, 0, 512 );

		va_start( ap, text );
		vsprintf( tmp, text, ap );
		va_end( ap );

		addMessageToChatWindow( tmp, D3DCOLOR_ARGB(255, 0, 200, 200) );
	}
	else
	{
		va_list ap;
		if ( text == NULL )
			return;

		char	tmp[512];
		memset( tmp, 0, 512 );

		va_start( ap, text );
		vsprintf( tmp, text, ap );
		va_end( ap );

		cheat_state_text( tmp, D3DCOLOR_ARGB(255, 0, 200, 200) );
	}
}

#define FUNC_ADDTOCHATWND	0x61AB0 //0.3z R2  
void addToChatWindow ( char *text, D3DCOLOR textColor, int playerID )
{
	if ( g_SAMP == NULL || g_Chat == NULL )
		return;

	if ( text == NULL )
		return;

	if ( playerID < -1 )
		playerID = -1;

	uint32_t	chatinfo = g_dwSAMP_Addr + SAMP_CHAT_INFO_OFFSET;
	uint32_t	func = g_dwSAMP_Addr + FUNC_ADDTOCHATWND;

	if ( playerID != -1 )
	{
		// getPlayerName does the needed validity checks, no need for doubles
		char *playerName = (char*)getPlayerName(playerID);
		if ( playerName == NULL )
			return;

		D3DCOLOR playerColor = samp_color_get(playerID);

		__asm mov eax, dword ptr[chatinfo]
		__asm mov ecx, dword ptr[eax]
		__asm push playerColor
		__asm push textColor
		__asm push playerName
		__asm push text
		__asm push 2
		__asm call func
		return;
	}

	__asm mov eax, dword ptr[chatinfo]
	__asm mov ecx, dword ptr[eax]
	__asm push 0
	__asm push textColor
	__asm push 0
	__asm push text
	__asm push 8
	__asm call func
	return;
}

#define FUNC_RESTARTGAME	0x91C0 //0.3z R2   
void restartGame ()
{
	if ( g_SAMP == NULL )
		return;

	uint32_t	samp_info = g_dwSAMP_Addr + SAMP_INFO_OFFSET;
	uint32_t	func = g_dwSAMP_Addr + FUNC_RESTARTGAME;
	__asm mov eax, dword ptr[samp_info]
	__asm mov ecx, dword ptr[eax]
	__asm call func
	__asm pop eax
	__asm pop ecx
}

void say ( char *text, ... )
{
	if ( g_SAMP == NULL )
		return;

	if ( text == NULL )
		return;
	if ( isBadPtr_readAny(text, 128) )
		return;
	traceLastFunc( "say()" );

	va_list ap;
	char	tmp[128];
	memset( tmp, 0, 128 );

	va_start( ap, text );
	vsprintf( tmp, text, ap );
	va_end( ap );

	addSayToChatWindow( tmp );
}

#define FUNC_SAY		0x4C30 //0.3z R2 
#define FUNC_SENDCMD	0x63390 //0.3z R2 
void addSayToChatWindow ( char *msg )
{
	if ( g_SAMP == NULL )
		return;

	if ( msg == NULL )
		return;
	if ( isBadPtr_readAny(msg, 128) )
		return;
	traceLastFunc( "addSayToChatWindow()" );

	if ( msg[0] == '/' )
	{
		uint32_t	func = g_dwSAMP_Addr + FUNC_SENDCMD;
		__asm push msg
		__asm call func
	}
	else
	{
		uint32_t	func = g_dwSAMP_Addr + FUNC_SAY;
		void		*lpPtr = g_Players->pLocalPlayer;
		__asm mov ebx, dword ptr[lpPtr]
		__asm push msg
		__asm call func
		__asm pop ebx
	}
}

#define FUNC_GAMETEXT	0x98720 //0.3z R2  
void showGameText ( char *text, int time, int textsize )
{
	if ( g_SAMP == NULL )
		return;

	uint32_t	func = g_dwSAMP_Addr + FUNC_GAMETEXT;
	__asm push textsize
	__asm push time
	__asm push text
	__asm call func
}

#define FUNC_SPAWN			0x3690 //0.3z R2   
void playerSpawn ( void )
{
	if ( g_SAMP == NULL )
		return;


	uint32_t	func_spawn = g_dwSAMP_Addr + FUNC_SPAWN;
	void		*lpPtr = g_Players->pLocalPlayer;

	__asm mov ecx, dword ptr[lpPtr]
	__asm push ecx
	__asm call func_spawn
	__asm pop ecx
}

void disconnect ( int reason /*0=timeout, 500=quit*/ )
{
	if ( g_SAMP == NULL )
		return;

	void	*rakptr = g_SAMP->pRakClientInterface;
	__asm mov ecx, dword ptr[rakptr]
	__asm mov eax, dword ptr[ecx]
	__asm push 0
	__asm push reason
	__asm call dword ptr[eax + 8]
	__asm pop ecx
	__asm pop eax
}

void setPassword ( char *password )
{
	if ( g_SAMP == NULL )
		return;

	void	*rakptr = g_SAMP->pRakClientInterface;
	__asm mov ecx, dword ptr[rakptr]
	__asm mov eax, dword ptr[ecx]
	__asm push password
	__asm call dword ptr[eax + 16]
	__asm pop ecx
	__asm pop eax
}

#define FUNC_SENDINTERIOR	0x4B80 //0.3z R2   
void sendSetInterior ( uint8_t interiorID )
{
	if ( g_SAMP == NULL )
		return;

	uint32_t	func = g_dwSAMP_Addr + FUNC_SENDINTERIOR;
	void		*lpPtr = g_Players->pLocalPlayer;
	__asm mov ecx, dword ptr[interiorID]
	__asm push ecx
	__asm mov ecx, dword ptr[lpPtr]
	__asm call func
	__asm pop ecx
}

#define FUNC_SETSPECIALACTION	0x2C70 // 0.3z-RC3 by iMaddy (dont't change)
void setSpecialAction ( uint8_t byteSpecialAction )
{
	if ( g_SAMP == NULL )
		return;

	if ( g_Players->pLocalPlayer == NULL )
		return;

	g_Players->pLocalPlayer->onFootData.byteSpecialAction = byteSpecialAction;

	uint32_t	func = g_dwSAMP_Addr + FUNC_SETSPECIALACTION;
	void		*lpPtr = g_Players->pLocalPlayer;
	__asm mov ecx, dword ptr[byteSpecialAction]
	__asm push ecx
	__asm mov ecx, dword ptr[lpPtr]
	__asm call func
	__asm pop ecx
}

//#define FUNC_SENDSCMEVENT	0x18A0
//void sendSCMEvent ( int iEvent, int iVehicleID, int iParam1, int iParam2 )
//{
//	if ( g_SAMP == NULL )
//		return;
//
//	uint32_t	func = g_dwSAMP_Addr + FUNC_SENDSCMEVENT;
//	__asm push iParam2
//	__asm push iParam1
//	__asm push iVehicleID
//	__asm push iEvent
//	__asm call func
//}

void sendSCMEvent ( int iEvent, int iVehicleID, int iParam1, int iParam2 )
{
	g_RakClient->SendSCMEvent( iVehicleID, iEvent, iParam1, iParam2 );
}

/*
// this doesn't work when wrapped around the toggle below, samp sux
CMatrix toggleSAMPCursor_Camera = CMatrix();
void _cdecl toggleSAMPCursor_SaveCamera ( void )
{
	pGame->GetCamera()->GetMatrix(&toggleSAMPCursor_Camera);
}

void _cdecl toggleSAMPCursor_RestoreCamera ( void )
{
	pGame->GetCamera()->SetMatrix(&toggleSAMPCursor_Camera);
}
*/

#define FUNC_TOGGLECURSOR               0x98190 //0.3z R2    
#define FUNC_CURSORUNLOCKACTORCAM       0x98070 //0.3z R2 
void toggleSAMPCursor(int iToggle)
{
	if(g_Input->iInputEnabled) return;

	uint32_t	func = g_dwSAMP_Addr + FUNC_TOGGLECURSOR;

	if(iToggle)
	{
		_asm
		{
			//call toggleSAMPCursor_SaveCamera;
			mov ecx, g_Input;
			push 0;
			push 2;
			call func;
			//call toggleSAMPCursor_RestoreCamera;
		}
		g_iCursorEnabled = 1;
	}
	else
	{
		_asm
		{
			mov ecx, g_Input;
			push 1;
			push 0;
			call func;
		}
		uint32_t funcunlock = g_dwSAMP_Addr + FUNC_CURSORUNLOCKACTORCAM;
		_asm
		{
			mov ecx, g_Input;
			call funcunlock;
		}
		g_iCursorEnabled = 0;
	}
}

#define HOOK_EXIT_ANTICARJACKED_HOOK    0x1125C //0.3z R2  
uint16_t	anticarjacked_vehid;
DWORD		anticarjacked_ebx_backup;
DWORD		anticarjacked_jmp;
uint8_t _declspec ( naked ) carjacked_hook ( void )
{
	__asm mov anticarjacked_ebx_backup, ebx
	__asm mov ebx, [ebx + 7]
	__asm mov anticarjacked_vehid, bx
	__asm pushad
	cheat_state->_generic.anti_carjackTick = GetTickCount();
	cheat_state->_generic.car_jacked = true;

	if ( g_Vehicles != NULL && g_Vehicles->pGTA_Vehicle[anticarjacked_vehid] != NULL )
		vect3_copy( &g_Vehicles->pGTA_Vehicle[anticarjacked_vehid]->base.matrix[4 * 3],
					cheat_state->_generic.car_jacked_lastPos );

	__asm popad
	__asm mov ebx, g_dwSAMP_Addr
	__asm add ebx, HOOK_EXIT_ANTICARJACKED_HOOK
	__asm mov anticarjacked_jmp, ebx
	__asm xor ebx, ebx
	__asm mov ebx, anticarjacked_ebx_backup
	__asm jmp anticarjacked_jmp
}

#define HOOK_EXIT_SERVERMESSAGE_HOOK	0x62091 //0.3z R2 
int		g_iNumPlayersMuted = 0;
bool	g_bPlayerMuted[SAMP_PLAYER_MAX];
uint8_t _declspec ( naked ) server_message_hook ( void )
{
	int		thismsg;
	DWORD	thiscolor;

	__asm mov thismsg, esi
	__asm mov thiscolor, eax
	thiscolor = ( thiscolor >> 8 ) | 0xFF000000;

	static char		last_servermsg[256];
	static DWORD	allow_show_again;
	if ( !set.anti_spam || cheat_state->_generic.cheat_panic_enabled
	 || (strcmp(last_servermsg, (char *)thismsg) != NULL || GetTickCount() > allow_show_again) )
	{
		// might be a personal message by muted player - look for name in server message
		// ignore message, if name was found
		if ( set.anti_spam && g_iNumPlayersMuted > 0 )
		{
			int i, j;
			char *playerName = NULL;
			for ( i = 0, j = 0; i < SAMP_PLAYER_MAX && j < g_iNumPlayersMuted; i++ )
			{
				if ( g_bPlayerMuted[i] )
				{
					playerName = (char*)getPlayerName(i);

					if ( playerName == NULL )
					{
						// Player not connected anymore - remove player from muted list
						g_bPlayerMuted[i] = false;
						g_iNumPlayersMuted--;
						continue;
					}
					else if ( strstr((char*)thismsg, playerName) != NULL )
						goto ignoreThisServChatMsg;
					j++;
				}
			}
		}
		strncpy_s( last_servermsg, (char *)thismsg, sizeof(last_servermsg)-1 );
		addToChatWindow( (char *)thismsg, thiscolor );
		allow_show_again = GetTickCount() + 5000;
		
		if( set.chatbox_logging )
			LogChatbox( false, "%s", thismsg );
	}

ignoreThisServChatMsg:
	__asm mov ebx, g_dwSAMP_Addr
	__asm add ebx, HOOK_EXIT_SERVERMESSAGE_HOOK
	__asm jmp ebx
}

#define HOOK_EXIT_CLIENTMESSAGE_HOOK	0xDE18 //0.3z R2 
uint8_t _declspec ( naked ) client_message_hook ( void )
{
	static char last_clientmsg[SAMP_PLAYER_MAX][256];
	int			thismsg;
	uint16_t	id;

	__asm mov id, dx
	__asm lea edx, [esp+0x128]
	__asm mov thismsg, edx

	if ( id >= 0 && id <= SAMP_PLAYER_MAX )
	{
		if( id == g_Players->sLocalPlayerID )
		{
			addToChatWindow( (char*)thismsg, g_Chat->clTextColor, id );

			if( set.chatbox_logging )
				LogChatbox( false, "%s: %s", getPlayerName(id), thismsg );
			goto client_message_hook_jump_out;
		}

		static DWORD	allow_show_again = GetTickCount();
		if ( !set.anti_spam
		 ||  (strcmp(last_clientmsg[id], (char *)thismsg) != NULL || GetTickCount() > allow_show_again)
		 ||	 cheat_state->_generic.cheat_panic_enabled )
		{
			// ignore chat from muted players
			if ( set.anti_spam && g_iNumPlayersMuted > 0 && g_bPlayerMuted[id] )
				goto client_message_hook_jump_out;

			// nothing to copy anymore, after chatbox_logging, so copy this before
			strncpy_s( last_clientmsg[id], (char *)thismsg, sizeof(last_clientmsg[id])-1 );

			if( set.chatbox_logging )
				LogChatbox( false, "%s: %s", getPlayerName(id), thismsg );

			addToChatWindow( (char*)thismsg, g_Chat->clTextColor, id );
			allow_show_again = GetTickCount() + 5000;
		}
	}

client_message_hook_jump_out:;
	__asm mov ebx, g_dwSAMP_Addr
	__asm add ebx, HOOK_EXIT_CLIENTMESSAGE_HOOK
	__asm jmp ebx
}

#define HOOK_CALL_STREAMEDOUTINFO	0x987A0 //0.3z R2 
DWORD dwStreamedOutInfoOrigFunc;
float fStreamedOutInfoPosX, fStreamedOutInfoPosY, fStreamedOutInfoPosZ;
uint16_t wStreamedOutInfoPlayerID;
uint8_t _declspec ( naked ) StreamedOutInfo ( void )
{
	_asm
	{
		push eax
		mov eax, dword ptr [esp+12]
		mov fStreamedOutInfoPosX, eax
		mov eax, dword ptr [esp+16]
		mov fStreamedOutInfoPosY, eax
		mov eax, dword ptr [esp+20]
		mov fStreamedOutInfoPosZ, eax
		mov ax, word ptr [esp+24]
		mov wStreamedOutInfoPlayerID, ax
		pop eax
	}

	_asm pushad
	g_stStreamedOutInfo.iPlayerID[wStreamedOutInfoPlayerID] = (int)wStreamedOutInfoPlayerID;
	g_stStreamedOutInfo.fPlayerPos[wStreamedOutInfoPlayerID][0] = fStreamedOutInfoPosX;
	g_stStreamedOutInfo.fPlayerPos[wStreamedOutInfoPlayerID][1] = fStreamedOutInfoPosY;
	g_stStreamedOutInfo.fPlayerPos[wStreamedOutInfoPlayerID][2] = fStreamedOutInfoPosZ;
	_asm popad

	_asm
	{
		push eax
		mov eax, g_dwSAMP_Addr
		add eax, HOOK_CALL_STREAMEDOUTINFO
		mov dwStreamedOutInfoOrigFunc, eax
		pop eax

		jmp dwStreamedOutInfoOrigFunc
	}
}

void HandleRPCPacketFunc( unsigned char byteRPCID, RPCParameters *rpcParams, void ( *functionPointer ) ( RPCParameters * ) )
{
	// use this if you wanna log received RPCs (can help you with finding samp RPC-patches)
	/*if ( byteRPCID != RPC_UpdateScoresPingsIPs )
	{
		int len = rpcParams ? rpcParams->numberOfBitsOfData / 8 : 0;
		addMessageToChatWindow( "> [RPC Recv] id: %d, func offset: %p, len: %d", byteRPCID, (DWORD)functionPointer - g_dwSAMP_Addr, len );
	}*/

	if (rpcParams)
	{
		if (!cheat_state->_generic.cheat_panic_enabled)
		{
			if (g_SAMP->iGameState == GAMESTATE_CONNECTED)
			{
				if (cheat_state->stuff.check_admins == 0)
				{
					if (byteRPCID == RPC_ServerJoin)
					{
						BitStream bsData(rpcParams->input, (rpcParams->numberOfBitsOfData / 8) + 1, false);
						CHAR szPlayerName[24];
						USHORT playerId;
						BYTE byteNameLen = 0;
						memset_safe(szPlayerName, 0x0, sizeof(szPlayerName));
						bsData.Read(playerId);
						int iNPC = 0;//?
						bsData.Read(iNPC);
						BYTE bUnk = 0;
						bsData.Read(bUnk);
						bsData.Read(byteNameLen);
						if (byteNameLen > 20 || byteNameLen < 3) return;
						bsData.Read(szPlayerName, byteNameLen);
						if (!strlen(szPlayerName)) return;
						if (playerId < 0 || playerId > SAMP_PLAYER_MAX) return;
						admin_text(" -> %s[ %d ] connect", szPlayerName, playerId);
					}
					else if (byteRPCID == RPC_ServerQuit)
					{
						char szDisconnectReason[][14] =
						{
							{ "Timeout/Crash" },
							{ "Quit" },
							{ "Kick/Ban" }
						};

						BitStream bsData(rpcParams->input, (rpcParams->numberOfBitsOfData / 8) + 1, false);
						USHORT sPlayerID;
						BYTE byteReason;

						bsData.Read(sPlayerID);

						bsData.Read(byteReason);

						if (sPlayerID < 0 || sPlayerID >= SAMP_PLAYER_MAX) return;

						offadmin_text(" -> %s[ %d ] disconnect (%s)", getPlayerName(sPlayerID), sPlayerID, szDisconnectReason[byteReason]);
					}
				}
			}
		}
	}
	if ( set.enable_extra_godmode && cheat_state->_generic.hp_cheat && rpcParams )
	{
		if ( byteRPCID == RPC_ScrSetPlayerHealth )
		{
			actor_info *self = actor_info_get( ACTOR_SELF, NULL );
			if ( self )
			{
				BitStream bsData( rpcParams->input, ( rpcParams->numberOfBitsOfData / 8 ) + 1, false );
				float fHealth;
				bsData.Read( fHealth );
				if ( fHealth < self->hitpoints )
				{
					cheat_state_text( "Warning: Server tried change your health to %0.1f", fHealth );
					return; // exit from the function without processing RPC
				}
			}
		}
		else if ( byteRPCID == RPC_ScrSetVehicleHealth )
		{
			vehicle_info *vself = vehicle_info_get( VEHICLE_SELF, NULL );
			if ( vself )
			{
				BitStream bsData( rpcParams->input, ( rpcParams->numberOfBitsOfData / 8 ) + 1, false );
				short sId;
				float fHealth;
				bsData.Read( sId );
				bsData.Read( fHealth );
				if ( sId == g_Players->pLocalPlayer->sCurrentVehicleID && fHealth < vself->hitpoints )
				{
					cheat_state_text( "Warning: Server tried change your vehicle health to %0.1f", fHealth );
					return; // exit from the function without processing RPC
				}
			}
		}
	}

	functionPointer( rpcParams );
}

DWORD dwTemp1, dwTemp2;
#define SAMP_HOOKEXIT_HANDLE_RPC  0x34F93 //0.3z R2 
uint8_t _declspec ( naked ) hook_handle_rpc_packet ( void )
{
	__asm pushad;
	__asm mov dwTemp1, eax; // RPCParameters rpcParms
	__asm mov dwTemp2, edi; // RPCNode *node
	
	HandleRPCPacketFunc( *( unsigned char *)dwTemp2, (RPCParameters *)dwTemp1, *( void ( ** ) ( RPCParameters *rpcParams ) )( dwTemp2 + 1 ) );
	dwTemp1 = g_dwSAMP_Addr + SAMP_HOOKEXIT_HANDLE_RPC;

	__asm popad;
	// execute overwritten code
	__asm add esp, 4
	// exit from the custom code
	__asm jmp dwTemp1;
}

#define SAMP_HOOKEXIT_HANDLE_RPC2	0x34FA1 //0.3z R2 
uint8_t _declspec ( naked ) hook_handle_rpc_packet2 ( void )
{
	__asm pushad;
	__asm mov dwTemp1, ecx; // RPCParameters rpcParms
	__asm mov dwTemp2, edi; // RPCNode *node
	
	HandleRPCPacketFunc( *( unsigned char *)dwTemp2, (RPCParameters *)dwTemp1, *( void ( ** ) ( RPCParameters *rpcParams ) )( dwTemp2 + 1 ) );
	dwTemp1 = g_dwSAMP_Addr + SAMP_HOOKEXIT_HANDLE_RPC2;

	__asm popad;
	// exit from the custom code
	__asm jmp dwTemp1;
}

#define FUNC_CNETGAMEDESTRUCTOR                 0x8520 //0.3z R2 
void __stdcall CNetGame__destructor(void)
{
	// release hooked rakclientinterface, restore original rakclientinterface address and call CNetGame destructor
	if (g_SAMP->pRakClientInterface != NULL)
		delete g_SAMP->pRakClientInterface;
	g_SAMP->pRakClientInterface = g_RakClient->GetRakClientInterface();
	return ((void(__thiscall *) (void *)) (g_dwSAMP_Addr + FUNC_CNETGAMEDESTRUCTOR))(g_SAMP);
}


void SetupSAMPHook( char *szName, DWORD dwFuncOffset, void *Func, int iType, int iSize, char *szCompareBytes )
{
	CDetour api;
	int strl = strlen( szCompareBytes );
	uint8_t *bytes = hex_to_bin( szCompareBytes );

	if ( !strl || !bytes || memcmp_safe( (uint8_t *)g_dwSAMP_Addr + dwFuncOffset, bytes, strl / 2 ) )
	{
		if ( api.Create( (uint8_t *)( (uint32_t)g_dwSAMP_Addr ) + dwFuncOffset, (uint8_t *)Func, iType, iSize ) == 0 )
			Log( "Failed to hook %s.", szName );
	}
	else
	{
		Log("Failed to hook %s (memcmp)", szName );
	}

	if ( bytes )
		free( bytes );
}

#define SAMP_HOOKPOS_ServerMessage          0x6207A //0.3z R2  
#define SAMP_HOOKPOS_ClientMessage          0xDDBE //0.3z R2  
#define SAMP_HOOK_STATECHANGE               0x1124B //0.3z R2 
#define SAMP_HOOK_StreamedOutInfo           0xF76A //0.3z R2 
#define SAMP_HOOKENTER_HANDLE_RPC			0x34F8D //0.3z R2
#define SAMP_HOOKENTER_HANDLE_RPC2			0x34F19 //0.3z R2
#define SAMP_HOOKENTER_CNETGAME_DESTR       0xAD3D3 //0.3z R2
#define SAMP_HOOKENTER_CNETGAME_DESTR2      0xAE562 //0.3z R2

void installSAMPHooks ()
{
	if( g_SAMP == NULL )
		return;

	if ( set.anti_spam || set.chatbox_logging )
	{
		SetupSAMPHook( "ServerMessage", SAMP_HOOKPOS_ServerMessage, server_message_hook, DETOUR_TYPE_JMP, 5, "6A00C1E8" );
		SetupSAMPHook( "ClientMessage", SAMP_HOOKPOS_ClientMessage, client_message_hook, DETOUR_TYPE_JMP, 5, "663BD175" );
	}

	if ( set.anti_carjacking )
	{
		SetupSAMPHook( "AntiCarJack", SAMP_HOOK_STATECHANGE, carjacked_hook, DETOUR_TYPE_JMP, 5, "6A0568E8" );
	}

	SetupSAMPHook( "StreamedOutInfo", SAMP_HOOK_StreamedOutInfo, StreamedOutInfo, DETOUR_TYPE_CALL_FUNC, 5, "E8" );
	SetupSAMPHook( "HandleRPCPacket", SAMP_HOOKENTER_HANDLE_RPC, hook_handle_rpc_packet, DETOUR_TYPE_JMP, 6, "FF570183C404" );
	SetupSAMPHook( "HandleRPCPacket2", SAMP_HOOKENTER_HANDLE_RPC2, hook_handle_rpc_packet2, DETOUR_TYPE_JMP, 6, "FF5701E980000000" );
	//FYP's crash fix
	SetupSAMPHook("CNETGAMEDESTR1", SAMP_HOOKENTER_CNETGAME_DESTR, CNetGame__destructor, DETOUR_TYPE_CALL_FUNC, 5, "E8");
	SetupSAMPHook("CNETGAMEDESTR2", SAMP_HOOKENTER_CNETGAME_DESTR2, CNetGame__destructor, DETOUR_TYPE_CALL_FUNC, 5, "E8");
	//FYP's crash fix

}

#define SAMP_ONFOOTSENDRATE		0xE6098 // at 1000369A  MOV ECX,DWORD PTR DS:[100E6098]
#define SAMP_INCARSENDRATE		0xE609C
#define SAMP_AIMSENDRATE		0xE60A0
//didnt change since 0.3x
void setSAMPCustomSendRates ( int iOnFoot, int iInCar, int iAim, int iHeadSync )
{
	if ( !set.samp_custom_sendrates_enable )
		return;
	if ( g_dwSAMP_Addr == NULL )
		return;
	if ( g_SAMP == NULL )
		return;

	memcpy_safe( (void *)(g_dwSAMP_Addr + SAMP_ONFOOTSENDRATE), &iOnFoot, sizeof(int) );
	memcpy_safe( (void *)(g_dwSAMP_Addr + SAMP_INCARSENDRATE), &iInCar, sizeof(int) );
	memcpy_safe( (void *)(g_dwSAMP_Addr + SAMP_AIMSENDRATE), &iAim, sizeof(int) );
}

#define SAMP_DISABLE_NAMETAGS           0x6C8B0 //0.3z R2
#define SAMP_DISABLE_NAMETAGS_HP        0x6D9B0 //0.3z R2
int sampPatchDisableNameTags ( int iEnabled )
{
	static struct patch_set sampPatchEnableNameTags_patch =
	{
		"Remove player status",
		0,
		0,
		{
			{ 1, (void *)( (uint8_t *)g_dwSAMP_Addr + SAMP_DISABLE_NAMETAGS ), NULL, (uint8_t *)"\xC3", NULL },
			{ 1, (void *)( (uint8_t *)g_dwSAMP_Addr + SAMP_DISABLE_NAMETAGS_HP ), NULL, (uint8_t *)"\xC3", NULL }
		}
	};
	if ( iEnabled && !sampPatchEnableNameTags_patch.installed )
		return patcher_install( &sampPatchEnableNameTags_patch );
	else if ( !iEnabled && sampPatchEnableNameTags_patch.installed )
		return patcher_remove( &sampPatchEnableNameTags_patch );
	return NULL;
}

#define SAMP_SKIPSENDINTERIOR 0x68EA //0.3z R2
int sampPatchDisableInteriorUpdate ( int iEnabled )
{
	static struct patch_set sampPatchDisableInteriorUpdate_patch =
	{
		"NOP sendinterior",
		0,
		0,
		{
			{ 1, (void *)( (uint8_t *)g_dwSAMP_Addr + SAMP_SKIPSENDINTERIOR ), NULL, (uint8_t *)"\xEB", NULL }
		}
	};

	if ( iEnabled && !sampPatchDisableInteriorUpdate_patch.installed )
		return patcher_install( &sampPatchDisableInteriorUpdate_patch );
	else if ( !iEnabled && sampPatchDisableInteriorUpdate_patch.installed )
		return patcher_remove( &sampPatchDisableInteriorUpdate_patch );
	return NULL;
}

#define SAMP_NOPSCOREBOARDTOGGLEON			0x67FC0 //0.3z R2 
#define SAMP_NOPSCOREBOARDTOGGLEONKEYLOCK	0x68280 //0.3z R2 
int sampPatchDisableScoreboardToggleOn ( int iEnabled )
{
	static struct patch_set sampPatchDisableScoreboard_patch =
	{
		"NOP Scoreboard Functions",
		0,
		0,
		{
			{ 1, (void *)( (uint8_t *)g_dwSAMP_Addr + SAMP_NOPSCOREBOARDTOGGLEON ), NULL, (uint8_t *)"\xC3", NULL },
			{ 1, (void *)( (uint8_t *)g_dwSAMP_Addr + SAMP_NOPSCOREBOARDTOGGLEONKEYLOCK ), NULL, (uint8_t *)"\xC3", NULL }
		}
	};
	if ( iEnabled && !sampPatchDisableScoreboard_patch.installed )
		return patcher_install( &sampPatchDisableScoreboard_patch );
	else if ( !iEnabled && sampPatchDisableScoreboard_patch.installed )
		return patcher_remove( &sampPatchDisableScoreboard_patch );
	return NULL;
}

#define SAMP_CHATINPUTADJUST_Y                          0x61A86 //0.3z R2
#define SAMP_CHATINPUTADJUST_X                          0x62FA5 //0.3z R2
int sampPatchDisableChatInputAdjust ( int iEnabled )
{
	static struct patch_set sampPatchDisableChatInputAdj_patch =
	{
		"NOP Adjust Chat input box",
		0,
		0,
		{
			{ 6, (void *)( (uint8_t *)g_dwSAMP_Addr + SAMP_CHATINPUTADJUST_Y ), NULL, (uint8_t *)"\x90\x90\x90\x90\x90\x90", NULL },
			{ 7, (void *)( (uint8_t *)g_dwSAMP_Addr + SAMP_CHATINPUTADJUST_X ), NULL, (uint8_t *)"\x90\x90\x90\x90\x90\x90\x90", NULL }
		}
	};
	if ( iEnabled && !sampPatchDisableChatInputAdj_patch.installed )
		return patcher_install( &sampPatchDisableChatInputAdj_patch );
	else if ( !iEnabled && sampPatchDisableChatInputAdj_patch.installed )
		return patcher_remove( &sampPatchDisableChatInputAdj_patch );
	return NULL;
}

#define FUNC_DEATH 0x4B80 //0.3z R2
void sendDeath ( void )
{
	if ( g_SAMP == NULL )
		return;

	uint32_t func = g_dwSAMP_Addr + FUNC_DEATH;
	void  *lpPtr = g_Players->pLocalPlayer;
	__asm mov ecx, dword ptr[lpPtr]
	__asm push ecx
	__asm call func
	__asm pop ecx
}
 
#define FUNC_ENCRYPT_PORT 0x197C0 //0.3z R2
#define SAMP_ENCRYPED_PORT_OFFSET 0xFE93C //0.3z R2
void changeServer( const char *pszIp, unsigned ulPort, const char *pszPassword )
{
	if ( !g_SAMP )
		return;

	// 1st version
	( ( void ( __cdecl * )( unsigned ) )( g_dwSAMP_Addr + FUNC_ENCRYPT_PORT ) )( ulPort );

	// 2st version
	// *(unsigned short *)( g_dwSAMP_Addr + SAMP_ENCRYPED_PORT_OFFSET ) = ( ulPort + 1 ) ^ 0xCCCC; // 0x5555 -> 0xCCCC

	disconnect( 500 );
	strcpy( g_SAMP->szIP, pszIp );
	g_SAMP->ulPort = ulPort;
	setPassword( (char *)pszPassword );
	joining_server = 1;
}