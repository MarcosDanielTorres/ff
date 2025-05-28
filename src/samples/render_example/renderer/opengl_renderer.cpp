#define OpenGLSetFunction(name) (opengl->name) = OpenGLGetFunction(name);
#define OpenGLDeclareMemberFunction(name) OpenGLType_##name name

struct OpenGL
{

    i32 vsynch;
    
    OpenGLDeclareMemberFunction(wglCreateContextAttribsARB);
    OpenGLDeclareMemberFunction(wglGetExtensionsStringEXT);
    OpenGLDeclareMemberFunction(wglSwapIntervalEXT);
    OpenGLDeclareMemberFunction(wglGetSwapIntervalEXT);
    OpenGLDeclareMemberFunction(glGenVertexArrays);
    OpenGLDeclareMemberFunction(glBindVertexArray);
    OpenGLDeclareMemberFunction(glDeleteVertexArrays);

	OpenGLDeclareMemberFunction(glGenBuffers);
	OpenGLDeclareMemberFunction(glBindBuffer);
	OpenGLDeclareMemberFunction(glBufferData);
    OpenGLDeclareMemberFunction(glBindBufferBase);
    OpenGLDeclareMemberFunction(glDeleteBuffers);
	OpenGLDeclareMemberFunction(glVertexAttribPointer);
	OpenGLDeclareMemberFunction(glVertexAttribIPointer);
	OpenGLDeclareMemberFunction(glEnableVertexAttribArray);

    OpenGLDeclareMemberFunction(glCreateShader);
    OpenGLDeclareMemberFunction(glCompileShader);
    OpenGLDeclareMemberFunction(glShaderSource);
    OpenGLDeclareMemberFunction(glCreateProgram);
    OpenGLDeclareMemberFunction(glAttachShader);
    OpenGLDeclareMemberFunction(glLinkProgram);
    OpenGLDeclareMemberFunction(glDeleteShader);
    OpenGLDeclareMemberFunction(glDeleteProgram);
    OpenGLDeclareMemberFunction(glUseProgram);
    OpenGLDeclareMemberFunction(glGetShaderiv);
    OpenGLDeclareMemberFunction(glGetShaderInfoLog);
    OpenGLDeclareMemberFunction(glGetProgramiv);
    OpenGLDeclareMemberFunction(glGetProgramInfoLog);

    OpenGLDeclareMemberFunction(glUniform1i);
    OpenGLDeclareMemberFunction(glUniform1f);
    OpenGLDeclareMemberFunction(glUniform2fv);
    OpenGLDeclareMemberFunction(glUniform2f);
    OpenGLDeclareMemberFunction(glUniform3fv);
    OpenGLDeclareMemberFunction(glUniform3f);
    OpenGLDeclareMemberFunction(glUniform4fv);
    OpenGLDeclareMemberFunction(glUniform4f);
    OpenGLDeclareMemberFunction(glUniformMatrix2fv);
    OpenGLDeclareMemberFunction(glUniformMatrix3fv);
    OpenGLDeclareMemberFunction(glUniformMatrix4fv);
    OpenGLDeclareMemberFunction(glGetUniformLocation);

    OpenGLDeclareMemberFunction(glActiveTexture);
    OpenGLDeclareMemberFunction(glGenerateMipmap);

    OpenGLDeclareMemberFunction(glDrawElementsInstanced);
    OpenGLDeclareMemberFunction(glBufferSubData);
    OpenGLDeclareMemberFunction(glBindBufferRange);

    OpenGLDeclareMemberFunction(glDebugMessageCallback);
    OpenGLDeclareMemberFunction(glDebugMessageControl);

    OpenGLDeclareMemberFunction(glDispatchCompute);
    OpenGLDeclareMemberFunction(glMemoryBarrier);
};



