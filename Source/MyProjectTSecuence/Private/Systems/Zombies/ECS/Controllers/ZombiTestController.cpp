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

    // Inicializar array de DeltaTimes acumulados
    AccumulatedDeltaTimes.SetNum(MaxUpdateGroups + 1);

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
void AZombiTestController::ProcessAllTurboSequenceOperations(float DeltaTime)
{
    if (!GetWorld())
    {
        return;
    }

    // Obtener el EntitySubsystem para iterar sobre las entidades
    if (UMassEntitySubsystem *EntitySubsystem = UWorld::GetSubsystem<UMassEntitySubsystem>(GetWorld()))
    {
        FMassEntityManager &EntityManager = EntitySubsystem->GetMutableEntityManager();

        // Crear query para obtener fragments que necesitan actualización
        FMassEntityQuery Query;
        Query.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
        Query.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadOnly);

        // Crear contexto de ejecución
        FMassExecutionContext ExecutionContext(EntityManager);

        // PASO 1: RECOPILAR TODAS las operaciones (animación + grupos) sin ejecutar
        struct FTurboSequenceOperation
        {
            FTurboSequence_MinimalMeshData_Lf MeshData;
            UAnimSequence *Animation = nullptr;
            FTurboSequence_AnimPlaySettings_Lf AnimSettings;
            FTransform Transform;
            bool bNeedsAnimation = false;
            bool bNeedsTransform = false;
            // Gestión de grupos
            int32 CurrentGroup = -1;
            int32 TargetGroup = -1;
            bool bNeedsGroupChange = false;
        };

        TArray<FTurboSequenceOperation> PendingOperations;

        // Obtener posición del jugador para cálculos de distancia
        FVector PlayerLocation = FVector::ZeroVector;
        if (APawn *PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
        {
            PlayerLocation = PlayerPawn->GetActorLocation();
        }

        // Recopilar operaciones SIN ejecutar TurboSequence NI modificar fragmentos
        Query.ForEachEntityChunk(EntityManager, ExecutionContext, [&PendingOperations, PlayerLocation](FMassExecutionContext &Context)
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
                
                // RECOPILAR todas las operaciones (animación + grupos)
                FTurboSequenceOperation Op;
                Op.MeshData = TurboFragment.MeshData;
                bool bHasOperations = false;
                
                // 1. Operaciones de animación
                if (TurboFragment.bNeedsAnimationUpdate && TurboFragment.CurrentAnimation)
                {
                    Op.Animation = TurboFragment.CurrentAnimation;
                    Op.AnimSettings = TurboFragment.PendingAnimationSettings;
                    Op.bNeedsAnimation = true;
                    bHasOperations = true;
                }
                
                // 2. Operaciones de transformación
                if (TurboFragment.bNeedsTransformUpdate)
                {
                    Op.Transform = TurboFragment.PendingTransform;
                    Op.bNeedsTransform = true;
                    bHasOperations = true;
                }
                
                // 3. Operaciones de grupos por distancia (SIMPLIFICADO - SIN consultar grupo actual)
                float DistanceToPlayer = FVector::Dist(CoreFragment.Position, PlayerLocation);
                
                // Determinar grupo objetivo basado en distancia
                int32 TargetGroup = 0; // Máxima calidad por defecto
                if (DistanceToPlayer > 500.0f) TargetGroup = 1;
                if (DistanceToPlayer > 1000.0f) TargetGroup = 2;
                if (DistanceToPlayer > 2000.0f) TargetGroup = 3;
                
                // ✅ SIMPLIFICADO: Reasignar SIEMPRE (evita consultar grupo actual durante iteración)
                // Esto es seguro porque TurboSequence maneja internamente si ya está en el grupo correcto
                Op.CurrentGroup = -1; // Valor especial para "remover de cualquier grupo"
                Op.TargetGroup = TargetGroup;
                Op.bNeedsGroupChange = true;
                bHasOperations = true;
                
                // Solo agregar si hay alguna operación
                if (bHasOperations)
                {
                    PendingOperations.Add(Op);
                }
            } });

        // PASO 2: EJECUTAR todas las operaciones FUERA del loop (seguro para concurrencia)
        for (const FTurboSequenceOperation &Op : PendingOperations)
        {
            // Animaciones
            if (Op.bNeedsAnimation && Op.Animation)
            {
                ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(
                    Op.MeshData, Op.Animation, Op.AnimSettings);
            }

            // Transformaciones
            if (Op.bNeedsTransform)
            {
                ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_Concurrent(
                    Op.MeshData, Op.Transform);
            }

            // Grupos
            if (Op.bNeedsGroupChange)
            {
                // ✅ PATRÓN SIMPLIFICADO: Solo agregar al grupo objetivo
                // TurboSequence automáticamente maneja la remoción del grupo anterior
                ATurboSequence_Manager_Lf::AddInstanceToUpdateGroup_Concurrent(Op.TargetGroup, Op.MeshData);
            }
        }

        // PASO 3: RESETEAR flags EN ITERACIÓN SEPARADA (evita crash Mass Entity)
        if (PendingOperations.Num() > 0)
        {
            Query.ForEachEntityChunk(EntityManager, ExecutionContext, [](FMassExecutionContext &Context)
                                     {
                TArrayView<FZombiTurboSequenceFragment> TurboFragments = Context.GetMutableFragmentView<FZombiTurboSequenceFragment>();
                
                for (int32 i = 0; i < Context.GetNumEntities(); ++i)
                {
                    FZombiTurboSequenceFragment& TurboFragment = TurboFragments[i];
                    
                    // Resetear flags después de procesamiento exitoso
                    if (TurboFragment.bNeedsAnimationUpdate || TurboFragment.bNeedsTransformUpdate)
                    {
                        TurboFragment.bNeedsAnimationUpdate = false;
                        TurboFragment.bNeedsTransformUpdate = false;
                    }
                } });
        }
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
