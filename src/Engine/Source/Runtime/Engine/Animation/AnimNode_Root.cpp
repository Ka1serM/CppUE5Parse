#include "Engine/Source/Runtime/Engine/Animation/AnimNode_Root.h"

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

FName FAnimNode_Root::GetGroup() const {
    //! UE 5.4 renames the folded Group to LayerGroup.
    if (GetRuntimeProvider().getGame() >= EGame::GAME_UE5_4)
        return GET_ANIM_NODE_DATA(FName, LayerGroup);
    return GET_ANIM_NODE_DATA(FName, Group);
}

}
