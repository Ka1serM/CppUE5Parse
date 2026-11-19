#include "RefGenCommon.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureCube.h"
#include "Engine/TextureLightProfile.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/Material.h"
#if __has_include("MaterialDomain.h")
#include "MaterialDomain.h" // EMaterialDomain lives here on 5.x, not always pulled in transitively
#endif
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionStaticSwitchParameter.h"
#include "Materials/MaterialExpressionStaticBoolParameter.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionPanner.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionParticleSubUV.h"
#include "Materials/MaterialExpressionCollectionParameter.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionDesaturation.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/MaterialFunctionFactoryNew.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Factories/MaterialParameterCollectionFactoryNew.h"
#include "Misc/FileHelper.h"

namespace
{
	// Some factories aren't exported from UnrealEd in older engines (fine on Linux, a link error on Windows):
	// create them through reflection
	UFactory* NewFactoryByName(const TCHAR* ClassName)
	{
		UClass* Class = LoadClass<UFactory>(nullptr, *FString::Printf(TEXT("/Script/UnrealEd.%s"), ClassName));
		return Class ? NewObject<UFactory>(GetTransientPackage(), Class) : nullptr;
	}

	using namespace RefGen;

	template <typename T>
	T* MakeTexture(const FString& Name, int32 Size, int32 Slices, ETextureSourceFormat Format, TFunctionRef<void(int32, int32, int32, uint8*)> Pixel, int32 BytesPerPixel)
	{
		T* Texture = NewAsset<T>(Name, TEXT("/Game/Textures"));
		TArray<uint8> Data;
		Data.SetNumZeroed(Size * Size * Slices * BytesPerPixel);
		for (int32 Slice = 0; Slice < Slices; ++Slice)
			for (int32 Y = 0; Y < Size; ++Y)
				for (int32 X = 0; X < Size; ++X)
					Pixel(Slice, X, Y, &Data[((Slice * Size + Y) * Size + X) * BytesPerPixel]);
		Texture->Source.Init(Size, Size, Slices, 1, Format, Data.GetData());
		return Texture;
	}

	void Bgra(uint8* Out, uint8 R, uint8 G, uint8 B, uint8 Alpha = 255)
	{
		Out[0] = B;
		Out[1] = G;
		Out[2] = R;
		Out[3] = Alpha;
	}

	FExpressionInput& BaseColorOf(UMaterial* M)
	{
#if UE_AT_LEAST(5, 1)
		return M->GetEditorOnlyData()->BaseColor;
#else
		return M->BaseColor;
#endif
	}

#if UE_AT_LEAST(5, 1)
#define REF_MAT_INPUT(M, Name) (M)->GetEditorOnlyData()->Name
#else
#define REF_MAT_INPUT(M, Name) (M)->Name
#endif

	// by UClass so unexported expression classes (no ENGINE_API, e.g. ParticleColor on 5.5) can still be created
	UMaterialExpression* ExprOfClass(UClass* Class, UObject* Owner, int32 X, int32 Y)
	{
		UMaterialExpression* E = NewObject<UMaterialExpression>(Owner, Class, NAME_None, RF_Transactional);
		E->MaterialExpressionEditorX = X;
		E->MaterialExpressionEditorY = Y;
		if (UMaterial* M = Cast<UMaterial>(Owner))
		{
			E->Material = M;
#if UE_AT_LEAST(5, 1)
			M->GetExpressionCollection().AddExpression(E);
#else
			M->Expressions.Add(E);
#endif
		}
		else if (UMaterialFunction* F = Cast<UMaterialFunction>(Owner))
		{
#if UE_AT_LEAST(5, 1)
			F->GetExpressionCollection().AddExpression(E);
#else
			F->FunctionExpressions.Add(E);
#endif
		}
		return E;
	}

	UMaterialExpression* ExprByName(const TCHAR* ClassName, UObject* Owner, int32 X, int32 Y)
	{
		UClass* Class = FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/Engine.%s"), ClassName));
		check(Class);
		return ExprOfClass(Class, Owner, X, Y);
	}

