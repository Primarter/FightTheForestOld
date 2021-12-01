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
// #include <Magnum/Primitives/Icosphere.h>


#include <Magnum/Math/Color.h>
#include <Magnum/Math/Functions.h>

#include <Magnum/SceneGraph/Object.h>

// #include <Magnum/SceneGraph/Camera.h>
// #include <Magnum/SceneGraph/Drawable.h>
// #include <Magnum/SceneGraph/MatrixTransformation3D.h>
// #include <Magnum/SceneGraph/Scene.h>

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
#include <Corrade/Containers/Pointer.h>
// #include <Corrade/Utility/Arguments.h>
// #include <Corrade/Utility/Resource.h>

#include <Magnum/BulletIntegration/Integration.h>
#include <Magnum/BulletIntegration/MotionState.h>
#include <Magnum/BulletIntegration/DebugDraw.h>

#include <btBulletDynamicsCommon.h>


#include <map>

#include <chrono>
#include <thread>

#include "StopWatch.hpp"
#include "utils.hpp"
// #include "Rigidbody.hpp"
// #include "WallShader.hpp"

#define CELL_SIZE (1.0f)
#define CELL_SIZE2 (CELL_SIZE/2.0f)
#define SIZE_X (6)
#define SIZE_Z (6)
#define NB_TOTAL (SIZE_X * SIZE_Z)
#define WALL_THICCNESS (0.5f)
#define GROUND_HEIGHT (-0.25f)

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

class RigidBody: public Object3D {
    public:
        RigidBody(Float mass, btCollisionShape* bShape, btDynamicsWorld& bWorld):
            Object3D{},
            _bWorld(bWorld)
        {
            /* Calculate inertia so the object reacts as it should with
               rotation and everything */
            btVector3 bInertia(0.0f, 0.0f, 0.0f);
            if(!Math::TypeTraits<Float>::equals(mass, 0.0f))
                bShape->calculateLocalInertia(mass, bInertia);

            /* Bullet rigid body setup */
            auto* motionState = new BulletIntegration::MotionState{*this}; // needs an Object3D
            _bRigidBody.emplace(btRigidBody::btRigidBodyConstructionInfo{
                mass, &motionState->btMotionState(), bShape, bInertia});
            // _bRigidBody->forceActivationState(DISABLE_DEACTIVATION);
            bWorld.addRigidBody(_bRigidBody.get());
        }

        ~RigidBody() {
            _bWorld.removeRigidBody(_bRigidBody.get());
        }

        btRigidBody& rigidBody() { return *_bRigidBody; }

        /* needed after changing the pose from Magnum side */
        void syncPose() {
            _bRigidBody->setWorldTransform(btTransform(transformationMatrix()));
        }

    private:
        btDynamicsWorld& _bWorld;
        Containers::Pointer<btRigidBody> _bRigidBody;
};

struct InstancedData {
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

        Containers::Array<InstancedData> _cubeInstanceData;
        Containers::Array<InstancedData> _wallInstanceData;
        Containers::Array<InstancedData> _groundInstanceData;

        GL::Buffer _cubeInstanceBuffer{NoCreate};
        GL::Buffer _wallInstanceBuffer{NoCreate};
        GL::Buffer _groundInstanceBuffer{NoCreate};

        GL::Mesh _meshCube;
        GL::Mesh _meshWall;
        GL::Mesh _meshGround;

        Shaders::PhongGL _shader{NoCreate};

        Matrix4 _projection;
        Vector3 _cameraPosition{0.0f, -5.0f, -10.0f};
        Vector3 _cameraRotation{0.0f, 0.0f, 0.0f};

        /*  bullet */
        BulletIntegration::DebugDraw _debugDraw{NoCreate};

        btDbvtBroadphase _bBroadphase;
        btDefaultCollisionConfiguration _bCollisionConfig;
        btCollisionDispatcher _bDispatcher{&_bCollisionConfig};
        btSequentialImpulseConstraintSolver _bSolver;

        /* The world has to live longer than the scene because RigidBody
           instances have to remove themselves from it on destruction */
        btDiscreteDynamicsWorld _bWorld{&_bDispatcher, &_bBroadphase, &_bSolver, &_bCollisionConfig};
        Timeline _timeline;

        btBoxShape _bBoxShape{{0.5f, 0.5f, 0.5f}};
        btBoxShape _bGroundShape{{10.0f, 0.5f, 10.0f}};

        RigidBody *_rigidBody;
};

