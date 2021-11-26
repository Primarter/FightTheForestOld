#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/Platform/GlfwApplication.h>
#include <Magnum/ImGuiIntegration/Context.hpp>

#include <Magnum/GL/Version.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
// #include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/AbstractShaderProgram.h>
// #include <Magnum/Shaders/VertexColorGL.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Plane.h>

#include <Magnum/Math/Color.h>

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

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/GrowableArray.h>
// #include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Reference.h>
// #include <Corrade/Containers/Pointer.h>
// #include <Corrade/Utility/Arguments.h>
// #include <Corrade/Utility/Resource.h>

#include <map>

#include <chrono>
#include <thread>

#include "StopWatch.hpp"
// #include "WallShader.hpp"

#define CELL_SIZE (1.0f)
#define CELL_SIZE2 (CELL_SIZE/2.0f)
#define SIZE_X (6)
#define SIZE_Z (6)
#define NB_TOTAL (SIZE_X * SIZE_Z)

#define WALL_THICCNESS (0.1f)

const bool map[] = {
    0, 0, 0, 0, 1, 1,
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 0, 1, 0,
    0, 1, 0, 0, 1, 0,
    0, 1, 1, 1, 1, 1,
    0, 1, 1, 1, 0, 0,
};

using namespace Magnum;
using namespace Corrade;
using namespace Math::Literals;

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;

struct WallInstanceData {
    Matrix4 transformation; // translation, rotation, scale
    Matrix3x3 normal;
    Color3 color; // rgb
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
        const double _frametime = 1.0/60.0;

        Containers::Array<WallInstanceData> _wallInstanceData;
        GL::Buffer _wallInstanceBuffer{NoCreate};

        Shaders::PhongGL _shader{NoCreate};
        GL::Mesh _meshPlane;
        GL::Mesh _meshCube;

        Object3D _cameraObject;
        SceneGraph::Camera3D _camera;

        Matrix4 _projection;
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

    _projection = Matrix4::perspectiveProjection(60.0_degf,
            Vector2{framebufferSize()}.aspectRatio(), 0.1f, 1000.0f);

    _camera
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(60.0_degf, 1.0f, 0.1f, 1000.0f))
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

    _shader = Shaders::PhongGL{};
        // Shaders::PhongGL::Flag::
        // Shaders::PhongGL::Flag::VertexColor |
        // Shaders::PhongGL::Flag::InstancedTransformation};

    _meshCube = MeshTools::compile(Primitives::cubeSolid());
    _meshPlane = MeshTools::compile(Primitives::planeSolid());

    _wallInstanceData = Containers::Array<WallInstanceData>{NoInit, 10000};
    _wallInstanceBuffer = GL::Buffer{};

    // for (int i = 0 ; i < 10000 ; ++i) {
    //     // Matrix4 m = Matrix4::translation({4.f*floorf(i/100), 0.f, -i%100});
    //     // Containers::arrayAppend(_wallInstanceData, {m, Color3::cyan(), {}});
    // }

    // int i = 0;
    // for (auto &instance : _wallInstanceData) {
    //     instance.transformation = Matrix4::translation({4.f*floorf(i/100), 4.0f*(i%10), -i%100});
    //     instance.normal = instance.transformation.normalMatrix();
    //     instance.color = Color3::cyan();
    //     i += 1;
    // }

    // _meshCube
    //     .setInstanceCount(_wallInstanceData.size())
    //     .addVertexBufferInstanced(_wallInstanceBuffer, 1, 0,
    //         Shaders::PhongGL::TransformationMatrix{},
    //         Shaders::PhongGL::NormalMatrix{},
    //         Shaders::PhongGL::Color3{}
    //     );

    // _wallInstanceBuffer.setData(_wallInstanceData, GL::BufferUsage::DynamicDraw);
    // _meshPlane
    //     .setInstanceCount(_wallInstanceData.size())
    //     .addVertexBufferInstanced(_wallInstanceBuffer, 1, 0,
    //         Shaders::PhongGL::TransformationMatrix{},
    //         Shaders::PhongGL::NormalMatrix{},
    //         Shaders::PhongGL::Color3{}
    // );

    _sw.start();
}