	// Many UMaterialExpression* classes are not ENGINE_API exported (varies per version), so T::StaticClass()
	// fails to link. Look the class up by name instead; field access on T is header-only and fine.
	template <typename T>
	T* ExprNamed(const TCHAR* UClassName, UObject* Owner, int32 X, int32 Y)
	{
		return static_cast<T*>(ExprByName(UClassName + 1 /* drop the U prefix */, Owner, X, Y));
	}

	UMaterialExpressionScalarParameter* Scalar(UMaterial* M, const TCHAR* Name, float Value, int32 Y)
	{
		auto* P = ExprNamed<UMaterialExpressionScalarParameter>(TEXT("UMaterialExpressionScalarParameter"), M, -600, Y);
		P->ParameterName = Name;
		P->DefaultValue = Value;
		P->Group = TEXT("Surface");
		return P;
	}

	UMaterialExpressionVectorParameter* Vector(UMaterial* M, const TCHAR* Name, FLinearColor Value, int32 Y)
	{
		auto* P = ExprNamed<UMaterialExpressionVectorParameter>(TEXT("UMaterialExpressionVectorParameter"), M, -600, Y);
		P->ParameterName = Name;
		P->DefaultValue = Value;
		return P;
	}

	UMaterialExpressionTextureSampleParameter2D* TextureParam(UMaterial* M, const TCHAR* Name, UTexture* Texture, EMaterialSamplerType Sampler, int32 Y)
	{
		auto* P = ExprNamed<UMaterialExpressionTextureSampleParameter2D>(TEXT("UMaterialExpressionTextureSampleParameter2D"), M, -900, Y);
		P->ParameterName = Name;
		P->Texture = Texture;
		P->SamplerType = Sampler;
		return P;
	}

	UMaterial* NewMaterial(const FString& Name)
	{
		return Create<UMaterial>(Name, TEXT("/Game/Materials"), NewObject<UMaterialFactoryNew>());
	}

	void Finish(UMaterial* M)
	{
#if !UE_AT_LEAST(5, 0) && UE_AT_LEAST(4, 15) // on 4.14 the material resources only exist with a real client (null deref)
		// 4.27 only rebuilds the cached parameter list (static switches, ...) when GIsClient, which commandlets are not
		TGuardValue<bool> ClientGuard(GIsClient, true);
#endif
#if !UE_AT_LEAST(4, 15)
		// 4.14 flushes shader maps before allocating resources on a fresh material: allocate first
		// (commandlets never allocate them since FApp::CanEverRender() is false, so pretend we may render for the allocation)
		{
#if UE_AT_LEAST(4, 9)
			TGuardValue<bool> AllowRendering(PRIVATE_GAllowCommandletRendering, true);
#else // 4.8's CanEverRender() is just !IsRunningCommandlet()
			TGuardValue<bool> AllowRendering(PRIVATE_GIsRunningCommandlet, false);
#endif
			M->ForceRecompileForRendering();
		}
#endif
		M->PreEditChange(nullptr);
		M->PostEditChange();
		M->MarkPackageDirty();
	}
}

