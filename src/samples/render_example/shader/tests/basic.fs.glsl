#version 330 core
layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;
uniform sampler2D screenTexture;

void main()
{
    vec2 pixel_size = vec2(1.0 / 320, 1.0 / 240);
    vec2 pixelatedUV = floor(TexCoords / pixel_size) * pixel_size;
    FragColor = texture(screenTexture, TexCoords);
    //float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;
    //FragColor = vec4(average, average, average, 1.0);
}

//const float offset = 1.0 / 300.0;  
//
//void main()
//{
//    vec2 offsets[9] = vec2[](
//        vec2(-offset,  offset), // top-left
//        vec2( 0.0f,    offset), // top-center
//        vec2( offset,  offset), // top-right
//        vec2(-offset,  0.0f),   // center-left
//        vec2( 0.0f,    0.0f),   // center-center
//        vec2( offset,  0.0f),   // center-right
//        vec2(-offset, -offset), // bottom-left
//        vec2( 0.0f,   -offset), // bottom-center
//        vec2( offset, -offset)  // bottom-right    
//    );
//
//    // psichodelics
//    //float kernel[9] = float[](
//    //    -1, -1, -1,
//    //    -1,  9, -1,
//    //    -1, -1, -1
//    //);
//
//    // blur
//    //float kernel[9] = float[](
//    //    1.0 / 12, 2.0 / 16, 4.0 / 12,
//    //    2.0 / 16, 4.0 / 16, 2.0 / 16,
//    //    1.0 / 12, 2.0 / 16, 1.0 / 12  
//    //);
//
//    // psichodelics
//    float kernel[9] = float[](
//        1, 1, 1,
//        1,  -8, 1,
//        1, 1, 1
//    );
//    
//    vec3 sampleTex[9];
//    for(int i = 0; i < 9; i++)
//    {
//        sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));
//    }
//    vec3 col = vec3(0.0);
//    for(int i = 0; i < 9; i++)
//        col += sampleTex[i] * kernel[i];
//    
//    FragColor = vec4(col, 1.0);
//}  