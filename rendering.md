sooooo 
struct OpenGL has many things interesting:
    u8 PushBufferMemory[65536];
    textured_vertex *VertexArray;
    u16 *IndexArray;
    game_render_commands RenderCommands;             <- this one is interesting it's what is called "Commands"



I have to check who uses this. Because all render groups have a *commands but it seems like is the same for all debug, ui, and game rendergroups




game_render_commands *Frame = 0;
if(RendererCode.IsValid)
{
    Frame = RendererFunctions.BeginFrame(
        Renderer, Dimension, RenderDim, DrawRegion);
}

...
if(Game.UpdateAndRender)
{
    Game.UpdateAndRender(&GameMemory, NewInput, Frame);
    if(NewInput->QuitRequested)
    {
        GlobalRunning = false;
    }
}

...
 if(Game.DEBUGFrameEnd)
{
    Game.DEBUGFrameEnd(&GameMemory, NewInput, Frame);
}
...
BEGIN_BLOCK("Frame Display");

if(RendererCode.IsValid)
{
    if(RendererWasReloaded)
    {
        ++Frame->Settings.Version;
        RendererWasReloaded = false;
    }
    RendererFunctions.EndFrame(Renderer, Frame);
}





---------------------------

Esto esta en el GameUpdateAndRender!

    b32 Rerun = false;
    do
    {
        switch(GameState->GameMode)
        {
            case GameMode_TitleScreen:
            {
                Rerun = UpdateAndRenderTitleScreen(GameState, RenderCommands,
                                                   Input, GameState->TitleScreen);
            } break;

            case GameMode_CutScene:
            {
                Rerun = UpdateAndRenderCutScene(GameState, RenderCommands,
                                                Input, GameState->CutScene);
            } break;

            case GameMode_World:
            {
                Rerun = UpdateAndRenderWorld(GameState, GameState->WorldMode,
                                             Input, RenderCommands, &HitTest);
            } break;

            InvalidDefaultCase;
        }
    } while(Rerun);

    EndHitTest(&GameState->Editor, Input, &HitTest);
        
    BeginUIFrame(&GameState->DevUI, GameState->Assets, RenderCommands, Input);
    UpdateAndRenderEditor(&GameState->Editor, &GameState->DevUI, GameState);
    EditorInteract(&GameState->Editor, &GameState->DevUI, Input);
    EndUIFrame(&GameState->DevUI);
    
    DIAGRAM_Reset();





TODO poner 90 breakpoints y chequear que sea el mismo address!



////////.........................///////////////





internal game_render_commands *
OpenGLBeginFrame(open_gl *OpenGL, v2u OSWindowDim, v2u RenderDim, rectangle2i DrawRegion)
{
    game_render_commands *Commands = &OpenGL->RenderCommands; <- TODO see where it is initialized!!!
    
    Commands->Settings = OpenGL->CurrentSettings;
    Commands->Settings.RenderDim = RenderDim;
    
    Commands->OSWindowDim = OSWindowDim;
    Commands->OSDrawRegion = DrawRegion;

    Commands->MaxPushBufferSize = sizeof(OpenGL->PushBufferMemory);
    Commands->PushBufferBase = OpenGL->PushBufferMemory;
    Commands->PushBufferDataAt = OpenGL->PushBufferMemory;

    Commands->MaxVertexCount = OpenGL->MaxVertexCount;
    Commands->VertexCount = 0;
    Commands->VertexArray = OpenGL->VertexArray;

    Commands->MaxIndexCount = OpenGL->MaxIndexCount;
    Commands->IndexCount = 0;
    Commands->IndexArray = OpenGL->IndexArray;
    
    Commands->DiffuseLightAtlas = OpenGL->DiffuseLightAtlas;
    Commands->SpecularLightAtlas = OpenGL->SpecularLightAtlas;
    
    Commands->MaxLightOccluderCount = MAX_LIGHT_OCCLUDER_COUNT;
    Commands->LightOccluderCount = 0;
    Commands->LightOccluders = OpenGL->LightOccluders;
    
    Commands->MaxQuadTextureCount = OpenGL->MaxQuadTextureCount;
    Commands->QuadTextureCount = 0;
    Commands->QuadTextures = OpenGL->BitmapArray;

    return Commands;
}

internal void
OpenGLEndFrame()
{
    does the rendering

    for(u8 *HeaderAt = Commands->PushBufferBase;
            HeaderAt < Commands->PushBufferDataAt;
            )
        {
            render_group_entry_header *Header = (render_group_entry_header *)HeaderAt;
            HeaderAt += sizeof(render_group_entry_header);
            void *Data = (uint8 *)Header + sizeof(*Header);
        }
}