void MyApplication::update()
{
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);
    GL::defaultFramebuffer.clearDepth(1.0f);

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    // GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    const float yaw = _cameraRotation.x();

    if (_keys[KeyEvent::Key::W]) {
        _cameraPosition.x() += cosf(yaw + Constants::piHalf()) * 0.2f;
        _cameraPosition.z() += sinf(yaw + Constants::piHalf()) * 0.2f;
    }
    if (_keys[KeyEvent::Key::S]) {
        _cameraPosition.x() -= cosf(yaw + Constants::piHalf()) * 0.2f;
        _cameraPosition.z() -= sinf(yaw + Constants::piHalf()) * 0.2f;
    }
    if (_keys[KeyEvent::Key::A]) {
        _cameraPosition.x() += cosf(yaw) * 0.2f;
        _cameraPosition.z() += sinf(yaw) * 0.2f;
    }
    if (_keys[KeyEvent::Key::D]) {
        _cameraPosition.x() -= cosf(yaw) * 0.2f;
        _cameraPosition.z() -= sinf(yaw) * 0.2f;
    }
    if (_keys[KeyEvent::Key::Q]){
        _cameraPosition.y() += 0.2f;
    }
    if (_keys[KeyEvent::Key::E]) {
        _cameraPosition.y() -= 0.2f;
    }

    if(_keys[KeyEvent::Key::Right]) {
        _cameraRotation.x() += 0.04f;
    } else if (_keys[KeyEvent::Key::Left]) {
        _cameraRotation.x() -= 0.04f;
    }
    if(_keys[KeyEvent::Key::Up]) {
        _cameraRotation.y() -= 0.02f;
    } else if (_keys[KeyEvent::Key::Down]) {
        _cameraRotation.y() += 0.02f;
    }


    Matrix4 cameraTransform =
        Matrix4::translation(-_cameraPosition) *
        Matrix4::rotationY(Rad{-_cameraRotation.x()}) *
        Matrix4::rotationX(Rad{-_cameraRotation.y()});

    // for (auto &instance : _wallInstanceData) {
    //     instance.transformation = Matrix4::rotationY(Rad{0.01f}) * instance.transformation;
    //     instance.normal = instance.transformation.normalMatrix();
    // }
    // _wallInstanceBuffer.setData(_wallInstanceData, GL::BufferUsage::DynamicDraw);


    Color4 color{1.0f, 1.0f, 1.0f, 1.0f};
    // Vector4 light = Vector4{_cameraObject.transformation().translation(), 1.0f};


    Matrix4 directionalLight = Matrix4::translation({0.0f, 0.0f, 0.0f}); // camera-relative

    // .setLightPositions({Vector4{directionalLight.up(), 0.0f},
    //                     Vector4{pointLight1.translation(), 1.0f},
    //                     Vector4{pointLight2.translation(), 1.0f}})

    // _shader.setAmbientColor(0x111111_rgbf)
    //     .setSpecularColor(0x330000_rgbf);

    Matrix4 modelRotation =
        Matrix4::translation({2.0f, 0.0f, 0.0f}) *
        Matrix4::rotationX(30.0_degf) *
        Matrix4::rotationZ(15.0_degf);

    _shader
        .setLightPositions({cameraTransform.inverted() * Vector4{directionalLight.up(), 0.0f}})
        // .setShininess(1.0f)
        .setSpecularColor({1.0, 1.0, 1.0, 1.0})
        .setDiffuseColor(color)
        .setAmbientColor(Color3::fromHsv({color.hue(), 1.0f, 0.3f}))
        // .setNormalMatrix(cameraTransform.inverted().normalMatrix())
        .setProjectionMatrix(_camera.projectionMatrix());
        // .setProjectionMatrix(_projection)

    _shader
        .setNormalMatrix(cameraTransform.inverted().normalMatrix())
        .setTransformationMatrix(cameraTransform.inverted())
        .draw(_meshCube);

    _shader
        .setNormalMatrix((cameraTransform.inverted() * modelRotation).normalMatrix())
        .setTransformationMatrix(cameraTransform.inverted() * modelRotation)
        .draw(_meshCube);
        // .draw(_meshPlane);


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
    if (_elapsed > _frametime) {
        _sw.restart();

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
