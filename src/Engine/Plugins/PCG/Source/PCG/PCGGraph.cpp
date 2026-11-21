#include "Engine/Plugins/PCG/Source/PCG/PCGGraph.h"

#include <string>

#include "Engine/Plugins/PCG/Source/PCG/Graph/PCGGraphCompilationData.h"
#include "Engine/Plugins/PCG/Source/PCG/PCGInputOutputSettings.h"
#include "Engine/Plugins/PCG/Source/PCG/PCGNode.h"

namespace ue5parse::runtime {

namespace {

UPCGNode* CreateDefaultNode(UPCGGraph* Graph, const FObjectInitializer& ObjectInitializer, const char* NodeName, const char* SettingsName, bool bIsInput,
                            int32& NumAllocatedPins) {
    auto* Node = static_cast<UPCGNode*>(ObjectInitializer.CreateDefaultSubobject(Graph, MakeName(NodeName), UPCGNode::StaticClass(), UPCGNode::StaticClass()));
    auto* Settings = static_cast<UPCGGraphInputOutputSettings*>(ObjectInitializer.CreateDefaultSubobject(
        Graph, MakeName(SettingsName), UPCGGraphInputOutputSettings::StaticClass(), UPCGGraphInputOutputSettings::StaticClass()));
    FPCGPinProperties Pin;
    Pin.Label = MakeName(bIsInput ? "In" : "Out");
    Pin.AllowedTypes.LegacyDataType = bIsInput ? EPCGDataType::Spatial : EPCGDataType::Any;
    Pin.PinStatus = EPCGPinStatus::Advanced;
    Settings->Pins.Add(Pin);
    Settings->bHasAddedDefaultPin = true;
    Node->DefaultSettings = Settings;
    Node->SettingsInterface = Settings;
    for (TArray<TObjectPtr<UPCGPin>>* Pins : {&Node->InputPins, &Node->OutputPins}) {
        auto* NewPin = static_cast<UPCGPin*>(ObjectInitializer.CreateDefaultSubobject(Node, MakeName("DefaultPin_" + std::to_string(NumAllocatedPins++)),
                                                                                       UPCGPin::StaticClass(), UPCGPin::StaticClass()));
        NewPin->Node = Node;
        NewPin->Properties = Pin;
        NewPin->Properties.bInvisiblePin = (Pins == &Node->InputPins) == bIsInput;
        Pins->Add(NewPin);
    }
    return Node;
}

}

UPCGGraph::UPCGGraph(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    int32 NumAllocatedPins = 0;
    InputNode = CreateDefaultNode(this, ObjectInitializer, "DefaultInputNode", "DefaultInputNodeSettings", true, NumAllocatedPins);
    OutputNode = CreateDefaultNode(this, ObjectInitializer, "DefaultOutputNode", "DefaultOutputNodeSettings", false, NumAllocatedPins);
}

}
