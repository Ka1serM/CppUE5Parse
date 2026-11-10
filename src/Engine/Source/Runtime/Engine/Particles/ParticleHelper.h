#pragma once

#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/Core/Math/RandomStream.h"
#include "Engine/Source/Runtime/Core/Math/Vector.h"

namespace ue5parse::runtime {

enum EParticleDetailMode : int32 { PDM_Low, PDM_Medium, PDM_High, PDM_Epic, PDM_MAX };
inline constexpr int32 PDM_DefaultValue = 0xFFFF;

#define INDEX_TYPEDATAMODULE (INDEX_NONE - 1)
#define INDEX_REQUIREDMODULE (INDEX_NONE - 2)
#define INDEX_SPAWNMODULE (INDEX_NONE - 3)

constexpr double HALF_WORLD_MAX = 8796093022208.0 * 0.5;
constexpr double UE_OLD_WORLD_MAX = 2097152.0;

struct FBaseParticle {
    FVector OldLocation;
    FVector Location;
    FVector3f BaseVelocity;
    float Rotation;
    FVector3f Velocity;
    float BaseRotationRate;
    FVector3f BaseSize;
    float RotationRate;
    FVector3f Size;
    int32 Flags;
    FLinearColor Color;
    FLinearColor BaseColor;
    float RelativeTime;
    float OneOverMaxLifetime;
    float Placeholder0;
    float Placeholder1;
};

enum EParticleStates : uint32 {
    STATE_Particle_JustSpawned = 0x02000000,
    STATE_Particle_Freeze = 0x04000000,
    STATE_Particle_IgnoreCollisions = 0x08000000,
    STATE_Particle_FreezeTranslation = 0x10000000,
    STATE_Particle_FreezeRotation = 0x20000000,
    STATE_Particle_CollisionIgnoreCheck =
        STATE_Particle_Freeze | STATE_Particle_IgnoreCollisions | STATE_Particle_FreezeTranslation | STATE_Particle_FreezeRotation,
    STATE_Particle_DelayCollisions = 0x40000000,
    STATE_Particle_CollisionHasOccurred = 0x80000000,
    STATE_Mask = 0xFE000000,
    STATE_CounterMask = ~STATE_Mask,
};

#define DECLARE_PARTICLE(Name, Address) FBaseParticle& Name = *((FBaseParticle*)(Address));
#define DECLARE_PARTICLE_CONST(Name, Address) const FBaseParticle& Name = *((const FBaseParticle*)(Address));
#define DECLARE_PARTICLE_PTR(Name, Address) FBaseParticle* Name = (FBaseParticle*)(Address);

#define BEGIN_UPDATE_LOOP                                                                                                                                      \
    {                                                                                                                                                          \
        [[maybe_unused]] int32& ActiveParticles = Owner->ActiveParticles;                                                                                      \
        [[maybe_unused]] uint32 CurrentOffset = Offset;                                                                                                        \
        const uint8* ParticleData = Owner->ParticleData;                                                                                                       \
        [[maybe_unused]] const uint32 ParticleStride = Owner->ParticleStride;                                                                                  \
        uint16* ParticleIndices = Owner->ParticleIndices;                                                                                                      \
        for (int32 i = ActiveParticles - 1; i >= 0; i--) {                                                                                                     \
            const int32 CurrentIndex = ParticleIndices[i];                                                                                                     \
            const uint8* ParticleBase = ParticleData + CurrentIndex * ParticleStride;                                                                          \
            FBaseParticle& Particle = *((FBaseParticle*)ParticleBase);                                                                                         \
            if ((Particle.Flags & STATE_Particle_Freeze) == 0) {
#define END_UPDATE_LOOP                                                                                                                                        \
    }                                                                                                                                                          \
    CurrentOffset = Offset;                                                                                                                                    \
    }                                                                                                                                                          \
    }

#define CONTINUE_UPDATE_LOOP                                                                                                                                   \
    CurrentOffset = Offset;                                                                                                                                    \
    continue;

#define SPAWN_INIT                                                                                                                                             \
    [[maybe_unused]] const int32 ActiveParticles = Owner->ActiveParticles;                                                                                     \
    [[maybe_unused]] const uint32 ParticleStride = Owner->ParticleStride;                                                                                      \
    [[maybe_unused]] uint32 CurrentOffset = Offset;                                                                                                            \
    FBaseParticle& Particle = *(ParticleBase);

#define PARTICLE_ELEMENT(Type, Name)                                                                                                                           \
    Type& Name = *((Type*)((uint8*)ParticleBase + CurrentOffset));                                                                                             \
    CurrentOffset += sizeof(Type);

#define KILL_CURRENT_PARTICLE                                                                                                                                  \
    {                                                                                                                                                          \
        ParticleIndices[i] = ParticleIndices[ActiveParticles - 1];                                                                                             \
        ParticleIndices[ActiveParticles - 1] = CurrentIndex;                                                                                                   \
        ActiveParticles--;                                                                                                                                     \
    }

inline void Particle_SetColorFromVector(const FVector3f& InColorVec, const float InAlpha, FLinearColor& OutColor) {
    OutColor.R = InColorVec.X;
    OutColor.G = InColorVec.Y;
    OutColor.B = InColorVec.Z;
    OutColor.A = InAlpha;
}

inline FVector3f GetParticleBaseSize(const FBaseParticle& Particle, bool bKeepFlipScale = false) {
    return bKeepFlipScale ? Particle.BaseSize : FVector3f(FMath::Abs(Particle.BaseSize.X), FMath::Abs(Particle.BaseSize.Y), FMath::Abs(Particle.BaseSize.Z));
}

inline FVector2D GetParticleSizeWithUVFlipInSign(const FBaseParticle& Particle, const FVector2D& ScaledSize) {
    return FVector2D(Particle.BaseSize.X >= 0.0f ? ScaledSize.X : -ScaledSize.X, Particle.BaseSize.Y >= 0.0f ? ScaledSize.Y : -ScaledSize.Y);
}

struct FFullSubUVPayload {
    float ImageIndex;
    float RandomImageTime;
};

struct FAttractorParticlePayload {
    int32 SourceIndex;
    uint32 SourcePointer;
    FVector3f SourceVelocity;
};

struct FLightParticlePayload {
    FVector3f ColorScale;
    uint64 LightId;
    float RadiusScale;
    float LightExponent;
    float InverseExposureBlend;
    bool bValid;
    bool bAffectsTranslucency;
    bool bHighQuality;
};

struct FMeshRotationPayloadData {
    FVector3f InitialOrientation;
    FVector3f InitRotation;
    FVector3f Rotation;
    FVector3f CurContinuousRotation;
    FVector3f RotationRate;
    FVector3f RotationRateBase;
};

struct FMeshMotionBlurPayloadData {
    FVector3f BaseParticlePrevVelocity;
    FVector3f BaseParticlePrevSize;
    FVector3f PayloadPrevRotation;
    FVector3f PayloadPrevOrbitOffset;
    float BaseParticlePrevRotation;
    float PayloadPrevCameraOffset;
};

struct FModuleLocationBoneSocketParticlePayload {
    int32 SourceIndex;
};

struct FOrbitChainModuleInstancePayload {
    FVector3f BaseOffset;
    FVector3f Offset;
    FVector3f Rotation;
    FVector3f BaseRotationRate;
    FVector3f RotationRate;
    FVector3f PreviousOffset;
};

struct FParticleSpawnPerUnitInstancePayload {
    float CurrentDistanceTravelled;
};

struct FParticleCollisionPayload {
    FVector3f UsedDampingFactor;
    FVector3f UsedDampingFactorRotation;
    int32 UsedCollisions;
    float Delay;
};

struct FParticleCollisionInstancePayload {
    uint8 CurrentLODBoundsCheckCount;
    uint8 Padding1;
    uint8 Padding2;
    uint8 Padding3;
};

struct FParticleEventInstancePayload {
    bool bSpawnEventsPresent;
    bool bDeathEventsPresent;
    bool bCollisionEventsPresent;
    bool bBurstEventsPresent;
    int32 SpawnTrackingCount;
    int32 DeathTrackingCount;
    int32 CollisionTrackingCount;
    int32 BurstTrackingCount;
};

struct FEmitterDynamicParameterPayload {
    float DynamicParameterValue[4];
};

struct FCameraOffsetParticlePayload {
    float BaseOffset;
    float Offset;
};

struct FParticleRandomSeedInstancePayload {
    FRandomStream RandomStream;
};

struct FLocationEmitterInstancePayload {
    int32 LastSelectedIndex;
};

template <class ElementType>
class TPreallocatedArrayProxy {
public:
    TPreallocatedArrayProxy() = default;
    TPreallocatedArrayProxy(uint8* ArrayStart, int32 ArraySize) : ArrayMax(ArraySize), Array((ElementType*)ArrayStart) {}

    ElementType& operator[](int32 Index) { return Array[Index]; }
    const ElementType& operator[](int32 Index) const { return Array[Index]; }
    int32 Find(const ElementType& Item) const {
        for (int32 i = 0; i < ArrayMax; i++)
            if (Array[i] == Item)
                return i;
        return INDEX_NONE;
    }
    bool Contains(const ElementType& Item) const { return Find(Item) != INDEX_NONE; }
    int32 Num() const { return ArrayMax; }

private:
    int32 ArrayMax = 0;
    ElementType* Array = nullptr;
};

}
