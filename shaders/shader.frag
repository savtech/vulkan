#version 450

layout(location = 0) in vec3 vertex_color;
//layout(location = 1) in float vertex_distance;

layout(location = 0) out vec4 out_color;

void main() {
    // fade_distance = 1.0; // Increase this value for quicker fade-out
    //float alpha = clamp(1.0 - vertex_distance / fade_distance, 0.0, 1.0);

    out_color = vec4(vertex_color, 1.0);
}