// Exercise the real client resolver without loading an engine or game DLL.
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include "../privatehook.cpp"

metahook_api_t* g_pMetaHookAPI = nullptr;
cl_enginefunc_t gEngfuncs{};
int g_iEngineType = ENGINE_SVENGINE;
bool g_bIsSvenCoop = false;
bool g_bIsCounterStrike = false;
bool g_bIsDayOfDefeat = false;
extra_player_info_t (*g_PlayerExtraInfo)[65] = nullptr;
extra_player_info_czds_t (*g_PlayerExtraInfo_CZDS)[65] = nullptr;
cl_entity_t* r_worldentity = nullptr;
model_t** cl_worldmodel = nullptr;
int* cl_max_edicts = nullptr;
cl_entity_t** cl_entities = nullptr;
int* cl_numvisedicts = nullptr;
cl_entity_t** cl_visedicts = nullptr;
float* r_origin = nullptr;
bool g_bIsCreatingClCorpse = false;
int g_iCreatingClCorpsePlayerIndex = 0;

// These engine handlers are linked with privatehook.cpp but never called here.
void R_NewMap() { throw std::runtime_error("unexpected engine hook"); }
void R_RenderView() { throw std::runtime_error("unexpected engine hook"); }
void R_RenderView_SvEngine(int) { throw std::runtime_error("unexpected engine hook"); }
void _SpewInfo(SpewType_t, const char*, int) { throw std::runtime_error("SDK assertion"); }
SpewRetval_t _SpewMessage(const char*, ...) { throw std::runtime_error("SDK assertion"); }
void _ExitOnFatalAssert(const char*, int) { throw std::runtime_error("SDK assertion"); }
bool ShouldUseNewAssertDialog() { return false; }
bool DoNewAssertDialog(const char*, int, const char*) { throw std::runtime_error("SDK assertion"); }

static int s_ClientIdentity{};
static int s_User1{}, s_User2{}, s_ViewEntity{};
static bool s_RenderingPortals{};
static pitchdrift_t s_PitchDrift{};
static bool s_HasViewEntity = false;
static const char* s_MissingSymbol = nullptr;
static std::string s_Error;

static mh_gamesymbol_status_t ResolveSymbol(PVOID module, const char* symbol, mh_gamesymbol_kind_t kind, PVOID* address)
{
	assert(&s_ClientIdentity == module);
	assert(MH_GAMESYMBOL_KIND_GLOBAL == kind);
	*address = nullptr;
	if (s_MissingSymbol && 0 == strcmp(s_MissingSymbol, symbol))
		return MH_GAMESYMBOL_SYMBOL_NOT_FOUND;
	if (0 == strcmp("g_iUser1", symbol)) *address = &s_User1;
	else if (0 == strcmp("g_iUser2", symbol)) *address = &s_User2;
	else if (0 == strcmp("g_bRenderingPortals_SCClient", symbol)) *address = &s_RenderingPortals;
	else if (0 == strcmp("g_pitchdrift", symbol)) *address = &s_PitchDrift;
	else if (0 == strcmp("g_ViewEntityIndex_SCClient", symbol))
	{
		if (!s_HasViewEntity) return MH_GAMESYMBOL_SYMBOL_NOT_FOUND;
		*address = &s_ViewEntity;
	}
	else throw std::runtime_error("unexpected symbol");
	return MH_GAMESYMBOL_OK;
}

static IBaseInterface* ClientFactory(const char* name, int*)
{
	assert(0 == strcmp("SCClientDLL001", name));
	return reinterpret_cast<IBaseInterface*>(&s_ClientIdentity);
}

static CreateInterfaceFn GetClientFactory() { return ClientFactory; }
static const char* GetGameDirectory() { return "svencoop"; }
static const char* StatusString(mh_gamesymbol_status_t) { return "symbol not found"; }
static void ReportError(const char* format, ...)
{
	char message[512];
	va_list arguments;
	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	s_Error = message;
	throw std::runtime_error(message);
}

static void CheckResolvedGlobals()
{
	assert(g_bIsSvenCoop);
	assert(&s_User1 == g_iUser1);
	assert(&s_User2 == g_iUser2);
	assert(&s_RenderingPortals == g_bRenderingPortals_SCClient);
	assert(&s_PitchDrift == g_pitchdrift);
}

int main()
{
	metahook_api_t api{};
	api.ResolveGameSymbol = ResolveSymbol;
	api.GetClientFactory = GetClientFactory;
	api.GetGameSymbolStatusString = StatusString;
	api.SysError = ReportError;
	g_pMetaHookAPI = &api;
	gEngfuncs.pfnGetGameDirectory = GetGameDirectory;

	// 10257 publishes the view slot. Resolving it preserves the actual address.
	s_HasViewEntity = true;
	Client_FillAddress(&s_ClientIdentity);
	CheckResolvedGlobals();
	assert(&s_ViewEntity == g_ViewEntityIndex_SCClient);
	assert(s_Error.empty());

	// 8948 has no slot. Also clear any address left from the preceding client.
	s_HasViewEntity = false;
	Client_FillAddress(&s_ClientIdentity);
	CheckResolvedGlobals();
	assert(nullptr == g_ViewEntityIndex_SCClient);
	assert(s_Error.empty());

	// Absence of the optional slot must not weaken the other required symbols.
	for (const char* symbol : { "g_iUser1", "g_iUser2", "g_bRenderingPortals_SCClient", "g_pitchdrift" })
	{
		s_MissingSymbol = symbol;
		s_Error.clear();
		bool failed = false;
		try { Client_FillAddress(&s_ClientIdentity); }
		catch (const std::runtime_error&) { failed = true; }
		assert(failed);
		assert(std::string::npos != s_Error.find(symbol));
		assert(std::string::npos != s_Error.find("module client"));
	}
	puts("client gamedata regression tests passed");
	return 0;
}
