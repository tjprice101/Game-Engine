cmake_minimum_required(VERSION 3.20)
project(GameEngine VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output to bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

# ------------------------------------------------------------------ GLFW ----
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.10
    GIT_SHALLOW    TRUE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ------------------------------------------------------------------- GLM ----
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(glm)

# ------------------------------------------------------------------ GLAD2 ---
FetchContent_Declare(glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v2.0.6
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  cmake)
FetchContent_MakeAvailable(glad)
glad_add_library(glad_gl_core_45 REPRODUCIBLE LOADER API gl:core=4.5)

# ------------------------------------------------------------------- STB ----
FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(stb)

# --------------------------------------------------------------- miniaudio ---
FetchContent_Declare(miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        0.11.21
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(miniaudio)

# --------------------------------------------------------------- Sources ----
file(GLOB_RECURSE ENGINE_SOURCES "src/*.cpp")

add_executable(GameEngine ${ENGINE_SOURCES})

target_include_directories(GameEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${stb_SOURCE_DIR}
    ${miniaudio_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ecs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vfx
    ${CMAKE_CURRENT_SOURCE_DIR}/src/world)

target_link_libraries(GameEngine PRIVATE
    glfw
    glm::glm
    glad_gl_core_45)

if(WIN32)
    target_link_libraries(GameEngine PRIVATE opengl32)
    target_compile_definitions(GameEngine PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN)
elseif(APPLE)
    target_link_libraries(GameEngine PRIVATE "-framework OpenGL" "-framework AudioToolbox" "-framework CoreAudio")
else()
    target_link_libraries(GameEngine PRIVATE GL dl pthread)
endif()

# ----------------------------------------------------------- Copy Assets ----
add_custom_command(TARGET GameEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:GameEngine>/assets
    COMMENT "Copying assets to output directory")

# --------------------------------------------------------- MSVC Settings ----
if(MSVC)
    target_compile_options(GameEngine PRIVATE /W3 /MP /utf-8)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTY VS_STARTUP_PROJECT GameEngine)
endif()
