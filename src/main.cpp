#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/Platform/GlfwApplication.h>
#include <Magnum/ImGuiIntegration/Context.hpp>

// #include <Magnum/GL/Version.h>
// #include <Magnum/GL/Buffer.h>
// #include <Magnum/GL/Context.h>

// #include <Magnum/GL/Mesh.h>
// #include <Magnum/GL/Shader.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Renderer.h>
// #include <Magnum/GL/AbstractShaderProgram.h>
// #include <Magnum/Shaders/VertexColorGL.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Plane.h>
#include <Magnum/Math/Color.h>
// #include <Magnum/Math/Constants.h>

#include <Magnum/SceneGraph/Camera.h>
// #include <Magnum/SceneGraph/Object.h>
// #include <Magnum/SceneGraph/Scene.h>
// #include <Magnum/SceneGraph/Drawable.h>
// #include <Magnum/SceneGraph/DualQuaternionTransformation.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>

#include <Magnum/Trade/MeshData.h>
// #include <Magnum/MeshTools/Interleave.h>
// #include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Compile.h>

// #include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Reference.h>
// #include <Corrade/Utility/Resource.h>

#include <map>

#include <chrono>
#include <thread>

#include "StopWatch.hpp"

using namespace Magnum;
using namespace Math::Literals;

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;

/* components */
struct Transform
{
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
};

struct Event
{
    Platform::Application::KeyEvent::Key key;
};


/* singletons components */
struct Camera
{
    Vector3 position;
    Vector3 rotation; // pitch, yaw, roll

    Matrix4 view;
    Matrix4 projection;
};


class MyApplication: public Platform::Application
{
    public:
        explicit MyApplication(const Arguments& arguments);

        void update();

    private:
        void drawEvent() override;

        void viewportEvent(ViewportEvent& event) override;

        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;

        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;
        void textInputEvent(TextInputEvent& event) override;

    private:
        ImGuiIntegration::Context _imgui{NoCreate};
        std::map<KeyEvent::Key, bool> _keys;

        Color4 _clearColor = {0.3, 0.3, 0.3, 1.0};

        lib::StopWatch _sw;
        double _elapsed;

        GL::Mesh _meshCube;
        GL::Mesh _meshPlane;
        Shaders::PhongGL _shader;

        Object3D _cameraObject;
        SceneGraph::Camera3D _camera;

        Vector3 _cameraPosition{0.0f, 0.0f, -10.0f};
        Vector2 _cameraRotation{0.0f, 0.0f}; // yaw pitch
};

MyApplication::MyApplication(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}.setTitle("Magnum Triangle Example").setSize({1280, 720})},
    _camera{_cameraObject}
{
    MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL430);
    MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL440);
    MAGNUM_ASSERT_GL_EXTENSION_SUPPORTED(GL::Extensions::ARB::geometry_shader4);
    MAGNUM_ASSERT_GL_EXTENSION_SUPPORTED(GL::Extensions::ARB::draw_instanced);

    _camera
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(60.0_degf, 1.0f, 0.01f, 100.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size());

    _imgui = ImGuiIntegration::Context(Vector2{windowSize()}/dpiScaling(),
        windowSize(), framebufferSize());

    /* Set up proper blending to be used by ImGui. There's a great chance
       you'll need this exact behavior for the rest of your scene. If not, set
       this only for the drawFrame() call. */
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
        GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    setSwapInterval(1); // vsync on
    GL::Renderer::setClearColor(_clearColor);

    _meshCube = MeshTools::compile(Primitives::cubeSolid());
    _meshPlane = MeshTools::compile(Primitives::planeSolid());

    _sw.start();
}

void MyApplication::update()
{
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);
    GL::defaultFramebuffer.clearDepth(1.0f);

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    _cameraObject
        .resetTransformation()
        .translate(_cameraPosition)
        .rotateY(Rad{_cameraRotation.x()})
        .rotateX(Rad{_cameraRotation.y()});

    Color4 color{1.0f, 1.0f, 1.0f, 1.0f};
    _shader
        .setLightPositions({{4.4f, 10.0f, 0.75f, 0.0f}})
        .setDiffuseColor(color)
        .setAmbientColor(Color3::fromHsv({color.hue(), 1.0f, 0.3f}))
        .setTransformationMatrix(_cameraObject.transformation())
        .setProjectionMatrix(_camera.projectionMatrix())
        .draw(_meshCube);

    _imgui.newFrame();
    {
        ImGui::Text("Hello, world!");
        if (ImGui::ColorEdit3("Clear Color", _clearColor.data()))
            GL::Renderer::setClearColor(_clearColor);
        ImGui::Text("average %.3f ms/frame (%.1f FPS)",
            1000.0/Double(ImGui::GetIO().Framerate), Double(ImGui::GetIO().Framerate));
    }

    /* Update application cursor */
    _imgui.updateApplicationCursor(*this);

    /* Set appropriate states. If you only draw ImGui, it is sufficient to
       just enable blending and scissor test in the constructor. */
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);

    _imgui.drawFrame();
}

void MyApplication::drawEvent()
{
    _elapsed = _sw.getElapsedTime();
    if (_elapsed > 0.016666) {
        _sw.start();

        update();
        swapBuffers();
    }

    redraw();
}

void MyApplication::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    _imgui.relayout(Vector2{event.windowSize()}/event.dpiScaling(),
        event.windowSize(), event.framebufferSize());
}

void MyApplication::keyPressEvent(KeyEvent& event) {
    KeyEvent::Key key = event.key();
    _keys[key] = true;

    if(key == KeyEvent::Key::Esc) {
        exit();
    }

    if(_imgui.handleKeyPressEvent(event)) return;
}

void MyApplication::keyReleaseEvent(KeyEvent& event) {
    if(_imgui.handleKeyReleaseEvent(event)) return;

    KeyEvent::Key key = event.key();
    _keys[key] = false;
    // _world.get<Events>()->keys[key] = false;
    // _events->keys[key] = false;
}

void MyApplication::mousePressEvent(MouseEvent& event) {
    if(_imgui.handleMousePressEvent(event)) return;
}

void MyApplication::mouseReleaseEvent(MouseEvent& event) {
    if(_imgui.handleMouseReleaseEvent(event)) return;
}

void MyApplication::mouseMoveEvent(MouseMoveEvent& event) {
    if(_imgui.handleMouseMoveEvent(event)) return;
    // const Vector2 delta = Vector2{event.relativePosition()} / Vector2{windowSize()};
}

void MyApplication::mouseScrollEvent(MouseScrollEvent& event) {
    if(_imgui.handleMouseScrollEvent(event)) {
        /* Prevent scrolling the page */
        event.setAccepted();
        return;
    }
}

void MyApplication::textInputEvent(TextInputEvent& event) {
    if(_imgui.handleTextInputEvent(event)) return;
}

MAGNUM_APPLICATION_MAIN(MyApplication)
