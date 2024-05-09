#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_texture_coord;

layout (location = 0) out vec3 frag_vertex_color;
layout (location = 1) out vec2 frag_texture_coord;

void main() {
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(in_position, 0.0, 1.0);
    frag_vertex_color = in_color;
    frag_texture_coord = in_texture_coord;
    //gl_Position = vec4(in_position, 0.0, 1.0);
}