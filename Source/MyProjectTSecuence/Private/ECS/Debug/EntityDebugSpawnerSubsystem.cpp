// EntityDebugSpawnerSubsystem.cpp
#include "ECS/Debug/EntityDebugSpawnerSubsystem.h"

#include "MassEntityTemplateRegistry.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassEntityView.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "MassMovementFragments.h"
#include "ECS/Fragments/ZombiCoreFragment.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
// TurboSequence
#include "TurboSequence_Lf/Public/TurboSequence_Manager_Lf.h"
#include "TurboSequence_Lf/Public/TurboSequence_MinimalData_Lf.h"
#include "TurboSequence_Lf/Public/TurboSequence_MeshAsset_Lf.h"
#include "ECS/Fragments/TurboSequenceInstanceFragment.h"
#include "ECS/Fragments/TileLODFragment.h"
#include "ECS/Tags/TSPendingCleanupTag.h"


namespace
{
    // Default TS Mesh Asset can be overridden at runtime via console command Entity.SetTSAsset <ObjectPath>
    static TSoftObjectPtr<UTurboSequence_MeshAsset_Lf> GDefaultTSAsset = TSoftObjectPtr<UTurboSequence_MeshAsset_Lf>(
        FSoftObjectPath(TEXT("/Script/TurboSequence_Lf.TurboSequence_MeshAsset_Lf'/Game/Characters/Mannequins/TurboSequence/TS_Zombie_MeshAsset.TS_Zombie_MeshAsset'"))
    );
}
TArray<FAutoConsoleCommand*> UEntityDebugSpawnerSubsystem::ConsoleCommands;

void UEntityDebugSpawnerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogTemp, Log, TEXT("EntityDebugSpawnerSubsystem::Initialize"));

    // Intento inicial de cachear el Mass subsystem (puede no estar listo aún)
    if (UWorld* World = GetWorld())
    {
        MassEntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>();
    }
    // No llamamos SetupArchetype aún: lo hacemos bajo demanda en SpawnEntities si todavía no está listo

    // Register console commands regardless of Mass state
    RegisterConsoleCommands();
}

void UEntityDebugSpawnerSubsystem::Deinitialize()
{
    UnregisterConsoleCommands();

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DebugTimerHandle);
    }

    // Remove any TS instances before destroying entities to avoid shutdown crashes
    if (MassEntitySubsystem)
    {
        FMassEntityManager& EntityManager = MassEntitySubsystem->GetMutableEntityManager();
        for (const FMassEntityHandle& Entity : SpawnedEntities)
        {
            if (Entity.IsValid())
            {
                FMassEntityView View(EntityManager, Entity);
                if (const FTurboSequenceInstanceFragment* TS = View.GetFragmentDataPtr<FTurboSequenceInstanceFragment>())
                {
                    if (TS->bInstanceCreated && TS->MeshData.IsMeshDataValid())
                    {
                        ATurboSequence_Manager_Lf::RemoveSkinnedMeshInstance_GameThread(TS->MeshData, GetWorld());
                    }
                }
            }
        }
    }
    // Destroy any spawned entities
    if (MassEntitySubsystem)
    {
        FMassEntityManager& EntityManager = MassEntitySubsystem->GetMutableEntityManager();
        for (const FMassEntityHandle& Entity : SpawnedEntities)
        {
            if (Entity.IsValid())
            {
                EntityManager.DestroyEntity(Entity);
            }
        }
    }

    SpawnedEntities.Reset();

    Super::Deinitialize();
}

void UEntityDebugSpawnerSubsystem::SetupArchetype()
{
    if (!MassEntitySubsystem)
        return;

    TArray<const UScriptStruct*> Fragments = {
        FTransformFragment::StaticStruct(),
        FCellLocationFragment::StaticStruct(),
        FFlowReadFragment::StaticStruct(),
        FMoveFragment::StaticStruct(),
        FZombiCoreFragment::StaticStruct(),
        FTurboSequenceInstanceFragment::StaticStruct(),
        FTileLODFragment::StaticStruct() // <-- agregar
    };
    // Tags required by processors
    const UScriptStruct* RequiredTag = FZombiTag::StaticStruct();

    FMassEntityManager& EntityManager = MassEntitySubsystem->GetMutableEntityManager();
    // Try to create archetype including tag if API supports tags; otherwise we will add tag per-entity after creation
    Archetype = EntityManager.CreateArchetype(Fragments);
    UE_LOG(LogTemp, Log, TEXT("EntityDebugSpawnerSubsystem: Archetype created (Valid=%s)"), Archetype.IsValid() ? TEXT("true") : TEXT("false"));
}

