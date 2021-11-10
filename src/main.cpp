#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Platform/GLContext.h>
#include <Magnum/Shaders/VertexColorGL.h>
#include <GLFW/glfw3.h>

using namespace Magnum;

int main(int argc, char** argv) {
    /* Initialize the library */
    if(!glfwInit()) return -1;

    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow* const window = glfwCreateWindow(
        800, 600, "Magnum Plain GLFW Triangle Example", nullptr, nullptr);
    if(!window) {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    {
        /* Create Magnum context in an isolated scope */
        Platform::GLContext ctx{argc, argv};

        /* Setup the colored triangle */
        using namespace Math::Literals;

        struct TriangleVertex {
            Vector2 position;
            Color3 color;
        };
        const TriangleVertex data[]{
            {{-0.5f, -0.5f}, 0xff0000_rgbf},    /* Left vertex, red color */
            {{ 0.5f, -0.5f}, 0x00ff00_rgbf},    /* Right vertex, green color */
            {{ 0.0f,  0.5f}, 0x0000ff_rgbf}     /* Top vertex, blue color */
        };

        GL::Buffer buffer;
        buffer.setData(data);

        GL::Mesh mesh;
        mesh.setPrimitive(GL::MeshPrimitive::Triangles)
            .setCount(3)
            .addVertexBuffer(buffer, 0,
                Shaders::VertexColorGL2D::Position{},
                Shaders::VertexColorGL2D::Color3{});

        Shaders::VertexColorGL2D shader;

        /* Loop until the user closes the window */
        while(!glfwWindowShouldClose(window)) {

            /* Render here */
            GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);
            shader.draw(mesh);

            /* Swap front and back buffers */
            glfwSwapBuffers(window);

            /* Poll for and process events */
            glfwPollEvents();
        }
    }

    glfwTerminate();
}
