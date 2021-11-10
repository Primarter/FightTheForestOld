cmake_minimum_required(VERSION 3.4)
project(MyApplication)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules/" ${CMAKE_MODULE_PATH})

# Add Corrade as a subproject
add_subdirectory(corrade EXCLUDE_FROM_ALL)

# Add Magnum as a subproject, enable GLFWApplication
set(CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/glfw-3.3.bin.WIN64 ${CMAKE_PREFIX_PATH})
set(WITH_GLFWAPPLICATION ON)
set(WITH_IMGUI ON)
set(WITH_SCENEGRAPH ON)
set(WITH_GL ON)
set(WITH_SHADERS ON)
set(WITH_MESHTOOLS ON)
set(WITH_PRIMITIVES ON)
set(WITH_TRADE ON)
# set(WITH_AUDIO ON)
# set(WITH_DEBUGTOOLS ON)
# set(WITH_SHADERTOOLS ON)
# set(WITH_TEXT ON)
# set(WITH_TEXTURETOOLS ON)
# set(WITH_VK ON)
add_subdirectory(magnum EXCLUDE_FROM_ALL)

# imgui integration
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
set(WITH_IMGUI ON CACHE BOOL "" FORCE)
add_subdirectory(magnum-integration EXCLUDE_FROM_ALL)

add_subdirectory(src)