internal
void WINAPI opengl_debug_callback(GLenum source,
                                           GLenum type,
                                           GLuint id,
                                           GLenum severity,
                                           GLsizei length,
                                           const GLchar* message,
                                           const void* userParam)
{
    // Ignore notifications, only show warnings/errors
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;

    const char* _source = "";
    switch (source) {
        case GL_DEBUG_SOURCE_API:             _source = "API";            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   _source = "Window System";  break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: _source = "Shader Compiler";break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     _source = "Third Party";    break;
        case GL_DEBUG_SOURCE_APPLICATION:     _source = "Application";    break;
        case GL_DEBUG_SOURCE_OTHER:           _source = "Other";          break;
    }

    const char* _type = "";
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:               _type = "Error";             break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: _type = "Deprecated";        break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  _type = "Undefined Behavior";break;
        case GL_DEBUG_TYPE_PORTABILITY:         _type = "Portability";       break;
        case GL_DEBUG_TYPE_PERFORMANCE:         _type = "Performance";       break;
        case GL_DEBUG_TYPE_MARKER:              _type = "Marker";            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          _type = "Push Group";        break;
        case GL_DEBUG_TYPE_POP_GROUP:           _type = "Pop Group";         break;
        case GL_DEBUG_TYPE_OTHER:               _type = "Other";             break;
    }

    const char* _severity = "";
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:         _severity = "High";       break;
        case GL_DEBUG_SEVERITY_MEDIUM:       _severity = "Medium";     break;
        case GL_DEBUG_SEVERITY_LOW:          _severity = "Low";        break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: _severity = "Notification";break;
    }

    fprintf(stderr,
            "GL DEBUG [%s] Type=%s, Severity=%s, ID=%u:\n%s\n\n",
            _source, _type, _severity, id, message);
}

internal
void opengl_enable_debug(OpenGL *opengl)
{
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    opengl->glDebugMessageCallback(opengl_debug_callback, NULL);

    // Optionally filter out notifications:
    opengl->glDebugMessageControl(
        GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION,
        0, NULL, GL_FALSE);
}

internal GLint
opengl_shader_check_compile_errors(OpenGL *opengl, GLuint shader, GLenum type)
{
    GLint success;
    GLchar infoLog[1024];

    switch(type)
    {
        case GL_VERTEX_SHADER:
        {
            opengl->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                opengl->glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                printf("ERROR::PROGRAM_LINKING_ERROR of type: GL_VERTEX_SHADER\n%s\n -- --------------------------------------------------- -- ", infoLog);
            }
        }break;
        case GL_FRAGMENT_SHADER:
        {
            opengl->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                opengl->glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                printf("ERROR::PROGRAM_LINKING_ERROR of type: GL_FRAGMENT_SHADER\n%s\n -- --------------------------------------------------- -- ", infoLog);
            }

        }break;
        case GL_COMPUTE_SHADER:
        {
            opengl->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                opengl->glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                printf("ERROR::PROGRAM_LINKING_ERROR of type: GL_COMPUTE_SHADER\n%s\n -- --------------------------------------------------- -- ", infoLog);
            }
        }break;
        case GL_PROGRAM:
        {
            opengl->glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                opengl->glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                printf("ERROR::PROGRAM_LINKING_ERROR of type: GL_PROGRAM\n%s\n -- --------------------------------------------------- -- ", infoLog);
            }

        }break;
    }
    return success;
}

internal u32
create_shader(OpenGL *opengl, Str8 shader_filename, GLenum shader_type)
{
    TempArena temp = temp_begin(&g_transient_arena);
    u32 shader_id = 0;
	Str8 shader_full_path = str8_concat(temp.arena, str8("src/samples/render_example/shader/"), shader_filename);
	printf("Looking for shader at: %.*s\n", (u32)shader_full_path.size, shader_full_path.str);
	const char *c_shader_path = str8_to_cstring(temp.arena, shader_full_path);
	OS_FileReadResult shader_file = os_file_read(temp.arena, c_shader_path);
    if(shader_file.data)
    {

        const char* shader_code = (const char*)shader_file.data;
        shader_id = opengl->glCreateShader(shader_type);
        opengl->glShaderSource(shader_id, 1, &shader_code, NULL);
        opengl->glCompileShader(shader_id);
        if(!opengl_shader_check_compile_errors(opengl, shader_id, shader_type))
        {
            shader_id = 0;
        }
    }
    temp_end(temp);
    return shader_id;

}

