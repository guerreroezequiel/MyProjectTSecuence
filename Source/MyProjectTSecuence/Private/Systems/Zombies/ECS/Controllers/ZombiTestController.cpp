// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Controllers/ZombiTestController.h"
#include "Systems/Zombies/ECS/Subsystems/ZombiSpawnerSubsystem.h"
#include "Systems/Zombies/ECS/Subsystems/ZombiMassSubsystem.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTurboSequenceFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "TurboSequence_Manager_Lf.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"

AZombiTestController::AZombiTestController()
{
    PrimaryActorTick.bCanEverTick = true;

    // No llamamos SetActorLabel aquí para evitar problemas durante la creación del Blueprint
}

void AZombiTestController::BeginPlay()
{
    Super::BeginPlay();

    // Hace que este actor sea fácil de encontrar en el mundo (más seguro aquí)
    SetActorLabel(TEXT("ZombiTestController"));

    // Obtiene el subsystem de spawning
    SpawnerSubsystem = GetWorld()->GetSubsystem<UZombiSpawnerSubsystem>();

    if (SpawnerSubsystem)
    {
        // Configura el asset de TurboSequence si está asignado
        if (ZombiTurboSequenceAsset)
        {
            SpawnerSubsystem->SetZombiTurboSequenceAsset(ZombiTurboSequenceAsset);
            UE_LOG(LogTemp, Log, TEXT("ZombiTestController: TurboSequence Asset configurado: %s"), *ZombiTurboSequenceAsset->GetName());

            // Spawn automático con delay para asegurar que los subsystems estén listos
            GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &AZombiTestController::DelayedSpawn, 1.0f, false);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ZombiTestController: No hay TurboSequence Asset asignado"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiTestController: SpawnerSubsystem no disponible"));
    }
}

void AZombiTestController::SpawnZombiBatch(int32 Count)
{
    if (!SpawnerSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiTestController: SpawnerSubsystem no disponible"));
        return;
    }

    // Usa el centro de spawning configurado o la posición del actor
    FVector SpawnLocation = SpawnCenter;
    if (SpawnLocation.IsZero())
    {
        SpawnLocation = GetActorLocation();
    }

    SpawnerSubsystem->SpawnZombiBatch(Count, SpawnLocation, SpawnRadius);
}

void AZombiTestController::SpawnSingleZombi()
{
    if (!SpawnerSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiTestController: SpawnerSubsystem no disponible"));
        return;
    }

    // Usa el centro de spawning configurado o la posición del actor
    FVector SpawnLocation = SpawnCenter;
    if (SpawnLocation.IsZero())
    {
        SpawnLocation = GetActorLocation();
    }

    SpawnerSubsystem->SpawnSingleZombi(SpawnLocation);
}

void AZombiTestController::ClearAllZombis()
{
    if (!SpawnerSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiTestController: SpawnerSubsystem no disponible"));
        return;
    }

    SpawnerSubsystem->ClearAllZombis();
}

int32 AZombiTestController::GetActiveZombiCount()
{
    if (!SpawnerSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiTestController: SpawnerSubsystem no disponible"));
        return 0;
    }

    int32 Count = SpawnerSubsystem->GetActiveZombiCount();
    return Count;
}

void AZombiTestController::SetTurboSequenceAsset(UTurboSequence_MeshAsset_Lf *Asset)
{
    ZombiTurboSequenceAsset = Asset;

    if (SpawnerSubsystem && Asset)
    {
        SpawnerSubsystem->SetZombiTurboSequenceAsset(Asset);
        UE_LOG(LogTemp, Log, TEXT("ZombiTestController: TurboSequence Asset configurado: %s"), *Asset->GetName());
    }
}

void AZombiTestController::DelayedSpawn()
{
    SpawnZombiBatch(ZombisPerBatch);
}