void UEntityDebugSpawnerSubsystem::SpawnEntities(int32 Count, float Radius, FVector Origin)
{
    // Lazy resolve del Mass subsystem si no está cacheado
    if (!MassEntitySubsystem)
    {
        UWorld* ActiveWorld = ResolveActiveWorld();
        if (ActiveWorld)
        {
            MassEntitySubsystem = ActiveWorld->GetSubsystem<UMassEntitySubsystem>();
            UE_LOG(LogTemp, Log, TEXT("SpawnEntities: Resolved MassEntitySubsystem at runtime -> %s"), MassEntitySubsystem ? TEXT("OK") : TEXT("NULL"));
        }
    }
    // Si tenemos Mass y no hay arquetipo aún, créalo ahora
    if (MassEntitySubsystem && !Archetype.IsValid())
    {
        SetupArchetype();
    }
    if (!MassEntitySubsystem || !Archetype.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnEntities: Missing Mass subsystem or archetype not built yet"));
        return;
    }

    FMassEntityManager& EntityManager = MassEntitySubsystem->GetMutableEntityManager();

    TArray<FMassEntityHandle> NewEntities;
    NewEntities.Reserve(Count);
    EntityManager.BatchCreateEntities(Archetype, Count, NewEntities);
    UE_LOG(LogTemp, Log, TEXT("SpawnEntities: BatchCreateEntities requested=%d created=%d"), Count, NewEntities.Num());
    if (NewEntities.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnEntities: Batch create returned 0. Falling back to CreateEntity per-entity."));
        for (int32 i = 0; i < Count; ++i)
        {
            TArray<FInstancedStruct> FragmentList;

            FInstancedStruct TransformIS; TransformIS.InitializeAs<FTransformFragment>();
            FragmentList.Add(TransformIS);

            FInstancedStruct CellLocIS; CellLocIS.InitializeAs<FCellLocationFragment>();
            FragmentList.Add(CellLocIS);

            FInstancedStruct FlowIS; FlowIS.InitializeAs<FFlowReadFragment>();
            FragmentList.Add(FlowIS);

            FInstancedStruct MoveIS; MoveIS.InitializeAs<FMoveFragment>();
            FragmentList.Add(MoveIS);

            FInstancedStruct CoreIS; CoreIS.InitializeAs<FZombiCoreFragment>();
            FragmentList.Add(CoreIS);

            // Ensure the TurboSequence fragment is present even in fallback
            FInstancedStruct TSIS; TSIS.InitializeAs<FTurboSequenceInstanceFragment>();
            FragmentList.Add(TSIS);

            FInstancedStruct LODIS; LODIS.InitializeAs<FTileLODFragment>();
FragmentList.Add(LODIS);

            const FMassEntityHandle H = EntityManager.CreateEntity(FragmentList);
            if (H.IsValid())
            {
                EntityManager.AddTagToEntity(H, FZombiTag::StaticStruct());
                NewEntities.Add(H);
            }
        }
        UE_LOG(LogTemp, Log, TEXT("SpawnEntities: Fallback CreateEntity created=%d"), NewEntities.Num());
        if (NewEntities.Num() == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("SpawnEntities: Fallback creation also failed. Aborting."));
            return;
        }
    }

    // Iterate over actually created entities to ensure all get initialized
    const int32 NumSpawned = NewEntities.Num();

    // Cargar una sola vez el asset TS por batch (evita LoadSynchronous por entidad)
    UTurboSequence_MeshAsset_Lf* DefaultTSAsset = GDefaultTSAsset.LoadSynchronous();
    if (!DefaultTSAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnEntities: Default TS Mesh Asset failed to load. TS instances will be skipped until set."));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("SpawnEntities: Using DefaultTSAsset %s"), *DefaultTSAsset->GetPathName());
    }

    for (int32 i = 0; i < NumSpawned; ++i)
    {
        const float Angle = FMath::FRand() * 2.0f * PI;
        const float Distance = FMath::FRand() * Radius;
        const FVector SpawnLocation = Origin + FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, 0.0f);

        FMassEntityView View(EntityManager, NewEntities[i]);

        FTransformFragment& Transform = View.GetFragmentData<FTransformFragment>();
        Transform.GetMutableTransform().SetLocation(SpawnLocation);

        FMoveFragment& Move = View.GetFragmentData<FMoveFragment>();
        Move.Speed = FMath::RandRange(50.0f, 150.0f);

        // Initialize core fragment with minimal defaults
        FZombiCoreFragment& Core = View.GetFragmentData<FZombiCoreFragment>();
        Core.Speed = Move.Speed;

        FCellLocationFragment& CellLoc = View.GetFragmentData<FCellLocationFragment>();
        CellLoc.bValid = false;

        FFlowReadFragment& Flow = View.GetFragmentData<FFlowReadFragment>();
        Flow.bValid = false;

        // Ensure required tag is present so processors pick up the entity
        EntityManager.AddTagToEntity(NewEntities[i], FZombiTag::StaticStruct());

        // Initialize TurboSequence fragment so the TS sync processor can create the visual instance
        FTurboSequenceInstanceFragment& TS = View.GetFragmentData<FTurboSequenceInstanceFragment>();
        TS.bInstanceCreated = false;
        TS.MeshData = FTurboSequence_MinimalMeshData_Lf(false);

        // Assign shared MeshAsset (cargado una sola vez por batch)
        TS.MeshAsset = DefaultTSAsset;
        if (!TS.MeshAsset)
        {
            UE_LOG(LogTemp, Warning, TEXT("SpawnEntities: Default TS Mesh Asset is not set. Entity will skip TS creation until set."));
        }
        UE_LOG(LogTemp, Log, TEXT("SpawnEntities: entity[%d] TS.MeshAsset=%s"),i, TS.MeshAsset ? *TS.MeshAsset->GetPathName() : TEXT("NULL"));
        SpawnedEntities.Add(NewEntities[i]);

        UE_LOG(LogTemp, Verbose, TEXT("SpawnEntities: entity[%d] valid=%d index=%d"), i, NewEntities[i].IsValid() ? 1 : 0, NewEntities[i].Index);
    }
    // Fix-up: asegurar que todas las entidades tengan MeshAsset asignado
    if (DefaultTSAsset)
    {
        for (int32 k = 0; k < NewEntities.Num(); ++k)
        {
            FMassEntityView ViewFix(EntityManager, NewEntities[k]);
            if (!ViewFix.IsValid()) continue;
            if (FTurboSequenceInstanceFragment* TSFix = ViewFix.GetFragmentDataPtr<FTurboSequenceInstanceFragment>())
            {
                if (TSFix->MeshAsset == nullptr)
                {
                    TSFix->MeshAsset = DefaultTSAsset;
                    UE_LOG(LogTemp, Warning, TEXT("SpawnEntities: Fix-up applied: entity[%d] MeshAsset was NULL -> assigned default"), k);
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("SpawnEntities: spawned %d (initialized %d)"), Count, NumSpawned);

    // Enable debug spheres to visualize positions immediately
    SetDebugVisualization(true);
}

void UEntityDebugSpawnerSubsystem::ClearAllEntities()
{
    if (!MassEntitySubsystem)
        return;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DebugTimerHandle);
    }

    FMassEntityManager& EntityManager = MassEntitySubsystem->GetMutableEntityManager();
    for (const FMassEntityHandle& Entity : SpawnedEntities)
    {
        if (Entity.IsValid())
        {
            // Remove TurboSequence instance if present
            FMassEntityView View(EntityManager, Entity);
            if (const FTurboSequenceInstanceFragment* TS = View.GetFragmentDataPtr<FTurboSequenceInstanceFragment>())
            {
                if (TS->bInstanceCreated && TS->MeshData.IsMeshDataValid())
                {
                    ATurboSequence_Manager_Lf::RemoveSkinnedMeshInstance_GameThread(TS->MeshData, GetWorld());
                }
            }
            EntityManager.DestroyEntity(Entity);
        }
    }
    SpawnedEntities.Reset();

    UE_LOG(LogTemp, Log, TEXT("ClearAllEntities: done"));
}