void RefGen::MakeTextures(FAssets& A)
{
	A.Albedo = MakeTexture<UTexture2D>(TEXT("T_Albedo"), 256, 1, TSF_BGRA8, [](int32, int32 X, int32 Y, uint8* P)
	{
		const bool Check = ((X / 32) + (Y / 32)) % 2 == 0;
		Bgra(P, Check ? 220 : 40, uint8(X), uint8(Y));
	}, 4);
	A.Albedo->SRGB = true;
	Touch(A.Albedo);

	A.Normal = MakeTexture<UTexture2D>(TEXT("T_Normal"), 256, 1, TSF_BGRA8, [](int32, int32 X, int32 Y, uint8* P)
	{
		const float NX = FMath::Sin(X * 0.1f) * 0.4f;
		const float NY = FMath::Cos(Y * 0.1f) * 0.4f;
		const float NZ = FMath::Sqrt(FMath::Max(0.f, 1.f - NX * NX - NY * NY));
		Bgra(P, uint8((NX * 0.5f + 0.5f) * 255), uint8((NY * 0.5f + 0.5f) * 255), uint8((NZ * 0.5f + 0.5f) * 255));
	}, 4);
	A.Normal->SRGB = false;
	A.Normal->CompressionSettings = TC_Normalmap;
	A.Normal->LODGroup = TEXTUREGROUP_WorldNormalMap;
	Touch(A.Normal);

	A.Mask = MakeTexture<UTexture2D>(TEXT("T_Mask"), 128, 1, TSF_G8, [](int32, int32 X, int32 Y, uint8* P)
	{
		P[0] = uint8((X ^ Y) & 0xFF);
	}, 1);
	A.Mask->SRGB = false;
	A.Mask->CompressionSettings = TC_Grayscale;
	Touch(A.Mask);

	A.Hdr = MakeTexture<UTexture2D>(TEXT("T_Hdr"), 64, 1, TSF_RGBA16F, [](int32, int32 X, int32 Y, uint8* P)
	{
		FFloat16* H = reinterpret_cast<FFloat16*>(P);
		H[0] = FFloat16(X / 8.f);
		H[1] = FFloat16(Y / 8.f);
		H[2] = FFloat16(4.f);
		H[3] = FFloat16(1.f);
	}, 8);
	A.Hdr->CompressionSettings = TC_HDR;
	A.Hdr->SRGB = false;
	Touch(A.Hdr);

	A.SubUV = MakeTexture<UTexture2D>(TEXT("T_SubUV"), 256, 1, TSF_BGRA8, [](int32, int32 X, int32 Y, uint8* P)
	{
		const int32 Cell = (Y / 128) * 2 + (X / 128);
		const float DX = (X % 128) - 64.f;
		const float DY = (Y % 128) - 64.f;
		const float Radius = 20.f + Cell * 10.f;
		const uint8 Alpha = DX * DX + DY * DY < Radius * Radius ? 255 : 0;
		Bgra(P, 255, uint8(64 * Cell), 32, Alpha);
	}, 4);
	A.SubUV->CompressionSettings = TC_EditorIcon;
	Touch(A.SubUV);

	A.Cube = MakeTexture<UTextureCube>(TEXT("T_Cube"), 64, 6, TSF_BGRA8, [](int32 Face, int32 X, int32 Y, uint8* P)
	{
		Bgra(P, uint8(40 * Face), uint8(X * 4), uint8(Y * 4));
	}, 4);
	Touch(A.Cube);

	A.RenderTarget = NewAsset<UTextureRenderTarget2D>(TEXT("RT_Ref"), TEXT("/Game/Textures"));
	RG_RT_SET_RGBA16F(A.RenderTarget);
	A.RenderTarget->InitAutoFormat(256, 256);
	A.RenderTarget->ClearColor = FLinearColor(0.1f, 0.2f, 0.3f, 1.f);
	Touch(A.RenderTarget);

	A.PhysMaterial = NewAsset<UPhysicalMaterial>(TEXT("PM_Ref"), TEXT("/Game/Materials"));
	A.PhysMaterial->Friction = 0.42f;
	A.PhysMaterial->Restitution = 0.3f;
	Touch(A.PhysMaterial);
}