internal u32
create_program(OpenGL* opengl, Str8 vertex_shader_filename = str8(0, 0), Str8 fragment_shader_filename = str8(0, 0), Str8 compute_shader_filename = str8(0, 0))
{
    u32 program_id = 0;
    u32 vertex = create_shader(opengl, vertex_shader_filename, GL_VERTEX_SHADER);
    u32 fragment = create_shader(opengl, fragment_shader_filename, GL_FRAGMENT_SHADER);
    u32 compute = create_shader(opengl, compute_shader_filename, GL_COMPUTE_SHADER);

    u32 ID = opengl->glCreateProgram();
    program_id = ID;

    if (vertex)
    {
        opengl->glAttachShader(ID, vertex);
    }

    if (fragment)
    {
        opengl->glAttachShader(ID, fragment);
    }

    if (compute)
    {
        opengl->glAttachShader(ID, compute);
    }

    opengl->glLinkProgram(ID);
    if(!opengl_shader_check_compile_errors(opengl, ID, GL_PROGRAM))
    {
        program_id = 0;
    }
    opengl->glDeleteShader(vertex);
    opengl->glDeleteShader(fragment);
    opengl->glDeleteShader(compute);

    return program_id;
}

struct TestPackerResult
{
    u8* data;
    size_t size;
    u32 width;
    u32 height;
};