///////// MAIN FLOW ////////////
GameUpdateAndRender calls:
- UpdateAndRenderTitleScreen
- UpdateAndRenderCutScene
- UpdateAndRenderWorld

TODO(debug): see this sequence of calls: 
    - BeginUIFrame(&GameState->DevUI, GameState->Assets, RenderCommands, Input);
    - UpdateAndRenderEditor(&GameState->Editor, &GameState->DevUI, GameState);
    - EditorInteract(&GameState->Editor, &GameState->DevUI, Input);
    - EndUIFrame(&GameState->DevUI);

and others which are not soo important but still.


//////////////////////////////////////////////////////////////
//////////////////////////// RENDERING ///////////////////////
//////////////////////////////////////////////////////////////
The rendering is done in "two steps", first everything is added to a game_commands_buffer belonging to a render_group. Using a series of
Push* calls

Then there is probably there is some sorting here. And then the renderer shows it!



There are 5 calls each frame in total to BeginRenderGroup
2 in UpdateAndRenderTitleScreen
2 in UpdateAndRenderCutScene
1 in UpdateAndRenderWorld crea
1 in BeginUIFrame crea debug/ui



 Hay un solo RenderGroup por aplicacion, Debug, titlescreen, cutscenes.
// RenderGroup -> Commands, pero a EndFrame entran los Commands, mientras que en PushElement entra el RenderGroup //
// DEBUGEnd --|
//            |-----------> EndUIFrame -> EndRenderGroup(&UI->RenderGroup)
// GameUpdateAndRender -| 


Differences in the Begin/EndUIFrame calls:
Caso 1:
- DEBUGGameFrameEnd -> DEBUGEnd -> EndUIFrame
- DEBUGGameFrameStart -> DEBUGStart -> BeginUIFrame

Caso 2:
- GameUpdateAndRender -> BeginUIFrame y despues EndUIFrame

