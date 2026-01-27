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
#include "ECS/Fragments/AnimLocomotionFragment.h"
#include "ECS/Fragments/ZombieStateFragment.h"
#include "ECS/Fragments/AnimRequestFragment.h"
#include "ECS/Fragments/TileLODFragment.h"
#include "ECS/Fragments/TurboSequenceFragment.h"
#include "ECS/Tags/TurboSequenceTag.h"
#include "ECS/Tags/HiddenTag.h"

TArray<FAutoConsoleCommand*> UEntityDebugSpawnerSubsystem::ConsoleCommands;

void UEntityDebugSpawnerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogTemp, Log, TEXT("EntityDebugSpawnerSubsystem::Initialize"));

    if (!IsValid(DebugTurboSequenceMeshAsset))
    {
        DebugTurboSequenceMeshAsset = LoadObject<UTurboSequence_MeshAsset_Lf>(
            nullptr,
            TEXT("/Script/TurboSequence_Lf.TurboSequence_MeshAsset_Lf'/Game/Characters/Mannequins/TurboSequence/TS_Zombie_MeshAsset.TS_Zombie_MeshAsset'"),
            nullptr,
            LOAD_None,
            nullptr
        );
    }

    if (!IsValid(DebugTurboSequenceAnim))
    {
        DebugTurboSequenceAnim = LoadObject<UAnimSequence>(
            nullptr,
            TEXT("/Script/Engine.AnimSequence'/Game/Characters/Mannequins/TurboSequence/MM_Walk_Fwd.MM_Walk_Fwd'"),
            nullptr,
            LOAD_None,
            nullptr
        );
    }

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

    // Destroy any spawned entities only if the Mass subsystem is still valid
    if (IsValid(MassEntitySubsystem))
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
        FZombieStateFragment::StaticStruct(),
        FAnimLocomotionFragment::StaticStruct(),
        FAnimRequestFragment::StaticStruct(),
        FTileLODFragment::StaticStruct(),
        FTurboSequenceFragment::StaticStruct()
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

            FInstancedStruct StateIS; StateIS.InitializeAs<FZombieStateFragment>();
            FragmentList.Add(StateIS);

            FInstancedStruct LocoIS; LocoIS.InitializeAs<FAnimLocomotionFragment>();
            FragmentList.Add(LocoIS);

            FInstancedStruct AnimReqIS; AnimReqIS.InitializeAs<FAnimRequestFragment>();
            FragmentList.Add(AnimReqIS);

            FInstancedStruct LODIS; LODIS.InitializeAs<FTileLODFragment>();
            FragmentList.Add(LODIS);

            FInstancedStruct TSIS; TSIS.InitializeAs<FTurboSequenceFragment>();
            FragmentList.Add(TSIS);

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

        // TurboSequence queda deshabilitado al spawn: se habilita con Entity.EnableTS

        SpawnedEntities.Add(NewEntities[i]);

        UE_LOG(LogTemp, Verbose, TEXT("SpawnEntities: entity[%d] valid=%d index=%d"), i, NewEntities[i].IsValid() ? 1 : 0, NewEntities[i].Index);
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

    auto EnableTSCmd = new FAutoConsoleCommand(
        TEXT("Entity.EnableTS"),
        TEXT("Enable TurboSequence for all spawned debug entities"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&UEntityDebugSpawnerSubsystem::ExecuteEnableTurboSequence)
    );
    ConsoleCommands.Add(EnableTSCmd);

    auto HideCmd = new FAutoConsoleCommand(
        TEXT("Entity.Hide"),
        TEXT("Mark all spawned debug entities with HiddenTag (triggers TS DestroyProcessor)"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&UEntityDebugSpawnerSubsystem::ExecuteMarkTSForCleanup)
    );
    ConsoleCommands.Add(HideCmd);

    UE_LOG(LogTemp, Log, TEXT("EntityDebugSpawnerSubsystem: Commands ready -> Entity.Spawn, Entity.Clear, Entity.Debug, Entity.ReRegister, Entity.EnableTS, Entity.Hide"));
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

void UEntityDebugSpawnerSubsystem::EnableTurboSequenceOnSpawnedEntities()
{
    if (!MassEntitySubsystem)
        return;

    FMassEntityManager& EntityManager = MassEntitySubsystem->GetMutableEntityManager();

    for (const FMassEntityHandle& Entity : SpawnedEntities)
    {
        if (!Entity.IsValid())
            continue;

        FMassEntityView View(EntityManager, Entity);
        FTurboSequenceFragment& TSFrag = View.GetFragmentData<FTurboSequenceFragment>();
        TSFrag.UpdateGroupIndex = DebugTurboSequenceUpdateGroupIndex;
        TSFrag.Anim = DebugTurboSequenceAnim;
        TSFrag.SpawnData = FTurboSequence_MeshSpawnData_Lf();
        TSFrag.SpawnData.RootMotionMesh.Mesh = DebugTurboSequenceMeshAsset;

        if (IsValid(TSFrag.SpawnData.RootMotionMesh.Mesh))
        {
            EntityManager.AddTagToEntity(Entity, FTurboSequenceTag::StaticStruct());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("EntityDebugSpawnerSubsystem: DebugTurboSequenceMeshAsset is not set/loaded. Skipping EnableTS for one entity."));
        }
    }
}

void UEntityDebugSpawnerSubsystem::ExecuteEnableTurboSequence(const TArray<FString>& Args)
{
    UWorld* World = ResolveActiveWorld();
    if (!World) return;

    if (auto* Subsystem = World->GetSubsystem<UEntityDebugSpawnerSubsystem>())
    {
        Subsystem->EnableTurboSequenceOnSpawnedEntities();
        UE_LOG(LogTemp, Log, TEXT("Entity.EnableTS -> done"));
    }
}

void UEntityDebugSpawnerSubsystem::MarkSpawnedEntitiesHidden()
{
    if (!MassEntitySubsystem)
        return;

    FMassEntityManager& EntityManager = MassEntitySubsystem->GetMutableEntityManager();
    for (const FMassEntityHandle& Entity : SpawnedEntities)
    {
        if (!Entity.IsValid())
            continue;
        EntityManager.AddTagToEntity(Entity, FHiddenTag::StaticStruct());
    }
}

void UEntityDebugSpawnerSubsystem::ExecuteMarkTSForCleanup(const TArray<FString>& Args)
{
    UWorld* World = ResolveActiveWorld();
    if (!World) return;

    if (auto* Subsystem = World->GetSubsystem<UEntityDebugSpawnerSubsystem>())
    {
        Subsystem->MarkSpawnedEntitiesHidden();
        UE_LOG(LogTemp, Log, TEXT("Entity.Hide -> done"));
    }
}