internal TestPackerResult
test_packer(UIState *ui_state)
{
    TestPackerResult result = {};
    u32 result_width;
    u32 result_height;
    u32 result_size;
    u8* result_data;

    OS_FileReadResult font_file = os_file_read(&g_arena, "C:\\Windows\\Fonts\\CascadiaMono.ttf");
    FT_Face face = {0};
    if(font_file.data)
    {
        FT_Open_Args args = {0};
        args.flags = FT_OPEN_MEMORY;
        args.memory_base = (u8*) font_file.data;
        args.memory_size = font_file.size;

        FT_Error opened_face = FT_Open_Face(library, &args, 0, &face);
        if (opened_face) 
        {
            const char* err_str = FT_Error_String(opened_face);
            printf("FT_Open_Face: %s\n", err_str);
            exit(1);
        }


        FT_Error set_char_size_err = FT_Set_Char_Size(face, 4 * 64, 4 * 64, 300, 300);
        // NOTE If `glyph_count` is not even everything goes to hell!!!
        // TODO fix this  hahah wtf is going on!?!?!?
        u32 glyph_count = u32('~') - u32('!') + 1;
        u32 glyphs_per_row = round(sqrtf(glyph_count));
        glyphs_per_row = 14;
        //glyph_count = 8;
        printf("Glyph count: %d\n", glyph_count);
        printf("Glyphs per row: %d\n", glyphs_per_row);
        u32 max_height_per_cell = face->size->metrics.height >> 6;
        u32 max_width_per_cell = face->size->metrics.max_advance >> 6;

        //u32 atlas_width = max_width_per_cell * 2;
        //u32 atlas_height = max_height_per_cell * 2;

        u32 texture_atlas_rows = round(glyph_count / glyphs_per_row);
        u32 margin_per_glyph = 2;
        u32 texture_atlas_left_padding = 2;

        result_width = (max_width_per_cell + margin_per_glyph + texture_atlas_left_padding)  * (glyphs_per_row + 3);
        result_height = max_height_per_cell * (texture_atlas_rows + 3);
        result_size = result_width * result_height;
        result_data = (u8*) arena_push_size(&g_arena, u8, result_size); // space for 4 glyphs

        u32 char_code = 0;
        u32 min_index = UINT32_MAX;
        u32 max_index = 0;
        for(;;) 
        {
            u32 glyph_index = 0;
            char_code = FT_Get_Next_Char(face, char_code, &glyph_index);
            if (char_code == 0) 
            {
                break;
            }
            // here each char_code corresponds to ascii representantion. Also the glyph index is the same as if I did:
            // `FT_Get_Char_Index(face, 'A');`. So A would be: (65, 36)
            //printf("(%d, %d)\n", char_code, glyph_index);
            min_index = Min(min_index, glyph_index);
            max_index = Max(max_index, glyph_index);

        }
        printf("min and max indexes: (%d, %d)\n", min_index, max_index);

        u32 prev_h = 0;
        u32 glyph_start_x = texture_atlas_left_padding;
        u32 glyph_start_y = 0;
        u32 count = 0;
        //for(u32 glyph_index = min_index; glyph_index <= max_index; glyph_index++) 
        for(u32 codepoint = '!'; codepoint <= '~'; codepoint++) 
        {

            if((count % (glyphs_per_row + 1)) == 0)
            {
                glyph_start_x = texture_atlas_left_padding;
                glyph_start_y += max_height_per_cell;
                count = 0;
            }
            u32 glyph_index = FT_Get_Char_Index(face, char(codepoint));

            if (glyph_index)
            {
                FT_Error load_glyph_err = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
                if (load_glyph_err) 
                {
                    const char* err_str = FT_Error_String(load_glyph_err);
                    printf("FT_Load_Glyph: %s\n", err_str);
                }

                FT_Error render_glyph_err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
                if (render_glyph_err) 
                {
                    const char* err_str = FT_Error_String(render_glyph_err);
                    printf("FT_Load_Glyph: %s\n", err_str);
                }

                u8 *pixel = result_data + glyph_start_x + glyph_start_y * (result_width);

                u8 *ptr_uv3 = pixel;
                u8 *ptr_uv0 = pixel + (face->glyph->bitmap.rows - 1) * result_width;
                u8 *ptr_uv1 = pixel + face->glyph->bitmap.width - 1 + (face->glyph->bitmap.rows - 1) * result_width;
                u8 *ptr_uv2 = pixel + face->glyph->bitmap.width - 1;

                u8 *src_buffer = face->glyph->bitmap.buffer;
                u32 y_offset = glyph_start_y;
                for(u32 y = 0; y < face->glyph->bitmap.rows; y++)
                {
                    for(u32 x = 0; x < face->glyph->bitmap.width; x++)
                    {
                        *pixel++ = *src_buffer++;
                    }

                    y_offset += 1;
                    pixel = result_data + glyph_start_x + y_offset * (result_width);
                }
                prev_h = face->glyph->bitmap.rows;

                /*
                glm::vec2 a_uv0 = glm::vec2(0.0084, 0.2256);
                glm::vec2 a_uv1 = glm::vec2(0.0462, 0.2256);
                glm::vec2 a_uv3 = glm::vec2(0.0084, 0.1429);
                glm::vec2 a_uv2 = glm::vec2(0.0462, 0.1429);
                I know the total width
                I know the total height
                v0 = 
                */

                // for debug!!!
                #if 0
                *ptr_uv3 = 0xFF;
                *ptr_uv2 = 0xFF;
                *ptr_uv1 = 0xFF;
                *ptr_uv0 = 0xFF;
                #endif


                // this one seemed promising but only works for the height and also.. i thought 
                // this would have calclated thi value for width... so yeah
                // f32(uv3 - texture_atlas_start) / f32((max_width_per_cell + margin_per_glyph + texture_atlas_left_padding)  * (glyphs_per_row + 3))


                u32 start_x = glyph_start_x;
                int x0 = glyph_start_x;
				int x1 = glyph_start_x + face->glyph->bitmap.width ;
				int y0 = glyph_start_y;
				int y1 = glyph_start_y + face->glyph->bitmap.rows ;

				float u0 = float(x0) / float(result_width);
				float v0 = float(y0) / float(result_height);

				float u1 = float(x1) / float(result_width);
				float v1 = float(y1) / float(result_height);

                //f32 f_uv3_x = start_x / f32(result_width);
                //f32 f_uv3_x = f32((uv3 - result_data) % result_width) / f32(result_width);
                //f32 f_uv3_y = (uv3 - result_data) / result_width / f32(result_height);
                f32 f_uv3_x = u0;
                f32 f_uv3_y = v0;

                //f32 f_uv2_x = start_x / f32(result_width);
                //f32 f_uv2_x = f32((uv2 - result_data) % result_width) / f32(result_width);
                //f32 f_uv2_y = (uv2 - result_data) / result_width / f32(result_height);
                f32 f_uv2_x = u1;
                f32 f_uv2_y = v0;

                //f32 f_uv1_x = start_x / f32(result_width) + (face->glyph->bitmap.width) / f32(result_width);
                //f32 f_uv1_x = f32((uv1 - result_data) % result_width) / f32(result_width);
                //f32 f_uv1_y = (uv1 - result_data) / result_width / f32(result_height);
                f32 f_uv1_x = u1;
                f32 f_uv1_y = v1;

                //f32 f_uv0_x = start_x / f32(result_width) + (face->glyph->bitmap.width) / f32(result_width);
                //f32 f_uv0_x = f32((uv0 - result_data) % result_width) / f32(result_width);
                //f32 f_uv0_y = (uv0 - result_data) / result_width / f32(result_height);
                f32 f_uv0_x = u0;
                f32 f_uv0_y = v1;

                FontGlyph *glyph = &ui_state->font_info.font_table[codepoint];

                glyph->uv0_x = f_uv0_x;
                glyph->uv1_x = f_uv1_x;
                glyph->uv2_x = f_uv2_x;
                glyph->uv3_x = f_uv3_x;

                glyph->uv0_y = f_uv0_y;
                glyph->uv1_y = f_uv1_y;
                glyph->uv2_y = f_uv2_y;
                glyph->uv3_y = f_uv3_y;

                glyph_start_x += max_width_per_cell + margin_per_glyph + face->glyph->bitmap_left;
            }
            count++;
        }
    }
    result.width = result_width;
    result.height = result_height;
    result.size = result_size;


    result.data = result_data; // space for 4 glyphs
    return result;
}
/*
    When rendering I want the rect to be as tall as line height, everyone will share the same height!
    The width of the rectangle is going to be equal to the width of the char, for monospace
    (I wont worry about kerning for now!)

    When texture packing:
    I can make it so the texture is max_advance * glyph_count but when actually filling it
    i will use the right dimensions for each glyph. So there will be some space wasted in the texture.
    The thing is I believe is going to be easier to render, but maybe not!

    I will have to deal with centering the letter inside the rectangle of height height

*/

