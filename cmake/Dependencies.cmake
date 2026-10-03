set(BULLETPHYSICS_DEPENDENCY_CACHE_DIR "${PROJECT_SOURCE_DIR}/thirdparty/cache" CACHE PATH "Downloaded dependency cache")
set(VC_LTL_Root "${BULLETPHYSICS_DEPENDENCY_CACHE_DIR}/VC-LTL-5.3.1" CACHE PATH "VC-LTL binary package root")
set(METAHOOK_SOURCE_PATH "$ENV{METAHOOK_SOURCE_PATH}" CACHE PATH "MetaHook source tree; empty fetches the pinned SDK")
set(VGUI2EXTENSION_SOURCE_PATH "$ENV{VGUI2EXTENSION_SOURCE_PATH}" CACHE PATH "VGUI2Extension source tree providing public interface headers; empty fetches the pinned commit")
set(GLEW_SOURCE_PATH "$ENV{GLEW_SOURCE_PATH}" CACHE PATH "glew-cmake source tree providing libglew_static; empty fetches the pinned commit")
set(BULLET3_SOURCE_PATH "$ENV{BULLET3_SOURCE_PATH}" CACHE PATH "Bullet3 source tree; empty fetches the pinned fork")
set(SCOPEEXIT_SOURCE_PATH "$ENV{SCOPEEXIT_SOURCE_PATH}" CACHE PATH "ScopeExit source tree; empty fetches the pinned commit")
set(TINYOBJLOADER_SOURCE_PATH "$ENV{TINYOBJLOADER_SOURCE_PATH}" CACHE PATH "tinyobjloader source tree; empty fetches the pinned commit")
set(CHOCOBO1HASH_SOURCE_PATH "$ENV{CHOCOBO1HASH_SOURCE_PATH}" CACHE PATH "Chocobo1Hash source tree; empty fetches the pinned commit")