MyApplication::MyApplication(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}.setTitle("Magnum Triangle Example").setSize({1280, 720})}
{
    MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL440);
    MAGNUM_ASSERT_GL_EXTENSION_SUPPORTED(GL::Extensions::ARB::geometry_shader4);
    MAGNUM_ASSERT_GL_EXTENSION_SUPPORTED(GL::Extensions::ARB::draw_instanced);

    // _projection = Matrix4::perspectiveProjection(51.45_degf*Vector2{framebufferSize()}.aspectRatio(),
    //     Vector2{framebufferSize()}.aspectRatio(), 0.1f, 1000.0f);
    _projection = Matrix4::perspectiveProjection(60.0_degf,
        1.0/Vector2{framebufferSize()}.aspectRatio(), 0.1f, 1000.0f);
    const float tmp = _projection[0][0];
    _projection[0][0] = _projection[1][1];
    _projection[1][1] = tmp;

    _imgui = ImGuiIntegration::Context(Vector2{windowSize()}/dpiScaling(),
        windowSize(), framebufferSize());

    /* Set up proper blending to be used by ImGui. There's a great chance
       you'll need this exact behavior for the rest of your scene. If not, set
       this only for the drawFrame() call. */
    GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
        GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    setSwapInterval(1); // 1:vsync on // 2:vsync off
    GL::Renderer::setClearColor(_clearColor);

    _shader = Shaders::PhongGL{
        Shaders::PhongGL::Flag::VertexColor |
        Shaders::PhongGL::Flag::InstancedTransformation};

    _meshCube = MeshTools::compile(Primitives::cubeSolid());
    _meshWall = MeshTools::compile(Primitives::cubeSolid());
    _meshGround = MeshTools::compile(Primitives::planeSolid());

    _cubeInstanceData = Containers::Array<InstancedData>{NoInit, 0};
    _cubeInstanceBuffer = GL::Buffer{};

    _wallInstanceData = Containers::Array<InstancedData>{NoInit, 0};
    _wallInstanceBuffer = GL::Buffer{};

    _groundInstanceData = Containers::Array<InstancedData>{NoInit, 0};
    _groundInstanceBuffer = GL::Buffer{};

    // fill _cubeInstanceData
    for (int i = 0 ; i < NB_TOTAL ; ++i) {
        int x = i % SIZE_X;
        int z = floor(i / SIZE_X);

        Vector3 p = Vector3{
            x - SIZE_X/2.0f,
            0.0f,
            z - SIZE_Z/2.0f
        } * 10.0f;

        Matrix4 m = Matrix4::translation(p) * Matrix4::scaling(Vector3{0.5f});
        Matrix3x3 n = m.normalMatrix();
        Color3 c = map[i] ? Color3(1.0f) : Color3::red();

        Containers::arrayAppend(_cubeInstanceData, {m, n, c});
    }

    // fill _wallInstanceData
    for (int i = 0 ; i < NB_TOTAL ; ++i) {
        int x = i % SIZE_X;
        int z = floor(i / SIZE_X);

        // x axis
        if (x < SIZE_X - 1 && map[i] != map[i + 1]) {
            Matrix4 m = Matrix4::translation(
                Vector3{x - SIZE_X/2.f + CELL_SIZE2, 0, z - SIZE_Z/2.f});
            m.translation() *= 10.0f;
            m = m * Matrix4::scaling({WALL_THICCNESS, 1.0f, 5.5f});

            Containers::arrayAppend(_wallInstanceData, {
                m, m.normalMatrix(), Color3::green()
            });
        }
        // z axis
        if (z < SIZE_Z-1 && map[i] != map[i + SIZE_X]) {
            Matrix4 m = Matrix4::translation(
                Vector3{x - SIZE_X/2.f, 0, z - SIZE_Z/2.f + CELL_SIZE2});
            m.translation() *= 10.0f;
            m = m * Matrix4::scaling({5.5f, 1.0f, WALL_THICCNESS});

            Containers::arrayAppend(_wallInstanceData, {
                m, m.normalMatrix(), Color3::green()
            });
        }
    }

    // fill _groundInstanceData
    for (int i = 0 ; i < NB_TOTAL ; ++i) {
        int x = i % SIZE_X;
        int z = floor(i / SIZE_X);

        Matrix4 m = Matrix4::translation({
            x * CELL_SIZE - SIZE_X/2.f,
            GROUND_HEIGHT,
            z * CELL_SIZE - SIZE_Z/2.f});
        m = m * Matrix4::rotationX(-90.0_degf);
        m.translation() *= 10.0f;
        m = m * Matrix4::scaling({5.0f, 5.0f, 5.0f});


        Matrix3x3 n = m.normalMatrix();
        Color3 c = map[i] ? Color3(1.0f) : Color3::red();

        Containers::arrayAppend(_groundInstanceData, {m, n, c});
    }

    _meshCube
        .setInstanceCount(_cubeInstanceData.size())
        .addVertexBufferInstanced(_cubeInstanceBuffer, 1, 0,
            Shaders::PhongGL::TransformationMatrix{},
            Shaders::PhongGL::NormalMatrix{},
            Shaders::PhongGL::Color3{}
        );

    _meshWall
        .setInstanceCount(_wallInstanceData.size())
        .addVertexBufferInstanced(_wallInstanceBuffer, 1, 0,
            Shaders::PhongGL::TransformationMatrix{},
            Shaders::PhongGL::NormalMatrix{},
            Shaders::PhongGL::Color3{}
        );

    _meshGround
        .setInstanceCount(_groundInstanceData.size())
        .addVertexBufferInstanced(_groundInstanceBuffer, 1, 0,
            Shaders::PhongGL::TransformationMatrix{},
            Shaders::PhongGL::NormalMatrix{},
            Shaders::PhongGL::Color3{}
        );

    _cubeInstanceBuffer.setData(_cubeInstanceData, GL::BufferUsage::StaticDraw);
    _wallInstanceBuffer.setData(_wallInstanceData, GL::BufferUsage::StaticDraw);
    _groundInstanceBuffer.setData(_groundInstanceData, GL::BufferUsage::StaticDraw);

    /* bullet */
    _debugDraw = BulletIntegration::DebugDraw{};
    _debugDraw.setMode(
        BulletIntegration::DebugDraw::Mode::DrawWireframe |
        BulletIntegration::DebugDraw::Mode::DrawContactPoints);
    _bWorld.setDebugDrawer(&_debugDraw);

    _bWorld.setGravity({0.0f, -9.81f, 0.0f});

    RigidBody *ground = new RigidBody{0.0f, &_bGroundShape, _bWorld};
    ground->rotateZ(45.0_degf);
    ground->syncPose();

    ground = new RigidBody{0.0f, &_bGroundShape, _bWorld};
    ground->rotateZ(-45.0_degf);
    ground->translate({-15.0f, -15.0f, 0.0f});
    ground->syncPose();

    ground = new RigidBody{0.0f, &_bGroundShape, _bWorld};
    ground->rotateZ(45.0_degf);
    ground->translate({0.0f, -30.0f, 0.0f});
    ground->syncPose();

    // for(int i = 0; i != 6; ++i) {
    //     for(int j = 0; j != 6; ++j) {
    //         for(int k = 0; k != 6; ++k) {
    //             auto* o = new RigidBody{1.0f, &_bBoxShape, _bWorld};
    //             o->translate({i - 2.0f, j + 4.0f, k - 2.0f});
    //             o->syncPose();
    //         }
    //     }
    // }

    for (int i = 0 ; i < 150 ; ++i) {
        auto* o = new RigidBody{1.0f, &_bBoxShape, _bWorld};
        o->translate({0.0f, i + 5.0f, 0.0f});
        o->syncPose();
    }
    // for (int i = 0 ; i < 100 ; ++i)
    //     new RigidBody{1.0f, &_bBoxShape, _bWorld};


    _timeline.start();
    _sw.start();
}