void UEntityDebugSpawnerSubsystem::SetDebugVisualization(bool bEnable)
{
    bDebugVisualizationEnabled = bEnable;

    if (UWorld* World = GetWorld())
    {
        if (bDebugVisualizationEnabled)
        {
            UpdateDebugVisualization();
            World->GetTimerManager().SetTimer(DebugTimerHandle, this, &UEntityDebugSpawnerSubsystem::UpdateDebugVisualization, 0.1f, true);
        }
        else
        {
            World->GetTimerManager().ClearTimer(DebugTimerHandle);
        }
    }
}

void UEntityDebugSpawnerSubsystem::UpdateDebugVisualization()
{
    if (!bDebugVisualizationEnabled || !GetWorld() || !MassEntitySubsystem)
        return;

    FMassEntityManager& EntityManager = MassEntitySubsystem->GetMutableEntityManager();

    for (const FMassEntityHandle& Entity : SpawnedEntities)
    {
        if (!Entity.IsValid())
            continue;

        FMassEntityView View(EntityManager, Entity);
        if (const FTransformFragment* Transform = View.GetFragmentDataPtr<FTransformFragment>())
        {
            const FVector Location = Transform->GetTransform().GetLocation();
            DrawDebugSphere(GetWorld(), Location, DebugSphereRadius, 8, DebugSphereColor, false, 0.1f);

            if (const FFlowReadFragment* Flow = View.GetFragmentDataPtr<FFlowReadFragment>())
            {
                if (Flow->bValid)
                {
                    const FVector EndPoint = Location + Flow->DirWS * 100.0f;
                    DrawDebugDirectionalArrow(GetWorld(), Location, EndPoint, 50.0f, FColor::Green, false, 0.1f);
                }
            }
        }
    }
}