function(bulletphysics_fetch_source name url tag out_var)
    include(FetchContent)
    FetchContent_Populate(${name}
        GIT_REPOSITORY "${url}"
        GIT_TAG "${tag}"
        GIT_SUBMODULES ""
        GIT_SUBMODULES_RECURSE FALSE
        SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/${name}-src")
    string(TOLOWER "${name}" name_lower)
    set(${out_var} "${${name_lower}_SOURCE_DIR}" PARENT_SCOPE)
endfunction()

function(bulletphysics_validate_source variable source)
    foreach(required IN LISTS ARGN)
        if(NOT EXISTS "${source}/${required}" OR IS_DIRECTORY "${source}/${required}")
            message(FATAL_ERROR "${variable} is missing ${required}: ${source}")
        endif()
    endforeach()
endfunction()

function(bulletphysics_prepare_dependencies)
    set(metahook_headers include/metahook.h include/HLSDK/common/interface.cpp
        include/SourceSDK/filesystem.cpp include/vgui_controls/Panel.cpp include/Interface/VGUI/IPanel2.h)
    set(vgui2extension_headers include/Interface/IVGUI2Extension.h include/Interface/IDpiManager.h
        include/Interface/VGUI/IInput2.h include/Interface/VGUI/IScheme2.h include/Interface/VGUI/ISurface2.h)
    set(glew_headers CMakeLists.txt include/GL/glew.h)
    set(bullet3_headers CMakeLists.txt src/btBulletDynamicsCommon.h src/LinearMath/btScalar.h)
    # Validate all explicit paths before downloads. External sources are read-only inputs.
    foreach(dependency METAHOOK VGUI2EXTENSION GLEW BULLET3)
        string(TOLOWER "${dependency}" lower)
        if(${dependency}_SOURCE_PATH)
            get_filename_component(${lower}_source "${${dependency}_SOURCE_PATH}" ABSOLUTE BASE_DIR "${PROJECT_SOURCE_DIR}")
            bulletphysics_validate_source(${dependency}_SOURCE_PATH "${${lower}_source}" ${${lower}_headers})
        endif()
    endforeach()

    if(NOT METAHOOK_SOURCE_PATH)
        include(FetchContent)
        FetchContent_Declare(bulletphysics_metahook
            GIT_REPOSITORY https://github.com/MetaHookSv/MetaHook
            GIT_TAG 4d23b6fecd79dc949aabc2e145480cd1328d4a35
            GIT_SUBMODULES ""
            GIT_SUBMODULES_RECURSE FALSE
            SOURCE_SUBDIR include)
        FetchContent_MakeAvailable(bulletphysics_metahook)
        set(metahook_source "${bulletphysics_metahook_SOURCE_DIR}")
    endif()
    if(NOT VGUI2EXTENSION_SOURCE_PATH)
        bulletphysics_fetch_source(bulletphysics_vgui2extension
            "https://github.com/MetaHookSv/VGUI2Extension"
            "cd7ef6e3b7fb51d3c98e6d7dadec02dd1fa08c4f" vgui2extension_source)
    endif()
    if(NOT GLEW_SOURCE_PATH)
        bulletphysics_fetch_source(bulletphysics_glew
            "https://github.com/hzqst/glew-cmake"
            "56ed32d4a929f993f0e6b7f905af9be4d38fda04" glew_source)
    endif()
    if(NOT BULLET3_SOURCE_PATH)
        bulletphysics_fetch_source(bulletphysics_bullet3
            "https://github.com/hzqst/bullet3"
            "1ece383aeb5a148533ad5c0cd829cd10d38117c3" bullet3_source)
    endif()
    foreach(dependency METAHOOK VGUI2EXTENSION GLEW BULLET3)
        string(TOLOWER "${dependency}" lower)
        bulletphysics_validate_source(${dependency}_SOURCE_PATH "${${lower}_source}" ${${lower}_headers})
        set(${dependency}_SOURCE_PATH "${${lower}_source}" PARENT_SCOPE)
        message(STATUS "${dependency}_SOURCE_PATH: ${${lower}_source}")
    endforeach()
    set(BULLETPHYSICS_GLEW_INCLUDE_DIRS "${glew_source}/include" "${glew_source}/include/GL" PARENT_SCOPE)
    set(BULLETPHYSICS_BULLET3_INCLUDE_DIRS "${bullet3_source}/src" PARENT_SCOPE)
    # ScopeExit: shared external tree, otherwise fetch the pinned commit.
    if(SCOPEEXIT_SOURCE_PATH)
        get_filename_component(scopeexit_source "${SCOPEEXIT_SOURCE_PATH}" ABSOLUTE BASE_DIR "${PROJECT_SOURCE_DIR}")
    else()
        bulletphysics_fetch_source(bulletphysics_scopeexit
            "https://github.com/SergiusTheBest/ScopeExit"
            "bd345da594a4675d04de663d93d00cb81b6678b2" scopeexit_source)
    endif()
    bulletphysics_validate_source(SCOPEEXIT_SOURCE_PATH "${scopeexit_source}" include/ScopeExit/ScopeExit.h)
    set(BULLETPHYSICS_SCOPEEXIT_INCLUDE_DIRS "${scopeexit_source}/include" PARENT_SCOPE)
    set(SCOPEEXIT_SOURCE_PATH "${scopeexit_source}" PARENT_SCOPE)
    message(STATUS "SCOPEEXIT_SOURCE_PATH: ${scopeexit_source}")
    # tinyobjloader: shared external tree, otherwise fetch the pinned commit.
    if(TINYOBJLOADER_SOURCE_PATH)
        get_filename_component(tinyobjloader_source "${TINYOBJLOADER_SOURCE_PATH}" ABSOLUTE BASE_DIR "${PROJECT_SOURCE_DIR}")
    else()
        bulletphysics_fetch_source(bulletphysics_tinyobjloader
            "https://github.com/hzqst/tinyobjloader"
            "cab4ad7254cbf7eaaafdb73d272f99e92f166df8" tinyobjloader_source)
    endif()
    bulletphysics_validate_source(TINYOBJLOADER_SOURCE_PATH "${tinyobjloader_source}" tiny_obj_loader.cc tiny_obj_loader.h)
    set(BULLETPHYSICS_TINYOBJLOADER_INCLUDE_DIRS "${tinyobjloader_source}" PARENT_SCOPE)
    set(TINYOBJLOADER_SOURCE_PATH "${tinyobjloader_source}" PARENT_SCOPE)
    message(STATUS "TINYOBJLOADER_SOURCE_PATH: ${tinyobjloader_source}")
    # Chocobo1Hash: shared external tree, otherwise fetch the pinned commit.
    if(CHOCOBO1HASH_SOURCE_PATH)
        get_filename_component(chocobo1hash_source "${CHOCOBO1HASH_SOURCE_PATH}" ABSOLUTE BASE_DIR "${PROJECT_SOURCE_DIR}")
    else()
        bulletphysics_fetch_source(bulletphysics_chocobo1hash
            "https://github.com/hzqst/Chocobo1Hash"
            "f455b0e350dce4c3b2415bad5f10484842b0a605" chocobo1hash_source)
    endif()
    bulletphysics_validate_source(CHOCOBO1HASH_SOURCE_PATH "${chocobo1hash_source}" src/crc_32.h)
    set(BULLETPHYSICS_CHOCOBO1HASH_INCLUDE_DIRS "${chocobo1hash_source}/src" PARENT_SCOPE)
    set(CHOCOBO1HASH_SOURCE_PATH "${chocobo1hash_source}" PARENT_SCOPE)
    message(STATUS "CHOCOBO1HASH_SOURCE_PATH: ${chocobo1hash_source}")
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/VCLTL.cmake")
    bulletphysics_prepare_vcltl()
endfunction()
