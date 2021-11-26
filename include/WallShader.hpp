using namespace Magnum;
using namespace Corrade;
using namespace Math::Literals;

struct WallInstanceData {
    Matrix4 transformationMatrix; // translation, rotation, scale
};

class WallShader: public GL::AbstractShaderProgram
{
    public:
        typedef GL::Attribute<0, Vector3> Position;

        // instanced data
        typedef GL::Attribute<1, Matrix4> instancedTransformationMatrix;

        explicit WallShader() {
            MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL430);

            Utility::Resource rs{"WallShader"};

            GL::Shader vert{GL::Version::GL440, GL::Shader::Type::Vertex};
            GL::Shader frag{GL::Version::GL440, GL::Shader::Type::Fragment};

            vert.addSource(rs.get("wall.vert"));
            frag.addSource(rs.get("wall.frag"));

            CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({vert, frag}));
            attachShaders({vert, frag});
            CORRADE_INTERNAL_ASSERT_OUTPUT(link());

            _projectionMatrixUniform = uniformLocation("u_projectionMatrix");
            _viewMatrixUniform = uniformLocation("u_viewMatrix");
        }

        WallShader& setProjectionMatrix(const Matrix4 &matrix) {
            setUniform(_projectionMatrixUniform, matrix);
            return *this;
        }

        WallShader& setViewMatrix(const Matrix4 &matrix) {
            setUniform(_viewMatrixUniform, matrix);
            return *this;
        }

    private:
        Int _projectionMatrixUniform;
        Int _viewMatrixUniform;
};
