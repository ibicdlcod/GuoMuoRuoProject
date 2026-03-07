#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

varying vec2 v_texcoord;
varying vec3 v_normal;

uniform sampler2D u_texture;
uniform vec3 u_lightDirection; // Direction of your "sun"

void main() {
    vec3 n = normalize(v_normal);
    vec3 l = normalize(u_lightDirection);

    // Calculate diffuse "brightness" (Lambert's Cosine Law)
    float diff = max(dot(n, l), 0.2); // 0.2 is a "fake" ambient light

    vec4 texColor = texture2D(u_texture, v_texcoord);
    gl_FragColor = vec4(texColor.rgb * diff, texColor.a);
}

