#include "RefGenGraph.h"
#include "RefTypes.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/TimelineTemplate.h"
#define REF_DISPLAY_TRACKS UE_AT_LEAST(4, 26) // AddDisplayTrack is not in 4.25 and older
#include "Engine/UserDefinedEnum.h"
#if __has_include("StructUtils/UserDefinedStruct.h")
#include "StructUtils/UserDefinedStruct.h"
#else
#include "Engine/UserDefinedStruct.h"
#endif
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/DataTable.h"
#include "Engine/CurveTable.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimSequence.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#if UE_AT_LEAST(4, 21)
#include "Components/RectLightComponent.h"
#endif
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/SplineComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Engine/StaticMeshActor.h"
#include "Curves/CurveFloat.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_Timeline.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_SwitchEnum.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_MakeStruct.h"
#if RG_GAMEPLAY
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#endif
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystem.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/Material.h"

namespace
{
	using namespace RefGen;

	UBlueprint* NewBlueprint(const FString& Name, const FString& Path, UClass* Parent, EBlueprintType Type = BPTYPE_Normal)
	{
#if UE_AT_LEAST(4, 26)
		UPackage* Package = CreatePackage(*(Path / Name));
#else
		UPackage* Package = CreatePackage(nullptr, *(Path / Name));
#endif
		UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(Parent, Package, FName(*Name), Type, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
		if (!Blueprint)
		{
			Fail(FString::Printf(TEXT("blueprint %s"), *Name));
			return nullptr;
		}
		FAssetRegistryModule::AssetCreated(Blueprint);
		Package->MarkPackageDirty();
		return Blueprint;
	}

	void Compile(UBlueprint* Blueprint)
	{
		if (!Blueprint)
			return;
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (Blueprint->Status == BS_Error)
			Fail(FString::Printf(TEXT("%s compile error"), *Blueprint->GetName()));
		Blueprint->MarkPackageDirty();
	}

	template <typename T>
	T* Template(USCS_Node* Node)
	{
		return Node ? Cast<T>(Node->ComponentTemplate) : nullptr;
	}

	USCS_Node* AddComponent(UBlueprint* Blueprint, UClass* Class, const TCHAR* Name, USCS_Node* Parent = nullptr)
	{
		USimpleConstructionScript* Scs = Blueprint->SimpleConstructionScript;
		USCS_Node* Node = Scs->CreateNode(Class, Name);
		if (Parent)
			Parent->AddChildNode(Node);
		else
			Scs->AddNode(Node);
		return Node;
	}

	void AddVariable(UBlueprint* Blueprint, const TCHAR* Name, FEdGraphPinType Type, EPinContainerType Container = EPinContainerType::None, FEdGraphTerminalType Value = FEdGraphTerminalType())
	{
		RG_SET_CONTAINER(Type, Container);
#if UE_AT_LEAST(4, 14)
		Type.PinValueType = Value;
#endif
		if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, Name, Type))
			Fail(FString::Printf(TEXT("variable %s on %s"), Name, *Blueprint->GetName()));
	}

	UEdGraph* EventGraphOf(UBlueprint* Blueprint)
	{
		return Blueprint->UbergraphPages.Num() > 0 ? Blueprint->UbergraphPages[0] : nullptr;
	}

	UEdGraph* AddFunction(UBlueprint* Blueprint, const TCHAR* Name)
	{
		UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, Name, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
		FBlueprintEditorUtils::AddFunctionGraph<UClass>(Blueprint, Graph, true, nullptr);
		return Graph;
	}

	template <typename T>
	T* FindNode(UEdGraph* Graph)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
			if (T* Typed = Cast<T>(Node))
				return Typed;
		return nullptr;
	}

	UK2Node_FunctionResult* ResultNode(FGraph& G, UK2Node_FunctionEntry* Entry)
	{
		if (UK2Node_FunctionResult* Existing = FindNode<UK2Node_FunctionResult>(G.Graph))
			return Existing;
		auto* Result = G.Add<UK2Node_FunctionResult>([Entry](UK2Node_FunctionResult* Node)
		{
#if UE_AT_LEAST(4, 19)
			Node->FunctionReference = Entry->FunctionReference;
#else
			Node->SignatureClass = Entry->SignatureClass;
			Node->SignatureName = Entry->SignatureName;
#endif
		}, 600, 0);
		return Result;
	}

	FEdGraphTerminalType TerminalFloat()
	{
		FEdGraphTerminalType Terminal;
#if UE_AT_LEAST(5, 0)
		Terminal.TerminalCategory = UEdGraphSchema_K2::PC_Real;
		Terminal.TerminalSubCategory = UEdGraphSchema_K2::PC_Float;
#else
		Terminal.TerminalCategory = UEdGraphSchema_K2::PC_Float;
#endif
		return Terminal;
	}

	void MakeInterface(FAssets& A)
	{
		A.Interface = NewBlueprint(TEXT("BPI_Ref"), TEXT("/Game/Blueprints"), UInterface::StaticClass(), BPTYPE_Interface);
		if (!A.Interface)
			return;
		UEdGraph* Function = AddFunction(A.Interface, TEXT("Interact"));
		FGraph G(Function);
		UK2Node_FunctionEntry* Entry = FindNode<UK2Node_FunctionEntry>(Function);
		Entry->CreateUserDefinedPin(TEXT("Strength"), FloatPin(), EGPD_Output);
		ResultNode(G, Entry)->CreateUserDefinedPin(TEXT("Handled"), PinType(UEdGraphSchema_K2::PC_Boolean), EGPD_Input);
		UEdGraph* Notify = AddFunction(A.Interface, TEXT("OnNotified"));
		FindNode<UK2Node_FunctionEntry>(Notify)->CreateUserDefinedPin(TEXT("Message"), PinType(UEdGraphSchema_K2::PC_String), EGPD_Output);
		Compile(A.Interface);
	}

	void MakeLibraries(FAssets& A)
	{
		A.FunctionLibrary = NewBlueprint(TEXT("BFL_Ref"), TEXT("/Game/Blueprints"), UBlueprintFunctionLibrary::StaticClass(), BPTYPE_FunctionLibrary);
		if (A.FunctionLibrary)
		{
			UEdGraph* Function = AddFunction(A.FunctionLibrary, TEXT("ScaleValue"));
			FGraph G(Function);
			UK2Node_FunctionEntry* Entry = FindNode<UK2Node_FunctionEntry>(Function);
			Entry->CreateUserDefinedPin(TEXT("Value"), FloatPin(), EGPD_Output);
			Entry->CreateUserDefinedPin(TEXT("Factor"), FloatPin(), EGPD_Output);
			UK2Node_FunctionResult* Result = ResultNode(G, Entry);
			Result->CreateUserDefinedPin(TEXT("Scaled"), FloatPin(), EGPD_Input);
			auto* Mul = G.Call(UKismetMathLibrary::StaticClass(), REF_MATH(Multiply), 200);
			G.Link(FGraph::Pin(Entry, TEXT("Value"), EGPD_Output), FGraph::Pin(Mul, TEXT("A"), EGPD_Input));
			G.Link(FGraph::Pin(Entry, TEXT("Factor"), EGPD_Output), FGraph::Pin(Mul, TEXT("B"), EGPD_Input));
			G.Link(FGraph::Pin(Mul, TEXT("ReturnValue"), EGPD_Output), FGraph::Pin(Result, TEXT("Scaled"), EGPD_Input));
			G.Then(Entry, Result);
			Compile(A.FunctionLibrary);
		}
		A.MacroLibrary = NewBlueprint(TEXT("BML_Ref"), TEXT("/Game/Blueprints"), AActor::StaticClass(), BPTYPE_MacroLibrary);
		Compile(A.MacroLibrary);
	}

	void MakeSimple(FAssets& A)
	{
		A.Simple = NewBlueprint(TEXT("BP_RefSimple"), TEXT("/Game/Blueprints"), AActor::StaticClass());
		if (!A.Simple)
			return;
		USCS_Node* Mesh = AddComponent(A.Simple, UStaticMeshComponent::StaticClass(), TEXT("Mesh"));
		if (auto* Component = Template<UStaticMeshComponent>(Mesh))
		{
			Component->SetStaticMesh(A.StaticMesh);
			Component->SetRelativeScale3D(FVector(0.5f));
		}
		Compile(A.Simple);
	}

	void MakeActor(FAssets& A)
	{
		A.Actor = NewBlueprint(TEXT("BP_RefActor"), TEXT("/Game/Blueprints"), ARefNativeActor::StaticClass());
		UBlueprint* Bp = A.Actor;
		if (!Bp)
			return;

		USCS_Node* Body = AddComponent(Bp, UStaticMeshComponent::StaticClass(), TEXT("Body"));
		if (auto* C = Template<UStaticMeshComponent>(Body))
		{
			C->SetStaticMesh(A.StaticMesh);
			C->SetMaterial(0, A.InstanceBlue);
			C->SetRelativeLocation(FVector(0, 0, 50));
		}
		USCS_Node* Skel = AddComponent(Bp, USkeletalMeshComponent::StaticClass(), TEXT("Skel"), Body);
		if (auto* C = Template<USkeletalMeshComponent>(Skel))
		{
#if UE_AT_LEAST(5, 1)
			C->SetSkeletalMeshAsset(A.SkeletalMesh);
#else
			C->SkeletalMesh = A.SkeletalMesh;
#endif
			C->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			if (A.AnimBlueprint)
				C->SetAnimInstanceClass(A.AnimBlueprint->GeneratedClass);
			C->SetRelativeLocation(FVector(150, 0, 0));
		}
		USCS_Node* Point = AddComponent(Bp, UPointLightComponent::StaticClass(), TEXT("PointLight"), Skel);
		Point->AttachToName = TEXT("socket_tip");
		if (auto* C = Template<UPointLightComponent>(Point))
		{
			C->SetIntensity(5000.f);
			C->SetLightColor(FLinearColor(1, 0.6f, 0.3f));
			C->SetAttenuationRadius(800.f);
		}
		if (auto* C = Template<USpotLightComponent>(AddComponent(Bp, USpotLightComponent::StaticClass(), TEXT("SpotLight"), Body)))
		{
			C->SetInnerConeAngle(15.f);
			C->SetOuterConeAngle(35.f);
			C->SetRelativeRotation(FRotator(-90, 0, 0));
			C->IESTexture = A.LightProfile;
		}
#if UE_AT_LEAST(4, 21)
		if (auto* C = Template<URectLightComponent>(AddComponent(Bp, URectLightComponent::StaticClass(), TEXT("RectLight"))))
		{
			C->SourceWidth = 120.f;
			C->SourceHeight = 40.f;
			C->SetRelativeLocation(FVector(0, 200, 100));
		}
#endif
		if (auto* C = Template<UCameraComponent>(AddComponent(Bp, UCameraComponent::StaticClass(), TEXT("Camera"))))
		{
			RG_SET_FOV(C, 75.f);
			C->SetRelativeLocation(FVector(-300, 0, 150));
		}
		AddComponent(Bp, UArrowComponent::StaticClass(), TEXT("Arrow"));
		AddComponent(Bp, UBillboardComponent::StaticClass(), TEXT("Billboard"));
		if (auto* C = Template<UTextRenderComponent>(AddComponent(Bp, UTextRenderComponent::StaticClass(), TEXT("Text"))))
		{
			RG_SET_RENDER_TEXT(C, TEXT("RefGen"));
			C->SetTextRenderColor(FColor(243, 156, 18)); // FColor::Orange (4.11+)
			C->SetRelativeLocation(FVector(0, 0, 250));
		}
		if (auto* C = Template<USplineComponent>(AddComponent(Bp, USplineComponent::StaticClass(), TEXT("Spline"))))
		{
#if UE_AT_LEAST(4, 13)
			C->ClearSplinePoints(false);
			C->AddSplinePoint(FVector(0, 0, 0), ESplineCoordinateSpace::Local, false);
			C->AddSplinePoint(FVector(300, 200, 0), ESplineCoordinateSpace::Local, false);
			C->AddSplinePoint(FVector(600, 0, 100), ESplineCoordinateSpace::Local, false);
			C->SetSplinePointType(1, ESplinePointType::CurveClamped, false);
#elif UE_AT_LEAST(4, 9) // no bUpdateSpline parameters before 4.13
			C->ClearSplinePoints();
			C->AddSplinePoint(FVector(0, 0, 0), ESplineCoordinateSpace::Local);
			C->AddSplinePoint(FVector(300, 200, 0), ESplineCoordinateSpace::Local);
			C->AddSplinePoint(FVector(600, 0, 100), ESplineCoordinateSpace::Local);
			C->SetSplinePointType(1, ESplinePointType::CurveClamped);
#else // 4.8: no coordinate spaces
			C->ClearSplinePoints();
			C->AddSplineLocalPoint(FVector(0, 0, 0));
			C->AddSplineLocalPoint(FVector(300, 200, 0));
			C->AddSplineLocalPoint(FVector(600, 0, 100));
#if UE_AT_LEAST(4, 7) // per-point types are 4.7+
			C->SetSplinePointType(1, ESplinePointType::CurveClamped);
#endif
#endif
			C->UpdateSpline();
		}
		if (auto* C = Template<UChildActorComponent>(AddComponent(Bp, UChildActorComponent::StaticClass(), TEXT("ChildActor"))))
		{
			RG_SET_CHILD_ACTOR_CLASS(C, A.Simple ? TSubclassOf<AActor>(*A.Simple->GeneratedClass) : TSubclassOf<AActor>(AStaticMeshActor::StaticClass()));
			C->SetRelativeLocation(FVector(0, -250, 0));
		}
		if (auto* C = Template<UParticleSystemComponent>(AddComponent(Bp, UParticleSystemComponent::StaticClass(), TEXT("Particles"), Body)))
			C->SetTemplate(A.Particles);
		if (auto* C = Template<UAudioComponent>(AddComponent(Bp, UAudioComponent::StaticClass(), TEXT("Audio"))))
		{
			C->SetSound(A.Cue);
			C->bAutoActivate = false;
		}
		USCS_Node* Box = AddComponent(Bp, UBoxComponent::StaticClass(), TEXT("Box"));
		if (auto* C = Template<UBoxComponent>(Box))
		{
			C->SetBoxExtent(FVector(200, 200, 100));
#if UE_AT_LEAST(4, 20)
			C->SetGenerateOverlapEvents(true);
#else
			C->bGenerateOverlapEvents = true;
#endif
		}
		if (auto* C = Template<USphereComponent>(AddComponent(Bp, USphereComponent::StaticClass(), TEXT("Sphere"))))
			C->SetSphereRadius(150.f);
		if (auto* C = Template<UCapsuleComponent>(AddComponent(Bp, UCapsuleComponent::StaticClass(), TEXT("Capsule"))))
			C->SetCapsuleSize(40.f, 90.f);
		if (auto* C = Template<UDecalComponent>(AddComponent(Bp, UDecalComponent::StaticClass(), TEXT("Decal"))))
		{
			C->SetDecalMaterial(A.Decal);
#if UE_AT_LEAST(4, 11) // DecalSize is 4.11+ (older decals are sized by the component scale)
			C->DecalSize = FVector(64, 128, 128);
#endif
			C->SetRelativeRotation(FRotator(-90, 0, 0));
		}
		if (auto* C = Template<UInstancedStaticMeshComponent>(AddComponent(Bp, UInstancedStaticMeshComponent::StaticClass(), TEXT("Instances"))))
		{
			C->SetStaticMesh(A.StaticMesh);
			for (int32 Index = 0; Index < 5; ++Index)
			{
				FInstancedStaticMeshInstanceData Instance;
				Instance.Transform = FTransform(FRotator(0, Index * 30.f, 0), FVector(Index * 150.f, 400, 0), FVector(0.3f)).ToMatrixWithScale();
				C->PerInstanceSMData.Add(Instance);
			}
#if UE_AT_LEAST(4, 25)
			C->NumCustomDataFloats = 2;
			C->PerInstanceSMCustomData.SetNumZeroed(10);
			for (int32 Index = 0; Index < 10; ++Index)
				C->PerInstanceSMCustomData[Index] = Index * 0.1f;
#endif
		}
		if (auto* C = Template<UHierarchicalInstancedStaticMeshComponent>(AddComponent(Bp, UHierarchicalInstancedStaticMeshComponent::StaticClass(), TEXT("HierarchicalInstances"))))
		{
			C->SetStaticMesh(A.StaticMesh);
			for (int32 Index = 0; Index < 20; ++Index)
			{
				FInstancedStaticMeshInstanceData Instance;
				Instance.Transform = FTransform(FRotator::ZeroRotator, FVector((Index % 5) * 120.f, -600 - (Index / 5) * 120.f, 0), FVector(0.2f)).ToMatrixWithScale();
				C->PerInstanceSMData.Add(Instance);
			}
		}
		if (auto* C = Template<UProjectileMovementComponent>(AddComponent(Bp, UProjectileMovementComponent::StaticClass(), TEXT("Projectile"))))
		{
			C->InitialSpeed = 0.f;
			C->bAutoActivate = false;
		}
		if (auto* C = Template<URotatingMovementComponent>(AddComponent(Bp, URotatingMovementComponent::StaticClass(), TEXT("Rotating"))))
			C->RotationRate = FRotator(0, 45, 0);

		AddVariable(Bp, TEXT("Health"), FloatPin());
		AddVariable(Bp, TEXT("Ammo"), PinType(UEdGraphSchema_K2::PC_Int));
		AddVariable(Bp, TEXT("bArmed"), PinType(UEdGraphSchema_K2::PC_Boolean));
		AddVariable(Bp, TEXT("Label"), PinType(UEdGraphSchema_K2::PC_String));
		AddVariable(Bp, TEXT("Title"), PinType(UEdGraphSchema_K2::PC_Text));
		AddVariable(Bp, TEXT("Id"), PinType(UEdGraphSchema_K2::PC_Name));
		AddVariable(Bp, TEXT("Offset"), PinType(UEdGraphSchema_K2::PC_Struct, TBaseStructure<FVector>::Get()));
		AddVariable(Bp, TEXT("Spawn"), PinType(UEdGraphSchema_K2::PC_Struct, TBaseStructure<FTransform>::Get()));
		AddVariable(Bp, TEXT("Tint"), PinType(UEdGraphSchema_K2::PC_Struct, TBaseStructure<FLinearColor>::Get()));
		if (A.Enum)
			AddVariable(Bp, TEXT("State"), PinType(UEdGraphSchema_K2::PC_Byte, A.Enum));
		if (A.Struct)
			AddVariable(Bp, TEXT("Data"), PinType(UEdGraphSchema_K2::PC_Struct, A.Struct));
		AddVariable(Bp, TEXT("NativeData"), PinType(UEdGraphSchema_K2::PC_Struct, FRefInnerStruct::StaticStruct()));
		AddVariable(Bp, TEXT("MeshAsset"), PinType(UEdGraphSchema_K2::PC_Object, UStaticMesh::StaticClass()));
		AddVariable(Bp, TEXT("SoftMesh"), PinType(UEdGraphSchema_K2::PC_SoftObject, USkeletalMesh::StaticClass()));
		AddVariable(Bp, TEXT("SpawnClass"), PinType(UEdGraphSchema_K2::PC_Class, AActor::StaticClass()));
		AddVariable(Bp, TEXT("Scores"), PinType(UEdGraphSchema_K2::PC_Int), EPinContainerType::Array);
#if UE_AT_LEAST(4, 15) // Blueprint set/map variables are 4.15+
		AddVariable(Bp, TEXT("RefTags"), PinType(UEdGraphSchema_K2::PC_Name), EPinContainerType::Set);
		AddVariable(Bp, TEXT("Weights"), PinType(UEdGraphSchema_K2::PC_Name), EPinContainerType::Map, TerminalFloat());
#endif
		AddVariable(Bp, TEXT("Target"), PinType(UEdGraphSchema_K2::PC_Object, AActor::StaticClass()));
		AddVariable(Bp, TEXT("Row"), PinType(UEdGraphSchema_K2::PC_Struct, FDataTableRowHandle::StaticStruct()));

		if (A.Interface)
#if UE_AT_LEAST(5, 1)
			FBlueprintEditorUtils::ImplementNewInterface(Bp, FTopLevelAssetPath(A.Interface->GeneratedClass->GetPathName()));
#else
			FBlueprintEditorUtils::ImplementNewInterface(Bp, A.Interface->GeneratedClass->GetFName());
#endif
		UTimelineTemplate* Timeline = FBlueprintEditorUtils::AddNewTimeline(Bp, TEXT("Pulse"));
		Compile(Bp);

		if (Timeline)
		{
			Timeline->TimelineLength = 2.f;
			Timeline->bLoop = true;
			FTTFloatTrack Track;
			RG_SET_TRACK_NAME(Track, TEXT("Alpha"), Timeline);
			Track.CurveFloat = NewObject<UCurveFloat>(Bp->GeneratedClass, NAME_None, RF_Public);
			Track.CurveFloat->FloatCurve.AddKey(0.f, 0.f);
			Track.CurveFloat->FloatCurve.AddKey(1.f, 1.f);
			Track.CurveFloat->FloatCurve.AddKey(2.f, 0.f);
			Timeline->FloatTracks.Add(Track);
#if REF_DISPLAY_TRACKS
			Timeline->AddDisplayTrack(FTTTrackId(FTTTrackBase::TT_FloatInterp, Timeline->FloatTracks.Num() - 1));
#endif
			FTTEventTrack Event;
			RG_SET_TRACK_NAME(Event, TEXT("Halfway"), Timeline);
			Event.CurveKeys = NewObject<UCurveFloat>(Bp->GeneratedClass, NAME_None, RF_Public);
			Event.CurveKeys->FloatCurve.AddKey(1.f, 0.f);
			Timeline->EventTracks.Add(Event);
#if REF_DISPLAY_TRACKS
			Timeline->AddDisplayTrack(FTTTrackId(FTTTrackBase::TT_Event, Timeline->EventTracks.Num() - 1));
#endif
			FTTVectorTrack Vector;
			RG_SET_TRACK_NAME(Vector, TEXT("Wobble"), Timeline);
			Vector.CurveVector = A.CurveVector;
			Vector.bIsExternalCurve = true;
			Timeline->VectorTracks.Add(Vector);
#if REF_DISPLAY_TRACKS
			Timeline->AddDisplayTrack(FTTTrackId(FTTTrackBase::TT_VectorInterp, Timeline->VectorTracks.Num() - 1));
#endif
		}

		UEdGraph* EventGraph = EventGraphOf(Bp);
		FGraph E(EventGraph);
		UK2Node_Event* BeginPlay = E.Event(Bp, AActor::StaticClass(), TEXT("ReceiveBeginPlay"));
		auto* Print = E.Call(UKismetSystemLibrary::StaticClass(), TEXT("PrintString"));
		E.Default(Print, TEXT("InString"), TEXT("RefGen BeginPlay"));
		E.Then(BeginPlay, Print);
		auto* Branch = E.Add<UK2Node_IfThenElse>();
		E.Link(FGraph::Value(E.Get(TEXT("bArmed")), EGPD_Output), FGraph::Pin(Branch, TEXT("Condition"), EGPD_Input));
		E.Then(Print, Branch);
		auto* Delay = E.Call(UKismetSystemLibrary::StaticClass(), TEXT("Delay"));
		E.Default(Delay, TEXT("Duration"), TEXT("1.5"));
		E.Link(FGraph::Pin(Branch, TEXT("then"), EGPD_Output), FGraph::Exec(Delay, EGPD_Input));
		UK2Node_Timeline* TimelineNode = nullptr;
		if (Timeline)
		{
			TimelineNode = E.Add<UK2Node_Timeline>([Timeline](UK2Node_Timeline* Node) { Node->TimelineName = RG_TIMELINE_VARIABLE(Timeline); });
			E.Link(FGraph::Exec(Delay, EGPD_Output), FGraph::Pin(TimelineNode, TEXT("Play"), EGPD_Input));
			auto* SetHealth = E.Set(TEXT("Health"), 300);
			E.Link(FGraph::Pin(TimelineNode, TEXT("Update"), EGPD_Output), FGraph::Exec(SetHealth, EGPD_Input));
			E.Link(FGraph::Pin(TimelineNode, TEXT("Alpha"), EGPD_Output), FGraph::Value(SetHealth, EGPD_Input));
		}
		if (A.Enum)
		{
			auto* Switch = E.Add<UK2Node_SwitchEnum>([&](UK2Node_SwitchEnum* Node) {
#if UE_AT_LEAST(5, 0)
				static_cast<INodeDependingOnEnumInterface*>(Node)->ReloadEnum(A.Enum); // SetEnum is not exported on 5.5
#else
				Node->Enum = A.Enum; // UE4: CreateCasePins calls SetEnum(Enum) when the pins are allocated
#endif
			});
			E.Link(FGraph::Value(E.Get(TEXT("State")), EGPD_Output), FGraph::Pin(Switch, TEXT("Selection"), EGPD_Input));
			E.Link(FGraph::Pin(Branch, TEXT("else"), EGPD_Output), FGraph::Exec(Switch, EGPD_Input));
		}

		if (UBlueprint* Macros = LoadObject<UBlueprint>(nullptr, TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros")))
		{
			UEdGraph* ForEach = nullptr;
			for (UEdGraph* Graph : Macros->MacroGraphs)
				if (Graph->GetName() == TEXT("ForEachLoop"))
					ForEach = Graph;
			UK2Node_Event* Tick = E.Event(Bp, AActor::StaticClass(), TEXT("ReceiveTick"));
			if (ForEach)
			{
				auto* Loop = E.Add<UK2Node_MacroInstance>([ForEach](UK2Node_MacroInstance* Node) { Node->SetMacroGraph(ForEach); }, INT_MIN, 600);
				E.Then(Tick, Loop);
				E.Link(FGraph::Value(E.Get(TEXT("Scores"), 700), EGPD_Output), FGraph::Pin(Loop, TEXT("Array"), EGPD_Input));
				auto* ToString = E.Call(UKismetStringLibrary::StaticClass(), TEXT("Conv_IntToString"), 700);
				E.Link(FGraph::Pin(Loop, TEXT("Array Element"), EGPD_Output), FGraph::Pin(ToString, TEXT("InInt"), EGPD_Input));
				auto* PrintScore = E.Call(UKismetSystemLibrary::StaticClass(), TEXT("PrintString"), 600);
				E.Link(FGraph::Pin(ToString, TEXT("ReturnValue"), EGPD_Output), FGraph::Pin(PrintScore, TEXT("InString"), EGPD_Input));
				E.Link(FGraph::Pin(Loop, TEXT("LoopBody"), EGPD_Output), FGraph::Exec(PrintScore, EGPD_Input));
			}
		}

		auto* Custom = E.Add<UK2Node_CustomEvent>([](UK2Node_CustomEvent* Node) { Node->CustomFunctionName = TEXT("OnRefEvent"); }, -400, 1000);
		Custom->CreateUserDefinedPin(TEXT("Amount"), PinType(UEdGraphSchema_K2::PC_Int), EGPD_Output);
		auto* SetAmmo = E.Set(TEXT("Ammo"), 1000);
		E.Then(Custom, SetAmmo);
		E.Link(FGraph::Pin(Custom, TEXT("Amount"), EGPD_Output), FGraph::Value(SetAmmo, EGPD_Input));
		auto* Cast = E.Add<UK2Node_DynamicCast>([](UK2Node_DynamicCast* Node) { Node->TargetType = ACharacter::StaticClass(); }, INT_MIN, 1000);
		auto* PlayerPawn = E.Call(UGameplayStatics::StaticClass(), TEXT("GetPlayerPawn"), 1100);
		E.Link(FGraph::Pin(PlayerPawn, TEXT("ReturnValue"), EGPD_Output), FGraph::Pin(Cast, TEXT("Object"), EGPD_Input));
		E.Then(SetAmmo, Cast);

		if (FObjectProperty* BoxProperty = FindFProperty<FObjectProperty>(Bp->SkeletonGeneratedClass, TEXT("Box")))
		{
			FMulticastDelegateProperty* Overlap = FindFProperty<FMulticastDelegateProperty>(UPrimitiveComponent::StaticClass(), TEXT("OnComponentBeginOverlap"));
			auto* Bound = E.Add<UK2Node_ComponentBoundEvent>([&](UK2Node_ComponentBoundEvent* Node) { Node->InitializeComponentBoundEventParams(BoxProperty, Overlap); }, -400, 1400);
			auto* PrintOverlap = E.Call(UKismetSystemLibrary::StaticClass(), TEXT("PrintString"), 1400);
			E.Default(PrintOverlap, TEXT("InString"), TEXT("Overlap"));
			E.Then(Bound, PrintOverlap);
		}
		else
			Fail(TEXT("Box property missing on skeleton class"));

		UEdGraph* Damage = AddFunction(Bp, TEXT("ComputeDamage"));
		{
			FGraph F(Damage);
			UK2Node_FunctionEntry* Entry = FindNode<UK2Node_FunctionEntry>(Damage);
			Entry->CreateUserDefinedPin(TEXT("Base"), FloatPin(), EGPD_Output);
			UK2Node_FunctionResult* Result = ResultNode(F, Entry);
			Result->CreateUserDefinedPin(TEXT("Damage"), FloatPin(), EGPD_Input);
			auto* Mul = F.Call(UKismetMathLibrary::StaticClass(), REF_MATH(Multiply), 200);
			F.Link(FGraph::Pin(Entry, TEXT("Base"), EGPD_Output), FGraph::Pin(Mul, TEXT("A"), EGPD_Input));
			F.Link(FGraph::Value(F.Get(TEXT("Health"), 300), EGPD_Output), FGraph::Pin(Mul, TEXT("B"), EGPD_Input));
			F.Link(FGraph::Pin(Mul, TEXT("ReturnValue"), EGPD_Output), FGraph::Pin(Result, TEXT("Damage"), EGPD_Input));
			F.Then(Entry, Result);
		}
		if (A.Struct)
		{
			UEdGraph* MakeData = AddFunction(Bp, TEXT("BuildData"));
			FGraph F(MakeData);
			UK2Node_FunctionEntry* Entry = FindNode<UK2Node_FunctionEntry>(MakeData);
			auto* Make = F.Add<UK2Node_MakeStruct>([&](UK2Node_MakeStruct* Node) { Node->StructType = A.Struct; }, 200, 200);
#if !UE_AT_LEAST(4, 26)
			// older Blueprint compilers reject unconnected array inputs on Make <struct>: hide those pins (default = empty)
			for (FOptionalPinFromProperty& Shown : Make->ShowPinForProperties)
				if (FindField<UArrayProperty>(A.Struct, Shown.PropertyName))
					Shown.bShowPin = false;
			Make->ReconstructNode();
#endif
			auto* SetData = F.Set(TEXT("Data"));
			F.Link(FGraph::Value(Make, EGPD_Output), FGraph::Value(SetData, EGPD_Input));
			F.Then(Entry, SetData);
		}

		for (FBPInterfaceDescription& Implemented : Bp->ImplementedInterfaces)
			for (UEdGraph* Graph : Implemented.Graphs)
			{
				FGraph F(Graph);
				UK2Node_FunctionEntry* Entry = FindNode<UK2Node_FunctionEntry>(Graph);
				UK2Node_FunctionResult* Result = FindNode<UK2Node_FunctionResult>(Graph);
				if (!Entry || !Result)
					continue;
				F.Then(Entry, Result);
				F.Default(Result, TEXT("Handled"), TEXT("true"));
			}

		if (UEdGraph* Construction = FBlueprintEditorUtils::FindUserConstructionScript(Bp))
		{
			FGraph C(Construction);
			UK2Node_FunctionEntry* Entry = FindNode<UK2Node_FunctionEntry>(Construction);
			auto* Scale = C.Call(USceneComponent::StaticClass(), TEXT("SetRelativeScale3D"));
			C.Link(FGraph::Value(C.Get(TEXT("Body")), EGPD_Output), FGraph::Pin(Scale, TEXT("self"), EGPD_Input));
			C.Default(Scale, TEXT("NewScale3D"), TEXT("1.0,1.0,1.25"));
			C.Then(Entry, Scale);
		}

		Compile(Bp);
		UObject* Cdo = Bp->GeneratedClass->GetDefaultObject();
		SetPropertyText(Cdo, TEXT("Health"), TEXT("100.0"));
		SetPropertyText(Cdo, TEXT("Ammo"), TEXT("30"));
		SetPropertyText(Cdo, TEXT("bArmed"), TEXT("True"));
		SetPropertyText(Cdo, TEXT("Label"), TEXT("\"Reference actor\""));
		SetPropertyText(Cdo, TEXT("Title"), TEXT("NSLOCTEXT(\"RefGen\", \"Title\", \"Reference Title\")"));
		SetPropertyText(Cdo, TEXT("Id"), TEXT("RefActorId"));
		SetPropertyText(Cdo, TEXT("Offset"), TEXT("(X=10.0,Y=20.0,Z=30.0)"));
		SetPropertyText(Cdo, TEXT("Tint"), TEXT("(R=0.5,G=0.25,B=1.0,A=1.0)"));
		SetPropertyText(Cdo, TEXT("MeshAsset"), *FString::Printf(TEXT("\"%s\""), *A.StaticMesh->GetPathName()));
		SetPropertyText(Cdo, TEXT("SoftMesh"), *FString::Printf(TEXT("\"%s\""), *A.SkeletalMesh->GetPathName()));
		SetPropertyText(Cdo, TEXT("SpawnClass"), TEXT("\"/Script/Engine.PointLight\""));
		SetPropertyText(Cdo, TEXT("Scores"), TEXT("(1,2,3,40)"));
#if UE_AT_LEAST(4, 15) // Blueprint set/map variables are 4.15+
		SetPropertyText(Cdo, TEXT("RefTags"), TEXT("(\"Hot\",\"Cold\")"));
		SetPropertyText(Cdo, TEXT("Weights"), TEXT("((\"Head\", 2.0),(\"Body\", 1.0))"));
#endif
		SetPropertyText(Cdo, TEXT("Row"), *FString::Printf(TEXT("(DataTable=\"%s\",RowName=\"Weapon_1\")"), *A.Table->GetPathName()));
		Cdo->MarkPackageDirty();
	}

	void MakeChild(FAssets& A)
	{
		if (!A.Actor)
			return;
		A.ActorChild = NewBlueprint(TEXT("BP_RefActor_Child"), TEXT("/Game/Blueprints"), A.Actor->GeneratedClass);
		UBlueprint* Bp = A.ActorChild;
		if (!Bp)
			return;
#if UE_AT_LEAST(4, 7) // inherited component overrides (ICH) are 4.7+
		UInheritableComponentHandler* Handler = Bp->GetInheritableComponentHandler(true);
		for (USCS_Node* Node : A.Actor->SimpleConstructionScript->GetAllNodes())
		{
			const FName Name = Node->GetVariableName();
			if (Name != TEXT("Body") && Name != TEXT("PointLight") && Name != TEXT("Text"))
				continue;
			UActorComponent* Override = Handler->CreateOverridenComponentTemplate(FComponentKey(Node));
			if (auto* Mesh = Cast<UStaticMeshComponent>(Override))
			{
				Mesh->SetMaterial(0, A.InstanceRed);
				Mesh->SetRelativeScale3D(FVector(1.5f));
			}
			else if (auto* Light = Cast<UPointLightComponent>(Override))
			{
				Light->SetIntensity(20000.f);
				Light->SetLightColor(FLinearColor::Blue);
			}
			else if (auto* Text = Cast<UTextRenderComponent>(Override))
				RG_SET_RENDER_TEXT(Text, TEXT("Child override"));
			else
				Fail(FString::Printf(TEXT("ICH override %s"), *Name.ToString()));
		}
#endif
		if (auto* Extra = Template<UStaticMeshComponent>(AddComponent(Bp, UStaticMeshComponent::StaticClass(), TEXT("ExtraMesh"))))
		{
			Extra->SetStaticMesh(A.StaticMesh);
			Extra->SetRelativeLocation(FVector(0, 0, 400));
		}
		Compile(Bp);
		UObject* Cdo = Bp->GeneratedClass->GetDefaultObject();
		SetPropertyText(Cdo, TEXT("Health"), TEXT("50.0"));
		SetPropertyText(Cdo, TEXT("Scores"), TEXT("(9,8)"));
		Cdo->MarkPackageDirty();
	}

	void MakeGameplay(FAssets& A)
	{
		A.Character = NewBlueprint(TEXT("BP_RefCharacter"), TEXT("/Game/Blueprints"), ACharacter::StaticClass());
		Compile(A.Character);
		if (A.Character)
		{
			ACharacter* Cdo = CastChecked<ACharacter>(A.Character->GeneratedClass->GetDefaultObject());
#if UE_AT_LEAST(5, 1)
			RG_CHAR_MESH(Cdo)->SetSkeletalMeshAsset(A.SkeletalMesh);
#else
			RG_CHAR_MESH(Cdo)->SkeletalMesh = A.SkeletalMesh;
#endif
			if (A.AnimBlueprint)
				RG_CHAR_MESH(Cdo)->SetAnimInstanceClass(A.AnimBlueprint->GeneratedClass);
			RG_CHAR_MESH(Cdo)->SetRelativeLocation(FVector(0, 0, -90));
			RG_CHAR_MOVEMENT(Cdo)->MaxWalkSpeed = 450.f;
			RG_CHAR_MOVEMENT(Cdo)->JumpZVelocity = 600.f;
			Cdo->MarkPackageDirty();
		}
		A.Controller = NewBlueprint(TEXT("BP_RefController"), TEXT("/Game/Blueprints"), APlayerController::StaticClass());
		Compile(A.Controller);
		A.GameMode = NewBlueprint(TEXT("BP_RefGameMode"), TEXT("/Game/Blueprints"), AGameModeBase::StaticClass());
		Compile(A.GameMode);
		if (A.GameMode && A.Character && A.Controller)
		{
			AGameModeBase* Cdo = CastChecked<AGameModeBase>(A.GameMode->GeneratedClass->GetDefaultObject());
			Cdo->DefaultPawnClass = *A.Character->GeneratedClass;
			Cdo->PlayerControllerClass = *A.Controller->GeneratedClass;
			Cdo->MarkPackageDirty();
		}

#if RG_GAMEPLAY
		A.Effect = NewBlueprint(TEXT("GE_RefDamage"), TEXT("/Game/Abilities"), UGameplayEffect::StaticClass());
		Compile(A.Effect);
		if (A.Effect)
		{
			UGameplayEffect* Cdo = CastChecked<UGameplayEffect>(A.Effect->GeneratedClass->GetDefaultObject());
			Cdo->DurationPolicy = EGameplayEffectDurationType::HasDuration;
			Cdo->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.f));
			FGameplayModifierInfo Modifier;
			Modifier.Attribute = FGameplayAttribute(FindFProperty<FProperty>(URefAttributeSet::StaticClass(), TEXT("Health")));
			Modifier.ModifierOp = EGameplayModOp::Additive;
			FScalableFloat Magnitude(-1.f);
			Magnitude.Curve.CurveTable = A.CurveTable;
			Magnitude.Curve.RowName = TEXT("Damage");
			Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Magnitude);
			Cdo->Modifiers.Add(Modifier);
			Cdo->Period = FScalableFloat(0.5f);
			Cdo->MarkPackageDirty();
		}
		A.Ability = NewBlueprint(TEXT("GA_RefDash"), TEXT("/Game/Abilities"), UGameplayAbility::StaticClass());
		if (A.Ability)
		{
			FGraph E(EventGraphOf(A.Ability));
			UK2Node_Event* Activate = E.Event(A.Ability, UGameplayAbility::StaticClass(), TEXT("K2_ActivateAbility"));
			auto* End = E.Call(UGameplayAbility::StaticClass(), TEXT("K2_EndAbility"));
			E.Then(Activate, End);
			Compile(A.Ability);
			UObject* Cdo = A.Ability->GeneratedClass->GetDefaultObject();
			SetPropertyText(Cdo, TEXT("CostGameplayEffectClass"), *FString::Printf(TEXT("\"%s\""), *A.Effect->GeneratedClass->GetPathName()));
			Cdo->MarkPackageDirty();
		}
#endif
	}
}

void RefGen::MakeBlueprints(FAssets& A)
{
	MakeInterface(A);
	MakeLibraries(A);
	MakeSimple(A);
	MakeActor(A);
	MakeChild(A);
	MakeGameplay(A);
}
