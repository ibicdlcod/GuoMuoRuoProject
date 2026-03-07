#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

attribute vec4 a_position;
attribute vec3 a_normal;
attribute vec2 a_texcoord;

varying vec2 v_texcoord;
varying vec3 v_normal;

uniform mat4 mvp_matrix;
uniform mat3 normal_matrix; // New: To transform normals correctly

void main() {
    gl_Position = mvp_matrix * a_position;
    v_texcoord = a_texcoord;
    // Transform normal to view space and pass to fragment shader
    v_normal = vec3(normal_matrix * a_normal);
}
