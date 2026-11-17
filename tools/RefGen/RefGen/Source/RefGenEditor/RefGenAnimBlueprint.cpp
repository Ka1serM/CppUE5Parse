#include "RefGenGraph.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimComposite.h"
#include "Animation/BlendSpace.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/AimOffsetBlendSpace.h"
#include "Animation/PoseAsset.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Engine/SkeletalMesh.h"
#include "Factories/AnimMontageFactory.h"
#include "Factories/AnimCompositeFactory.h"
#include "Factories/BlendSpaceFactoryNew.h"
#include "Factories/BlendSpaceFactory1D.h"
#include "Factories/AimOffsetBlendSpaceFactoryNew.h"
#include "Factories/PoseAssetFactory.h"
#include "Factories/AnimBlueprintFactory.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_StateMachine.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "AnimGraphNode_SaveCachedPose.h"
#include "AnimGraphNode_UseCachedPose.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_ApplyAdditive.h"
#include "AnimGraphNode_RotationOffsetBlendSpace.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "AnimGraphNode_PoseByName.h"
#include "AnimGraphNode_LocalToComponentSpace.h"
#include "AnimGraphNode_ComponentToLocalSpace.h"
#include "AnimGraphNode_ModifyBone.h"
#include "AnimGraphNode_TwoBoneIK.h"
#include "AnimGraphNode_StateResult.h"
#include "AnimGraphNode_TransitionResult.h"
#include "AnimationStateMachineGraph.h"
#include "AnimationStateGraph.h"
#include "AnimationTransitionGraph.h"
#include "AnimStateNode.h"
#include "AnimStateTransitionNode.h"
#include "AnimStateEntryNode.h"
#include "AnimationGraph.h"

namespace
{
	using namespace RefGen;

	void SetAxis(UBlendSpaceAny* Space, int32 Axis, const TCHAR* Name, float Min, float Max, int32 Grid)
	{
		FStructProperty* Property = FindFProperty<FStructProperty>(UBlendSpaceAny::StaticClass(), TEXT("BlendParameters"));
		FBlendParameter* Params = Property->ContainerPtrToValuePtr<FBlendParameter>(Space, Axis);
		Params->DisplayName = Name;
		Params->Min = Min;
		Params->Max = Max;
		Params->GridNum = Grid;
	}

	bool IsPosePin(const UEdGraphPin* Pin)
	{
		const UScriptStruct* Struct = Cast<UScriptStruct>(Pin->PinType.PinSubCategoryObject.Get());
		return Struct && (Struct == FPoseLink::StaticStruct() || Struct == FComponentSpacePoseLink::StaticStruct());
	}

	UEdGraphPin* PosePin(UEdGraphNode* Node, EEdGraphPinDirection Direction, int32 Index = 0)
	{
		if (!Node)
			return nullptr;
		for (UEdGraphPin* Pin : Node->Pins)
			if (Pin->Direction == Direction && IsPosePin(Pin) && Index-- == 0)
				return Pin;
		Fail(FString::Printf(TEXT("pose pin %d on %s"), Index, *Node->GetName()));
		return nullptr;
	}

	void LinkPose(FGraph& G, UEdGraphNode* From, UEdGraphNode* To, int32 InputIndex = 0)
	{
		G.Link(PosePin(From, EGPD_Output), PosePin(To, EGPD_Input, InputIndex));
	}

	template <typename T>
	T* FindNode(UEdGraph* Graph)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
			if (T* Typed = Cast<T>(Node))
				return Typed;
		return nullptr;
	}

	UAnimStateNode* AddState(UAnimationStateMachineGraph* Machine, const TCHAR* Name, int32 X)
	{
		FGraph G(Machine);
		UAnimStateNode* State = G.Add<UAnimStateNode>([](UAnimStateNode*) {}, X, 0);
		FBlueprintEditorUtils::RenameGraph(State->BoundGraph, Name);
		return State;
	}

	UAnimStateTransitionNode* AddTransition(UAnimationStateMachineGraph* Machine, UAnimStateNode* From, UAnimStateNode* To)
	{
		FGraph G(Machine);
		UAnimStateTransitionNode* Transition = G.Add<UAnimStateTransitionNode>([](UAnimStateTransitionNode*) {}, 0, 200);
		Transition->CreateConnections(From, To);
		Transition->CrossfadeDuration = 0.25f;
		return Transition;
	}
}

