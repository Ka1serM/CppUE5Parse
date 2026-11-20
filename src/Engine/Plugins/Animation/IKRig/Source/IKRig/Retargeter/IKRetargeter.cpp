#include "Engine/Plugins/Animation/IKRig/Source/IKRig/Retargeter/IKRetargeter.h"

namespace ue5parse::runtime {

UIKRetargeter::UIKRetargeter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootSettings = ObjectInitializer.CreateDefaultSubobject<URetargetRootSettings>(FName("RootSettings"));
    GlobalSettings = ObjectInitializer.CreateDefaultSubobject<UIKRetargetGlobalSettings>(FName("GlobalSettings"));
    OpStack = ObjectInitializer.CreateDefaultSubobject<URetargetOpStack>(FName("PostSettings"));
}

}
