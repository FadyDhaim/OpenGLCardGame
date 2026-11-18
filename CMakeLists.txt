cmake_minimum_required(VERSION 4.4)
project(OpenGLCardGame)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

add_executable(OpenGLCardGame)
file(
        GLOB_RECURSE implementationFiles
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Source/Private/*.cpp"
)
target_sources(OpenGLCardGame PRIVATE ${implementationFiles})
target_include_directories(OpenGLCardGame PUBLIC Source/Public)

target_compile_options(
        OpenGLCardGame PUBLIC
        -fmodules
        -fconcepts
        -freflection
        -fcontracts
        -fcontract-evaluation-semantic=enforce
        -Wdeprecated-copy
        -Wdeprecated-copy-dtor
        -Wno-literal-suffix
)

include(FetchContent)

FetchContent_Declare(
        OpenGLLoader
        GIT_REPOSITORY https://github.com/FadyDhaim/OpenGLLoader.git
        GIT_TAG main
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(OpenGLLoader)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_SHALLOW TRUE
)

set(CACHE{GLFW_BUILD_DOCS} TYPE BOOL FORCE VALUE OFF)
set(CACHE{GLFW_BUILD_TESTS} TYPE BOOL FORCE VALUE OFF)
set(CACHE{GLFW_BUILD_EXAMPLES} TYPE BOOL FORCE VALUE OFF)

FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG master
        GIT_SHALLOW TRUE
)

set(CACHE{ASSIMP_BUILD_TESTS} TYPE BOOL FORCE VALUE OFF)
set(CACHE{ASSIMP_BUILD_SAMPLES} TYPE BOOL FORCE VALUE OFF)
set(CACHE{ASSIMP_BUILD_ASSIMP_TOOLS} TYPE BOOL FORCE VALUE OFF)
set(CACHE{ASSIMP_INSTALL} TYPE BOOL FORCE VALUE OFF)

FetchContent_MakeAvailable(assimp)

FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/FadyDhaim/stb.git
        GIT_TAG cmake/stb-image-static-library
        GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(stb)

find_package(OpenGL REQUIRED)

target_link_libraries(
        OpenGLCardGame PRIVATE
        stdc++exp
        glfw
        OpenGLLoader
        glm
        stb::stb_image
        assimp::assimp
        OpenGL::GL
)

# post configure phase dependency, at ninja generation
#file(
#    GENERATE
#    OUTPUT "${CMAKE_BINARY_DIR}/../compile_commands.json"
#    INPUT "${CMAKE_BINARY_DIR}/compile_commands.json"
#)