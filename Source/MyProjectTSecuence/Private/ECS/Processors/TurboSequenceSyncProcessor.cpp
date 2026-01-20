// Copyright 2024 MyProjectTSec

#include "ECS/Processors/TurboSequenceSyncProcessor.h"
#include "ECS/Processors/TurboSequenceSolveProcessor.h"
#include "ECS/Processors/TurboSequenceCleanupProcessor.h"

#include "MassExecutionContext.h"
#include "MassEntitySubsystem.h"
#include "MassCommonFragments.h"
#include "ECS/Fragments/TurboSequenceInstanceFragment.h"
#include "ECS/Fragments/TileLODFragment.h"
#include "ECS/Tags/ZombiTag.h"

// TS
#include "TurboSequence_Lf/Public/TurboSequence_Manager_Lf.h"
#include "TurboSequence_Lf/Public/TurboSequence_MinimalData_Lf.h"
#include "TurboSequence_Lf/Public/TurboSequence_MeshAsset_Lf.h"

// Per-World subsystem
#include "ECS/TurboSequence/TurboSequenceWorldSubsystem.h"

// Utilities
#include "EngineUtils.h"
#include "Misc/ConfigCacheIni.h"

// CVars para gating
static TAutoConsoleVariable<float> CVarTsSyncEpsPos(
    TEXT("ts.Sync.EpsPos"),
    3.0f,
    TEXT("Threshold in UU for position delta to consider a TS transform dirty."),
    ECVF_Default);

static TAutoConsoleVariable<float> CVarTsSyncEpsYaw(
    TEXT("ts.Sync.EpsYaw"),
    5.0f,
    TEXT("Threshold in degrees for yaw delta to consider a TS transform dirty."),
    ECVF_Default);

static TAutoConsoleVariable<int32> CVarTsSyncWarmPeriod(
    TEXT("ts.Sync.WarmPeriod"),
    0,
    TEXT("If > 0, apply a periodic sync every N frames even if not dirty (coarse WARM gating)."),
    ECVF_Default);

static TAutoConsoleVariable<int32> CVarTsSyncColdEnabled(
    TEXT("ts.Sync.ColdEnabled"),
    0,
    TEXT("Reserved for future COLD mode; if 0, COLD is disabled."),
    ECVF_Default);

UTurboSequenceSyncProcessor::UTurboSequenceSyncProcessor()
{
    bAutoRegisterWithProcessingPhases = true;
    ProcessingPhase = EMassProcessingPhase::PostPhysics;
    bRequiresGameThreadExecution = true;
    // Ensure Cleanup runs before Sync within the same phase
    ExecutionOrder.ExecuteAfter.Add(UTurboSequenceCleanupProcessor::StaticClass()->GetFName());
    // Ensure Sync runs before Solve within the same phase
    ExecutionOrder.ExecuteBefore.Add(UTurboSequenceSolveProcessor::StaticClass()->GetFName());
    UE_LOG(LogTemp, Log, TEXT("TurboSequenceSyncProcessor: Initialized (Phase=PostPhysics)"));
}

void UTurboSequenceSyncProcessor::ConfigureQueries()
{
    EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
    EntityQuery.AddRequirement<FTurboSequenceInstanceFragment>(EMassFragmentAccess::ReadWrite);
    EntityQuery.AddRequirement<FTileLODFragment>(EMassFragmentAccess::ReadOnly);
    EntityQuery.AddTagRequirement<FZombiTag>(EMassFragmentPresence::All);
    EntityQuery.RegisterWithProcessor(*this);
}

static float GetDeltaYawDegrees(float A, float B)
{
    float Delta = FMath::FindDeltaAngleDegrees(A, B);
    return FMath::Abs(Delta);
}

void UTurboSequenceSyncProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    UWorld* World = EntityManager.GetWorld();
    if (!World || !World->IsGameWorld() || World->bIsTearingDown || GIsRequestingExit)
    {
        return;
    }

    // Resolver Manager per-World (sin usar globals cross-world)
    TWeakObjectPtr<ATurboSequence_Manager_Lf> ManagerWeak;
    if (UTurboSequenceWorldSubsystem* TSWorld = UTurboSequenceWorldSubsystem::Get(World))
    {
        ManagerWeak = TSWorld->GetOrFindManager(World);
    }

    const float EpsPos = FMath::Max(0.f, CVarTsSyncEpsPos.GetValueOnAnyThread());
    const float EpsPosSq = EpsPos * EpsPos;
    const float EpsYaw = FMath::Max(0.f, CVarTsSyncEpsYaw.GetValueOnAnyThread());
    const int32 WarmPeriod = FMath::Max(0, CVarTsSyncWarmPeriod.GetValueOnAnyThread());
    const bool bColdEnabled = (CVarTsSyncColdEnabled.GetValueOnAnyThread() != 0);

    EntityQuery.ForEachEntityChunk(EntityManager, Context, [World, ManagerWeak, EpsPosSq, EpsYaw, WarmPeriod, bColdEnabled](FMassExecutionContext& Context)
    {
        // Convertir el weak ptr a strong ptr dentro del lambda
        ATurboSequence_Manager_Lf* Manager = ManagerWeak.Get();
        if (!IsValid(Manager))
        {
            UE_LOG(LogTemp, Warning, TEXT("TS Sync: Skipping chunk - Manager is not valid"));
            return;
        }

        const TConstArrayView<FTransformFragment> TransformFragments = Context.GetFragmentView<FTransformFragment>();
        const TArrayView<FTurboSequenceInstanceFragment> TSFragments = Context.GetMutableFragmentView<FTurboSequenceInstanceFragment>();
        const TConstArrayView<FTileLODFragment> LODFrags = Context.GetFragmentView<FTileLODFragment>();

        // Diagnóstico: contadores por causa
        int32 ChunkCreated = 0;
        int32 ChunkAlreadyCreated = 0;
        int32 ChunkNoAsset = 0;
        int32 ChunkNoManager = 0;
        int32 ChunkInvalidMeshData = 0;

        const int32 NumEntities = Context.GetNumEntities();
        for (int32 i = 0; i < NumEntities; ++i)
        {
            const FTransform& Xf = TransformFragments[i].GetTransform();
            const FVector Loc = Xf.GetLocation();
            const float Yaw = Xf.Rotator().Yaw;

            FTurboSequenceInstanceFragment& TS = TSFragments[i];

            if (!TS.bInstanceCreated)
            {
                if (!TS.MeshAsset)
                {
                    // MeshAsset must be assigned by the spawner; skip entity until available
                    ++ChunkNoAsset;
                    UE_LOG(LogTemp, Warning, TEXT("TS Sync: MeshAsset is null; skipping TS creation for entity idx=%d"), i);
                    continue;
                }
                else if (!Manager)
                {
                    ++ChunkNoManager;
                    UE_LOG(LogTemp, Warning, TEXT("TS Sync: TurboSequence Manager not available in this World"));
                }
                else
                {
                    FTurboSequence_MeshSpawnData_Lf SpawnData;
                    SpawnData.RootMotionMesh.Mesh = TS.MeshAsset;
                    FTurboSequence_MinimalMeshData_Lf MeshData = ATurboSequence_Manager_Lf::AddSkinnedMeshInstance_GameThread(SpawnData, Xf, World);
                    if (MeshData.IsMeshDataValid())
                    {
                        TS.MeshData = MeshData;
                        TS.bInstanceCreated = true;

                        // Inicializar estado de gating
                        TS.LastSyncedLocation = Loc;
                        TS.LastSyncedYaw = Yaw;
                        TS.LastSyncedFrame = GFrameCounter;
                        TS.WarmCounter = WarmPeriod;

                        ATurboSequence_Manager_Lf::AddInstanceToUpdateGroup_RawID_Concurrent(0, MeshData.RootMotionMeshID);
                        ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_RawID_Concurrent(MeshData.RootMotionMeshID, Xf, true);

                        ++ChunkCreated;
                        UE_LOG(LogTemp, Log, TEXT("TS Sync: Instance created OK (ID=%d)"), MeshData.RootMotionMeshID);
                        if (TS.MeshAsset->OverrideDefaultAnimation)
                        {
                            FTurboSequence_AnimPlaySettings_Lf Settings;
                            ATurboSequence_Manager_Lf::PlayAnimation_RawID_Concurrent(MeshData.RootMotionMeshID, TS.MeshAsset->OverrideDefaultAnimation, Settings);
                            UE_LOG(LogTemp, Log, TEXT("TS Sync: Play OverrideDefaultAnimation"));
                        }
                    }
                    else
                    {
                        ++ChunkInvalidMeshData;
                        UE_LOG(LogTemp, Error, TEXT("TS Sync: AddSkinnedMeshInstance returned invalid MeshData for entity idx=%d"), i);
                    }
                }
            }
            else
            {
                ++ChunkAlreadyCreated;

                if (!TS.MeshData.IsMeshDataValid())
                {
                    continue;
                }

                // Gating por LOD: HOT/WARM/COLD
                const float PosDeltaSq = FVector::DistSquared2D(TS.LastSyncedLocation, Loc);
                const float YawDelta = GetDeltaYawDegrees(TS.LastSyncedYaw, Yaw);
                const bool bDirty = (PosDeltaSq > EpsPosSq) || (YawDelta > EpsYaw);

                bool bDoSync = false;
                const FTileLODFragment& LODFrag = LODFrags[i];
                switch (LODFrag.LOD)
                {
                    case ETileLOD::Hot:
                        bDoSync = bDirty;
                        break;

                    case ETileLOD::Warm:
                        bDoSync = bDirty;
                        if (!bDoSync && WarmPeriod > 0)
                        {
                            TS.WarmCounter = FMath::Max(0, TS.WarmCounter - 1);
                            if (TS.WarmCounter <= 0)
                            {
                                bDoSync = true;
                                TS.WarmCounter = WarmPeriod;
                            }
                        }
                        break;

                    case ETileLOD::Cold:
                        bDoSync = bColdEnabled && bDirty;
                        break;

                    default:
                        bDoSync = false;
                        break;
                }

                if (bDoSync)
                {
                    if (Manager && Manager->IsValidLowLevel() && !Manager->IsPendingKill())
                    {
                        ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_RawID_Concurrent(TS.MeshData.RootMotionMeshID, Xf, true);
                    }

                    TS.LastSyncedLocation = Loc;
                    TS.LastSyncedYaw = Yaw;
                    TS.LastSyncedFrame = GFrameCounter;
                }
            }
        }

        // Log summary for this chunk
        UE_LOG(LogTemp, Log, TEXT("TS Sync: Chunk summary -> Created=%d Already=%d NoAsset=%d NoManager=%d InvalidMeshData=%d"),
            ChunkCreated, ChunkAlreadyCreated, ChunkNoAsset, ChunkNoManager, ChunkInvalidMeshData);
    });
}