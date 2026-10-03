#pragma once

#include <cmath>
#include <filesystem>
#include <memory>
#include <string>

#include "ClientPhysicManager.h"

// Installs the services that the engine and vgui2.dll normally provide to the plugin and
// creates the Bullet physics manager. Collision meshes resolve against gameDirectory.
void TestInitPluginRuntime(const std::filesystem::path& gameDirectory = {});
void TestShutdownPluginRuntime();

// Last line printed through gEngfuncs.Con_Printf or gEngfuncs.Con_DPrintf.
const std::string& TestGetLastConsoleMessage();
void TestClearConsoleMessage();

// A model without studio data, so configuration integrity checks are skipped.
model_t* TestGetModelWithoutStudioData();

// The on-disk *_physics.txt text written by bv_save_configs and read by the loader.
std::string TestSavePhysicObjectConfigToText(const CClientPhysicObjectConfig* pPhysicObjectConfig);
std::shared_ptr<CClientPhysicObjectConfig> TestLoadPhysicObjectConfigFromText(const std::string& text, model_t* mod);

std::string TestReadTextFile(const std::filesystem::path& path);

// Defined in BasePhysicManager.cpp without a header declaration.
std::shared_ptr<CClientPhysicObjectConfig> LoadPhysicObjectConfigFromLegacyFileBuffer(const char* buf);

inline bool IsNear(float expected, float actual, float tolerance = 1e-4f)
{
	return std::fabs(expected - actual) <= tolerance;
}

inline bool IsNearVector(const float* expected, const float* actual, float tolerance = 1e-4f)
{
	return IsNear(expected[0], actual[0], tolerance) && IsNear(expected[1], actual[1], tolerance) && IsNear(expected[2], actual[2], tolerance);
}

// Factors use NaN for "not provided", which the Bullet backend replaces with defaults.
inline bool IsSameFactor(float expected, float actual)
{
	return (std::isnan(expected) && std::isnan(actual)) || IsNear(expected, actual);
}
