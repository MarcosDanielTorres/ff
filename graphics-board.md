DONE try framebuffers. put the cubes in it, they wont show
DONE change vao, vbo, ebo to GLuint instead of u32
TODO fill a selection texture with ids from the cubes id = idx
    - Create selection texture and attach it to colorattachment 1
    - upload ssbo to skninning shader contaning the ids of the cubes
    - see it in renderdoc or in opengl I want to see the numbers (the colors wont matter as is only one red channel of 32 bits)
    - then read prixel from texture and pixel = id and then box[id == idx] and color it somehow. I could print though first
    - color it
    - Think about how to do it globally, not just cubes but cubes + instancing + the universe