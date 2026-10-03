cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(BulletPhysics LANGUAGES C CXX RC)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "BulletPhysics requires Windows MSVC x86. Configure with -A Win32.")
endif()
get_property(multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(multi_config)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported configurations" FORCE)
elseif(NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release)$")
    message(FATAL_ERROR "Select Debug or Release.")
endif()

include(cmake/Dependencies.cmake)
bulletphysics_prepare_dependencies()
include("${VC_LTL_Root}/VC-LTL helper for cmake.cmake")

# Vendor projects inherit the parent's VC-LTL integration.
set(USE_VCLTL OFF CACHE BOOL "Separate vendor VC-LTL integration" FORCE)
set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(glew-cmake_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ONLY_LIBS ON CACHE BOOL "" FORCE)
add_subdirectory("${GLEW_SOURCE_PATH}" "${CMAKE_CURRENT_BINARY_DIR}/thirdparty/GLEW" EXCLUDE_FROM_ALL)
if(NOT TARGET libglew_static)
    message(FATAL_ERROR "GLEW_SOURCE_PATH must provide the libglew_static CMake target.")
endif()

# Keep the fork's single-precision static libraries and suppress its SDK install.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
set(BUILD_BULLET3 ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(USE_DOUBLE_PRECISION OFF CACHE BOOL "" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
foreach(option BUILD_EXTRAS BUILD_BULLET2_DEMOS BUILD_OPENGL3_DEMOS BUILD_CPU_DEMOS
    BUILD_PYBULLET BUILD_UNIT_TESTS USE_GLUT INSTALL_LIBS INSTALL_CMAKE_FILES)
    set(${option} OFF CACHE BOOL "" FORCE)
endforeach()
add_subdirectory("${BULLET3_SOURCE_PATH}" "${CMAKE_CURRENT_BINARY_DIR}/thirdparty/Bullet3" EXCLUDE_FROM_ALL)
set(bullet_libraries Bullet3Collision Bullet3Common Bullet3Dynamics Bullet3Geometry
    BulletCollision BulletDynamics LinearMath)
foreach(library IN LISTS bullet_libraries)
    if(NOT TARGET ${library})
        message(FATAL_ERROR "BULLET3_SOURCE_PATH must provide the ${library} CMake target.")
    endif()
endforeach()

add_library(bulletphysics_settings INTERFACE)
target_compile_features(bulletphysics_settings INTERFACE cxx_std_20)
target_include_directories(bulletphysics_settings INTERFACE
    src
    "${VGUI2EXTENSION_SOURCE_PATH}/include/Interface"
    "${VGUI2EXTENSION_SOURCE_PATH}/include/Interface/VGUI"
    "${METAHOOK_SOURCE_PATH}/include"
    "${METAHOOK_SOURCE_PATH}/include/Interface"
    "${METAHOOK_SOURCE_PATH}/include/Interface/VGUI"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/cl_dll"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/common"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/engine"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/pm_shared"
    "${METAHOOK_SOURCE_PATH}/include/HLSDK/public"
    "${METAHOOK_SOURCE_PATH}/include/VGUI"
    "${METAHOOK_SOURCE_PATH}/include/vgui_controls"
    "${METAHOOK_SOURCE_PATH}/include/SourceSDK"
    "${METAHOOK_SOURCE_PATH}/include/SourceSDK/tier0"
    "${METAHOOK_SOURCE_PATH}/include/SourceSDK/tier1"
    "${METAHOOK_SOURCE_PATH}/include/SourceSDK/vstdlib"
    ${BULLETPHYSICS_GLEW_INCLUDE_DIRS}
    ${BULLETPHYSICS_BULLET3_INCLUDE_DIRS}
    thirdparty/ScopeExit/include
    thirdparty/tinyobjloader
    thirdparty/Chocobo1Hash/src
)
target_compile_definitions(bulletphysics_settings INTERFACE WIN32 _WINDOWS _USRDLL _MBCS
    GLEW_STATIC VGUI_USE_SURFACE2 VGUI_USE_SCHEME2 VGUI_USE_INPUT2
    NO_MALLOC_OVERRIDE NO_TIER0_MINIDUMP NO_VCR)
target_compile_options(bulletphysics_settings INTERFACE /permissive /W3
    /wd4311 /wd4312 /wd4819 /wd4996
    $<$<CONFIG:Release>:/wd4244;/wd4267;/wd4305>)

include(cmake/Sources.cmake)
add_library(BulletPhysics SHARED ${BULLETPHYSICS_SOURCES})
target_link_libraries(BulletPhysics PRIVATE bulletphysics_settings libglew_static ${bullet_libraries} opengl32)
target_compile_definitions(BulletPhysics PRIVATE $<$<CONFIG:Debug>:_DEBUG>)
target_compile_options(BulletPhysics PRIVATE /MP /Zi $<$<CONFIG:Debug>:/arch:SSE2>)
target_link_options(BulletPhysics PRIVATE /DEBUG
    $<$<CONFIG:Debug>:/INCREMENTAL>
    $<$<CONFIG:Release>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/LARGEADDRESSAWARE>)
# Bullet's CMake project sets a global debug postfix; the plugin keeps its loader name.
set_target_properties(BulletPhysics PROPERTIES DEBUG_POSTFIX "" INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

install(TARGETS BulletPhysics RUNTIME DESTINATION svencoop/metahook/plugins)
install(FILES "$<TARGET_PDB_FILE:BulletPhysics>" DESTINATION svencoop/metahook/plugins)
install(DIRECTORY assets/svencoop/bulletphysics DESTINATION svencoop)
install(DIRECTORY assets/svencoop_downloads DESTINATION .)

option(BULLETPHYSICS_SYNC_GAMEDATA "Download and prune BulletPhysics's gamedata catalog before building" ON)
set(BULLETPHYSICS_GAMEDATA_DIR "${CMAKE_BINARY_DIR}/assets/svencoop/metahook/gamedata/bulletphysics" CACHE PATH "BulletPhysics gamedata catalog directory")
if(BULLETPHYSICS_SYNC_GAMEDATA)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_custom_target(BulletPhysicsGameData
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/sync-gamedata.py"
            --manifest "${CMAKE_CURRENT_SOURCE_DIR}/scripts/manifests/bulletphysics.json"
            --target-dir "${BULLETPHYSICS_GAMEDATA_DIR}" --temp-root "${CMAKE_BINARY_DIR}/gamedata-sync"
        COMMENT "Synchronizing and validating BulletPhysics gamedata"
        VERBATIM
    )
    add_custom_target(BulletPhysicsGameDataValidate
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/validate-gamedata.py"
            "${BULLETPHYSICS_GAMEDATA_DIR}" --manifest "${CMAKE_CURRENT_SOURCE_DIR}/scripts/manifests/bulletphysics.json"
        COMMENT "Validating BulletPhysics gamedata against the manifest"
        VERBATIM
    )
    add_dependencies(BulletPhysicsGameDataValidate BulletPhysicsGameData)
    add_dependencies(BulletPhysics BulletPhysicsGameDataValidate)
endif()
install(DIRECTORY "${BULLETPHYSICS_GAMEDATA_DIR}/" DESTINATION svencoop/metahook/gamedata/bulletphysics OPTIONAL)

option(BULLETPHYSICS_BUILD_TESTS "Build the BulletPhysics regression tests" OFF)
if(BULLETPHYSICS_BUILD_TESTS)
    enable_testing()
    # Tests link the plugin's own objects instead of stubbing its internals. test_support.cpp
    # recompiles BasePhysicManager.cpp to reach its file-local configuration serializers.
    add_library(bulletphysics_test_support STATIC tests/test_support.cpp
        "$<FILTER:$<TARGET_OBJECTS:BulletPhysics>,EXCLUDE,/BasePhysicManager\\.obj$>")
    target_link_libraries(bulletphysics_test_support PUBLIC bulletphysics_settings libglew_static ${bullet_libraries} opengl32)
    # Match the plugin objects and keep assertions enabled in Release.
    target_compile_definitions(bulletphysics_test_support PUBLIC $<$<CONFIG:Debug>:_DEBUG>)
    target_compile_options(bulletphysics_test_support PUBLIC /UNDEBUG)
    # Release plugin objects carry /GL code.
    set_target_properties(bulletphysics_test_support PROPERTIES STATIC_LIBRARY_OPTIONS "$<$<CONFIG:Release>:/LTCG>")
    foreach(test physic_config_tests shipped_physic_assets_tests bullet_backend_tests)
        add_executable(${test} "tests/${test}.cpp")
        target_link_libraries(${test} PRIVATE bulletphysics_test_support)
        target_link_options(${test} PRIVATE /INCREMENTAL:NO $<$<CONFIG:Release>:/LTCG>)
    endforeach()
    add_test(NAME physic_config_tests COMMAND physic_config_tests)
    add_test(NAME shipped_physic_assets_tests COMMAND shipped_physic_assets_tests "${CMAKE_CURRENT_SOURCE_DIR}/assets/svencoop_downloads")
    add_test(NAME bullet_backend_tests COMMAND bullet_backend_tests)
endif()