internal void
init_ui(OpenGL *opengl, UIState *ui_state)
{
    u32 program_id = create_program(opengl, str8("ui_quad.vs.glsl"), str8("ui_quad.fs.glsl"));
    if(program_id)
    {
        ui_state->program_id = program_id;

        opengl->glGenVertexArrays(1, &ui_state->vao);
        opengl->glGenBuffers(1, &ui_state->vbo);
        opengl->glGenBuffers(1, &ui_state->ebo);
        glGenTextures(1, &ui_state->tex);

        opengl->glBindVertexArray(ui_state->vao);

        opengl->glBindBuffer(GL_ARRAY_BUFFER, ui_state->vbo);
        opengl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_state->ebo);

        // position attribute
        opengl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (void*)OffsetOf(UIVertex, p));
        // uv attribute
        opengl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (void*)OffsetOf(UIVertex, uv));
        // color attribute
        opengl->glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (void*)OffsetOf(UIVertex, c));

        opengl->glEnableVertexAttribArray(0);
        opengl->glEnableVertexAttribArray(1);
        opengl->glEnableVertexAttribArray(2);

        // NOTE this is going to be done outside this time, because this is the texture where the fonts will be...
        // this is provisory!
        // Only one glyph for now!

        TestPackerResult texture_atlas = test_packer(ui_state);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glBindTexture(GL_TEXTURE_2D, ui_state->tex);
        {
            int W = texture_atlas.width, H = texture_atlas.height;
            glTexImage2D(GL_TEXTURE_2D,
                        0,                // mip level
                        //GL_RGBA8,         // internal format
                        GL_R8,
                        W, H,
                        0,                // border
                        GL_RED,          // data format
                        //GL_RED,          // data format
                        GL_UNSIGNED_BYTE, // data type
                        texture_atlas.data);

            // set filtering & wrap modes
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

            #if 0
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            FontGlyph glyph = ui_state->font_info.font_table[u32('A')];
            int W = glyph.bitmap.width, H = glyph.bitmap.height;
            glTexImage2D(GL_TEXTURE_2D,
                        0,                // mip level
                        //GL_RGBA8,         // internal format
                        GL_RED,
                        W, H,
                        0,                // border
                        GL_RED,          // data format
                        //GL_RED,          // data format
                        GL_UNSIGNED_BYTE, // data type
                        glyph.bitmap.buffer);

            // set filtering & wrap modes
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
            #endif
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        ui_state->ortho_proj = opengl->glGetUniformLocation(ui_state->program_id, "ortho_proj");
        ui_state->texture_sampler = opengl->glGetUniformLocation(ui_state->program_id, "texture_sampler");

        opengl->glBindVertexArray(0);
    }
}

internal void
begin_ui_frame(OpenGL *opengl, UIState *ui_state, UIRenderGroup *render_group)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    opengl->glUseProgram(ui_state->program_id);
	opengl->glBindVertexArray(ui_state->vao);

	opengl->glBindBuffer(GL_ARRAY_BUFFER, ui_state->vbo);
	opengl->glBufferData(GL_ARRAY_BUFFER, render_group->vertex_count * sizeof(UIVertex), render_group->vertex_array, GL_STATIC_DRAW);

    opengl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ui_state->ebo);
    opengl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u16) * render_group->index_count, render_group->index_array, GL_STATIC_DRAW);

    // TODO see how im passing args!
    // glUniformMatrix4fv(p->orthographic, 1, GL_FALSE, (float*)&rg->orthographic);
    opengl->glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ui_state->tex);
}