void RefGen::MakeAnimBlueprint(FAssets& A)
{
	if (!A.Skeleton || !A.Bend || !A.Twist)
		return;

	{
		auto* Factory = NewObject<UAnimMontageFactory>();
		Factory->TargetSkeleton = A.Skeleton;
		Factory->SourceAnimation = A.Bend;
#if UE_AT_LEAST(4, 16)
		Factory->PreviewSkeletalMesh = A.SkeletalMesh;
#endif
		A.Montage = Create<UAnimMontage>(TEXT("AM_Ref"), TEXT("/Game/Animations"), Factory);
	}
	if (UAnimMontage* M = A.Montage)
	{
		M->AddAnimCompositeSection(TEXT("Middle"), 0.5f);
		M->AddAnimCompositeSection(TEXT("End"), 1.0f);
		if (M->SlotAnimTracks.Num() > 0)
		{
#if UE_AT_LEAST(4, 17)
			FSlotAnimationTrack& Upper = M->AddSlot(TEXT("UpperBody"));
#else
			FSlotAnimationTrack& Upper = RefGen::AddDefaulted(M->SlotAnimTracks);
			Upper.SlotName = TEXT("UpperBody");
#endif
			FAnimSegment Segment;
			RG_SET_SEGMENT_ANIM(Segment, A.Twist);
			Segment.StartPos = 0.f;
			Upper.AnimTrack.AnimSegments.Add(Segment);
		}
		FAnimNotifyEvent& Branch = RefGen::AddDefaulted(M->Notifies);
		Branch.NotifyName = TEXT("BranchPoint");
		RG_NOTIFY_LINK(Branch, M, 0.4f);
#if UE_AT_LEAST(4, 7) // 4.7+
		Branch.MontageTickType = EMontageNotifyTickType::BranchingPoint;
#endif
#if UE_AT_LEAST(4, 11)
		M->BlendIn.SetBlendTime(0.2f);
		M->BlendOut.SetBlendTime(0.3f);
#else // FAlphaBlend montage blends are 4.11+
		M->BlendInTime = 0.2f;
		M->BlendOutTime = 0.3f;
#endif
		// some montage factories (older ones, and 4.24's in a commandlet) create no notify track; notifies need track 0
		if (M->AnimNotifyTracks.Num() == 0)
			RefGen::AddDefaulted(M->AnimNotifyTracks).TrackName = TEXT("1");
		RG_REFRESH_CACHE(M);
		Touch(M);
	}

	{
		auto* Factory = NewObject<UAnimCompositeFactory>();
		Factory->TargetSkeleton = A.Skeleton;
		Factory->SourceAnimation = A.Twist;
		A.Composite = Create<UAnimComposite>(TEXT("AC_Ref"), TEXT("/Game/Animations"), Factory);
		if (A.Composite)
		{
			FAnimSegment Segment;
			RG_SET_SEGMENT_ANIM(Segment, A.Bend);
			Segment.StartPos = A.Composite->AnimationTrack.GetLength();
			A.Composite->AnimationTrack.AnimSegments.Add(Segment);
			Touch(A.Composite);
		}
	}

	{
		auto* Factory = NewObject<UBlendSpaceFactory1D>();
		Factory->TargetSkeleton = A.Skeleton;
		A.BlendSpace1D = Create<UBlendSpace1D>(TEXT("BS1D_Speed"), TEXT("/Game/Animations"), Factory);
		if (A.BlendSpace1D)
		{
			SetAxis(A.BlendSpace1D, 0, TEXT("Speed"), 0.f, 600.f, 4);
			RG_BS_ADD_SAMPLE(A.BlendSpace1D, A.Twist, FVector(0, 0, 0));
			RG_BS_ADD_SAMPLE(A.BlendSpace1D, A.Bend, FVector(600, 0, 0));
			RG_BS_VALIDATE(A.BlendSpace1D);
			Touch(A.BlendSpace1D);
		}
	}
	{
		auto* Factory = NewObject<UBlendSpaceFactoryNew>();
		Factory->TargetSkeleton = A.Skeleton;
		A.BlendSpace2D = Create<UBlendSpace>(TEXT("BS_Locomotion"), TEXT("/Game/Animations"), Factory);
		if (A.BlendSpace2D)
		{
			SetAxis(A.BlendSpace2D, 0, TEXT("Direction"), -180.f, 180.f, 4);
			SetAxis(A.BlendSpace2D, 1, TEXT("Speed"), 0.f, 600.f, 3);
			RG_BS_ADD_SAMPLE(A.BlendSpace2D, A.Twist, FVector(0, 0, 0));
			RG_BS_ADD_SAMPLE(A.BlendSpace2D, A.Bend, FVector(0, 600, 0));
			RG_BS_ADD_SAMPLE(A.BlendSpace2D, A.Bend, FVector(-180, 600, 0));
			RG_BS_ADD_SAMPLE(A.BlendSpace2D, A.Twist, FVector(180, 600, 0));
			RG_BS_VALIDATE(A.BlendSpace2D);
			Touch(A.BlendSpace2D);
		}
	}
	if (A.AimCenter && A.AimLeft && A.AimRight)
	{
		auto* Factory = NewObject<UAimOffsetBlendSpaceFactoryNew>();
		Factory->TargetSkeleton = A.Skeleton;
		A.AimOffset = Create<UAimOffsetBlendSpace>(TEXT("AO_Ref"), TEXT("/Game/Animations"), Factory);
		if (A.AimOffset)
		{
			SetAxis(A.AimOffset, 0, TEXT("Yaw"), -90.f, 90.f, 2);
			SetAxis(A.AimOffset, 1, TEXT("Pitch"), -90.f, 90.f, 2);
			RG_BS_ADD_SAMPLE(A.AimOffset, A.AimCenter, FVector(0, 0, 0));
			RG_BS_ADD_SAMPLE(A.AimOffset, A.AimLeft, FVector(-90, 0, 0));
			RG_BS_ADD_SAMPLE(A.AimOffset, A.AimRight, FVector(90, 0, 0));
			RG_BS_VALIDATE(A.AimOffset);
			Touch(A.AimOffset);
		}
	}
#if UE_AT_LEAST(4, 13) // pose assets are 4.13+
	{
		auto* Factory = NewObject<UPoseAssetFactory>();
		Factory->TargetSkeleton = A.Skeleton;
		Factory->SourceAnimation = A.Bend;
		A.PoseAsset = Create<UPoseAsset>(TEXT("PA_Ref"), TEXT("/Game/Animations"), Factory);
		if (A.PoseAsset && A.PoseAsset->GetNumPoses() == 0)
			A.PoseAsset->CreatePoseFromAnimation(A.Bend);
		// UpdatePoseFromAnimation also rebuilds the sorted curve index table, which CreatePoseFromAnimation skips
		// on some versions (5.3: evaluating PoseByName then asserts in FPoseDataContainer::GetPoseCurve)
		if (A.PoseAsset && A.PoseAsset->GetNumPoses() > 0)
			A.PoseAsset->UpdatePoseFromAnimation(A.Bend);
		Touch(A.PoseAsset);
	}
#endif

	A.Notify = FKismetEditorUtilities::CreateBlueprint(UAnimNotify::StaticClass(),
#if UE_AT_LEAST(4, 26)
		CreatePackage(TEXT("/Game/Animations/BP_RefNotify")),
#else
		CreatePackage(nullptr, TEXT("/Game/Animations/BP_RefNotify")),
#endif
		TEXT("BP_RefNotify"), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
	FAssetRegistryModule::AssetCreated(A.Notify);
	FKismetEditorUtilities::CompileBlueprint(A.Notify);
	Touch(A.Notify);
	if (A.Montage && A.Notify)
	{
		FAnimNotifyEvent& Event = RefGen::AddDefaulted(A.Montage->Notifies);
		RG_NOTIFY_LINK(Event, A.Montage, 0.7f);
		Event.Notify = NewObject<UAnimNotify>(A.Montage, A.Notify->GeneratedClass, NAME_None, RF_Transactional);
#if !UE_AT_LEAST(4, 15) // older montage factories create no notify track; notifies need track 0
		if (A.Montage->AnimNotifyTracks.Num() == 0)
			RefGen::AddDefaulted(A.Montage->AnimNotifyTracks).TrackName = TEXT("1");
#endif
		RG_REFRESH_CACHE(A.Montage);
		Touch(A.Montage);
	}

	auto* Factory = NewObject<UAnimBlueprintFactory>();
	Factory->TargetSkeleton = A.Skeleton;
	Factory->ParentClass = UAnimInstance::StaticClass();
	Factory->BlueprintType = BPTYPE_Normal;
#if UE_AT_LEAST(4, 16)
	Factory->PreviewSkeletalMesh = A.SkeletalMesh;
#endif
	UAnimBlueprint* Abp = Create<UAnimBlueprint>(TEXT("ABP_Ref"), TEXT("/Game/Animations"), Factory);
	A.AnimBlueprint = Abp;
	if (!Abp)
		return;

	FBlueprintEditorUtils::AddMemberVariable(Abp, TEXT("Speed"), FloatPin());
	FBlueprintEditorUtils::AddMemberVariable(Abp, TEXT("bMoving"), PinType(UEdGraphSchema_K2::PC_Boolean));
	FBlueprintEditorUtils::AddMemberVariable(Abp, TEXT("AimYaw"), FloatPin());
	FKismetEditorUtilities::CompileBlueprint(Abp);

	UEdGraph* AnimGraph = nullptr;
	for (UEdGraph* Graph : Abp->FunctionGraphs)
		if (Graph->IsA<UAnimationGraph>())
			AnimGraph = Graph;
	if (!AnimGraph)
	{
		Fail(TEXT("AnimGraph missing"));
		return;
	}
	FGraph G(AnimGraph);
	UAnimGraphNode_Root* Root = FindNode<UAnimGraphNode_Root>(AnimGraph);

	UAnimGraphNode_StateMachine* MachineNode = G.Add<UAnimGraphNode_StateMachine>();
	UAnimationStateMachineGraph* Machine = MachineNode->EditorStateMachineGraph;
	FBlueprintEditorUtils::RenameGraph(Machine, TEXT("Locomotion"));
	UAnimStateNode* Idle = AddState(Machine, TEXT("Idle"), 300);
	UAnimStateNode* Move = AddState(Machine, TEXT("Move"), 700);
	{
		FGraph M(Machine);
		M.Link(Machine->EntryNode->Pins[0], Idle->GetInputPin());
	}
	{
		UAnimationStateGraph* State = CastChecked<UAnimationStateGraph>(Idle->BoundGraph);
		FGraph S(State);
		auto* Player = S.Add<UAnimGraphNode_SequencePlayer>([&](UAnimGraphNode_SequencePlayer* Node) { RG_SET_SEQUENCE(Node->Node, A.Twist); }, -300, 0);
		LinkPose(S, Player, State->MyResultNode);
	}
	{
		UAnimationStateGraph* State = CastChecked<UAnimationStateGraph>(Move->BoundGraph);
		FGraph S(State);
		auto* Player = S.Add<UAnimGraphNode_BlendSpacePlayer>([&](UAnimGraphNode_BlendSpacePlayer* Node) { RG_SET_BLENDSPACE(Node->Node, A.BlendSpace2D); }, -300, 0);
		LinkPose(S, Player, State->MyResultNode);
		S.Link(FGraph::Value(S.Get(TEXT("Speed")), EGPD_Output), FGraph::Pin(Player, TEXT("Y"), EGPD_Input));
	}
	{
		UAnimStateTransitionNode* ToMove = AddTransition(Machine, Idle, Move);
		UAnimationTransitionGraph* Rule = CastChecked<UAnimationTransitionGraph>(ToMove->BoundGraph);
		FGraph R(Rule);
		R.Link(FGraph::Value(R.Get(TEXT("bMoving")), EGPD_Output), FGraph::Pin(Rule->MyResultNode, TEXT("bCanEnterTransition"), EGPD_Input));
		UAnimStateTransitionNode* ToIdle = AddTransition(Machine, Move, Idle);
		UAnimationTransitionGraph* Back = CastChecked<UAnimationTransitionGraph>(ToIdle->BoundGraph);
		FGraph B(Back);
		auto* Not = B.Call(UKismetMathLibrary::StaticClass(), TEXT("Not_PreBool"));
		B.Link(FGraph::Value(B.Get(TEXT("bMoving")), EGPD_Output), FGraph::Pin(Not, TEXT("A"), EGPD_Input));
		B.Link(FGraph::Pin(Not, TEXT("ReturnValue"), EGPD_Output), FGraph::Pin(Back->MyResultNode, TEXT("bCanEnterTransition"), EGPD_Input));
	}

	auto* Save = G.Add<UAnimGraphNode_SaveCachedPose>([](UAnimGraphNode_SaveCachedPose* Node) { Node->CacheName = TEXT("LocomotionCache"); });
	LinkPose(G, MachineNode, Save);
	auto* Use = G.Add<UAnimGraphNode_UseCachedPose>([&](UAnimGraphNode_UseCachedPose* Node) {
#if UE_AT_LEAST(4, 6)
		Node->SaveCachedPoseNode = Save;
#else // 4.5 links the cache by name
		Node->NameOfCache = TEXT("LocomotionCache");
#endif
	});
	auto* Slot = G.Add<UAnimGraphNode_Slot>([](UAnimGraphNode_Slot* Node) { Node->Node.SlotName = TEXT("DefaultSlot"); });
	LinkPose(G, Use, Slot);
	auto* Additive = G.Add<UAnimGraphNode_ApplyAdditive>();
	auto* AdditivePlayer = G.Add<UAnimGraphNode_SequencePlayer>([&](UAnimGraphNode_SequencePlayer* Node) { RG_SET_SEQUENCE(Node->Node, A.BendAdditive); });
	LinkPose(G, Slot, Additive, 0);
	LinkPose(G, AdditivePlayer, Additive, 1);
	UEdGraphNode* Last = Additive;
	if (A.AimOffset)
	{
		auto* Aim = G.Add<UAnimGraphNode_RotationOffsetBlendSpace>([&](UAnimGraphNode_RotationOffsetBlendSpace* Node) { RG_SET_BLENDSPACE(Node->Node, A.AimOffset); });
		LinkPose(G, Last, Aim);
		G.Link(FGraph::Value(G.Get(TEXT("AimYaw")), EGPD_Output), FGraph::Pin(Aim, TEXT("X"), EGPD_Input));
		Last = Aim;
	}
	auto* Layered = G.Add<UAnimGraphNode_LayeredBoneBlend>([](UAnimGraphNode_LayeredBoneBlend* Node)
	{
		if (Node->Node.LayerSetup.Num() == 0)
			RefGen::AddDefaulted(Node->Node.LayerSetup);
		FBranchFilter Filter;
		Filter.BoneName = TEXT("bone_2");
		Filter.BlendDepth = 1;
		Node->Node.LayerSetup[0].BranchFilters.Add(Filter);
	});
	LinkPose(G, Last, Layered, 0);
#if UE_AT_LEAST(4, 13)
	if (A.PoseAsset && A.PoseAsset->GetNumPoses() > 0)
	{
		auto* Pose = G.Add<UAnimGraphNode_PoseByName>([&](UAnimGraphNode_PoseByName* Node)
		{
			Node->Node.PoseAsset = A.PoseAsset;
			Node->Node.PoseName = A.PoseAsset->GetPoseNameByIndex(A.PoseAsset->GetNumPoses() - 1);
		});
		LinkPose(G, Pose, Layered, 1);
	}
#endif
	auto* ToComponent = G.Add<UAnimGraphNode_LocalToComponentSpace>();
	LinkPose(G, Layered, ToComponent);
	auto* Modify = G.Add<UAnimGraphNode_ModifyBone>([](UAnimGraphNode_ModifyBone* Node)
	{
		Node->Node.BoneToModify.BoneName = TEXT("bone_3");
		Node->Node.RotationMode = BMM_Additive;
		Node->Node.Rotation = FRotator(0, 0, 15);
	});
	LinkPose(G, ToComponent, Modify);
	auto* TwoBone = G.Add<UAnimGraphNode_TwoBoneIK>([](UAnimGraphNode_TwoBoneIK* Node)
	{
		Node->Node.IKBone.BoneName = TEXT("bone_3");
		Node->Node.EffectorLocationSpace = BCS_ComponentSpace;
		Node->Node.EffectorLocation = FVector(20, 0, 120);
	});
	LinkPose(G, Modify, TwoBone);
	auto* ToLocal = G.Add<UAnimGraphNode_ComponentToLocalSpace>();
	LinkPose(G, TwoBone, ToLocal);
	LinkPose(G, ToLocal, Root);

	UEdGraph* EventGraph = Abp->UbergraphPages.Num() > 0 ? Abp->UbergraphPages[0] : nullptr;
	if (EventGraph)
	{
		FGraph E(EventGraph);
		UK2Node_Event* Update = E.Event(Abp, UAnimInstance::StaticClass(), TEXT("BlueprintUpdateAnimation"));
		auto* SetSpeed = E.Set(TEXT("Speed"));
		auto* Add = E.Call(UKismetMathLibrary::StaticClass(), REF_MATH(Add), 200);
		E.Link(FGraph::Value(E.Get(TEXT("Speed"), 300), EGPD_Output), FGraph::Pin(Add, TEXT("A"), EGPD_Input));
		E.Link(FGraph::Pin(Update, TEXT("DeltaTimeX"), EGPD_Output), FGraph::Pin(Add, TEXT("B"), EGPD_Input));
		E.Link(FGraph::Pin(Add, TEXT("ReturnValue"), EGPD_Output), FGraph::Value(SetSpeed, EGPD_Input));
		E.Then(Update, SetSpeed);
		auto* SetMoving = E.Set(TEXT("bMoving"));
		auto* Greater = E.Call(UKismetMathLibrary::StaticClass(), REF_MATH(Greater), 200);
		E.Link(FGraph::Value(E.Get(TEXT("Speed"), 300), EGPD_Output), FGraph::Pin(Greater, TEXT("A"), EGPD_Input));
		E.Default(Greater, TEXT("B"), TEXT("100.0"));
		E.Link(FGraph::Pin(Greater, TEXT("ReturnValue"), EGPD_Output), FGraph::Value(SetMoving, EGPD_Input));
		E.Then(SetSpeed, SetMoving);
	}
	else
		Fail(TEXT("ABP event graph missing"));

	FKismetEditorUtilities::CompileBlueprint(Abp);
	if (Abp->Status == BS_Error)
		Fail(TEXT("ABP_Ref compile error"));
	Touch(Abp);

	auto* ChildFactory = NewObject<UAnimBlueprintFactory>();
	ChildFactory->TargetSkeleton = A.Skeleton;
	ChildFactory->ParentClass = *Abp->GeneratedClass;
	ChildFactory->BlueprintType = BPTYPE_Normal;
	A.AnimBlueprintChild = Create<UAnimBlueprint>(TEXT("ABP_Ref_Child"), TEXT("/Game/Animations"), ChildFactory);
	if (A.AnimBlueprintChild)
	{
		FKismetEditorUtilities::CompileBlueprint(A.AnimBlueprintChild);
		Touch(A.AnimBlueprintChild);
	}
}