void MyApplication::update()
{
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    { // camera movements
        const float yaw = _cameraRotation.y();

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
            _cameraRotation.y() += 0.04f;
        } else if (_keys[KeyEvent::Key::Left]) {
            _cameraRotation.y() -= 0.04f;
        }
        if(_keys[KeyEvent::Key::Up]) {
            _cameraRotation.x() -= 0.02f;
        } else if (_keys[KeyEvent::Key::Down]) {
            _cameraRotation.x() += 0.02f;
        }
        _cameraRotation.x() = Math::clamp(_cameraRotation.x(), -Constants::piHalf(), Constants::piHalf());
    }

    _bWorld.stepSimulation(_timeline.previousFrameDuration(), 1);

    Matrix4 cameraTransform =
        Matrix4::translation(-_cameraPosition) *
        Matrix4::rotationY(Rad{-_cameraRotation.y()}) *
        Matrix4::rotationX(Rad{-_cameraRotation.x()});


    Color4 color{1.0f, 1.0f, 1.0f, 1.0f};

    _shader
        .setLightPositions({cameraTransform.inverted() * Vector4{0.6f, 1.0f, 0.25f, 0.0f}})
        .setSpecularColor({1.f, 1.f, 1.f, 1.f})
        .setAmbientColor({0.2f, 0.2f, 0.2f, 1.0f})
        // .setShininess(100.0f)
        // .setDiffuseColor(color)
        .setProjectionMatrix(_projection);

    // _shader
    //     .setNormalMatrix(cameraTransform.inverted().normalMatrix())
    //     .setTransformationMatrix(cameraTransform.inverted())
        // .draw(_meshCube);
        // .draw(_meshGround);
        // .draw(_meshWall);

    // DebugDraw
    GL::Renderer::setDepthFunction(GL::Renderer::DepthFunction::LessOrEqual);
    _debugDraw.setTransformationProjectionMatrix(
        _projection * cameraTransform.inverted());
    _bWorld.debugDrawWorld();

    _imgui.newFrame();
    {
        ImGui::Text("Hello, world!");
        if (ImGui::ColorEdit3("Clear Color", _clearColor.data()))
            GL::Renderer::setClearColor(_clearColor);
        ImGui::Text("average %.3f ms/frame (%.1f FPS)",
            1000.0/Double(ImGui::GetIO().Framerate), Double(ImGui::GetIO().Framerate));
        // ImGui::SliderInt("COUNT", &_count, 1, 100000);
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

        _timeline.nextFrame(); // bullet
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
