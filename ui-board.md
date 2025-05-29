# Introduction!
Here are the possible changes for the UI

-- Fonts --
DONE render a letter
    - get the pixel info of a letter 
    - put it into a texture
    - show it
DONE render all alphabets letter (A-z)
DONE use uvs to show specific letters!
DONE implement the push_text function for strings
    (not account for multiline strings at the moment)
DONE use all the glyphs
TODO Fix the fucking mess with push_line requiring a persp and view matrices!! hence destroying the text rendering. UI, Debug, Game => models | regular primitives like lines, triangles, etc
TODO Inspect how, if i have a debug state, will i use it, because its a second pass and sometimes when im updating the game i want to debug... do some research!


TODO take extensive notes!
TODO cleanup the texture atlas logic (names, codepoints and glyphs logic, etc)

-- UI -- 
TODO the ui componenets structs, functions, initializers are all over the place!
TODO make it so the commands are not persistent each frame! Not really immediate
TODO change ui ortho matrix for 2D alternative (my own)
