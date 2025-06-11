#version 330

in vec4 OutColor;
in vec2 FragUV;
in float corner_radius;
in float border_thickness;
in float omit_texture;
uniform sampler2D texture_sampler;
out vec4 FragColor;


float sdRoundBox(in vec2 p, in vec2 b, in vec4 r ) 
{
    r.xy = (p.x>0.0)?r.xy : r.zw;
    r.x  = (p.y>0.0)?r.x  : r.y;
    vec2 q = abs(p)-b+r.x;
    return min(max(q.x,q.y),0.0) + length(max(q,0.0)) - r.x;
}

void main()
{

    vec4 base_color;
    if (omit_texture < 0.5) {
        float tex_alpha = texture(texture_sampler, FragUV).r;
        base_color = vec4(OutColor.rgb, OutColor.a * tex_alpha);
    } else {
        vec2 center = vec2(0.5, 0.5);
        vec2 p = FragUV - center;

        float r = corner_radius;
        float sdf = sdRoundBox(p, vec2(0.5, 0.5), vec4(r));
        float edge_aa = fwidth(sdf); // automatic derivative for AA (needs OpenGL 3+)
        float alpha = smoothstep(0.0, edge_aa, -sdf);
        if (sdf < 0)
        {
            base_color = vec4(OutColor.rgb, OutColor.a * alpha);
        }
    }
    FragColor = base_color;


    if (FragColor.a < 0.01)
        discard;
}

//void main()
//{
//
//    vec2 center = vec2(0.5, 0.5);
//    vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
//    if (omit_texture < 1)
//    {
//        float alpha = texture(texture_sampler, FragUV).r;
//        color = vec4(OutColor.rgb, OutColor.a * alpha);
//    }
//
//
//
//    // Texture/solid color handling
//    vec4 base_color = vec4(1.0);
//    if (omit_texture < 0.5)
//    {
//        float alpha = texture(texture_sampler, FragUV).r;
//        base_color = vec4(OutColor.rgb, OutColor.a * alpha);
//    }
//    else
//    {
//        vec2 p = FragUV - center;
//        vec2 half_size = center - vec2(corner_radius);
//        float sdf = roundedBoxSDF(p, half_size, corner_radius);
//        if (sdf < corner_radius)
//        {
//            discard;
//        }
//        base_color = OutColor;
//    }
//    FragColor = base_color;


    //vec2 p = FragUV - vec2(0.5); // Center at (0,0)
    //{
    //    vec2 half_size = vec2(0.5); // For full-quad
    //    float border_radius = 0.1;  // How round you want the corners

    //    float d = roundedBoxSDF(p, half_size - border_radius, border_radius);
    //    float border_thickness = 0.03; // whatever looks good
    //    float aa = fwidth(d); // for anti-aliasing

    //    float border = smoothstep(-border_thickness-aa, -border_thickness+aa, d) - 
    //               smoothstep(0.0-aa, 0.0+aa, d);
    //    vec4 color = mix(vec4(0.0, 0.0, 0.0, 1.0), vec4(1.0, 1.0, 0.0, 1.0), step(d, -border_thickness));
    //    //color.a *= border; // Or combine with fill however you want
    //    if (d > 0)
    //    {
    //        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    //    }
    //    else
    //    {

    //        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    //        FragColor = color;
    //    }
    //}



    //float border_thickness = 0.03; // whatever looks good
    //float aa = fwidth(d); // for anti-aliasing

    //float border = smoothstep(-border_thickness-aa, -border_thickness+aa, d) - 
    //           smoothstep(0.0-aa, 0.0+aa, d);
    //vec4 color = mix(border_color, fill_color, step(d, -border_thickness));
    //color.a *= border; // Or combine with fill however you want
    //FragColor = color;

    //float r = 0.5;
    //float d = length(FragUV - vec2(0.5, 0.5));
    //if (d < r)
    //{
    //    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    //}
    //else
    //{
    //    FragColor = vec4(0.0, 0.0, 0.0, 0.0);
    //}
        //FragColor = vec4(mix(background_color.rgb, OutColor.rgb, alpha), OutColor.a);

    // If I want the triangle to show!
    //FragColor = vec4(texture(texture_sampler, FragUV));
//}