find_package(Magnum REQUIRED GlfwApplication)
find_package(MagnumIntegration REQUIRED ImGui)

find_package(Corrade REQUIRED Main)

set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)

corrade_add_resource(RESOURCES resources.conf)

set(PROJECT_ROOT ${PROJECT_SOURCE_DIR}/src)
set(SRC_FILES
    ${PROJECT_ROOT}/main.cpp
)

add_executable(MyApplication ${SRC_FILES} ${RESOURCES})

target_link_libraries(MyApplication PRIVATE
    Magnum::Application
    Magnum::GL
    Magnum::Primitives
    # Magnum::Audio
    Magnum::MeshTools
    Magnum::SceneGraph
    Magnum::Shaders
    Magnum::Magnum
    MagnumIntegration::ImGui)

# Make the executable a default target to build & run in Visual Studio
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MyApplication)