internal void
end_ui_frame(OpenGL* opengl)
{
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, 0);
    opengl->glBindVertexArray(0);
    opengl->glUseProgram(0);
}

#if 0
internal void
opengl_free_resources(OpenGL *opengl, ...)
{
    UIRenderGroup *ui_rg = opengl->ui_render_group;
    opengl->glDeleteProgram(ui_rg->program_id);

    opengl->glDeleteVertexArrays(1, &ui_rg->vao);
    opengl->glDeleteBuffers(1, &ui_rg->vbo);
    opengl->glDeleteBuffers(1, &ui_rg->ebo);

    glDeleteTextures(1, &ui_rg->tex);
}
#endif

internal
void opengl_init(OpenGL *opengl, OS_Window window)
{
    
    opengl->vsynch = -1;

    HDC hdc = GetDC(window.handle);
    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixel_format = ChoosePixelFormat(hdc, &pfd);
    if(pixel_format) {
        SetPixelFormat(hdc, pixel_format, &pfd);
    }else{
        __debugbreak();
    }

    HGLRC tempRC = wglCreateContext(hdc);
    if(wglMakeCurrent(hdc, tempRC))
    {
        // NOTE It seems that in order to get anything from `wglGetProcAddress`, `wglMakeCurrent` must have been called!
        //wglCreateContextAttribsARB = OpenGLGetFunction(wglCreateContextAttribsARB);
        OpenGLSetFunction(wglCreateContextAttribsARB)

        i32 attrib_list[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 6,
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
            WGL_CONTEXT_PROFILE_MASK_ARB,
            WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0
        };
        HGLRC hglrc = opengl->wglCreateContextAttribsARB(hdc, 0, attrib_list);
        wglMakeCurrent(0, 0);
        wglDeleteContext(tempRC);
        wglMakeCurrent(hdc, hglrc);
    }
    OpenGLSetFunction(wglGetExtensionsStringEXT);
    {
        const char* extensions = opengl->wglGetExtensionsStringEXT();
        b32 swap_control_supported = false;
		const char* at = extensions;
        while(*at) 
        {
            while(*at == ' ') {
                at++;
            }

            const char* start = at;
            while(*at && *at != ' ') {
                at++; 
            }
            if (!*at) {
                break;
            }
            size_t count = at - start;
            Str8 extension = str8((char*)start, count);
            b32 res1 = str8_equals(extension, str8_lit("WGL_EXT_framebuffer_sRGB"));
            b32 res2 = str8_equals(extension, str8_lit("WGL_ARB_framebuffer_sRGB"));
            if(str8_equals(extension, str8_lit("WGL_EXT_swap_control")))
            {
                swap_control_supported = true;
            }

        }
        if(swap_control_supported) {
            OpenGLSetFunction(wglSwapIntervalEXT);
            OpenGLSetFunction(wglGetSwapIntervalEXT);
            // vsync: enable = 1, disable = 0
            if(opengl->wglSwapIntervalEXT(0)) 
            {
                opengl->vsynch = opengl->wglGetSwapIntervalEXT();
            }
        }
    }

    OpenGLSetFunction(glGenVertexArrays);
    OpenGLSetFunction(glBindVertexArray);
    OpenGLSetFunction(glDeleteVertexArrays);

	OpenGLSetFunction(glGenBuffers);
	OpenGLSetFunction(glBindBuffer);
	OpenGLSetFunction(glBufferData);
    OpenGLSetFunction(glBindBufferBase);
    OpenGLSetFunction(glDeleteBuffers);
	OpenGLSetFunction(glVertexAttribPointer);
	OpenGLSetFunction(glVertexAttribIPointer);
	OpenGLSetFunction(glEnableVertexAttribArray);

    OpenGLSetFunction(glCreateShader);
    OpenGLSetFunction(glCompileShader);
    OpenGLSetFunction(glShaderSource);
    OpenGLSetFunction(glCreateProgram);
    OpenGLSetFunction(glAttachShader);
    OpenGLSetFunction(glLinkProgram);
    OpenGLSetFunction(glDeleteShader);
    OpenGLSetFunction(glDeleteProgram);
    OpenGLSetFunction(glUseProgram);
    OpenGLSetFunction(glGetShaderiv);
    OpenGLSetFunction(glGetShaderInfoLog);
    OpenGLSetFunction(glGetProgramiv);
    OpenGLSetFunction(glGetProgramInfoLog);

    OpenGLSetFunction(glUniform1i);
    OpenGLSetFunction(glUniform1f);
    OpenGLSetFunction(glUniform2fv);
    OpenGLSetFunction(glUniform2f);
    OpenGLSetFunction(glUniform3fv);
    OpenGLSetFunction(glUniform3f);
    OpenGLSetFunction(glUniform4fv);
    OpenGLSetFunction(glUniform4f);
    OpenGLSetFunction(glUniformMatrix2fv);
    OpenGLSetFunction(glUniformMatrix3fv);
    OpenGLSetFunction(glUniformMatrix4fv);
    OpenGLSetFunction(glGetUniformLocation);

    OpenGLSetFunction(glActiveTexture);
    OpenGLSetFunction(glGenerateMipmap);

    OpenGLSetFunction(glDrawElementsInstanced);
    OpenGLSetFunction(glBufferSubData);
    OpenGLSetFunction(glBindBufferRange);

    OpenGLSetFunction(glDebugMessageCallback);
    OpenGLSetFunction(glDebugMessageControl);

    OpenGLSetFunction(glDispatchCompute);
    OpenGLSetFunction(glMemoryBarrier);
    opengl_enable_debug(opengl);

}

