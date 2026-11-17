#pragma once

#include "RefGenCommon.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet/KismetMathLibrary.h"

#if UE_AT_LEAST(5, 0)
#define REF_MATH(Op) TEXT(#Op) TEXT("_DoubleDouble")
#else
#define REF_MATH(Op) TEXT(#Op) TEXT("_FloatFloat")
#endif

namespace RefGen
{
	inline FEdGraphPinType PinType(const FRGPinName& Category, UObject* SubObject = nullptr, const FRGPinName& SubCategory = FRGPinName())
	{
		FEdGraphPinType Type;
		Type.PinCategory = Category;
		Type.PinSubCategory = SubCategory;
		Type.PinSubCategoryObject = SubObject;
		return Type;
	}

	inline FEdGraphPinType FloatPin()
	{
#if UE_AT_LEAST(5, 0)
		return PinType(UEdGraphSchema_K2::PC_Real, nullptr, UEdGraphSchema_K2::PC_Float);
#else
		return PinType(UEdGraphSchema_K2::PC_Float);
#endif
	}

	struct FGraph
	{
		UEdGraph* Graph;
		int32 NextX = 0;

		explicit FGraph(UEdGraph* InGraph) : Graph(InGraph) {}

		template <typename T>
		T* Add(TFunctionRef<void(T*)> Setup, int32 X = INT_MIN, int32 Y = 0)
		{
			FGraphNodeCreator<T> Creator(*Graph);
			T* Node = Creator.CreateNode(false);
			Setup(Node);
			Node->NodePosX = X == INT_MIN ? (NextX += 300) : X;
			Node->NodePosY = Y;
			Creator.Finalize();
			return Node;
		}

		template <typename T>
		T* Add()
		{
			return Add<T>([](T*) {});
		}

		UK2Node_CallFunction* Call(UClass* Class, const TCHAR* Function, int32 Y = 0)
		{
			UFunction* Found = Class->FindFunctionByName(Function);
			if (!Found)
			{
				Fail(FString::Printf(TEXT("function %s::%s"), *Class->GetName(), Function));
				return nullptr;
			}
			return Add<UK2Node_CallFunction>([Found](UK2Node_CallFunction* Node) { Node->SetFromFunction(Found); }, INT_MIN, Y);
		}

		UK2Node_Event* Event(UBlueprint* Blueprint, UClass* Class, const TCHAR* Name)
		{
			if (UK2Node_Event* Existing = FBlueprintEditorUtils::FindOverrideForFunction(Blueprint, Class, Name))
			{
#if UE_AT_LEAST(4, 18)
				Existing->SetEnabledState(ENodeEnabledState::Enabled, false);
#elif UE_AT_LEAST(4, 11) // node enable states are 4.11+
				Existing->EnableNode();
#endif
				return Existing;
			}
			return Add<UK2Node_Event>([&](UK2Node_Event* Node)
			{
				RG_SET_EVENT_REFERENCE(Node, Name, Class);
				Node->bOverrideFunction = true;
			}, -400, Graph->Nodes.Num() * 200);
		}

		UK2Node_VariableGet* Get(const TCHAR* Name, int32 Y = 200)
		{
			return Add<UK2Node_VariableGet>([Name](UK2Node_VariableGet* Node) { Node->VariableReference.SetSelfMember(Name); }, INT_MIN, Y);
		}

		UK2Node_VariableSet* Set(const TCHAR* Name, int32 Y = 0)
		{
			return Add<UK2Node_VariableSet>([Name](UK2Node_VariableSet* Node) { Node->VariableReference.SetSelfMember(Name); }, INT_MIN, Y);
		}

		static UEdGraphPin* Pin(UEdGraphNode* Node, const TCHAR* Name, EEdGraphPinDirection Direction = EGPD_MAX)
		{
			if (!Node)
				return nullptr;
			for (UEdGraphPin* Candidate : Node->Pins)
				if (Candidate->PinName == FRGPinName(Name) && (Direction == EGPD_MAX || Candidate->Direction == Direction))
					return Candidate;
			Fail(FString::Printf(TEXT("pin %s on %s"), Name, *Node->GetName()));
			return nullptr;
		}

		static UEdGraphPin* Exec(UEdGraphNode* Node, EEdGraphPinDirection Direction)
		{
			if (!Node)
				return nullptr;
			for (UEdGraphPin* Candidate : Node->Pins)
				if (Candidate->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Candidate->Direction == Direction)
					return Candidate;
			Fail(FString::Printf(TEXT("exec pin on %s"), *Node->GetName()));
			return nullptr;
		}

		static UEdGraphPin* Value(UEdGraphNode* Node, EEdGraphPinDirection Direction)
		{
			if (!Node)
				return nullptr;
			for (UEdGraphPin* Candidate : Node->Pins)
				if (Candidate->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec && Candidate->Direction == Direction && !Candidate->bHidden && Candidate->PinName != UEdGraphSchema_K2::PN_Self)
					return Candidate;
			Fail(FString::Printf(TEXT("value pin on %s"), *Node->GetName()));
			return nullptr;
		}

		void Link(UEdGraphPin* A, UEdGraphPin* B)
		{
			if (!A || !B)
				return;
			if (!Graph->GetSchema()->TryCreateConnection(A, B))
				Fail(FString::Printf(TEXT("link %s.%s -> %s.%s"), *A->GetOwningNode()->GetName(), *RG_PIN_STR(A->PinName), *B->GetOwningNode()->GetName(), *RG_PIN_STR(B->PinName)));
		}

		void Then(UEdGraphNode* From, UEdGraphNode* To)
		{
			Link(Exec(From, EGPD_Output), Exec(To, EGPD_Input));
		}

		void Default(UEdGraphNode* Node, const TCHAR* PinName, const FString& Value)
		{
			if (UEdGraphPin* Target = Pin(Node, PinName, EGPD_Input))
				Graph->GetSchema()->TrySetDefaultValue(*Target, Value);
		}
	};
}
