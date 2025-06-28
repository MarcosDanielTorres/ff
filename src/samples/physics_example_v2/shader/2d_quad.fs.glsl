#version 330

in vec4 OutColor;
in vec2 FragUV;
in float corner_radius;
in float border_thickness;
in float omit_texture;
uniform sampler2D texture_sampler;
out vec4 FragColor;

// This shader doesnt event work, border_thickness is not used. MUST clean this up!

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
        {
            float sdf = sdRoundBox(p, vec2(0.5, 0.5), vec4(r));
            float half_thick = border_thickness * 0.5;
            // compute how far we are from the ideal ring (sdf==0)
            // when abs(sdf) ≤ half_thick we’re inside the band
            float dist_band = abs(sdf) - half_thick;

            float edge_aa = fwidth(sdf); // automatic derivative for AA (needs OpenGL 3+)
            float alpha = smoothstep(0.0, edge_aa, -sdf);
            alpha = smoothstep(edge_aa, -edge_aa, dist_band);
            if (sdf < 0)
            {
                base_color = vec4(OutColor.rgb, OutColor.a * alpha);
            }
        }
    }
    FragColor = base_color;


    if (FragColor.a < 0.01)
        discard;
}