void AZombiTestController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        return;
    }

    // Inicialización del sistema
    if (!bSystemInitialized)
    {
        // CRÍTICO: Verificar que existe TurboSequence Manager en el mundo
        ATurboSequence_Manager_Lf *TSManager = nullptr;

        // Buscar directamente en el mundo por si Instance no está inicializada
        for (TActorIterator<ATurboSequence_Manager_Lf> ActorItr(GetWorld()); ActorItr; ++ActorItr)
        {
            TSManager = *ActorItr;
            break;
        }

        if (!TSManager && !ATurboSequence_Manager_Lf::Instance)
        {
            UE_LOG(LogTemp, Error, TEXT("❌ ZombiTestController: TurboSequence Manager no encontrado en el mundo. Debe agregar ATurboSequence_Manager_Lf al nivel."));
            return;
        }

        if (TSManager)
        {
            UE_LOG(LogTemp, Log, TEXT("✅ ZombiTestController: TurboSequence Manager encontrado en el mundo: %s"), *TSManager->GetName());
        }

        bSystemInitialized = true;
        UE_LOG(LogTemp, Log, TEXT("✅ ZombiTestController: Sistema inicializado correctamente"));
    }

    // Control centralizado del sistema
    UpdateSystemControl(DeltaTime);

    // Procesa instancias visuales pendientes
    if (SpawnerSubsystem)
    {
        SpawnerSubsystem->ProcessPendingVisualInstances(DeltaTime);
    }

    // ✅ PATRÓN OFICIAL: Procesar entidades después del "big ECS loop"
    // Según documentación: "you update all instance at once, in a big loop which can be multithreaded
    // and after this loop ends you need to solve the animations per update group"
    // ✅ CORRECCIÓN: Combinar ambas operaciones en UNA SOLA iteración para evitar condiciones de carrera
    ProcessAllTurboSequenceOperations(DeltaTime);

    // ✅ PATRÓN OFICIAL: Update Groups según TurboSequence_Demo_Lf.cpp
    static int32 CurrentBackgroundGroup = 1; // Empezar en 1, no en 0
    static TArray<float> AccumulatedDeltaTimes;
    const int32 MaxUpdateGroups = 4;

    // ✅ CORREGIDO: Inicializar UNA SOLA VEZ, no cada frame
    if (AccumulatedDeltaTimes.Num() == 0)
    {
        AccumulatedDeltaTimes.SetNum(MaxUpdateGroups + 1);
        // Inicializar todos en 0
        for (int32 i = 0; i <= MaxUpdateGroups; ++i)
        {
            AccumulatedDeltaTimes[i] = 0.0f;
        }
    }

    // Acumular DeltaTime para TODOS los grupos
    for (float &Delta : AccumulatedDeltaTimes)
    {
        Delta += DeltaTime;
    }

    // PASO 1: Grupo background rotativo (patrón oficial)
    if (CurrentBackgroundGroup <= MaxUpdateGroups)
    {
        FTurboSequence_UpdateContext_Lf UpdateContext;
        UpdateContext.GroupIndex = CurrentBackgroundGroup;

        // CRÍTICO: UNA llamada por grupo con DeltaTime acumulado
        ATurboSequence_Manager_Lf::SolveMeshes_GameThread(
            AccumulatedDeltaTimes[CurrentBackgroundGroup], GetWorld(), UpdateContext);

        AccumulatedDeltaTimes[CurrentBackgroundGroup] = 0.0f; // Reset después de procesar
    }

    // Rotar al siguiente grupo background (patrón oficial: excluye grupo 0)
    CurrentBackgroundGroup = (CurrentBackgroundGroup % MaxUpdateGroups) + 1;

    // PASO 2: Grupo 0 de alta calidad - SIEMPRE cada frame (patrón oficial)
    FTurboSequence_UpdateContext_Lf HighQualityContext;
    HighQualityContext.GroupIndex = 0;
    ATurboSequence_Manager_Lf::SolveMeshes_GameThread(DeltaTime, GetWorld(), HighQualityContext);
    AccumulatedDeltaTimes[0] = 0.0f; // Reset grupo 0
}

// Control centralizado del sistema
void AZombiTestController::UpdateSystemControl(float DeltaTime)
{
    // Actualizar timers de logs
    SystemLogTimer += DeltaTime;
    PerformanceLogTimer += DeltaTime;

    // Logs eliminados para optimización de rendimiento
}

// Logs centralizados del estado del sistema
void AZombiTestController::LogSystemStatus()
{
    if (!SpawnerSubsystem)
    {
        return;
    }

    int32 CurrentEntityCount = SpawnerSubsystem->GetActiveZombiCount();

    if (CurrentEntityCount != LastEntityCount)
    {

        LastEntityCount = CurrentEntityCount;
    }
}

