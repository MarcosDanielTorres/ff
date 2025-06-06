#version 330 core

layout (location = 0) in vec3 InPos;
layout (location = 1) in vec2 InUv;
layout (location = 2) in vec4 InColor;

uniform mat4 proj;
uniform mat4 view;
out vec2 FragUV;
out vec4 OutColor;

void main()
{
    gl_Position = proj * view * vec4(InPos, 1.0f);
    FragUV = InUv;
    OutColor = InColor;
}