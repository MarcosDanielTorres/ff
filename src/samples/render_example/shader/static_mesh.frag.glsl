#version 330

in vec4 OutColor;
in vec2 FragUV;
uniform sampler2D texture_sampler;
out vec4 FragColor;
void main()
{
    FragColor = vec4(texture(texture_sampler, FragUV));
}