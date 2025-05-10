# Get the version
# build it jaaaaaa
a new version of asssimp may break the version im using on my engine!

so i could:
- first try to build that version
- ignore what ive done in my engine and proceed reading the animation book and use that version
  instead! this will take a lot of time!
- Upgrade my engine to the version of the books and then everything will have the same vesrion. then build

This last one is probably what i should do! Maybe even use the same version as the vulkan book or maybe one that is 
close enough

will go with: "Upgrade my engine to the version of the books and then everything will have the same version"

I mean... I can even go with using cmake for the building. Which actually should be better
the problem with this is that i have to use compatible command line args for building my engine

Plan: Upgrade engine and see if it works, using cmake separate the assimp building to another folder (is almost done),
put it here and test! (test with my engine code).

What I did was to use the same assimp build Im using for the book "Mastering C animations" and ported my engine and it worked.

Now I have to clean up.
Im going to study what i've done and then do a copy and paste and start following from the book on my own
Im unsure on how im going to do it yet

OLD REASONING!
------------

Okay this is what I did
- I tried the same version of assimp I compiled in the book in my og engine and it worked
- Used the same assimp version im compiling in the book in my new engine.
- It works but its using instance drawing but with one model so it makes no sense. Doesn't use textures, materials, bones, animations. And also its using the std fully

TODO:
- When instance count is 1 use the regular draw, maybe its not useful though, evaluate!
- Clean up first (replace std as well, move to arena based, rething shader management)
- Take extensive notes of skinning, sadly! 
- Add textures and materials
- Add bones and animations
- Add instancing. Make the SSBO works, right now all these things are ignored because im using the identity matrix for everything, and because there are no animations it doesnt matter.
    Because I dont have animations im using the non-animated model ssbo, there are 2. And because there is no instancing im just using my regular shader which doesnt use a SSBO!!!

Some ambitious things:
- Maybe I should just do assimp as a DLL and build only the gltf and fbx loader
- Use my own math library for everything but quaternions
- Move away from Assimp, create my own format
- Create a DLL for the game. Maybe this must be done faster than I think. The compilation times are really bad!

