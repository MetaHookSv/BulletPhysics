// Host services for the regression tests and access to the configuration serializers that
// BasePhysicManager.cpp keeps file-local. The rest of the plugin is linked from its own objects.
#include <metahook.h>
// SourceSDK math headers must precede the macros from mathlib2.h.
#include <IKeyValuesSystem.h>
#include <utlbuffer.h>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

// Collision meshes are read from the test game directory instead of the engine filesystem.
static FileHandle_t TestFileOpen(const char* path, const char* mode);
static unsigned int TestFileSize(FileHandle_t file);
static int          TestFileRead(void* output, int size, FileHandle_t file);
static void         TestFileClose(FileHandle_t file);
#undef FILESYSTEM_ANY_OPEN
#undef FILESYSTEM_ANY_SIZE
#undef FILESYSTEM_ANY_READ
#undef FILESYSTEM_ANY_CLOSE
#define FILESYSTEM_ANY_OPEN(...)  TestFileOpen(__VA_ARGS__)
#define FILESYSTEM_ANY_SIZE(...)  TestFileSize(__VA_ARGS__)
#define FILESYSTEM_ANY_READ(...)  TestFileRead(__VA_ARGS__)
#define FILESYSTEM_ANY_CLOSE(...) TestFileClose(__VA_ARGS__)
#include "../src/BasePhysicManager.cpp"

#include "test_support.h"

extern IKeyValuesSystem* g_pKeyValuesSystem;

static std::filesystem::path s_GameDirectory;
static std::string           s_LastConsoleMessage;
static model_t               s_ModelWithoutStudioData{};

static FileHandle_t TestFileOpen(const char* path, const char* mode)
{
    assert(0 == strcmp("rb", mode));
    std::ifstream file(s_GameDirectory / path, std::ios::binary);
    if (!file)
        return nullptr;
    return new std::vector<char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

static unsigned int TestFileSize(FileHandle_t file)
{
    return (unsigned int)static_cast<std::vector<char>*>(file)->size();
}

static int TestFileRead(void* output, int size, FileHandle_t file)
{
    const auto& content = *static_cast<std::vector<char>*>(file);
    const int   count   = (std::min)(size, (int)content.size());
    memcpy(output, content.data(), count);
    return count;
}

static void TestFileClose(FileHandle_t file)
{
    delete static_cast<std::vector<char>*>(file);
}

// vgui2.dll resolves key names case-insensitively and keeps the first spelling it saw.
class CTestKeyValuesSystem : public IKeyValuesSystem
{
public:
    void RegisterSizeofKeyValues(int size) override
    {
    }

    void* AllocKeyValuesMemory(int size) override
    {
        return malloc(size);
    }

    void FreeKeyValuesMemory(void* pMem) override
    {
        free(pMem);
    }

    HKeySymbol GetSymbolForString(const char* name) override
    {
        if (!name)
            return INVALID_KEY_SYMBOL;

        std::string key = name;
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return (char)std::tolower(c); });

        auto [itor, inserted] = m_symbols.try_emplace(key, (HKeySymbol)m_names.size());
        if (inserted)
            m_names.emplace_back(name);

        return itor->second;
    }

    const char* GetStringForSymbol(HKeySymbol symbol) override
    {
        if (symbol < 0 || symbol >= (HKeySymbol)m_names.size())
            return "";

        return m_names[symbol].c_str();
    }

    void GetLocalizedFromANSI(const char* ansi, wchar_t* outBuf, int unicodeBufferSizeInBytes) override
    {
        throw std::logic_error("unexpected KeyValues localization");
    }

    void GetANSIFromLocalized(const wchar_t* wchar, char* outBuf, int ansiBufferSizeInBytes) override
    {
        throw std::logic_error("unexpected KeyValues localization");
    }

    void AddKeyValuesToMemoryLeakList(void* pMem, HKeySymbol name) override
    {
    }

    void RemoveKeyValuesFromMemoryLeakList(void* pMem) override
    {
    }

private:
    std::unordered_map<std::string, HKeySymbol> m_symbols;
    // A deque keeps the returned c_str pointers stable.
    std::deque<std::string> m_names;
};

static CTestKeyValuesSystem s_KeyValuesSystem;

static void TestConsolePrint(const char* format, ...)
{
    char    message[1024];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    s_LastConsoleMessage = message;
}

void TestInitPluginRuntime(const std::filesystem::path& gameDirectory)
{
    assert(!g_pClientPhysicManager);
    s_GameDirectory               = gameDirectory;
    s_ModelWithoutStudioData.type = mod_brush;
    gEngfuncs.Con_Printf          = TestConsolePrint;
    gEngfuncs.Con_DPrintf         = TestConsolePrint;
    g_pKeyValuesSystem            = &s_KeyValuesSystem;
    g_pClientPhysicManager        = BulletPhysicManager_CreateInstance();
}

void TestShutdownPluginRuntime()
{
    // Configurations unregister themselves from the manager, so all of them must be gone.
    g_pClientPhysicManager->Destroy();
    g_pClientPhysicManager = nullptr;
}

const std::string& TestGetLastConsoleMessage()
{
    return s_LastConsoleMessage;
}

void TestClearConsoleMessage()
{
    s_LastConsoleMessage.clear();
}

model_t* TestGetModelWithoutStudioData()
{
    return &s_ModelWithoutStudioData;
}

std::string TestSavePhysicObjectConfigToText(const CClientPhysicObjectConfig* pPhysicObjectConfig)
{
    auto pKeyValues = ConvertPhysicObjectConfigToKeyValues(pPhysicObjectConfig);
    assert(pKeyValues);
    SCOPE_EXIT { delete pKeyValues; };

    // KeyValues::SaveToFile writes the same text through the engine filesystem.
    CUtlBuffer buffer;
    pKeyValues->RecursiveSaveToFile(buffer, 0);
    return std::string((const char*)buffer.Base(), buffer.TellPut());
}

std::shared_ptr<CClientPhysicObjectConfig> TestLoadPhysicObjectConfigFromText(const std::string& text, model_t* mod)
{
    auto pKeyValues = new KeyValues("PhysicObjectConfig");
    SCOPE_EXIT { delete pKeyValues; };

    if (!pKeyValues->LoadFromBuffer("test_physics.txt", text.c_str()))
        return nullptr;

    return LoadPhysicObjectConfigFromKeyValues(mod, pKeyValues);
}

std::string TestReadTextFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    assert(file);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}