void UEntityDebugSpawnerSubsystem::ForceRegisterConsoleCommands()
{
    RegisterConsoleCommands();
}

UWorld* UEntityDebugSpawnerSubsystem::ResolveActiveWorld()
{
    if (!GEngine)
        return nullptr;

    for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
    {
        if (Ctx.WorldType == EWorldType::PIE || Ctx.WorldType == EWorldType::Game || Ctx.WorldType == EWorldType::GameRPC)
        {
            if (UWorld* W = Ctx.World())
            {
                return W;
            }
        }
    }

    if (UWorld* PlayWorld = GEngine->GetCurrentPlayWorld())
    {
        return PlayWorld;
    }

    return GEngine->GetWorld();
}

void UEntityDebugSpawnerSubsystem::RegisterConsoleCommands()
{
    // avoid duplicates
    UnregisterConsoleCommands();

    if (!GEngine)
        return;

    UE_LOG(LogTemp, Log, TEXT("EntityDebugSpawnerSubsystem: Registering console commands"));

    auto SpawnCmd = new FAutoConsoleCommand(
        TEXT("Entity.Spawn"),
        TEXT("Spawn entities. Usage: Entity.Spawn [Count=10] [Radius=1000] [X=0] [Y=0] [Z=0]"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&UEntityDebugSpawnerSubsystem::ExecuteSpawnEntities)
    );
    ConsoleCommands.Add(SpawnCmd);

    auto ClearCmd = new FAutoConsoleCommand(
        TEXT("Entity.Clear"),
        TEXT("Clear all spawned entities"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&UEntityDebugSpawnerSubsystem::ExecuteClearEntities)
    );
    ConsoleCommands.Add(ClearCmd);

    auto DebugCmd = new FAutoConsoleCommand(
        TEXT("Entity.Debug"),
        TEXT("Toggle debug visualization"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&UEntityDebugSpawnerSubsystem::ExecuteToggleDebug)
    );
    ConsoleCommands.Add(DebugCmd);

    auto ReregisterCmd = new FAutoConsoleCommand(
        TEXT("Entity.ReRegister"),
        TEXT("Re-register EntityDebugSpawnerSubsystem console commands"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&UEntityDebugSpawnerSubsystem::ExecuteReRegister)
    );
    ConsoleCommands.Add(ReregisterCmd);
    
    auto SetTSAssetCmd = new FAutoConsoleCommand(
        TEXT("Entity.SetTSAsset"),
        TEXT("Set default TurboSequence Mesh Asset for future spawns. Usage: Entity.SetTSAsset <ObjectPath> (e.g. /Script/TurboSequence_Lf.TurboSequence_MeshAsset_Lf'/Game/...')"),
        FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
        {
            if (Args.Num() < 1)
            {
                UE_LOG(LogTemp, Warning, TEXT("Entity.SetTSAsset: missing <ObjectPath>"));
                return;
            }
            const FString& Path = Args[0];
            // Load synchronously to validate path using StaticLoadObject
            UTurboSequence_MeshAsset_Lf* Loaded = Cast<UTurboSequence_MeshAsset_Lf>(StaticLoadObject(UTurboSequence_MeshAsset_Lf::StaticClass(), nullptr, *Path));
            if (!Loaded)
            {
                UE_LOG(LogTemp, Error, TEXT("Entity.SetTSAsset: failed to load asset at path: %s"), *Path);
                return;
            }
            // Assign by path to avoid deprecated operator= warnings
            GDefaultTSAsset = TSoftObjectPtr<UTurboSequence_MeshAsset_Lf>(FSoftObjectPath(Path));
            UE_LOG(LogTemp, Log, TEXT("Entity.SetTSAsset: set default TS asset to %s"), *Path);
        })
    );
    ConsoleCommands.Add(SetTSAssetCmd);

    auto MarkCleanupCmd = new FAutoConsoleCommand(
    TEXT("Entity.MarkTSForCleanup"),
    TEXT("Mark all spawned entities for TurboSequence cleanup via FTSPendingCleanupTag"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&UEntityDebugSpawnerSubsystem::ExecuteMarkTSForCleanup)
    );
    ConsoleCommands.Add(MarkCleanupCmd);

    auto ClearSafeCmd = new FAutoConsoleCommand(
        TEXT("Entity.ClearSafe"),
        TEXT("Mark all spawned entities for TS cleanup and then destroy them"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&UEntityDebugSpawnerSubsystem::ExecuteClearEntitiesSafe)
    );
    ConsoleCommands.Add(ClearSafeCmd);


    UE_LOG(LogTemp, Log, TEXT("EntityDebugSpawnerSubsystem: Commands ready -> Entity.Spawn, Entity.Clear, Entity.Debug, Entity.ReRegister"));
}