// Logs centralizados de métricas de rendimiento
void AZombiTestController::LogPerformanceMetrics()
{
    if (!SpawnerSubsystem)
    {
        return;
    }

    int32 CurrentEntityCount = SpawnerSubsystem->GetActiveZombiCount();
}

// ✅ PATRÓN OFICIAL UNIFICADO: Procesar TODAS las operaciones TurboSequence en UNA iteración
// Combina: actualizaciones pendientes + gestión de grupos para evitar condiciones de carrera
// ✅ PATRÓN OFICIAL TURBOSEQUENCE: SEPARACIÓN TOTAL ECS vs TurboSequence
// Fase 1: Recolectar datos → Fase 2: Aplicar TurboSequence FUERA del Mass loop
void AZombiTestController::ProcessAllTurboSequenceOperations(float DeltaTime)
{
    if (!GetWorld())
    {
        return;
    }

    UMassEntitySubsystem *EntitySubsystem = UWorld::GetSubsystem<UMassEntitySubsystem>(GetWorld());
    if (!EntitySubsystem)
    {
        return;
    }

    FMassEntityManager &EntityManager = EntitySubsystem->GetMutableEntityManager();
    FMassExecutionContext ExecutionContext(EntityManager);

    // ESTRUCTURA PARA OPERACIONES PENDIENTES
    struct FTurboSequenceOperation
    {
        FTurboSequence_MinimalMeshData_Lf MeshData;
        UAnimSequence *Animation = nullptr;
        FTurboSequence_AnimPlaySettings_Lf AnimSettings;
        FTransform Transform;
        bool bNeedsAnimation = false;
        bool bNeedsTransform = false;
        // ✅ SINCRONIZACIÓN ECS ↔ TURBOSEQUENCE
        int32 TargetGroup = -1;
        bool bNeedsGroupChange = false;
    };

    TArray<FTurboSequenceOperation> PendingOperations;

    // FASE 1: RECOLECTAR datos SIN modificar fragments NI llamar TurboSequence
    // ✅ SINCRONIZACIÓN ECS TAGS ↔ TURBOSEQUENCE GROUPS
    struct FLODQuery
    {
        FMassEntityQuery Query;
        int32 TurboSequenceGroup;
        FString LODName;
    };

    TArray<FLODQuery> LODQueries;

    // ✅ MAPEO DIRECTO: Tags ECS → TurboSequence Groups
    {
        FLODQuery Query60FPS;
        Query60FPS.Query.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadOnly);
        Query60FPS.Query.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadOnly);
        Query60FPS.Query.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
        Query60FPS.Query.AddTagRequirement<FUpdate60FPS>(EMassFragmentPresence::All);
        Query60FPS.TurboSequenceGroup = 0; // Alta calidad
        Query60FPS.LODName = TEXT("Update60FPS");
        LODQueries.Add(Query60FPS);
    }

    {
        FLODQuery Query30FPS;
        Query30FPS.Query.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadOnly);
        Query30FPS.Query.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadOnly);
        Query30FPS.Query.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
        Query30FPS.Query.AddTagRequirement<FUpdate30FPS>(EMassFragmentPresence::All);
        Query30FPS.TurboSequenceGroup = 1;
        Query30FPS.LODName = TEXT("Update30FPS");
        LODQueries.Add(Query30FPS);
    }

    {
        FLODQuery Query15FPS;
        Query15FPS.Query.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadOnly);
        Query15FPS.Query.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadOnly);
        Query15FPS.Query.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
        Query15FPS.Query.AddTagRequirement<FUpdate15FPS>(EMassFragmentPresence::All);
        Query15FPS.TurboSequenceGroup = 2;
        Query15FPS.LODName = TEXT("Update15FPS");
        LODQueries.Add(Query15FPS);
    }

    {
        FLODQuery Query5FPS;
        Query5FPS.Query.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadOnly);
        Query5FPS.Query.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadOnly);
        Query5FPS.Query.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
        Query5FPS.Query.AddTagRequirement<FUpdate5FPS>(EMassFragmentPresence::All);
        Query5FPS.TurboSequenceGroup = 3;
        Query5FPS.LODName = TEXT("Update5FPS");
        LODQueries.Add(Query5FPS);
    }

    // ✅ RECOLECTAR por LOD usando sincronización perfecta Tags → Groups
    for (const FLODQuery &LODQuery : LODQueries)
    {
        // Crear copia mutable del query para ForEachEntityChunk
        FMassEntityQuery MutableQuery = LODQuery.Query;
        MutableQuery.ForEachEntityChunk(EntityManager, ExecutionContext, [&PendingOperations, &LODQuery](FMassExecutionContext &Context)
                                        {
            TArrayView<const FZombiTurboSequenceFragment> TurboFragments = Context.GetFragmentView<FZombiTurboSequenceFragment>();
            TArrayView<const FZombiCoreFragment> CoreFragments = Context.GetFragmentView<FZombiCoreFragment>();

            for (int32 i = 0; i < Context.GetNumEntities(); ++i)
            {
                const FZombiTurboSequenceFragment &TurboFragment = TurboFragments[i];

                if (!TurboFragment.IsValid())
                {
                    continue;
                }

                // ✅ OPERACIÓN CON GRUPO SINCRONIZADO AUTOMÁTICAMENTE
                FTurboSequenceOperation Op;
                Op.MeshData = TurboFragment.MeshData;
                bool bHasOperations = false;

                // ✅ SINCRONIZACIÓN AUTOMÁTICA: Tag ECS → TurboSequence Group
                Op.TargetGroup = LODQuery.TurboSequenceGroup;
                Op.bNeedsGroupChange = true;
                bHasOperations = true;

                // Recolectar animaciones pendientes
                if (TurboFragment.bNeedsAnimationUpdate && TurboFragment.CurrentAnimation)
                {
                    Op.Animation = TurboFragment.CurrentAnimation;
                    Op.AnimSettings = TurboFragment.PendingAnimationSettings;
                    Op.bNeedsAnimation = true;
                    bHasOperations = true;
                }

                // Recolectar transforms pendientes
                if (TurboFragment.bNeedsTransformUpdate)
                {
                    Op.Transform = TurboFragment.PendingTransform;
                    Op.bNeedsTransform = true;
                    bHasOperations = true;
                }

                if (bHasOperations)
                {
                    PendingOperations.Add(Op);
                }
            } });
    }

    // FASE 2: APLICAR operaciones TurboSequence FUERA del Mass loop
    for (const FTurboSequenceOperation &Op : PendingOperations)
    {
        // ✅ SINCRONIZACIÓN: Asegurar que entidad esté en el grupo correcto
        if (Op.bNeedsGroupChange)
        {
            ATurboSequence_Manager_Lf::AddInstanceToUpdateGroup_Concurrent(Op.TargetGroup, Op.MeshData);
        }

        if (Op.bNeedsAnimation && Op.Animation)
        {
            ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(
                Op.MeshData,
                Op.Animation,
                Op.AnimSettings);
        }

        if (Op.bNeedsTransform)
        {
            ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_Concurrent(
                Op.MeshData,
                Op.Transform);
        }
    }

    // FASE 3: RESETEAR flags en iteración separada
    if (PendingOperations.Num() > 0)
    {
        FMassEntityQuery ResetQuery;
        ResetQuery.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);

        ResetQuery.ForEachEntityChunk(EntityManager, ExecutionContext, [](FMassExecutionContext &Context)
                                      {
            TArrayView<FZombiTurboSequenceFragment> TurboFragments = Context.GetMutableFragmentView<FZombiTurboSequenceFragment>();
            
            for (int32 i = 0; i < Context.GetNumEntities(); ++i)
            {
                FZombiTurboSequenceFragment& TurboFragment = TurboFragments[i];
                
                if (TurboFragment.bNeedsAnimationUpdate || TurboFragment.bNeedsTransformUpdate)
                {
                    TurboFragment.bNeedsAnimationUpdate = false;
                    TurboFragment.bNeedsTransformUpdate = false;
                }
            } });
    }
}

