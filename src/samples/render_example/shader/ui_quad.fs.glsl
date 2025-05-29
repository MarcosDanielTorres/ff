#version 330

in vec4 OutColor;
in vec2 FragUV;
uniform sampler2D texture_sampler;
out vec4 FragColor;
void main()
{
    // If I don't give a fuck about the triangle!
    //float alpha = texture(texture_sampler, FragUV).r;
    //FragColor = vec4(OutColor.rgb, OutColor.a * alpha);
    //vec4 background_color = vec4(1.0, 0.0, 0.0, 0.0);
    //if (background_color.a == 0.0)
    //{
    //    FragColor = vec4(OutColor.rgb, OutColor.a * alpha);
    //}
    //else
    //{
    //    FragColor = vec4(mix(background_color.rgb, OutColor.rgb, alpha), OutColor.a);
    //}

    // If I want the triangle to show!
    FragColor = vec4(texture(texture_sampler, FragUV));
}