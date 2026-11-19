#include "Engine/Source/Runtime/Engine/Animation/AnimSubsystem_Base.h"

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"

namespace ue5parse::runtime {

void FAnimSubsystem_Base::OnPostLoadDefaults(UClass* InClass) {
    if (GetRuntimeProvider().getGame() >= EGame::GAME_UE5_4) {
        PatchValueHandlers(InClass);
        return;
    }
    const auto* AnimClass = Cast<UAnimBlueprintGeneratedClass>(InClass);
    if (AnimClass == nullptr || AnimClass->GetPropertyAccessLibrary() == nullptr)
        return;
    for (FExposedValueHandler& Handler : ExposedValueHandlers)
        Handler.Initialize(InClass, *AnimClass->GetPropertyAccessLibrary());
}

void FAnimSubsystem_Base::PatchValueHandlers(UClass* InClass) {
    ExposedValueHandlers.Reset();
    void* SparseClassData = const_cast<void*>(InClass->GetSparseClassData());
    UE5PARSE_CHECK(SparseClassData != nullptr);
    for (const UStruct* Type = InClass->GetSparseClassDataStruct(); Type != nullptr; Type = Type->GetSuperStruct()) {
        for (FField* Field = Type->ChildProperties; Field != nullptr; Field = Field->Next) {
            auto* StructProperty = CastField<FStructProperty>(Field);
            if (StructProperty == nullptr || !StructProperty->Struct->IsChildOf(FAnimNodeExposedValueHandler::StaticStruct()))
                continue;
            FExposedValueHandler& NewHandler = ExposedValueHandlers.Emplace_GetRef();
            NewHandler.HandlerStruct = StructProperty->Struct;
            NewHandler.Handler = StructProperty->ContainerPtrToValuePtr<FAnimNodeExposedValueHandler>(SparseClassData);
            NewHandler.Handler->Initialize(InClass);
        }
    }
}

}