void RefGen::MakeMaterials(FAssets& A)
{
	A.Collection = Create<UMaterialParameterCollection>(TEXT("MPC_Ref"), TEXT("/Game/Materials"), NewFactoryByName(TEXT("MaterialParameterCollectionFactoryNew")));
	{
		FCollectionScalarParameter S;
		S.ParameterName = TEXT("GlobalWetness");
		S.DefaultValue = 0.25f;
		A.Collection->ScalarParameters.Add(S);
		FCollectionVectorParameter V;
		V.ParameterName = TEXT("GlobalTint");
		V.DefaultValue = FLinearColor(0.9f, 0.95f, 1.f, 1.f);
		A.Collection->VectorParameters.Add(V);
		Touch(A.Collection);
	}

	A.Function = Create<UMaterialFunction>(TEXT("MF_Tint"), TEXT("/Game/Materials"), NewFactoryByName(TEXT("MaterialFunctionFactoryNew")));
	{
		UMaterialFunction* F = A.Function;
		F->Description = TEXT("Tints a color and applies fresnel");
		F->bExposeToLibrary = true;
		auto* InColor = ExprNamed<UMaterialExpressionFunctionInput>(TEXT("UMaterialExpressionFunctionInput"), F, -400, 0);
		InColor->InputName = TEXT("Color");
		InColor->InputType = FunctionInput_Vector3;
		InColor->SortPriority = 0;
		auto* InAmount = ExprNamed<UMaterialExpressionFunctionInput>(TEXT("UMaterialExpressionFunctionInput"), F, -400, 150);
		InAmount->InputName = TEXT("Amount");
		InAmount->InputType = FunctionInput_Scalar;
		InAmount->SortPriority = 1;
		InAmount->bUsePreviewValueAsDefault = true;
		#if UE_AT_LEAST(5, 1)
		InAmount->PreviewValue = FVector4f(0.5f, 0, 0, 0);
#else
		InAmount->PreviewValue = FVector4(0.5f, 0, 0, 0);
#endif
		auto* Fresnel = ExprNamed<UMaterialExpressionFresnel>(TEXT("UMaterialExpressionFresnel"), F, -400, 300);
		auto* Tint = ExprNamed<UMaterialExpressionConstant3Vector>(TEXT("UMaterialExpressionConstant3Vector"), F, -400, 450);
		Tint->Constant = FLinearColor(1.f, 0.8f, 0.6f);
		auto* Lerp = ExprNamed<UMaterialExpressionLinearInterpolate>(TEXT("UMaterialExpressionLinearInterpolate"), F, -150, 100);
		Lerp->A.Expression = InColor;
		Lerp->B.Expression = Tint;
		auto* Mul = ExprNamed<UMaterialExpressionMultiply>(TEXT("UMaterialExpressionMultiply"), F, -300, 200);
		Mul->A.Expression = InAmount;
		Mul->B.Expression = Fresnel;
		Lerp->Alpha.Expression = Mul;
		auto* Out = ExprNamed<UMaterialExpressionFunctionOutput>(TEXT("UMaterialExpressionFunctionOutput"), F, 100, 100);
		Out->OutputName = TEXT("Result");
		Out->A.Expression = Lerp;
		InColor->ConditionallyGenerateId(true);
		InAmount->ConditionallyGenerateId(true);
		Out->ConditionallyGenerateId(true);
		Touch(F);
	}

	A.Opaque = NewMaterial(TEXT("M_Opaque"));
	{
		UMaterial* M = A.Opaque;
		auto* UV = ExprNamed<UMaterialExpressionTextureCoordinate>(TEXT("UMaterialExpressionTextureCoordinate"), M, -1200, 0);
		UV->UTiling = 2.f;
		UV->VTiling = 2.f;
		auto* Albedo = TextureParam(M, TEXT("BaseTexture"), A.Albedo, SAMPLERTYPE_Color, 0);
		Albedo->Coordinates.Expression = UV;
		auto* Tint = Vector(M, TEXT("Tint"), FLinearColor(1, 1, 1), 250);
		auto* Mul = ExprNamed<UMaterialExpressionMultiply>(TEXT("UMaterialExpressionMultiply"), M, -500, 0);
		Mul->A.Expression = Albedo;
		Mul->B.Expression = Tint;
		auto* Call = ExprNamed<UMaterialExpressionMaterialFunctionCall>(TEXT("UMaterialExpressionMaterialFunctionCall"), M, -300, 0);
		RG_SET_MATERIAL_FUNCTION(Call, A.Function);
		Call->FunctionInputs[0].Input.Expression = Mul;
		auto* Wet = ExprNamed<UMaterialExpressionCollectionParameter>(TEXT("UMaterialExpressionCollectionParameter"), M, -600, 500);
		Wet->Collection = A.Collection;
		Wet->ParameterName = TEXT("GlobalWetness");
		// same as UMaterialParameterCollection::GetParameterId, which 4.10 and older don't export
		for (const FCollectionScalarParameter& P : A.Collection->ScalarParameters)
			if (P.ParameterName == Wet->ParameterName)
				Wet->ParameterId = P.Id;
		Call->FunctionInputs[1].Input.Expression = Wet;
		auto* VC = ExprNamed<UMaterialExpressionVertexColor>(TEXT("UMaterialExpressionVertexColor"), M, -600, 650);
		auto* Switch = ExprNamed<UMaterialExpressionStaticSwitchParameter>(TEXT("UMaterialExpressionStaticSwitchParameter"), M, -100, 0);
		Switch->ParameterName = TEXT("UseVertexColor");
		Switch->DefaultValue = false;
		auto* VCMul = ExprNamed<UMaterialExpressionMultiply>(TEXT("UMaterialExpressionMultiply"), M, -250, 300);
		VCMul->A.Expression = Call;
		VCMul->B.Expression = VC;
		Switch->A.Expression = VCMul;
		Switch->B.Expression = Call;
		BaseColorOf(M).Expression = Switch;
		auto* Normal = TextureParam(M, TEXT("NormalTexture"), A.Normal, SAMPLERTYPE_Normal, 800);
		Normal->Coordinates.Expression = UV;
		REF_MAT_INPUT(M, Normal).Expression = Normal;
		auto* Mask = TextureParam(M, TEXT("MaskTexture"), A.Mask, SAMPLERTYPE_LinearGrayscale, 1100);
		auto* Rough = Scalar(M, TEXT("Roughness"), 0.6f, 1300);
		auto* RoughMul = ExprNamed<UMaterialExpressionMultiply>(TEXT("UMaterialExpressionMultiply"), M, -300, 1200);
		RoughMul->A.Expression = Mask;
		RoughMul->B.Expression = Rough;
		REF_MAT_INPUT(M, Roughness).Expression = RoughMul;
		REF_MAT_INPUT(M, Metallic).Expression = Scalar(M, TEXT("Metallic"), 0.1f, 1450);
		auto* Custom = ExprNamed<UMaterialExpressionCustom>(TEXT("UMaterialExpressionCustom"), M, -300, 1600);
		Custom->Code = TEXT("return saturate(sin(Time * 2.0) * 0.5 + 0.5) * Strength;");
		Custom->OutputType = CMOT_Float1;
		Custom->Description = TEXT("Pulse");
		Custom->Inputs.SetNum(2);
		Custom->Inputs[0].InputName = TEXT("Time");
		Custom->Inputs[0].Input.Expression = ExprNamed<UMaterialExpressionTime>(TEXT("UMaterialExpressionTime"), M, -600, 1600);
		Custom->Inputs[1].InputName = TEXT("Strength");
		Custom->Inputs[1].Input.Expression = Scalar(M, TEXT("PulseStrength"), 0.0f, 1750);
		auto* Emissive = ExprNamed<UMaterialExpressionMultiply>(TEXT("UMaterialExpressionMultiply"), M, -100, 1600);
		Emissive->A.Expression = Custom;
		Emissive->B.Expression = Tint;
		REF_MAT_INPUT(M, EmissiveColor).Expression = Emissive;
		M->bUsedWithSkeletalMesh = true;
		M->bUsedWithInstancedStaticMeshes = true;
		M->bUsedWithStaticLighting = true;
#if UE_AT_LEAST(5, 0)
		M->bUsedWithNanite = true;
#endif
		M->PhysMaterial = A.PhysMaterial;
		Finish(M);
	}

	A.Masked = NewMaterial(TEXT("M_Masked"));
	{
		UMaterial* M = A.Masked;
		M->BlendMode = BLEND_Masked;
		M->TwoSided = true;
		auto* Albedo = TextureParam(M, TEXT("BaseTexture"), A.SubUV, SAMPLERTYPE_Color, 0);
		BaseColorOf(M).Expression = Albedo;
		REF_MAT_INPUT(M, OpacityMask).Connect(4, Albedo);
		M->OpacityMaskClipValue = 0.4f;
		M->bUsedWithInstancedStaticMeshes = true;
		Finish(M);
	}

	A.Translucent = NewMaterial(TEXT("M_Translucent"));
	{
		UMaterial* M = A.Translucent;
		M->BlendMode = BLEND_Translucent;
#if UE_AT_LEAST(5, 0)
		M->TranslucencyLightingMode = TLM_SurfacePerPixelLighting;
#endif
		BaseColorOf(M).Expression = Vector(M, TEXT("Color"), FLinearColor(0.2f, 0.5f, 1.f), 0);
		auto* Fresnel = ExprNamed<UMaterialExpressionFresnel>(TEXT("UMaterialExpressionFresnel"), M, -400, 300);
		REF_MAT_INPUT(M, Opacity).Expression = Fresnel;
		auto* Panner = ExprNamed<UMaterialExpressionPanner>(TEXT("UMaterialExpressionPanner"), M, -900, 500);
		Panner->SpeedX = 0.1f;
		Panner->SpeedY = 0.05f;
		auto* Sample = ExprNamed<UMaterialExpressionTextureSample>(TEXT("UMaterialExpressionTextureSample"), M, -600, 500);
		Sample->Texture = A.Normal;
		Sample->SamplerType = SAMPLERTYPE_Normal;
		Sample->Coordinates.Expression = Panner;
		REF_MAT_INPUT(M, Normal).Expression = Sample;
		REF_MAT_INPUT(M, Refraction).Expression = Scalar(M, TEXT("Refraction"), 1.1f, 700);
		Finish(M);
	}

	A.Unlit = NewMaterial(TEXT("M_Unlit"));
	{
		UMaterial* M = A.Unlit;
		M->SetShadingModel(MSM_Unlit);
		auto* Color = Vector(M, TEXT("EmissiveColor"), FLinearColor(4, 2, 0.5f), 0);
		auto* Hdr = TextureParam(M, TEXT("HdrTexture"), A.Hdr, SAMPLERTYPE_LinearColor, 300);
		auto* Mul = ExprNamed<UMaterialExpressionMultiply>(TEXT("UMaterialExpressionMultiply"), M, -300, 100);
		Mul->A.Expression = Color;
		Mul->B.Expression = Hdr;
		REF_MAT_INPUT(M, EmissiveColor).Expression = Mul;
		M->bUsedWithStaticLighting = true;
		Finish(M);
	}

	A.Decal = NewMaterial(TEXT("M_Decal"));
	{
		UMaterial* M = A.Decal;
		M->MaterialDomain = MD_DeferredDecal;
		M->BlendMode = BLEND_Translucent;
		auto* Albedo = TextureParam(M, TEXT("DecalTexture"), A.SubUV, SAMPLERTYPE_Color, 0);
		BaseColorOf(M).Expression = Albedo;
		REF_MAT_INPUT(M, Opacity).Connect(4, Albedo);
		Finish(M);
	}

	A.PostProcess = NewMaterial(TEXT("M_PostProcess"));
	{
		UMaterial* M = A.PostProcess;
		M->MaterialDomain = MD_PostProcess;
		M->SetShadingModel(MSM_Unlit);
		auto* Scene = ExprNamed<UMaterialExpressionSceneTexture>(TEXT("UMaterialExpressionSceneTexture"), M, -700, 0);
		Scene->SceneTextureId = PPI_PostProcessInput0;
		auto* Desat = ExprNamed<UMaterialExpressionDesaturation>(TEXT("UMaterialExpressionDesaturation"), M, -400, 0);
		Desat->Input.Connect(0, Scene);
		Desat->Fraction.Expression = Scalar(M, TEXT("Desaturation"), 0.3f, 200);
		REF_MAT_INPUT(M, EmissiveColor).Expression = Desat;
		Finish(M);
	}

	A.LightFunction = NewMaterial(TEXT("M_LightFunction"));
	{
		UMaterial* M = A.LightFunction;
		M->MaterialDomain = MD_LightFunction;
		M->SetShadingModel(MSM_Unlit);
		auto* Panner = ExprNamed<UMaterialExpressionPanner>(TEXT("UMaterialExpressionPanner"), M, -900, 0);
		Panner->SpeedX = 0.2f;
		auto* Sample = ExprNamed<UMaterialExpressionTextureSample>(TEXT("UMaterialExpressionTextureSample"), M, -600, 0);
		Sample->Texture = A.Mask;
		Sample->SamplerType = SAMPLERTYPE_LinearGrayscale;
		Sample->Coordinates.Expression = Panner;
		REF_MAT_INPUT(M, EmissiveColor).Expression = Sample;
		Finish(M);
	}

	A.Particle = NewMaterial(TEXT("M_Particle"));
	{
		UMaterial* M = A.Particle;
		M->BlendMode = BLEND_Translucent;
		M->SetShadingModel(MSM_Unlit);
		auto* SubUV = ExprNamed<UMaterialExpressionParticleSubUV>(TEXT("UMaterialExpressionParticleSubUV"), M, -700, 0);
		SubUV->Texture = A.SubUV;
		UMaterialExpression* Color = ExprNamed<UMaterialExpression>(TEXT("UMaterialExpressionParticleColor"), M, -700, 300);
		auto* Mul = ExprNamed<UMaterialExpressionMultiply>(TEXT("UMaterialExpressionMultiply"), M, -400, 0);
		Mul->A.Expression = SubUV;
		Mul->B.Expression = Color;
		REF_MAT_INPUT(M, EmissiveColor).Expression = Mul;
		auto* Alpha = ExprNamed<UMaterialExpressionMultiply>(TEXT("UMaterialExpressionMultiply"), M, -400, 300);
		Alpha->A.Connect(4, SubUV);
		Alpha->B.Connect(4, Color);
		REF_MAT_INPUT(M, Opacity).Expression = Alpha;
		M->bUsedWithParticleSprites = true;
		M->bUsedWithMeshParticles = true;
		M->bUsedWithBeamTrails = true;
#if UE_AT_LEAST(4, 21)
		M->bUsedWithNiagaraSprites = false;
#endif
		Finish(M);
	}

	auto MakeInstance = [](const FString& Name, UMaterialInterface* Parent)
	{
		auto* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
		Factory->InitialParent = Parent;
		return Create<UMaterialInstanceConstant>(Name, TEXT("/Game/Materials"), Factory);
	};
	A.InstanceRed = MakeInstance(TEXT("MI_Red"), A.Opaque);
	A.InstanceRed->SetVectorParameterValueEditorOnly(RG_MPARAM(TEXT("Tint")), FLinearColor(1, 0.1f, 0.1f));
	A.InstanceRed->SetScalarParameterValueEditorOnly(RG_MPARAM(TEXT("Roughness")), 0.25f);
#if UE_AT_LEAST(5, 0)
	A.InstanceRed->SetStaticSwitchParameterValueEditorOnly(RG_MPARAM(TEXT("UseVertexColor")), true);
#else
	{
		FStaticParameterSet Statics;
		A.InstanceRed->GetStaticParameterValues(Statics);
		bool bFound = false;
		for (FStaticSwitchParameter& Switch : Statics.StaticSwitchParameters)
			if (RG_SWITCH_NAME(Switch) == TEXT("UseVertexColor"))
			{
				Switch.Value = true;
				Switch.bOverride = true;
				bFound = true;
			}
		if (!bFound)
			Fail(TEXT("static switch UseVertexColor"));
		A.InstanceRed->UpdateStaticPermutation(Statics);
	}
#endif
	Touch(A.InstanceRed);

	A.InstanceBlue = MakeInstance(TEXT("MI_Blue"), A.Opaque);
	A.InstanceBlue->SetVectorParameterValueEditorOnly(RG_MPARAM(TEXT("Tint")), FLinearColor(0.1f, 0.2f, 1));
	A.InstanceBlue->SetTextureParameterValueEditorOnly(RG_MPARAM(TEXT("BaseTexture")), A.SubUV);
	A.InstanceBlue->SetScalarParameterValueEditorOnly(RG_MPARAM(TEXT("PulseStrength")), 2.f);
	Touch(A.InstanceBlue);

	A.InstanceChild = MakeInstance(TEXT("MI_Blue_Child"), A.InstanceBlue);
	A.InstanceChild->SetScalarParameterValueEditorOnly(RG_MPARAM(TEXT("Metallic")), 0.9f);
	Touch(A.InstanceChild);
}
