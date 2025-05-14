#define OpenGLSetFunction(name) (opengl->name) = OpenGLGetFunction(name);
#define OpenGLDeclareMemberFunction(name) OpenGLType_##name name

struct OpenGL
{

    i32 vsynch;
    
    UIRenderGroup* ui_render_group;
    //GLuint glyph_tex;

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

    OpenGLDeclareMemberFunction(glDrawElementsInstanced);
    OpenGLDeclareMemberFunction(glBufferSubData);
    OpenGLDeclareMemberFunction(glBindBufferRange);
};

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
	Str8 shader_full_path = str8_concat(temp.arena, str8("build/"), shader_filename);
	printf("Looking for vert shader at: %.*s\n", (u32)shader_full_path.size, shader_full_path.str);
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

    
// TODO wtf are these functions names
internal u32
create_program(OpenGL* opengl, Str8 vertex_shader_filename, Str8 fragment_shader_filename, Str8 compute_shader_filename = str8(0, 0))
{
    u32 program_id = 0;
    u32 vertex = create_shader(opengl, vertex_shader_filename, GL_VERTEX_SHADER);
    u32 fragment = create_shader(opengl, fragment_shader_filename, GL_FRAGMENT_SHADER);
    u32 compute = create_shader(opengl, compute_shader_filename, GL_COMPUTE_SHADER);

    u32 ID = opengl->glCreateProgram();
    program_id = ID;

    opengl->glAttachShader(ID, vertex);
    opengl->glAttachShader(ID, fragment);
    opengl->glLinkProgram(ID);
    if(!opengl_shader_check_compile_errors(opengl, ID, GL_PROGRAM)){
        opengl->glDeleteShader(vertex);
        opengl->glDeleteShader(fragment);
        opengl->glDeleteShader(compute);
        program_id = 0;
    }

    return program_id;
}

internal void
init_ui(OpenGL *opengl, UIRenderGroup* render_group)
{
    u32 program_id = create_program(opengl, str8("ui_quad.vs.glsl"), str8("ui_quad.fs.glsl"));
    if(program_id)
    {
        render_group->program_id = program_id;

        opengl->glGenVertexArrays(1, &render_group->vao);
        opengl->glGenBuffers(1, &render_group->vbo);
        opengl->glGenBuffers(1, &render_group->ebo);
        glGenTextures(1, &render_group->tex);

        opengl->glBindVertexArray(render_group->vao);

        opengl->glBindBuffer(GL_ARRAY_BUFFER, render_group->vbo);
        opengl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, render_group->ebo);

        // position attribute
        opengl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (void*)OffsetOf(UIVertex, p));
        // uv attribute
        opengl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (void*)OffsetOf(UIVertex, uv));
        // color attribute
        opengl->glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(UIVertex), (void*)OffsetOf(UIVertex, c));

        opengl->glEnableVertexAttribArray(0);
        opengl->glEnableVertexAttribArray(1);
        opengl->glEnableVertexAttribArray(2);

        glBindTexture(GL_TEXTURE_2D, render_group->tex);
        {
            constexpr int W = 2, H = 2;
            u8 dummy[W*H*4] = 
            {
                255, 0, 0, 255,
                0, 255, 0, 255, 
                0, 0, 255, 255,
                255, 255, 0, 255,
            };

            // upload into the texture
            glTexImage2D(GL_TEXTURE_2D,
                        0,                // mip level
                        GL_RGBA8,         // internal format
                        W, H,
                        0,                // border
                        GL_RGBA,          // data format
                        GL_UNSIGNED_BYTE, // data type
                        dummy);

            // set filtering & wrap modes
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        render_group->ortho_proj = opengl->glGetUniformLocation(render_group->program_id, "ortho_proj");
        render_group->texture_sampler = opengl->glGetUniformLocation(render_group->program_id, "texture_sampler");

        opengl->glBindVertexArray(0);
    }
}

internal void
opengl_shader_ui_begin(OpenGL *opengl, UIRenderGroup *render_group)
{
    glDisable(GL_DEPTH_TEST);
    opengl->glUseProgram(render_group->program_id);
	opengl->glBindVertexArray(render_group->vao);

	opengl->glBindBuffer(GL_ARRAY_BUFFER, render_group->vbo);
	opengl->glBufferData(GL_ARRAY_BUFFER, render_group->vertex_count * sizeof(UIVertex), render_group->vertex_array, GL_STATIC_DRAW);

    opengl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, render_group->ebo);
    opengl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u16) * render_group->index_count, render_group->index_array, GL_STATIC_DRAW);

    // TODO see how im passing args!
    // glUniformMatrix4fv(p->orthographic, 1, GL_FALSE, (float*)&rg->orthographic);
    opengl->glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, render_group->tex);
}

internal void
opengl_shader_ui_end(OpenGL* opengl)
{
    glEnable(GL_DEPTH_TEST);
    opengl->glBindVertexArray(0);
    opengl->glUseProgram(0);
}

internal void
opengl_free_resources(OpenGL *opengl)
{
    UIRenderGroup *ui_rg = opengl->ui_render_group;
    opengl->glDeleteProgram(ui_rg->program_id);

    opengl->glDeleteVertexArrays(1, &ui_rg->vao);
    opengl->glDeleteBuffers(1, &ui_rg->vbo);
    opengl->glDeleteBuffers(1, &ui_rg->ebo);

    glDeleteTextures(1, &ui_rg->tex);
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