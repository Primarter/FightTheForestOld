find_package(Magnum REQUIRED GlfwApplication)
find_package(MagnumIntegration REQUIRED ImGui)
find_package(MagnumIntegration REQUIRED Bullet)
find_package(Bullet REQUIRED Dynamics)

find_package(Corrade REQUIRED Main)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

corrade_add_resource(RESOURCES resources.conf)

set(PROJECT_ROOT ${PROJECT_SOURCE_DIR}/src)
set(SRC_FILES
    ${PROJECT_ROOT}/main.cpp
)

add_executable(MyApplication ${SRC_FILES} ${RESOURCES})
target_include_directories(MyApplication PRIVATE ${PROJECT_ROOT}/../include/)

target_link_libraries(MyApplication PRIVATE
    Magnum::Application
    Magnum::GL
    Magnum::Primitives
    # Magnum::Audio
    Magnum::MeshTools
    Magnum::SceneGraph
    Magnum::Shaders
    Magnum::Magnum
    MagnumIntegration::ImGui
    MagnumIntegration::Bullet
    Bullet::Dynamics)
    # Bullet::LinearMath
    # Bullet::Collision
    # Bullet::SoftBody)

# Disable Bullet tests and demos
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
# While not needed for Magnum, you might actually want some of those
set(BUILD_ENET OFF CACHE BOOL "" FORCE)
set(BUILD_CLSOCKET OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET3 OFF CACHE BOOL "" FORCE)
# This is needed in case BUILD_EXTRAS is enabled, as you'd get a CMake syntax
# error otherwise
set(PKGCONFIG_INSTALL_PREFIX "lib${LIB_SUFFIX}/pkgconfig/")
# Bullet's buildsystem doesn't correctly express dependencies between static
# libs, causing linker errors on Magnum side. If you have CMake 3.13, the Find
# module is able to correct that on its own, otherwise you need to enable
# BUILD_SHARED_LIBS to build as shared.
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)


# Make the executable a default target to build & run in Visual Studio
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MyApplication)