// ❌ FUNCIÓN ELIMINADA: UpdateAllEntityGroups() - Integrada en ProcessAllTurboSequenceOperations()
// ✅ CORRECCIÓN: Gestión de grupos SEPARADA para evitar condiciones de carrera
/*void AZombiTestController::UpdateAllEntityGroups()
{
    if (!GetWorld())
    {
        return;
    }

    // Obtener el EntitySubsystem para iterar sobre las entidades SIN modificar durante iteración
    if (UMassEntitySubsystem *EntitySubsystem = UWorld::GetSubsystem<UMassEntitySubsystem>(GetWorld()))
    {
        FMassEntityManager &EntityManager = EntitySubsystem->GetMutableEntityManager();

        // Crear query para obtener fragments que necesitan gestión de grupos
        FMassEntityQuery Query;
        Query.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadOnly);
        Query.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadOnly);

        // Crear contexto de ejecución
        FMassExecutionContext ExecutionContext(EntityManager);

        // PASO 1: RECOPILAR cambios de grupo sin aplicar (evita condiciones de carrera)
        struct FGroupChange
        {
            FTurboSequence_MinimalMeshData_Lf MeshData;
            int32 CurrentGroup;
            int32 TargetGroup;
        };

        TArray<FGroupChange> PendingGroupChanges;

        // Obtener posición del jugador UNA VEZ para optimización
        FVector PlayerLocation = FVector::ZeroVector;
        if (GetWorld()->GetFirstPlayerController())
        {
            if (APawn *PlayerPawn = GetWorld()->GetFirstPlayerController()->GetPawn())
            {
                PlayerLocation = PlayerPawn->GetActorLocation();
            }
        }

        // Iterar y SOLO recopilar cambios
        Query.ForEachEntityChunk(EntityManager, ExecutionContext, [&PendingGroupChanges, PlayerLocation](FMassExecutionContext &Context)
                                 {
            TArrayView<const FZombiTurboSequenceFragment> TurboFragments = Context.GetFragmentView<FZombiTurboSequenceFragment>();
            TArrayView<const FZombiCoreFragment> CoreFragments = Context.GetFragmentView<FZombiCoreFragment>();

            for (int32 i = 0; i < Context.GetNumEntities(); ++i)
            {
                const FZombiTurboSequenceFragment& TurboFragment = TurboFragments[i];
                const FZombiCoreFragment& CoreFragment = CoreFragments[i];

                if (!TurboFragment.IsValid())
                {
                    continue;
                }

                // Calcular distancia al jugador
                float DistanceToPlayer = FVector::Dist(CoreFragment.Position, PlayerLocation);

                // Determinar grupo objetivo según patrón oficial
                int32 TargetGroup = 0; // Grupo 0 por defecto (alta calidad)
                if (DistanceToPlayer > 500.0f) TargetGroup = 1;       // Grupo 1
                if (DistanceToPlayer > 1000.0f) TargetGroup = 2;      // Grupo 2
                if (DistanceToPlayer > 1500.0f) TargetGroup = 3;      // Grupo 3+

                // Obtener grupo actual de la entidad
                int32 CurrentGroup = ATurboSequence_Manager_Lf::GetUpdateGroupIndexFromMeshID_Concurrent(
                    TurboFragment.MeshData.RootMotionMeshID);

                // Solo registrar cambio si es necesario
                if (CurrentGroup != TargetGroup && CurrentGroup != -1)
                {
                    FGroupChange Change;
                    Change.MeshData = TurboFragment.MeshData;
                    Change.CurrentGroup = CurrentGroup;
                    Change.TargetGroup = TargetGroup;
                    PendingGroupChanges.Add(Change);
                }
            } });

        // PASO 2: APLICAR cambios fuera del loop (seguro para concurrencia)
        for (const FGroupChange &Change : PendingGroupChanges)
        {
            // Remover del grupo actual
            ATurboSequence_Manager_Lf::RemoveInstanceFromUpdateGroup_Concurrent(Change.CurrentGroup, Change.MeshData);

            // Agregar al nuevo grupo
            ATurboSequence_Manager_Lf::AddInstanceToUpdateGroup_Concurrent(Change.TargetGroup, Change.MeshData);
        }
    }
}*/

// Función auxiliar mantenida para compatibilidad (ahora no se usa en el loop crítico)
void AZombiTestController::UpdateEntityGroupByDistance(FZombiTurboSequenceFragment &TurboFragment,
                                                       const FZombiCoreFragment &CoreFragment)
{
    // Esta función ya no se llama durante iteración crítica
    // Se mantiene para posibles usos futuros no críticos
}