La diferencia entre los casos 1 y 2 es que el 1 usa DebugState->DevUI y el 2 usa GameState->DevUI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PushRenderElement(Group, type) (type *)PushRenderElement_(Group, sizeof(type), RenderGroupEntryType_##type)
inline void *
PushRenderElement_(render_group *Group, uint32 Size, render_group_entry_type Type)
{
    game_render_commands *Commands = Group->Commands;
    
    void *Result = 0;
    
    Size += sizeof(render_group_entry_header);
    push_buffer_result Push = PushRenderBuffer(Group, Size);
    if(Push.Header)
    {
        render_group_entry_header *Header = Push.Header;
        Header->Type = (u16)Type;
        Result = (uint8 *)Header + sizeof(*Header);
#if HANDMADE_SLOW
        Header->DebugTag = Group->DebugTag;
#endif
    }
    else
    {
        InvalidCodePath;
    }
    
    Group->CurrentQuads = 0;
    
    return(Result);
}

inline push_buffer_result
PushRenderBuffer(render_group *RenderGroup, u32 DataSize)


enum render_group_entry_type
{
    RenderGroupEntryType_render_entry_textured_quads,
    RenderGroupEntryType_render_entry_full_clear,
    RenderGroupEntryType_render_entry_depth_clear,
    RenderGroupEntryType_render_entry_begin_peels,
    RenderGroupEntryType_render_entry_end_peels,
};
struct render_entry_textured_quads
{
    render_setup Setup;
    u32 QuadCount;
    u32 VertexArrayOffset; // NOTE(casey): Uses 4 vertices per quad
};

struct render_entry_blend_render_target
{
    u32 SourceTargetIndex;
    r32 Alpha;
};

struct render_entry_full_clear
{
    v4 ClearColor; // NOTE(casey): This color is NOT in linear space, it is in sRGB space directly?
};

struct render_entry_begin_peels
{
    v4 ClearColor; // NOTE(casey): This color is NOT in linear space, it is in sRGB space directly?
};

struct render_entry_lighting_transfer
{
    v4 *LightData0;
    v4 *LightData1;
};

struct render_group_entry_header
{
    u16 Type;
};


struct render_setup
{
    m4x4 Proj;
    v3 CameraP;
    ...
}


TODO
 check when does the proj matrix gets set and why is it set on every textured entry when it should be mostly the same. how many entties having this proj matrix 
 are there

struct render_setup has the matrix and cameraP

render_setup is important because is where the projection * view matrix is embbeded

This is the only entry that uses it: RenderGroupEntryType_render_entry_textured_quads when rendering.


The thing with PushSetup is weird. Because both the render_group and the entry_textured_quads has a setup, but the render_group always save the last one.
Now I have to check when is the entry setup set and used, and when is the render group setup set and use!

- When is the entry's setup set?
    is set on GetCurrentQuads(): 
    Group->CurrentQuads = (render_entry_textured_quads *) PushRenderElement_(...)
    Group->CurrentQuads->QuadCount = 0;
    Group->CurrentQuads->VertexArrayOffset = Commands->VertexCount;
    Group->CurrentQuads->IndexArrayOffset = Commands->IndexCount;
    Group->CurrentQuads->Setup = Group->LastSetup;
    Group->CurrentQuads->QuadTextures = 0;

- When is the entry's setup used?
When rendering

- When is the group's setup set?
CameraSetTransform
- When is the group's setup used?
Never in its own, is used when setting the entry's render_setup

PushSetup is only relevantly called from BeginRenderGroup to set the inital setup and on SetCameraTransform
PushRenderElement_ .And is done after the camera has been pushed to the render_group so that the setup is correct!



PushRect (y todas las demas) llaman a GetCurrentQuads(), which set the Group->CurrentQuads (which is a render_entry_textured_quads* basically an Entry)
Who calls GetCurrentQuads:

All of these call PushQuad:
    - PushLineSegment
    - PushCube
    - PushVolumeOutline
    - PushRect
    - PushUpright
    - PushSprite
TODO when is it used?
- OutputQuads


   if(!Group->CurrentQuads)
    {
        Group->CurrentQuads = (render_entry_textured_quads *)
            PushRenderElement_(Group, sizeof(render_entry_textured_quads), RenderGroupEntryType_render_entry_textured_quads);
        Group->CurrentQuads->QuadCount = 0;
        Group->CurrentQuads->VertexArrayOffset = Commands->VertexCount;
        Group->CurrentQuads->IndexArrayOffset = Commands->IndexCount;
        Group->CurrentQuads->Setup = Group->LastSetup;
        Group->CurrentQuads->QuadTextures = 0;
        if(ThisTextures)
        {
            Group->CurrentQuads->QuadTextures =
                (Commands->QuadTextures + Commands->QuadTextureCount);
        }
    }
    
    render_entry_textured_quads *Result = Group->CurrentQuads;
    return Result;

For rendering:
    render_group_entry_header *Header = (render_group_entry_header *)HeaderAt;
    HeaderAt += sizeof(render_group_entry_header);
    void *Data = (uint8 *)Header + sizeof(*Header);
    render_entry_textured_quads *Entry = (render_entry_textured_quads *)Data;



TODO entry explaination:

Who uses entry_textured_quad:
- PushCube...
- PushCube...
- PushCube...
TODO

This is a pointer to last grouped entry_textured_quad could be 1 quad, could be 6 quads (for a cube for example)
entry_textured_quad, it could mean many or 1 quad
and entry_textured_quad is a batch, when the batch has grown enough, then another batch is created (by ). So a render element can have many elements!

// // // PushRenderElement_ explaination:
Summary: 
params:
render_group* Group, u32 size = sizeof(render_entry_textured_quads), render_group_entry_type Type = RenderGroupEntryType_render_entry_textured_quads

returns: *render_entry_textured_quads

    void* result = 0;

    size += sizeof(render_group_entry_header); This ends up being always 16 + sizeof(render_entry_textured_quads) or whatever else
    // Thats why this works:
    // Header->Type = Type;
    // and then in the render: 
                render_group_entry_header* Header = Commands->PushBufferBase
    //          void* Data = Header + sizeof(*Header);
    push_buffer_result Push = PushRenderBuffer(Group, Size);
    if(Push.Header)
    {
        render_group_entry_header *Header = Push.Header;
        Header->Type = (u16)Type;
        Result = (uint8 *)Header + sizeof(*Header); // this can be done because the size pushed was sizeof(render_group_entry_header) + sizeof(render_entry_textured_quads)
    }
    else
    {
        InvalidCodePath;
    }
    
    Group->CurrentQuads = 0;
    
    return(Result);

inline push_buffer_result (its actually render_group_entry_header*)
PushRenderBuffer(render_group *RenderGroup, u32 DataSize)
{
params:
return:  
    game_render_commands *Commands = RenderGroup->Commands;
    
    push_buffer_result Result = {};
    
    u8 *PushBufferEnd = Commands->PushBufferBase + Commands->MaxPushBufferSize;
    if((Commands->PushBufferDataAt + DataSize) <= PushBufferEnd)
    {
        Result.Header = (render_group_entry_header *)Commands->PushBufferDataAt;
        Commands->PushBufferDataAt += DataSize;
    }
    else
    {
        InvalidCodePath;
    }
    
    return(Result);
}


...
The UseProgramBegin sets the uniform getting their values from the setup


TODO How is he choosing what texture to bind. Is this information on each entry?
        Where are the textures?