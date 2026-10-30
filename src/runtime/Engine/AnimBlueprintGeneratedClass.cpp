#include "ue5parse/runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"

#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"
#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"
#include "ue5parse/runtime/Engine/Animation/AnimNode_LinkedInputPose.h"
#include "ue5parse/runtime/Engine/Animation/AnimNode_Root.h"
#include "ue5parse/runtime/Engine/Animation/ExposedValueHandler.h"
#include "ue5parse/runtime/Engine/Animation/PropertyAccess.h"

namespace ue5parse::runtime {

namespace {

bool IsChildOfStruct(const UStruct* Struct, FName Name) {
    for (const UStruct* Type = Struct; Type != nullptr; Type = Type->GetSuperStruct())
        if (Type->GetFName() == Name) return true;
    return false;
}

// The properties of a struct and its supers, in UE's TFieldIterator order: the struct's own first.
template <class Visit>
void ForEachProperty(const UStruct* Struct, Visit&& InVisit) {
    for (const UStruct* Type = Struct; Type != nullptr; Type = Type->GetSuperStruct())
        for (FField* Field = Type->ChildProperties; Field != nullptr; Field = Field->Next)
            if (FProperty* Property = CastField<FProperty>(Field)) InVisit(Property);
}

// A mapped property of the class object itself (not of its instances), such as NodeTypeMap.
const FMapProperty* FindClassMap(const UAnimBlueprintGeneratedClass* Class, FName Name) {
    return CastField<FMapProperty>(Class->GetClass()->FindPropertyByName(Name));
}

}

void UAnimBlueprintGeneratedClass::Link() {
    Super::Link();
    BuildNodeProperties();
}

void UAnimBlueprintGeneratedClass::SerializeDefaultObject(UObject* Object, FArchive& Ar) {
    Super::SerializeDefaultObject(Object, Ar);
    BuildLinkTargets(Object);
    BuildConstantProperties();
    GenerateAnimationBlueprintFunctions();
    LinkFunctionsToDefaultObjectNodes(Object);
}

const UAnimBlueprintGeneratedClass* UAnimBlueprintGeneratedClass::GetNodeDataClass() const {
    const UAnimBlueprintGeneratedClass* DataClass = this;
    while (DataClass->AnimNodeData.Num() == 0 && Cast<UAnimBlueprintGeneratedClass>(DataClass->GetSuperClass()))
        DataClass = Cast<UAnimBlueprintGeneratedClass>(DataClass->GetSuperClass());
    return DataClass;
}

// A function with one output pose link is a graph; its other pose links are its inputs, its other parameters the
// properties it takes.
void UAnimBlueprintGeneratedClass::GenerateAnimationBlueprintFunctions() {
    AnimBlueprintFunctions.Empty();
    const FName DefaultAnimGraphName("AnimGraph");
    for (const UStruct* Type = this; Type != nullptr; Type = Type->GetSuperStruct()) {
        for (UField* Field = Type->Children; Field != nullptr; Field = Field->Next) {
            const auto* Function = Cast<UFunction>(Field);
            if (Function == nullptr) continue;
            bool bFoundOutput = false;
            FStructProperty* OutputPoseNodeProperty = nullptr;
            TArray<FName> InputPoseNames;
            TArray<int32> InputPoseNodeIndices;
            TArray<FStructProperty*> InputPoseNodeProperties;
            TArray<FAnimBlueprintFunction::FInputPropertyData> InputPropertyData;
            for (FField* ParamField = Function->ChildProperties; ParamField != nullptr; ParamField = ParamField->Next) {
                FProperty* Param = CastField<FProperty>(ParamField);
                if (Param == nullptr) continue;
                auto* StructProperty = CastField<FStructProperty>(Param);
                if (StructProperty == nullptr || !StructProperty->Struct->IsChildOf(FPoseLink::StaticStruct())) {
                    InputPropertyData.Add({Param->GetFName(), Param, nullptr});
                    continue;
                }
                if (StructProperty->HasAnyPropertyFlags(CPF_OutParm)) {
                    // Two output poses are not a graph.
                    OutputPoseNodeProperty = bFoundOutput ? nullptr : StructProperty;
                    bFoundOutput = true;
                } else {
                    InputPoseNames.Add(StructProperty->GetFName());
                    InputPoseNodeIndices.Add(INDEX_NONE);
                    InputPoseNodeProperties.Add(nullptr);
                }
            }
            if (OutputPoseNodeProperty == nullptr) continue;
            const FName FunctionName = Function->GetFName();
            FAnimBlueprintFunction NewFunction(FunctionName);
            NewFunction.InputPoseNames = InputPoseNames;
            NewFunction.InputPoseNodeIndices = InputPoseNodeIndices;
            NewFunction.InputPoseNodeProperties = InputPoseNodeProperties;
            NewFunction.InputPropertyData = InputPropertyData;
            if (FunctionName == DefaultAnimGraphName) AnimBlueprintFunctions.Insert(NewFunction, 0);
            else AnimBlueprintFunctions.Add(NewFunction);
        }
    }
}

// The roots and the linked input poses of the default object's nodes, by graph name.
void UAnimBlueprintGeneratedClass::LinkFunctionsToDefaultObjectNodes(UObject* DefaultObject) {
    const UAnimBlueprintGeneratedClass* DataClass = GetNodeDataClass();
    auto FindFunction = [this](FName Name) -> FAnimBlueprintFunction* {
        for (FAnimBlueprintFunction& Function : AnimBlueprintFunctions)
            if (Function.Name == Name) return &Function;
        return nullptr;
    };
    for (int32 AnimNodeIndex = 0; AnimNodeIndex < AnimNodeProperties.Num(); ++AnimNodeIndex) {
        FStructProperty* StructProperty = AnimNodeProperties[AnimNodeIndex];
        if (LinkTargets[AnimNodeIndex] != AnimNodeIndex || !DataClass->AnimNodeData.IsValidIndex(AnimNodeIndex)) continue;
        if (StructProperty->Struct->IsChildOf(FAnimNode_Root::StaticStruct())) {
            auto* RootNode = StructProperty->ContainerPtrToValuePtr<FAnimNode_Root>(DefaultObject);
            RootNode->SetNodeData(DataClass->AnimNodeData[AnimNodeIndex]);
            if (FAnimBlueprintFunction* FoundFunction = FindFunction(RootNode->GetName())) {
                FoundFunction->Group = RootNode->GetGroup();
                FoundFunction->OutputPoseNodeIndex = AnimNodeIndex;
                FoundFunction->OutputPoseNodeProperty = StructProperty;
            }
        } else if (StructProperty->Struct->IsChildOf(FAnimNode_LinkedInputPose::StaticStruct())) {
            auto* LinkedInputPoseNode = StructProperty->ContainerPtrToValuePtr<FAnimNode_LinkedInputPose>(DefaultObject);
            FAnimBlueprintFunction* FoundFunction = FindFunction(LinkedInputPoseNode->Graph);
            if (FoundFunction == nullptr) continue;
            for (int32 InputIndex = 0; InputIndex < FoundFunction->InputPoseNames.Num(); ++InputIndex) {
                if (FoundFunction->InputPoseNames[InputIndex] != LinkedInputPoseNode->Name) continue;
                FoundFunction->InputPoseNodeIndices[InputIndex] = AnimNodeIndex;
                FoundFunction->InputPoseNodeProperties[InputIndex] = StructProperty;
            }
            for (FAnimBlueprintFunction::FInputPropertyData& Input : FoundFunction->InputPropertyData) Input.ClassProperty = FindPropertyByName(Input.Name);
        }
    }
    for (FAnimBlueprintFunction& Function : AnimBlueprintFunctions) {
        if (Function.OutputPoseNodeProperty == nullptr) continue;
        const auto* RootNode = Function.OutputPoseNodeProperty->ContainerPtrToValuePtr<FAnimNode_Root>(DefaultObject);
        Function.bImplemented = RootNode->Result.LinkID != INDEX_NONE;
    }
}

const UAnimBlueprintGeneratedClass* UAnimBlueprintGeneratedClass::GetRootAnimClass() const {
    const UAnimBlueprintGeneratedClass* Root = this;
    while (const auto* Parent = Cast<UAnimBlueprintGeneratedClass>(Root->GetSuperClass())) Root = Parent;
    return Root;
}

void UAnimBlueprintGeneratedClass::BuildNodeProperties() {
    AnimNodeProperties.Empty();
    MutableProperties.Empty();
    MutableNodeDataProperty = nullptr;
    ForEachProperty(this, [this](FProperty* Property) {
        auto* StructProperty = CastField<FStructProperty>(Property);
        if (StructProperty == nullptr) return;
        if (IsChildOfStruct(StructProperty->Struct, FName("AnimNode_Base"))) {
            AnimNodeProperties.Add(StructProperty);
            return;
        }
        if (!IsChildOfStruct(StructProperty->Struct, FName("AnimBlueprintMutableData"))) return;
        MutableNodeDataProperty = StructProperty;
        ForEachProperty(StructProperty->Struct, [this](FProperty* Mutable) { MutableProperties.Add(Mutable); });
    });

}

void UAnimBlueprintGeneratedClass::BuildLinkTargets(const UObject* DefaultObject) {
    LinkTargets.SetNum(AnimNodeProperties.Num());
    for (int32 NodeIndex = 0; NodeIndex < AnimNodeProperties.Num(); ++NodeIndex) {
        LinkTargets[NodeIndex] = ResolveLinkTarget(DefaultObject, NodeIndex, 0);
        if (LinkTargets[NodeIndex] != NodeIndex)
            ReportLoadProblem("anim.node_unsupported", GetName() + FString(": ") + AnimNodeProperties[NodeIndex]->Struct->GetName());
    }

    OrderedSavedPoseNodeIndices.Empty();
    const FMapProperty* SavedPoses = FindClassMap(this, FName("OrderedSavedPoseIndicesMap"));
    if (SavedPoses == nullptr) return;
    const auto* Indices = CastField<FStructProperty>(SavedPoses->ValueProp);
    const auto* IndexArray = CastField<FArrayProperty>(Indices->Struct->FindPropertyByName(FName("OrderedSavedPoseNodeIndices")));
    FScriptMapHelper Map(SavedPoses, SavedPoses->ContainerPtrToValuePtr<void>(this));
    for (int32 Index = 0; Index < Map.GetMaxIndex(); ++Index) {
        if (!Map.IsValidIndex(Index)) continue;
        FScriptArrayHelper Array(IndexArray, IndexArray->ContainerPtrToValuePtr<void>(Map.GetValuePtr(Index)));
        TArray<int32>& Ordered = OrderedSavedPoseNodeIndices.Add(*reinterpret_cast<const FName*>(Map.GetKeyPtr(Index)));
        for (int32 Element = 0; Element < Array.Num(); ++Element) Ordered.Add(*reinterpret_cast<const int32*>(Array.GetRawPtr(Element)));
    }
}

int32 UAnimBlueprintGeneratedClass::ResolveLinkTarget(const UObject* DefaultObject, int32 NodeIndex, int32 Depth) const {
    if (NodeIndex == INDEX_NONE || Depth > AnimNodeProperties.Num()) return INDEX_NONE;
    const FStructProperty* Node = AnimNodeProperties[NodeIndex];
    const auto* NodeType = Cast<UScriptStruct>(Node->Struct);
    if (NodeType->GetCppStructOps() != nullptr) return NodeIndex;
    const void* NodeData = Node->ContainerPtrToValuePtr<void>(DefaultObject);
    int32 InputLinkID = INDEX_NONE;
    ForEachProperty(NodeType, [&](FProperty* Property) {
        const auto* Link = CastField<FStructProperty>(Property);
        if (InputLinkID != INDEX_NONE || Link == nullptr || !IsChildOfStruct(Link->Struct, FName("PoseLinkBase"))) return;
        const FProperty* LinkID = Link->Struct->FindPropertyByName(FName("LinkID"));
        InputLinkID = *LinkID->ContainerPtrToValuePtr<int32>(Link->ContainerPtrToValuePtr<void>(NodeData));
    });
    return ResolveLinkTarget(DefaultObject, InputLinkID, Depth + 1);
}

void UAnimBlueprintGeneratedClass::BuildConstantProperties() {
    ConstantProperties.Empty();
    ExposedValueHandlersProperty = nullptr;
    ExposedValueHandlers = nullptr;
    PropertyAccessLibrary = nullptr;
    if (SparseClassData == nullptr) return;
    ForEachProperty(SparseClassDataStruct, [this](FProperty* Property) {
        ConstantProperties.Add(Property);
        const auto* Subsystem = CastField<FStructProperty>(Property);
        if (Subsystem == nullptr) return;
        void* SubsystemData = Subsystem->ContainerPtrToValuePtr<void>(SparseClassData);
        if (IsChildOfStruct(Subsystem->Struct, FName("AnimSubsystem_Base"))) {
            ExposedValueHandlersProperty = CastField<FArrayProperty>(Subsystem->Struct->FindPropertyByName(FName("ExposedValueHandlers")));
            ExposedValueHandlers = ExposedValueHandlersProperty->ContainerPtrToValuePtr<void>(SubsystemData);
        } else if (IsChildOfStruct(Subsystem->Struct, FName("AnimSubsystem_PropertyAccess"))) {
            const FProperty* Library = Subsystem->Struct->FindPropertyByName(FName("Library"));
            PropertyAccessLibrary = Library->ContainerPtrToValuePtr<FPropertyAccessLibrary>(SubsystemData);
        }
    });
    // UE's FAnimSubsystem_PropertyAccess::OnPostLoad and FExposedValueHandler::ClassInitialization.
    if (PropertyAccessLibrary != nullptr) PropertyAccessLibrary->PatchPropertyOffsets();
    if (ExposedValueHandlersProperty == nullptr) return;
    FScriptArrayHelper Handlers(ExposedValueHandlersProperty, ExposedValueHandlers);
    for (int32 HandlerIndex = 0; HandlerIndex < Handlers.Num(); ++HandlerIndex) {
        auto* Handler = reinterpret_cast<FExposedValueHandler*>(Handlers.GetRawPtr(HandlerIndex));
        Handler->Function = Handler->BoundFunction.isNone() ? nullptr : FindFunctionByName(Handler->BoundFunction);
        UE5PARSE_CHECK(Handler->BoundFunction.isNone() || Handler->Function != nullptr);
    }
}

const TArray<int32>* UAnimBlueprintGeneratedClass::GetOrderedSavedPoseNodeIndices(FName GraphName) const {
    return OrderedSavedPoseNodeIndices.Find(GraphName);
}

// NodeTypeMap: for each node type, the index of each of its properties (sorted by offset in the editor's layout, so a
// super's properties keep their index in every subtype). Read once per call site of GET_ANIM_NODE_DATA.
int32 UAnimBlueprintGeneratedClass::GetAnimNodePropertyIndex(const UScriptStruct* NodeType, FName PropertyName) const {
    const FMapProperty* NodeTypes = FindClassMap(this, FName("NodeTypeMap"));
    if (NodeTypes == nullptr) return INDEX_NONE;
    const auto* Key = CastField<FObjectPropertyBase>(NodeTypes->KeyProp);
    const auto* StructData = CastField<FStructProperty>(NodeTypes->ValueProp);
    const auto* Names = CastField<FMapProperty>(StructData->Struct->FindPropertyByName(FName("NameToIndexMap")));
    FScriptMapHelper Map(NodeTypes, NodeTypes->ContainerPtrToValuePtr<void>(this));
    for (int32 Index = 0; Index < Map.GetMaxIndex(); ++Index) {
        if (!Map.IsValidIndex(Index) || Key->GetObjectPropertyValue(Map.GetKeyPtr(Index)) != NodeType) continue;
        FScriptMapHelper NameMap(Names, Names->ContainerPtrToValuePtr<void>(Map.GetValuePtr(Index)));
        const uint8* Value = NameMap.FindValueFromHash(&PropertyName);
        return Value ? *reinterpret_cast<const int32*>(Value) : INDEX_NONE;
    }
    return INDEX_NONE;
}

const void* UAnimBlueprintGeneratedClass::GetConstantNodeValueRaw(int32 Index) const {
    return ConstantProperties[Index]->ContainerPtrToValuePtr<void>(GetSparseClassData());
}

const void* UAnimBlueprintGeneratedClass::GetMutableNodeValueRaw(int32 Index, const UObject* Object) const {
    const void* MutableData = MutableNodeDataProperty->ContainerPtrToValuePtr<void>(Object);
    return MutableProperties[Index]->ContainerPtrToValuePtr<void>(MutableData);
}

const UObject* UAnimBlueprintGeneratedClass::GetObjectPtrFromAnimNode(const FAnimNode_Base* Node) const {
    const FStructProperty* NodeProperty = AnimNodeProperties[Node->GetNodeData()->GetNodeIndex()];
    return reinterpret_cast<const UObject*>(reinterpret_cast<const uint8*>(Node) - NodeProperty->GetOffset_ForInternal());
}

const FExposedValueHandler* UAnimBlueprintGeneratedClass::GetExposedValueHandler(int32 NodeIndex) const {
    if (ExposedValueHandlersProperty == nullptr) return nullptr;
    FScriptArrayHelper Handlers(ExposedValueHandlersProperty, ExposedValueHandlers);
    return reinterpret_cast<const FExposedValueHandler*>(Handlers.GetRawPtr(NodeIndex));
}

}