void UEntityDebugSpawnerSubsystem::UnregisterConsoleCommands()
{
    for (auto* Cmd : ConsoleCommands)
    {
        delete Cmd;
    }
    ConsoleCommands.Empty();
    UE_LOG(LogTemp, Log, TEXT("EntityDebugSpawnerSubsystem: Unregistered console commands"));
}

void UEntityDebugSpawnerSubsystem::ExecuteSpawnEntities(const TArray<FString>& Args)
{
    UWorld* World = ResolveActiveWorld();
    if (!World) return;

    if (auto* Subsystem = World->GetSubsystem<UEntityDebugSpawnerSubsystem>())
    {
        int32 Count = 10;
        float Radius = 1000.0f;
        FVector Location = FVector::ZeroVector;

        if (Args.Num() > 0) Count = FCString::Atoi(*Args[0]);
        if (Args.Num() > 1) Radius = FCString::Atof(*Args[1]);
        if (Args.Num() > 4)
        {
            Location.X = FCString::Atof(*Args[2]);
            Location.Y = FCString::Atof(*Args[3]);
            Location.Z = FCString::Atof(*Args[4]);
        }

        Subsystem->SpawnEntities(Count, Radius, Location);
        UE_LOG(LogTemp, Log, TEXT("Entity.Spawn -> Count=%d Radius=%.1f Origin=%s"), Count, Radius, *Location.ToString());
    }
}

