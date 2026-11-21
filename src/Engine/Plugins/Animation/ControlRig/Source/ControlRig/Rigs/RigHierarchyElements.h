#pragma once

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyControl.h"

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyMetadata.h"
#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyPose.h"

namespace ue5parse::runtime {

struct FRigConnectionRuleStash {
    FString ScriptStructPath;
    FString ExportedText;

    friend FArchive& operator<<(FArchive& Ar, FRigConnectionRuleStash& Rule) {
        return Ar << Rule.ScriptStructPath << Rule.ExportedText;
    }
};

struct FRigConnectorSettings {
    FString Description;
    uint8 Type = 0;
    bool bOptional = false;
    bool bIsArray = false;
    bool bPostConstruction = false;
    TArray<FRigConnectionRuleStash> Rules;

    void Load(FArchive& Ar);
};

struct FRigSingleParentElement {
    FRigCurrentAndInitialTransform Pose;
    FRigElementKey Parent;
};

struct FRigBoneElement : FRigSingleParentElement {
    FName BoneType;
};

struct FRigRigidBodyElement : FRigSingleParentElement {
    FGuid Solver;
    float Mass = 0;
};

struct FRigCurveElement {
    bool bIsValueSet = true;
    float Value = 0;
};

struct FRigHierarchyElement {
    FRigElementKey Key;
    TArray<FRigMetadataEntry> Metadata;
    std::variant<FRigBoneElement, FRigSingleParentElement, FRigCurveElement, FRigConnectorSettings,
                 FRigMultiParentElement, FRigControlElement, FRigRigidBodyElement> Data;

    void LoadStatic(FArchive& Ar, const FRigHierarchySerializationSettings& Settings);
    void LoadParents(FArchive& Ar);
};

}