// TODO
// ver como hace para tener diferentes shaders, supongo que es lo mismo que pense
// obviar las entries por ahora, ver la init y el render y si hay un only run logic
// ver como es el begin shader
// ver como calcula esta mierda: glVertexAttribPointer usan el offsetof() algunos
// ver que hace con el buffer y si hace lo de subdata

//Es confuso porque segun lo que me acuerdo el render group tiene los vao vbo ebo y una serie de commands. Todos estos commands siempre usan los mismo buffers
//entonces estos siempre estan en un render group. En este caso yo tengo como 3 diferentes tipos de vao porque funcionan con diferentes atributos, unos para la ui, otros para el skinning
//entonces aca es diferente, porque el approach de tener los buffers en un rendergroup ya no me sirve. Buffers should be picked by the type of operation im doing, because ui and skinning uses
//different buffers. Although I can always use the same shader for everything, problem is there is lighting, view projection matrixes, a lot of stuff I don't need

/*
    cada entry tiene un batch diferente?
    o no importa la entry y va todo en el mismo batch?

    Porque aca yo tengo ui y tengo modelos tambien, cada modelo tiene su propio vao. Aunque podria meterle una batcheada supongo y no tener cada uno en su vao sino
    todos los modelos en un solo vao?
*/





/*
OpenGL techniques!

4coder:
glBufferData( GL_ARRAY_BUFFER, vertex_count * sizeof(Render_Vertex),
    0,                                                                                                      // Orphan trick!
    GL_STREAM_DRAW);

i32 cursor = 0;                                                                                             // TODO check this cursor
for (Render_Vertex_Array_Node *node = group->vertex_list.first;
        node != 0;
        node = node->next){
    i32 size = node->vertex_count*sizeof(*node->vertices);
    glBufferSubData(GL_ARRAY_BUFFER, cursor, size, node->vertices);
    cursor += size;
}


------------------------------------------------------------------------------------------------------------------------------------------------

ChatGPT

init:
      gl->glBindBuffer(GL_ARRAY_BUFFER, p->vbo);
      gl->glBufferData(GL_ARRAY_BUFFER,
                       maxVertexCount * sizeof(BasicVertex),
                       nullptr,               // no data yet
                       GL_DYNAMIC_DRAW);

      gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p->ebo);
      gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                       maxIndexCount * sizeof(u16),
                       nullptr,
                       GL_DYNAMIC_DRAW);


render:            
    gl->glBindBuffer(GL_ARRAY_BUFFER, p->vbo);
    gl->glBufferSubData(GL_ARRAY_BUFFER,
                        0,
                        rg->vertex_count * sizeof(BasicVertex),
                        rg->vertex_array);

    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p->ebo);
    gl->glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
                        0,
                        rg->index_count * sizeof(u16),
                        rg->index_array);


Casey:

*/