void UEntityDebugSpawnerSubsystem::ExecuteClearEntities(const TArray<FString>& Args)
{
    UWorld* World = ResolveActiveWorld();
    if (!World) return;

    if (auto* Subsystem = World->GetSubsystem<UEntityDebugSpawnerSubsystem>())
    {
        Subsystem->ClearAllEntities();
    }
}

void UEntityDebugSpawnerSubsystem::ExecuteToggleDebug(const TArray<FString>& Args)
{
    UWorld* World = ResolveActiveWorld();
    if (!World) return;

    if (auto* Subsystem = World->GetSubsystem<UEntityDebugSpawnerSubsystem>())
    {
        const bool bNew = !Subsystem->bDebugVisualizationEnabled;
        Subsystem->SetDebugVisualization(bNew);
        UE_LOG(LogTemp, Log, TEXT("Entity.Debug -> %s"), bNew ? TEXT("ENABLED") : TEXT("DISABLED"));
    }
}

void UEntityDebugSpawnerSubsystem::ExecuteReRegister(const TArray<FString>& Args)
{
    // static context, just call the static registration again
    UEntityDebugSpawnerSubsystem::RegisterConsoleCommands();
    UE_LOG(LogTemp, Log, TEXT("Entity.ReRegister -> done"));
}


void UEntityDebugSpawnerSubsystem::ExecuteMarkTSForCleanup(const TArray<FString>& Args)
{
    UWorld* World = UEntityDebugSpawnerSubsystem::ResolveActiveWorld();
    if (!World) return;

    if (auto* Subsystem = World->GetSubsystem<UEntityDebugSpawnerSubsystem>())
    {
        if (!Subsystem->MassEntitySubsystem)
        {
            UE_LOG(LogTemp, Warning, TEXT("Entity.MarkTSForCleanup: Mass subsystem not available"));
            return;
        }

        FMassEntityManager& EntityManager = Subsystem->MassEntitySubsystem->GetMutableEntityManager();
        int32 Marked = 0;
        for (const FMassEntityHandle& Entity : Subsystem->SpawnedEntities)
        {
            if (!Entity.IsValid()) continue;
            FMassEntityView View(EntityManager, Entity);
            if (const FTurboSequenceInstanceFragment* TS = View.GetFragmentDataPtr<FTurboSequenceInstanceFragment>())
            {
                if (TS->bInstanceCreated && TS->MeshData.IsMeshDataValid())
                {
                    EntityManager.AddTagToEntity(Entity, FTSPendingCleanupTag::StaticStruct());
                    ++Marked;
                }
            }
        }
        UE_LOG(LogTemp, Log, TEXT("Entity.MarkTSForCleanup: marked %d entities"), Marked);
    }
}

void UEntityDebugSpawnerSubsystem::ExecuteClearEntitiesSafe(const TArray<FString>& Args)
{
    UWorld* World = UEntityDebugSpawnerSubsystem::ResolveActiveWorld();
    if (!World) return;

    if (auto* Subsystem = World->GetSubsystem<UEntityDebugSpawnerSubsystem>())
    {
        if (!Subsystem->MassEntitySubsystem)
        {
            UE_LOG(LogTemp, Warning, TEXT("Entity.ClearSafe: Mass subsystem not available"));
            return;
        }

        FMassEntityManager& EntityManager = Subsystem->MassEntitySubsystem->GetMutableEntityManager();

        // Marcar primero para cleanup TS
        for (const FMassEntityHandle& Entity : Subsystem->SpawnedEntities)
        {
            if (!Entity.IsValid()) continue;
            EntityManager.AddTagToEntity(Entity, FTSPendingCleanupTag::StaticStruct());
        }

        // Destruir entidades; el cleanup processor removerá las instancias antes del Solve
        for (const FMassEntityHandle& Entity : Subsystem->SpawnedEntities)
        {
            if (Entity.IsValid())
            {
                EntityManager.DestroyEntity(Entity);
            }
        }
        Subsystem->SpawnedEntities.Reset();
        UE_LOG(LogTemp, Log, TEXT("Entity.ClearSafe: marked & destroyed all spawned entities"));
    }
}