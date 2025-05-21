# Animations
Let's see if I can explain this:

## Description of the files
The model(the arms holding the gun): SK_FP_Manny_Simple.fbx
Has an Armature node which has bones:
Armature (NODE)
    root
        pelvis              (78)
        ik_foot_root        (2)
        ik_hand_root        (3)
            ik_hand_gun
                ik_hand_l
                ik_hand_r
        interaction         (0)
        center_of_mass      (0)

Important to remember regarding the AssaultRifle: AssaultRifle.fbx file is the only file with bones. The other components of the rifle move by using the tranforms of the bones after animation (magazine, casing)

SK_AssaultRifle.fbx
Mesh
Grip
    Trigger
    Bolt
    Magazine


# Performing animations:
Each animation is its onw file (its .FBX fault)
## Reloading:
For reloading there are two animations:

A_FP_AssaultRifle_Reload.fbx
Bones:
    root
        pelvis              (78)
        ik_foot_root        (2)
        ik_hand_root        (3)
            ik_hand_gun
                ik_hand_l
                ik_hand_r
        interaction         (0)
        center_of_mass      (0)

(NOTE: its has the exact same bones as the arms model SK_FP_Manny_Simple)
From this animation the transform of the bones corresponding the arms are taken from.


A_FP_WEP_AssaultRifle_Reload.fbx
Bones:
    Grip
        Trigger
        Bolt
        Magazine

(NOTE: its has the exact same bones as the rifle AssaultRifle)
From this animation the transform of the bones corresponding the parts of the gun are taken from.


So for reloading we have the rifle, the magazine and the arms.
The arms are the ones that move the rifle, which then move the magazine. So the initial root tranform should be that of the arms.
The rifle, on the other hand, must have the transform of ik_hand_gun (NOT ik_hand_r, although it may seem it should be this its not, I've tested it). 
For the magazine, should be: rifle transform * grip * mag (although grip here is not neccesary as it always the identity, at least with what I tested)

After one step in the animation:
- The arms can be anywhere
- Rifle should have the transform of: manny transform * Manny Armature * root * ik_hand_root * ik_hand_gun 
- Magazine should be in the position of the rifle * grip * mag
(The matrix containing this things is called the node matrix in my code, but it doesn't really matter)

NOTE The global inverse matrix here is always the identity

Example in Code:
TODO


# Shader for animations:

Optionals:
gl_InstanceID
aModelStride

Required:
boneMat (uniform or SSBO)
aBoneNum (or bone indices) (uvec4)
aBoneWeight(or weights) (vec4)


These two snippets are equivalent. gl_InstanceID is 0 indexed, so if there is 1 instance the value is 0.
```C
mat4 skinMat =
    aBoneWeight.x * boneMat[aBoneNum.x + gl_InstanceID * aModelStride] +  
    aBoneWeight.y * boneMat[aBoneNum.y + gl_InstanceID * aModelStride] +  
    aBoneWeight.z * boneMat[aBoneNum.z + gl_InstanceID * aModelStride] +  
    aBoneWeight.w * boneMat[aBoneNum.w + gl_InstanceID * aModelStride];   
```
```C
mat4 skinMat = 
    inJointWeights.x * jointMatrices[int(inJointIndices.x)] +
    inJointWeights.y * jointMatrices[int(inJointIndices.y)] +
    inJointWeights.z * jointMatrices[int(inJointIndices.z)] +
    inJointWeights.w * jointMatrices[int(inJointIndices.w)];

locPos = model * nodeMatrix * skinMat * vec4(aPos, 1.0);
Normal = normalize(transpose(inverse(mat3(model * nodeMatrix * skinMat))) * aNormal);
```



Uniforms are bind per program so each program has its own Uniform.
SSBOs are bind globally, so every program is going to use the current bounded SSBO. So no glUseProgram beforehand needed like uniforms.



Animation Cycle:
You need a a model(mesh) + bones, an and animation affecting a set of bones (skeleton) (normally mapped by name, so anim's boneName -> model's boneName)

Everyframe
For a given time, evaluate each animation channel to get a pose (translation/rotation/scaling for each bone/node).
you need the bindpose of each bone yes, this inverse bind matrix converts from world to the localspace of each bone.
then you apply the parent transforms to this bone
For each vertex, use its bone weights/indices to blend the corresponding bone matrices.
Pass bone matrices to the shader, where each vertex is transformed (usually in the vertex shader).


mRootMatrixTransform its in model and in every node but the one in model is the same as the one in the root node, the child nodes just use the identity!!!

See how the local transform corresponds to the bones and in what space do they end up being after multiplication

IMPORTANT: Skinning always happens in the mesh's local/model space, not in full world space!
- Mesh vertices are in model space (bind pose)
- If the bone was at the origin, where would this vertex be?
- Apply the inverse of the bone's bind pose world matrix, whic is called the inverse bind matrix!


mOffsetMatrix is the inverse bind matrix. From model space (bind pose) to bone's local space

InverseBindMatrix * position_model = vertex in bone's local space (as in the bind pose)
AnimatedWorldMatrix * (InverseBindMatrix * position_model) = vertex in model space, but in the current animation pose


```C
updateTRSMatrix()
{
    if (mParentNode) {
        mParentNodeMatrix = mParentNode->getTRSMatrix();
    }
    mLocalTRSMatrix = mRootTransformMatrix * mParentNodeMatrix * mTranslationMatrix * mRotationMatrix * mScalingMatrix;
}
...
mBoneMatrices.clear();
for (auto& node : mNodeList) {
    std::string nodeName = node->nodeName;
    node->updateTRSMatrix();
    if (mBoneOffsetMatrices.count(nodeName) > 0) {
        mBoneMatrices.emplace_back(mNodeMap.at(nodeName)->getTRSMatrix() * mBoneOffsetMatrices.at(nodeName));
    }
}
```
`mBoneMatrices = mNodeMap.at(nodeName)->getTRSMatrix() * mBoneOffsetMatrices.at(nodeName)`
`boneMat = animatedMatrix * inverseBindMatrix`


`locPos = model * nodeMatrix * skinMat * vec4(aPos, 1.0);`