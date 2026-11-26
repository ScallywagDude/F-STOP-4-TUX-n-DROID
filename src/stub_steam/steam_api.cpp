#define _CRT_SECURE_NO_WARNINGS
#define STEAM_API_EXPORTS

#if defined __GNUC__
 #define S_API extern "C" __attribute__ ((visibility("default"))) 
#elif defined _MSC_VER
#define S_API extern "C" __declspec(dllexport)
#endif
#define NULL 0

S_API void *g_pSteamClientGameServer;
void *g_pSteamClientGameServer = NULL;

//steam_api.h
S_API bool SteamAPI_Init() {
	return true;
}

S_API bool SteamAPI_InitSafe() {
	return true;
}

S_API void SteamAPI_Shutdown() {

}

S_API bool SteamAPI_RestartAppIfNecessary() {
	return false;
}

S_API void SteamAPI_ReleaseCurrentThreadMemory() {

}

S_API void SteamAPI_WriteMiniDump() {

}

S_API void SteamAPI_SetMiniDumpComment() {

}

S_API void SteamAPI_RunCallbacks() {
}

S_API void SteamAPI_RegisterCallback() {

}

S_API void SteamAPI_UnregisterCallback() {

}

S_API void SteamAPI_RegisterCallResult() {

}

S_API void SteamAPI_UnregisterCallResult() {

}

S_API bool SteamAPI_IsSteamRunning() {
	return false;
}

S_API void Steam_RunCallbacks() {

}

S_API void Steam_RegisterInterfaceFuncs() {
}

S_API int Steam_GetHSteamUserCurrent() {
	return 0;
}

S_API const char *SteamAPI_GetSteamInstallPath() {
	return NULL;
}

S_API int SteamAPI_GetHSteamPipe() {
	return 0;
}

S_API void SteamAPI_SetTryCatchCallbacks() {

}

S_API void SteamAPI_SetBreakpadAppID() {

}

S_API void SteamAPI_UseBreakpadCrashHandler() {

}

S_API int GetHSteamPipe() {
	return 0;
}

S_API int GetHSteamUser() {
	return 0;
}

S_API int SteamAPI_GetHSteamUser() {
	return 0;
}

S_API void *SteamInternal_ContextInit() {
	return NULL;
}

S_API void *SteamInternal_CreateInterface() {
	return NULL;
}

S_API void *SteamApps() {
	return NULL;
}

S_API void *SteamClient() {
	return NULL;
}

S_API void *SteamFriends() {
	return NULL;
}

S_API void *SteamHTTP() {
	return NULL;
}

S_API void *SteamMatchmaking() {
	return NULL;
}

S_API void *SteamMatchmakingServers() {
	return NULL;
}

S_API void *SteamNetworking() {
	return NULL;
}

S_API void *SteamRemoteStorage() {
	return NULL;
}

S_API void *SteamScreenshots() {
	return NULL;
}

S_API void *SteamUser() {
	return NULL;
}

S_API void *SteamUserStats() {
	return NULL;
}

S_API void *SteamUtils() {
	return NULL;
}

S_API int SteamGameServer_GetHSteamPipe() {
	return 0;
}

S_API int SteamGameServer_GetHSteamUser() {
	return 0;
}

S_API int SteamGameServer_GetIPCCallCount() {
	return 0;
}

S_API int SteamGameServer_InitSafe() {
	return 0;
}

S_API void SteamGameServer_RunCallbacks() {
}

S_API void SteamGameServer_Shutdown() {
}

// Steam game-server and networking-sockets entry points the partner engine links against.
// There is no Steam networking in this build: every interface is absent and every init reports failure,
// so the engine stays on its own UDP sockets.
S_API bool SteamInternal_GameServer_Init() {
	return false;
}

S_API void *SteamNetworkingSockets() {
	return NULL;
}

S_API void *SteamNetworkingSocketsGameServer() {
	return NULL;
}

S_API void *SteamNetworkingUtils() {
	return NULL;
}

S_API void SteamDatagramClient_SetPartner() {
}

S_API void SteamDatagramClient_Internal_SteamAPIKludge() {
}

S_API bool SteamDatagramClient_Init_InternalV6( char *errMsg ) {
	const char *msg = "Steam networking is not part of this build";
	if ( errMsg )
	{
		int i = 0;
		for ( ; msg[i]; i++ )
			errMsg[i] = msg[i];
		errMsg[i] = 0;
	}
	return false;
}

S_API bool SteamDatagramServer_Init_Internal( char *errMsg ) {
	return SteamDatagramClient_Init_InternalV6( errMsg );
}

S_API void SteamDatagramClient_Kill() {
}

S_API void SteamDatagramServer_Kill() {
}

S_API void SteamNetworkingSockets_SetDebugOutputFunction() {
}
