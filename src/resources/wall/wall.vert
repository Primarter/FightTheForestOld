uniform mat4 u_projectionMatrix;
uniform mat4 u_viewMatrix;

// in vec3 Position;
layout (location = 0) in vec3 Position;

// instanced data
// in mat4 Transformation;

in mat4 instancedTransformationMatrix;

void main() {
    vec4 p = instancedTransformationMatrix * vec4(Position, 1.0);
    // vec4 p = vec4(Position, 1.0);
    vec4 pos = u_projectionMatrix * u_viewMatrix * p;

    gl_Position = pos;
}
