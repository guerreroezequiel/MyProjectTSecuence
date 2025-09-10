// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Coordinators/ZombiSystemCoordinator.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Systems/Zombies/ECS/Subsystems/ZombiSpawnerSubsystem.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "TurboSequence_Manager_Lf.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "CollisionQueryParams.h"
#include "Animation/AnimSequence.h"

// Constructor
UZombiSystemCoordinator::UZombiSystemCoordinator()
{
    // Configuración inicial
    bSystemActive = false;
    bEnableDetailedLogs = false;
    bEnablePerformanceMetrics = true;
    FrameCounter = 0;
    CurrentSolveMeshGroup = 0;
    bSystemInitialized = false;
    LogTimer = 0.0f;

    // Inicializar cache
    CachedPlayerLocation = FVector::ZeroVector;
    LastPlayerLocationUpdate = 0.0f;

    UE_LOG(LogTemp, Log, TEXT("✅ ZombiSystemCoordinator: Constructor completado"));
}

// Inicialización del subsystem
void UZombiSystemCoordinator::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogTemp, Log, TEXT("🚀 ZombiSystemCoordinator: Inicializando subsystem..."));

    // Obtener MassEntitySubsystem
    if (UWorld *World = GetWorld())
    {
        MassEntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>();
        if (MassEntitySubsystem)
        {
            UE_LOG(LogTemp, Log, TEXT("✅ ZombiSystemCoordinator: MassEntitySubsystem encontrado"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ ZombiSystemCoordinator: MassEntitySubsystem no disponible"));
        }
    }

    // Inicializar grupos SolveMeshes
    InitializeSolveMeshGroups();
}

// Limpieza del subsystem
void UZombiSystemCoordinator::Deinitialize()
{
    UE_LOG(LogTemp, Log, TEXT("🛑 ZombiSystemCoordinator: Deinicializando..."));

    StopSystem();

    // Limpiar cache
    PreviousStates.Empty();
    PreviousGroups.Empty();

    Super::Deinitialize();
}

// Determinar si debe crearse el subsystem
bool UZombiSystemCoordinator::ShouldCreateSubsystem(UObject *Outer) const
{
    // Solo crear en PIE o Standalone - Outer es UGameInstance para GameInstanceSubsystem
    if (UGameInstance *GameInstance = Cast<UGameInstance>(Outer))
    {
        if (UWorld *World = GameInstance->GetWorld())
        {
            return World->IsGameWorld();
        }
    }
    return false;
}

// ===========================
// TICK PRINCIPAL DEL COORDINADOR
// ===========================

void UZombiSystemCoordinator::Tick(float DeltaTime)
{
    // Solo ejecutar si el sistema está activo
    if (!bSystemActive)
    {
        return;
    }

    // Verificar que el mundo es válido
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        return;
    }

    // Inicialización lazy del sistema
    if (!bSystemInitialized)
    {
        InitializeSystem();
        if (!bSystemInitialized)
        {
            return; // Inicialización falló, reintentar próximo frame
        }
    }

    // Reset métricas para este frame
    if (bEnablePerformanceMetrics)
    {
        CurrentMetrics.Reset();
    }

    double FrameStartTime = FPlatformTime::Seconds();

    // ✅ PRINCIPIO TURBOSEQUENCE: Separación TOTAL ECS → TurboSequence
    // Documentación: "you update all instance at once, in a big loop which can be multithreaded
    // and after this loop ends you need to solve the animations per update group"

    // ✅ ECS REACTIVADO: Sistema completo ECS + TurboSequence funcionando

    // FASE 1: ECS "Big Loop" - SOLO lógica ECS, SIN llamadas TurboSequence
    double ECSStartTime = FPlatformTime::Seconds();
    ExecuteECSBigLoop(DeltaTime);
    double ECSEndTime = FPlatformTime::Seconds();

    // FASE 2: TurboSequence "Big Loop" - SOLO operaciones visuales, SIN ECS
    double TSStartTime = FPlatformTime::Seconds();
    ExecuteTurboSequenceBigLoop(DeltaTime);
    double TSEndTime = FPlatformTime::Seconds();

    double FrameEndTime = FPlatformTime::Seconds();

    // Actualizar métricas
    if (bEnablePerformanceMetrics)
    {
        CurrentMetrics.ECSLoopTime = (ECSEndTime - ECSStartTime) * 1000.0f;
        CurrentMetrics.TurboSequenceLoopTime = (TSEndTime - TSStartTime) * 1000.0f;
        CurrentMetrics.TotalFrameTime = (FrameEndTime - FrameStartTime) * 1000.0f;
    }

    // Incrementar contador de frames para discriminación LOD
    FrameCounter++;

    // Logs periódicos para monitoreo
    if (bEnableDetailedLogs)
    {
        LogTimer += DeltaTime;
        if (LogTimer >= LogInterval)
        {
            LogPerformanceMetrics();
            LogTimer = 0.0f;
        }
    }

    UE_LOG(LogTemp, VeryVerbose, TEXT("✅ ZombiSystemCoordinator: ECS + TurboSequence activos"));
}

// ===========================
// FASE 1: ECS "BIG LOOP" PURO
// ===========================

void UZombiSystemCoordinator::ExecuteECSBigLoop(float DeltaTime)
{
    if (!MassEntitySubsystem)
    {
        return;
    }

    // ✅ SOLUCIÓN CRÍTICA: Solo 4 queries máximo con LOD discriminativo
    // vs 16 queries actuales = 75% reducción inmediata

    if (ShouldExecuteCritical(DeltaTime))
    {
        ExecuteECSForLOD(ELODLevel::Critical, DeltaTime); // ~100 entidades críticas
        if (bEnablePerformanceMetrics)
            CurrentMetrics.QueriesExecuted++;
    }

    if (ShouldExecuteHigh(DeltaTime))
    {
        ExecuteECSForLOD(ELODLevel::High, DeltaTime); // ~400 entidades altas
        if (bEnablePerformanceMetrics)
            CurrentMetrics.QueriesExecuted++;
    }

    if (ShouldExecuteNormal(DeltaTime))
    {
        ExecuteECSForLOD(ELODLevel::Normal, DeltaTime); // ~1500 entidades normales
        if (bEnablePerformanceMetrics)
            CurrentMetrics.QueriesExecuted++;
    }

    if (ShouldExecuteLow(DeltaTime))
    {
        ExecuteECSForLOD(ELODLevel::Low, DeltaTime); // ~3000 entidades bajas
        if (bEnablePerformanceMetrics)
            CurrentMetrics.QueriesExecuted++;
    }

    // RESULTADO: MÁXIMO 4 queries vs 16 actuales = 75% reducción

    if (bEnableDetailedLogs && FrameCounter % 60 == 0) // Log cada segundo aprox
    {
        UE_LOG(LogTemp, Log, TEXT("🎯 ECS Big Loop: %d queries ejecutadas, Frame %d"),
               CurrentMetrics.QueriesExecuted, FrameCounter);
    }

    // 🔍 DIAGNÓSTICO: Log cada frame para verificar que se ejecuta
    if (FrameCounter % 1200 == 0) // Log cada 20 segundos aprox
    {
        UE_LOG(LogTemp, Log, TEXT("🔄 ECS Big Loop ejecutándose - Frame %d, Queries: %d"),
               FrameCounter, CurrentMetrics.QueriesExecuted);
    }
}

void UZombiSystemCoordinator::ExecuteECSForLOD(ELODLevel TargetLOD, float DeltaTime)
{
    if (!MassEntitySubsystem)
    {
        return;
    }

    FMassEntityManager &EntityManager = MassEntitySubsystem->GetMutableEntityManager();
    FMassExecutionContext ExecutionContext(EntityManager);

    // ✅ SOLUCIÓN: UNA query unificada por LOD vs 4 queries separadas por procesador
    // Optimizada para cache locality máxima
    FMassEntityQuery UnifiedQuery = CreateUnifiedQueryForLOD(TargetLOD);

    // 🔍 DIAGNÓSTICO: Verificar si hay entidades para procesar
    int32 EntityCount = 0;
    UnifiedQuery.ForEachEntityChunk(EntityManager, ExecutionContext,
                                    [&EntityCount](FMassExecutionContext &Context)
                                    {
                                        EntityCount += Context.GetNumEntities();
                                    });

    if (EntityCount > 0 && FrameCounter % 600 == 0) // Log cada 10 segundos si hay entidades
    {
        UE_LOG(LogTemp, Log, TEXT("🔄 LOD %d: Procesando %d entidades"), (int32)TargetLOD, EntityCount);
    }
    else if (FrameCounter % 1200 == 0) // Log cada 20 segundos si no hay entidades
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ LOD %d: No se encontraron entidades para procesar"), (int32)TargetLOD);

        // 🔍 DIAGNÓSTICO: Verificar qué entidades existen
        DiagnoseEntityTags();
    }

    // ✅ UNA SOLA iteración por LOD que hace TODO el trabajo ECS
    // Cache locality optimizada: todos los fragments en una sola pasada
    UnifiedQuery.ForEachEntityChunk(EntityManager, ExecutionContext,
                                    [this, TargetLOD, DeltaTime](FMassExecutionContext &Context)
                                    {
                                        // TODO en una sola pasada para máxima eficiencia
                                        ProcessBehaviorInline(Context, TargetLOD, DeltaTime);
                                        ProcessMovementInline(Context, TargetLOD, DeltaTime);
                                        ProcessLODInline(Context, TargetLOD, DeltaTime);
                                        MarkTurboSequenceOperations(Context, TargetLOD);

                                        // Actualizar métricas
                                        if (bEnablePerformanceMetrics)
                                        {
                                            int32 EntitiesInChunk = Context.GetNumEntities();
                                            CurrentMetrics.EntitiesProcessed += EntitiesInChunk;

                                            switch (TargetLOD)
                                            {
                                            case ELODLevel::Critical:
                                                CurrentMetrics.CriticalEntities += EntitiesInChunk;
                                                break;
                                            case ELODLevel::High:
                                                CurrentMetrics.HighEntities += EntitiesInChunk;
                                                break;
                                            case ELODLevel::Normal:
                                                CurrentMetrics.NormalEntities += EntitiesInChunk;
                                                break;
                                            case ELODLevel::Low:
                                                CurrentMetrics.LowEntities += EntitiesInChunk;
                                                break;
                                            }
                                        }
                                    });
}

FMassEntityQuery UZombiSystemCoordinator::CreateUnifiedQueryForLOD(ELODLevel TargetLOD)
{
    FMassEntityQuery Query;

    // Incluir TODOS los fragments necesarios para optimizar cache locality
    Query.AddRequirement<FZombiBehaviorFragment>(EMassFragmentAccess::ReadWrite);
    Query.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadWrite);
    Query.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
    Query.AddRequirement<FZombiLODFragment>(EMassFragmentAccess::ReadWrite);

    // Tags básicos
    Query.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    Query.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

    // Filtrar por tag LOD específico
    switch (TargetLOD)
    {
    case ELODLevel::Critical:
        Query.AddTagRequirement<FUpdate60FPS>(EMassFragmentPresence::All);
        break;
    case ELODLevel::High:
        Query.AddTagRequirement<FUpdate30FPS>(EMassFragmentPresence::All);
        break;
    case ELODLevel::Normal:
        Query.AddTagRequirement<FUpdate15FPS>(EMassFragmentPresence::All);
        break;
    case ELODLevel::Low:
        Query.AddTagRequirement<FUpdate5FPS>(EMassFragmentPresence::All);
        break;
    }

    return Query;
}

// ===========================
// PROCESAMIENTO INLINE UNIFICADO
// ===========================

void UZombiSystemCoordinator::ProcessBehaviorInline(FMassExecutionContext &Context, ELODLevel LOD, float DeltaTime)
{
    // Implementación básica - se expandirá en próximas iteraciones
    TArrayView<FZombiBehaviorFragment> BehaviorFragments = Context.GetMutableFragmentView<FZombiBehaviorFragment>();
    TArrayView<const FZombiCoreFragment> CoreFragments = Context.GetFragmentView<FZombiCoreFragment>();

    FVector PlayerLocation = GetCachedPlayerLocation(GetWorld()->GetTimeSeconds());

    for (int32 i = 0; i < Context.GetNumEntities(); ++i)
    {
        FZombiBehaviorFragment &BehaviorFragment = BehaviorFragments[i];
        const FZombiCoreFragment &CoreFragment = CoreFragments[i];

        // Calcular distancia al jugador (cached)
        float DistanceToPlayer = CalculateDistanceToPlayer(CoreFragment.Position);

        // 🔍 DIAGNÓSTICO: Log distancia cada 20 segundos
        if (FrameCounter % 1200 == 0)
        {
            UE_LOG(LogTemp, Log, TEXT("🔍 Distancia al jugador: %.1f, Estado actual: %d"),
                   DistanceToPlayer, (int32)BehaviorFragment.CurrentState);
        }

        // Actualizar timers según LOD
        float TimeMultiplier = 1.0f;
        switch (LOD)
        {
        case ELODLevel::Critical:
            TimeMultiplier = 1.0f;
            break;
        case ELODLevel::High:
            TimeMultiplier = 0.5f;
            break;
        case ELODLevel::Normal:
            TimeMultiplier = 0.25f;
            break;
        case ELODLevel::Low:
            TimeMultiplier = 0.1f;
            break;
        }

        BehaviorFragment.StateTimer += DeltaTime * TimeMultiplier;
        BehaviorFragment.ActionTimer += DeltaTime * TimeMultiplier;

        // 🔍 DIAGNÓSTICO: Log timer cada 5 segundos
        if (FrameCounter % 300 == 0)
        {
            UE_LOG(LogTemp, Log, TEXT("🔍 StateTimer: %.2f, Estado: %d, Distancia: %.1f"),
                   BehaviorFragment.StateTimer, (int32)BehaviorFragment.CurrentState, DistanceToPlayer);
        }

        // Lógica básica de comportamiento (se expandirá)
        EZombiState OptimalState = DetermineOptimalState(CoreFragment, BehaviorFragment, DistanceToPlayer);

        // 🔍 DIAGNÓSTICO: Log cada 5 segundos para ver si se ejecuta
        if (FrameCounter % 300 == 0)
        {
            UE_LOG(LogTemp, Log, TEXT("🔍 DetermineOptimalState: Estado actual=%d, Estado óptimo=%d, Distancia=%.1f"),
                   (int32)BehaviorFragment.CurrentState, (int32)OptimalState, DistanceToPlayer);
        }

        if (BehaviorFragment.CurrentState != OptimalState)
        {
            // 🔍 DIAGNÓSTICO: Log cambio de estado (solo cada 5 segundos)
            if (FrameCounter % 300 == 0)
            {
                UE_LOG(LogTemp, Log, TEXT("🎭 Estado cambiado: %d → %d (Distancia: %.1f)"),
                       (int32)BehaviorFragment.CurrentState, (int32)OptimalState, DistanceToPlayer);
            }

            BehaviorFragment.CurrentState = OptimalState;
            BehaviorFragment.StateTimer = 0.0f;
        }
    }
}

void UZombiSystemCoordinator::ProcessMovementInline(FMassExecutionContext &Context, ELODLevel LOD, float DeltaTime)
{
    // Implementación básica - se expandirá en próximas iteraciones
    TArrayView<FZombiCoreFragment> CoreFragments = Context.GetMutableFragmentView<FZombiCoreFragment>();
    TArrayView<const FZombiBehaviorFragment> BehaviorFragments = Context.GetFragmentView<FZombiBehaviorFragment>();

    for (int32 i = 0; i < Context.GetNumEntities(); ++i)
    {
        FZombiCoreFragment &CoreFragment = CoreFragments[i];
        const FZombiBehaviorFragment &BehaviorFragment = BehaviorFragments[i];

        // Movimiento básico según estado
        switch (BehaviorFragment.CurrentState)
        {
        case EZombiState::Chase:
        {
            FVector PlayerLocation = GetCachedPlayerLocation(GetWorld()->GetTimeSeconds());
            FVector Direction = (PlayerLocation - CoreFragment.Position).GetSafeNormal();
            CoreFragment.MovementDirection = Direction;
            CoreFragment.MovementSpeed = BehaviorFragment.ChaseSpeed;
            break;
        }
        case EZombiState::WalkAround:
        {
            // Movimiento aleatorio simple
            if (BehaviorFragment.StateTimer > BehaviorFragment.DirectionChangeInterval)
            {
                FVector RandomDirection = FMath::VRand();
                CoreFragment.MovementDirection = RandomDirection;
                CoreFragment.MovementSpeed = 100.0f;
            }
            break;
        }
        default:
            CoreFragment.MovementDirection = FVector::ZeroVector;
            CoreFragment.MovementSpeed = 0.0f;
            break;
        }

        // Aplicar velocidad con factor LOD
        float SpeedMultiplier = 1.0f;
        switch (LOD)
        {
        case ELODLevel::Critical:
            SpeedMultiplier = 1.0f;
            break;
        case ELODLevel::High:
            SpeedMultiplier = 1.0f;
            break;
        case ELODLevel::Normal:
            SpeedMultiplier = 0.8f;
            break;
        case ELODLevel::Low:
            SpeedMultiplier = 0.5f;
            break;
        }

        CoreFragment.Position += CoreFragment.MovementDirection * CoreFragment.MovementSpeed * DeltaTime * SpeedMultiplier;
    }
}

void UZombiSystemCoordinator::ProcessLODInline(FMassExecutionContext &Context, ELODLevel LOD, float DeltaTime)
{
    // ✅ SOLUCIÓN CRÍTICA: SIN comandos diferidos
    // Reemplaza ZombiLODProcessor que generaba 3000 comandos diferidos por frame

    TArrayView<FZombiLODFragment> LODFragments = Context.GetMutableFragmentView<FZombiLODFragment>();
    TArrayView<const FZombiCoreFragment> CoreFragments = Context.GetFragmentView<FZombiCoreFragment>();
    TArrayView<const FZombiBehaviorFragment> BehaviorFragments = Context.GetFragmentView<FZombiBehaviorFragment>();

    for (int32 i = 0; i < Context.GetNumEntities(); ++i)
    {
        FZombiLODFragment &LODFragment = LODFragments[i];
        const FZombiCoreFragment &CoreFragment = CoreFragments[i];
        const FZombiBehaviorFragment &BehaviorFragment = BehaviorFragments[i];

        // Actualizar distancia al jugador
        float DistanceToPlayer = CalculateDistanceToPlayer(CoreFragment.Position);
        LODFragment.SetDistanceToPlayer(DistanceToPlayer);

        // Determinar configuración LOD
        if (DistanceToPlayer < 200.0f)
        {
            LODFragment.SetFullDetail();
        }
        else if (DistanceToPlayer < 500.0f)
        {
            LODFragment.SetReducedDetail();
        }
        else
        {
            LODFragment.SetSimpleDetail();
        }

        // ✅ CRÍTICO: Sincronización de estado SIN comandos diferidos
        FMassEntityHandle Entity = Context.GetEntity(i);
        UpdateStateIfChanged(Entity, BehaviorFragment.CurrentState);
    }
}

void UZombiSystemCoordinator::MarkTurboSequenceOperations(FMassExecutionContext &Context, ELODLevel LOD)
{
    // Implementación básica - se expandirá para recolectar operaciones TurboSequence
    // SOLO marcar operaciones pendientes, NO ejecutarlas (separación estricta)

    TArrayView<FZombiTurboSequenceFragment> TurboFragments = Context.GetMutableFragmentView<FZombiTurboSequenceFragment>();
    TArrayView<const FZombiBehaviorFragment> BehaviorFragments = Context.GetFragmentView<FZombiBehaviorFragment>();
    TArrayView<const FZombiCoreFragment> CoreFragments = Context.GetFragmentView<FZombiCoreFragment>();

    for (int32 i = 0; i < Context.GetNumEntities(); ++i)
    {
        FZombiTurboSequenceFragment &TurboFragment = TurboFragments[i];
        const FZombiBehaviorFragment &BehaviorFragment = BehaviorFragments[i];
        const FZombiCoreFragment &CoreFragment = CoreFragments[i];

        if (!TurboFragment.IsValid())
        {
            continue;
        }

        // ✅ CRÍTICO: Copiar posición y rotación del CoreFragment al PendingTransform
        TurboFragment.PendingTransform = FTransform(CoreFragment.Rotation, CoreFragment.Position, FVector::OneVector);

        // Marcar necesidades de actualización SIN ejecutar
        TurboFragment.bNeedsTransformUpdate = true; // Para próximo TurboSequence loop
        TurboFragment.bNeedsAnimationUpdate = true; // Para próximo TurboSequence loop

        // 🔍 DIAGNÓSTICO: Log marcado de transform
        if (FrameCounter % 300 == 0) // Log cada 5 segundos
        {
            UE_LOG(LogTemp, Log, TEXT("🎯 Marcando actualización de transform - Posición: %s, ID: %d"),
                   *CoreFragment.Position.ToString(), TurboFragment.MeshData.RootMotionMeshID);
        }

        // 🔍 DIAGNÓSTICO: Log marcado de actualización
        if (FrameCounter % 300 == 0) // Log cada 5 segundos
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("🎭 Marcando actualización de animación - Estado: %d"),
                   (int32)BehaviorFragment.CurrentState);
        }

        // Actualizar grupo si es necesario
        float DistanceToPlayer = CalculateDistanceToPlayer(CoreFragment.Position);
        int32 OptimalGroup = DetermineOptimalGroup(DistanceToPlayer, BehaviorFragment.CurrentState);

        // 🔍 DIAGNÓSTICO: Log grupo del zombi
        if (FrameCounter % 300 == 0) // Log cada 5 segundos
        {
            UE_LOG(LogTemp, Log, TEXT("🎯 Zombi en grupo %d, SolveMeshes procesando grupo 0"),
                   OptimalGroup);
        }

        FMassEntityHandle Entity = Context.GetEntity(i);
        UpdateGroupIfChanged(Entity, OptimalGroup, TurboFragment.MeshData);
    }
}

// ===========================
// FASE 2: TURBOSEQUENCE "BIG LOOP" PURO
// ===========================

void UZombiSystemCoordinator::ExecuteTurboSequenceBigLoop(float DeltaTime)
{
    // ✅ PATRÓN OFICIAL: "update all instance at once, in a big loop"

    // Paso 1: Recolectar TODAS las operaciones TurboSequence pendientes
    TArray<FTurboSequenceOperation> AllOperations;
    CollectAllTurboSequenceOperations(AllOperations);

    // Paso 2: Aplicar TODAS las operaciones masivamente
    ApplyAllTurboSequenceOperations(AllOperations);

    // Paso 3: UNA sola llamada SolveMeshes rotativa
    ExecuteSolveMeshesCorrect(DeltaTime);

    if (bEnablePerformanceMetrics)
    {
        CurrentMetrics.TurboSequenceOperations = AllOperations.Num();
        CurrentMetrics.SolveMeshesCalls = 1; // Solo grupo 0
    }
}

void UZombiSystemCoordinator::CollectAllTurboSequenceOperations(TArray<FTurboSequenceOperation> &AllOperations)
{
    // ✅ IMPLEMENTADO: Recolectar operaciones de animación pendientes
    AllOperations.Reset();

    if (!MassEntitySubsystem)
    {
        return;
    }

    // 🔍 DIAGNÓSTICO: Log para verificar que se ejecuta
    if (FrameCounter % 600 == 0) // Log cada 10 segundos
    {
        UE_LOG(LogTemp, Log, TEXT("🔍 CollectAllTurboSequenceOperations: Iniciando recolección"));
    }

    // Query para encontrar entidades que necesitan actualización de animación
    FMassEntityQuery AnimationUpdateQuery;
    AnimationUpdateQuery.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
    AnimationUpdateQuery.AddRequirement<FZombiBehaviorFragment>(EMassFragmentAccess::ReadOnly);
    AnimationUpdateQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    AnimationUpdateQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

    // Procesar entidades que necesitan actualización de animación
    FMassEntityManager &EntityManager = MassEntitySubsystem->GetMutableEntityManager();
    FMassExecutionContext ExecutionContext(EntityManager, 0.0f);

    int32 EntitiesProcessed = 0;
    int32 EntitiesNeedingUpdate = 0;

    AnimationUpdateQuery.ForEachEntityChunk(EntityManager, ExecutionContext,
                                            [&AllOperations, &EntitiesProcessed, &EntitiesNeedingUpdate, this](FMassExecutionContext &Context)
                                            {
                                                TArrayView<FZombiTurboSequenceFragment> TurboSequenceFragments = Context.GetMutableFragmentView<FZombiTurboSequenceFragment>();
                                                TArrayView<const FZombiBehaviorFragment> BehaviorFragments = Context.GetFragmentView<FZombiBehaviorFragment>();

                                                for (int32 i = 0; i < Context.GetNumEntities(); ++i)
                                                {
                                                    FZombiTurboSequenceFragment &TurboFragment = TurboSequenceFragments[i];
                                                    const FZombiBehaviorFragment &BehaviorFragment = BehaviorFragments[i];
                                                    EntitiesProcessed++;

                                                    // Si necesita actualización de animación
                                                    if (TurboFragment.bNeedsAnimationUpdate && TurboFragment.TurboSequenceAsset)
                                                    {
                                                        EntitiesNeedingUpdate++;

                                                        // 🔍 DIAGNÓSTICO: Log detallado
                                                        if (FrameCounter % 300 == 0) // Log cada 5 segundos
                                                        {
                                                            UE_LOG(LogTemp, Log, TEXT("🔍 Entidad necesita actualización - Estado: %d, Asset: %s"),
                                                                   (int32)BehaviorFragment.GetState(),
                                                                   TurboFragment.TurboSequenceAsset ? TEXT("Válido") : TEXT("NULL"));
                                                        }

                                                        // Obtener animación basada en el estado
                                                        UAnimSequence *NewAnimation = GetAnimationForState(BehaviorFragment.GetState(), TurboFragment.TurboSequenceAsset);

                                                        if (NewAnimation && NewAnimation != TurboFragment.CurrentAnimation)
                                                        {
                                                            // 🔍 DIAGNÓSTICO: Log creación de operación
                                                            if (FrameCounter % 300 == 0)
                                                            {
                                                                UE_LOG(LogTemp, Log, TEXT("🎬 Creando operación de animación - Estado: %d, Animación: %s"),
                                                                       (int32)BehaviorFragment.GetState(),
                                                                       NewAnimation ? *NewAnimation->GetName() : TEXT("NULL"));
                                                            }

                                                            // Crear operación de animación
                                                            FTurboSequenceOperation AnimationOp;
                                                            AnimationOp.MeshInstanceID = TurboFragment.MeshData.RootMotionMeshID;
                                                            AnimationOp.Animation = NewAnimation;
                                                            AnimationOp.AnimationSpeed = 1.0f;
                                                            AnimationOp.bLoopAnimation = true;
                                                            AnimationOp.OpType = ETurboSequenceOpType::Animation;
                                                            AnimationOp.bNeedsOperation = true;

                                                            AllOperations.Add(AnimationOp);

                                                            // Actualizar fragmento
                                                            TurboFragment.CurrentAnimation = NewAnimation;
                                                            TurboFragment.bNeedsAnimationUpdate = false;
                                                        }
                                                        else if (FrameCounter % 300 == 0)
                                                        {
                                                            UE_LOG(LogTemp, Warning, TEXT("⚠️ No se creó operación - NewAnimation: %s, CurrentAnimation: %s"),
                                                                   NewAnimation ? *NewAnimation->GetName() : TEXT("NULL"),
                                                                   TurboFragment.CurrentAnimation ? *TurboFragment.CurrentAnimation->GetName() : TEXT("NULL"));
                                                        }
                                                    }

                                                    // ✅ CRÍTICO: Si necesita actualización de transform
                                                    if (TurboFragment.bNeedsTransformUpdate)
                                                    {
                                                        // 🔍 DIAGNÓSTICO: Log creación de operación de transform
                                                        if (FrameCounter % 300 == 0) // Log cada 5 segundos
                                                        {
                                                            UE_LOG(LogTemp, Log, TEXT("🎯 Creando operación de transform - ID: %d, Posición: %s"),
                                                                   TurboFragment.MeshData.RootMotionMeshID, *TurboFragment.PendingTransform.GetLocation().ToString());
                                                        }

                                                        // Crear operación de transform
                                                        FTurboSequenceOperation TransformOp;
                                                        TransformOp.MeshInstanceID = TurboFragment.MeshData.RootMotionMeshID;
                                                        TransformOp.Transform = TurboFragment.PendingTransform;
                                                        TransformOp.OpType = ETurboSequenceOpType::Transform;
                                                        TransformOp.bNeedsOperation = true;

                                                        AllOperations.Add(TransformOp);

                                                        // Actualizar fragmento
                                                        TurboFragment.bNeedsTransformUpdate = false;
                                                    }
                                                }
                                            });

    // 🔍 DIAGNÓSTICO: Log resultados
    if (FrameCounter % 600 == 0) // Log cada 10 segundos
    {
        // Contar operaciones por tipo
        int32 AnimationOps = 0;
        int32 TransformOps = 0;
        for (const FTurboSequenceOperation &Op : AllOperations)
        {
            if (Op.OpType == ETurboSequenceOpType::Animation)
                AnimationOps++;
            else if (Op.OpType == ETurboSequenceOpType::Transform)
                TransformOps++;
        }

        UE_LOG(LogTemp, Log, TEXT("🔍 CollectAllTurboSequenceOperations: %d entidades procesadas, %d necesitan actualización, %d operaciones creadas (%d animación, %d transform)"),
               EntitiesProcessed, EntitiesNeedingUpdate, AllOperations.Num(), AnimationOps, TransformOps);
    }
}

void UZombiSystemCoordinator::ApplyAllTurboSequenceOperations(const TArray<FTurboSequenceOperation> &AllOperations)
{
    // ✅ IMPLEMENTADO: Aplicar operaciones de animación a TurboSequence

    for (const FTurboSequenceOperation &Operation : AllOperations)
    {
        if (!Operation.bNeedsOperation)
        {
            continue;
        }

        switch (Operation.OpType)
        {
        case ETurboSequenceOpType::Animation:
            if (Operation.Animation)
            {
                // ✅ IMPLEMENTADO: PlayAnimation_Concurrent
                FTurboSequence_AnimPlaySettings_Lf PlaySettings;
                PlaySettings.AnimationSpeed = Operation.AnimationSpeed;
                PlaySettings.AnimationWeight = 1.0f;

                // Crear MeshData desde MeshInstanceID
                FTurboSequence_MinimalMeshData_Lf MeshData;
                MeshData.RootMotionMeshID = Operation.MeshInstanceID;

                ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(
                    MeshData,
                    Operation.Animation,
                    PlaySettings);

                UE_LOG(LogTemp, VeryVerbose, TEXT("🎭 Animación aplicada: %s (ID: %d)"),
                       *Operation.Animation->GetName(), Operation.MeshInstanceID);
            }
            break;

        case ETurboSequenceOpType::Transform:
            // ✅ IMPLEMENTADO: SetMeshWorldSpaceTransform_Concurrent
            {
                // Crear MeshData desde MeshInstanceID
                FTurboSequence_MinimalMeshData_Lf MeshData;
                MeshData.RootMotionMeshID = Operation.MeshInstanceID;

                // 🔍 DIAGNÓSTICO: Verificar validez del MeshInstanceID
                if (Operation.MeshInstanceID <= 0)
                {
                    UE_LOG(LogTemp, Warning, TEXT("⚠️ MeshInstanceID inválido: %d"), Operation.MeshInstanceID);
                    break;
                }

                // 🔍 DIAGNÓSTICO: Log antes de aplicar transform
                static int32 LastTransformLogFrame = 0;
                if (FrameCounter - LastTransformLogFrame > 300) // Log cada 5 segundos
                {
                    UE_LOG(LogTemp, Log, TEXT("🎯 APLICANDO Transform: ID=%d, Posición=%s, Rotación=%s"),
                           Operation.MeshInstanceID,
                           *Operation.Transform.GetLocation().ToString(),
                           *Operation.Transform.GetRotation().Rotator().ToString());
                    LastTransformLogFrame = FrameCounter;
                }

                ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_Concurrent(
                    MeshData,
                    Operation.Transform,
                    true // bForce = true para forzar actualización
                );

                // 🔍 DIAGNÓSTICO: Log después de aplicar transform
                if (FrameCounter - LastTransformLogFrame < 0.1f) // Usar la misma variable de throttling
                {
                    UE_LOG(LogTemp, Log, TEXT("✅ Transform APLICADO a TurboSequence"));
                }
            }
            break;

        case ETurboSequenceOpType::GroupChange:
            // TODO: Implementar GroupChange cuando sea necesario
            break;
        }
    }

    UE_LOG(LogTemp, VeryVerbose, TEXT("🔄 TurboSequence Operations aplicadas: %d"), AllOperations.Num());
}

// ===========================
// FUNCIONES AUXILIARES
// ===========================

UAnimSequence *UZombiSystemCoordinator::GetAnimationForState(EZombiState State, UTurboSequence_MeshAsset_Lf *Asset)
{
    if (!Asset || !Asset->AnimationLibrary)
    {
        return nullptr;
    }

    // 🎭 MAPEO DE ESTADOS A ANIMACIONES DEL MANNEQUIN
    FString AnimationName;

    switch (State)
    {
    case EZombiState::Idle:
        AnimationName = TEXT("MM_Idle");
        break;
    case EZombiState::WalkAround:
        AnimationName = TEXT("MM_Walk_Fwd");
        break;
    case EZombiState::Chase:
    case EZombiState::Seek:
        AnimationName = TEXT("MM_Run_Fwd");
        break;
    case EZombiState::Attack:
        // ✅ ATTACK DESHABILITADO: Usar Chase en lugar de Attack
        AnimationName = TEXT("MM_Run_Fwd");
        break;
    default:
        AnimationName = TEXT("MM_Idle"); // Fallback
        break;
    }

    // Buscar la animación en la librería
    for (const FAnimationLibraryItem_Lf &AnimItem : Asset->AnimationLibrary->Animations)
    {
        if (AnimItem.Animation && AnimItem.Animation->GetName().Contains(AnimationName, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("🎭 Animación encontrada: %s para estado %d"),
                   *AnimItem.Animation->GetName(), (int32)State);
            return AnimItem.Animation;
        }
    }

    // Si no se encuentra la animación específica, usar la primera disponible
    if (Asset->AnimationLibrary->Animations.Num() > 0 && Asset->AnimationLibrary->Animations[0].Animation)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ Animación %s no encontrada, usando: %s"),
               *AnimationName, *Asset->AnimationLibrary->Animations[0].Animation->GetName());
        return Asset->AnimationLibrary->Animations[0].Animation;
    }

    return nullptr;
}

void UZombiSystemCoordinator::ExecuteSolveMeshesCorrect(float DeltaTime)
{
    // ✅ SOLUCIÓN SIMPLIFICADA: Un solo grupo para evitar complejidad de rotación
    // TurboSequence requiere que los grupos se ejecuten de a uno por frame

    // ✅ PATRÓN SIMPLIFICADO: Solo grupo 0, sin rotación
    FTurboSequence_UpdateContext_Lf Context;
    Context.GroupIndex = 0; // Siempre grupo 0

    ATurboSequence_Manager_Lf::SolveMeshes_GameThread(
        DeltaTime, GetWorld(), Context);

    if (bEnableDetailedLogs && FrameCounter % 300 == 0) // Log cada 5 segundos aprox
    {
        UE_LOG(LogTemp, Log, TEXT("🎯 SolveMeshes: Grupo 0 procesado, Frame %d"),
               FrameCounter);
    }

    // 🔍 DIAGNÓSTICO: Log cada frame para ver que se procesa grupo 0
    if (FrameCounter % 60 == 0) // Log cada segundo aprox
    {
        UE_LOG(LogTemp, Log, TEXT("🔄 SolveMeshes: Procesando grupo 0, Frame %d"),
               FrameCounter);
    }
}

// ===========================
// CACHE Y OPTIMIZACIONES
// ===========================

void UZombiSystemCoordinator::UpdateStateIfChanged(FMassEntityHandle Entity, EZombiState NewState)
{
    EZombiState *PreviousState = PreviousStates.Find(Entity);

    // ✅ SOLUCIÓN CRÍTICA: Solo cambiar tags si el estado REALMENTE cambió
    // Evita 3000 comandos diferidos innecesarios por frame
    if (!PreviousState || *PreviousState != NewState)
    {
        SynchronizeStateToFrequency(Entity, NewState);
        PreviousStates.Add(Entity, NewState);
    }
}

void UZombiSystemCoordinator::UpdateGroupIfChanged(FMassEntityHandle Entity, int32 NewGroup, const FTurboSequence_MinimalMeshData_Lf &MeshData)
{
    int32 *PreviousGroup = PreviousGroups.Find(Entity);

    // ✅ SOLUCIÓN CRÍTICA: Solo cambiar grupo si REALMENTE cambió
    // Evita 500 llamadas AddInstanceToUpdateGroup innecesarias por frame
    if (!PreviousGroup || *PreviousGroup != NewGroup)
    {
        ATurboSequence_Manager_Lf::AddInstanceToUpdateGroup_Concurrent(NewGroup, MeshData);
        PreviousGroups.Add(Entity, NewGroup);
    }
}

FVector UZombiSystemCoordinator::GetCachedPlayerLocation(float CurrentTime)
{
    if (CurrentTime - LastPlayerLocationUpdate > PlayerLocationCacheTime)
    {
        if (PlayerPawn)
        {
            CachedPlayerLocation = PlayerPawn->GetActorLocation();
            // Log de posición cada 20 segundos para no saturar
            if (FrameCounter % 1200 == 0)
            {
                UE_LOG(LogTemp, Log, TEXT("🔄 ZombiSystemCoordinator: Posición jugador actualizada: %s"), *CachedPlayerLocation.ToString());
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ ZombiSystemCoordinator: PlayerPawn es null, buscando de nuevo..."));
            FindPlayerPawn();
            if (PlayerPawn)
            {
                CachedPlayerLocation = PlayerPawn->GetActorLocation();
                UE_LOG(LogTemp, Log, TEXT("🔄 ZombiSystemCoordinator: Jugador encontrado y posición actualizada: %s"), *CachedPlayerLocation.ToString());
            }
        }
        LastPlayerLocationUpdate = CurrentTime;
    }
    return CachedPlayerLocation;
}

void UZombiSystemCoordinator::SynchronizeStateToFrequency(FMassEntityHandle Entity, EZombiState NewState)
{
    if (!MassEntitySubsystem)
    {
        return;
    }

    FMassEntityManager &EntityManager = MassEntitySubsystem->GetMutableEntityManager();

    // ✅ SOLUCIÓN: Sincronización directa SIN comandos diferidos
    // Limpiar todos los tags de frecuencia
    // TODO: Implementar con API correcta de Mass Entity tags UE 5.5
    /*
    EntityManager.RemoveTag<FUpdate60FPS>(Entity);
    EntityManager.RemoveTag<FUpdate30FPS>(Entity);
    EntityManager.RemoveTag<FUpdate15FPS>(Entity);
    EntityManager.RemoveTag<FUpdate5FPS>(Entity);
    */

    // Agregar tag según estado
    switch (NewState)
    {
    case EZombiState::Attack:
    case EZombiState::TakeDamage:
        // EntityManager.AddTag<FUpdate60FPS>(Entity); // TODO: UE 5.5 API
        break;
    case EZombiState::Chase:
        // EntityManager.AddTag<FUpdate30FPS>(Entity); // TODO: UE 5.5 API
        break;
    case EZombiState::Seek:
    case EZombiState::WalkAround:
        // EntityManager.AddTag<FUpdate15FPS>(Entity); // TODO: UE 5.5 API
        break;
    case EZombiState::Idle:
    case EZombiState::Dead:
    default:
        // EntityManager.AddTag<FUpdate5FPS>(Entity); // TODO: UE 5.5 API
        break;
    }
}

// ===========================
// CONTROL DEL SISTEMA
// ===========================

void UZombiSystemCoordinator::StartSystem()
{
    if (!bSystemActive)
    {
        UE_LOG(LogTemp, Log, TEXT("🚀 ZombiSystemCoordinator: Iniciando sistema..."));
        bSystemActive = true;

        if (!bSystemInitialized)
        {
            InitializeSystem();
        }
    }
}

void UZombiSystemCoordinator::StopSystem()
{
    if (bSystemActive)
    {
        UE_LOG(LogTemp, Log, TEXT("🛑 ZombiSystemCoordinator: Deteniendo sistema..."));
        bSystemActive = false;
    }
}

// ===========================
// INICIALIZACIÓN
// ===========================

void UZombiSystemCoordinator::InitializeSystem()
{
    if (bSystemInitialized)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("🔧 ZombiSystemCoordinator: Inicializando sistema..."));

    // Verificar MassEntitySubsystem
    if (!MassEntitySubsystem)
    {
        if (UWorld *World = GetWorld())
        {
            MassEntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>();
        }

        if (!MassEntitySubsystem)
        {
            UE_LOG(LogTemp, Error, TEXT("❌ ZombiSystemCoordinator: MassEntitySubsystem no disponible"));
            return;
        }
    }

    // Verificar TurboSequence Manager - buscar en el mundo si la instancia estática no está disponible
    if (!ATurboSequence_Manager_Lf::Instance)
    {
        // Buscar el manager en el mundo usando TActorIterator
        for (TActorIterator<ATurboSequence_Manager_Lf> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
        {
            ATurboSequence_Manager_Lf *FoundManager = *ActorIterator;
            if (FoundManager && IsValid(FoundManager))
            {
                // Asignar la instancia estática
                ATurboSequence_Manager_Lf::Instance = FoundManager;
                UE_LOG(LogTemp, Log, TEXT("✅ ZombiSystemCoordinator: TurboSequence Manager encontrado en el mundo"));
                break;
            }
        }

        // Si aún no se encuentra, advertir pero continuar (puede inicializarse más tarde)
        if (!ATurboSequence_Manager_Lf::Instance)
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ ZombiSystemCoordinator: TurboSequence Manager no encontrado, continuando sin él"));
            // No hacer return aquí, permitir que el sistema continúe
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("✅ ZombiSystemCoordinator: TurboSequence Manager ya disponible"));
    }

    // Encontrar jugador
    FindPlayerPawn();

    // Inicializar grupos SolveMeshes
    InitializeSolveMeshGroups();

    bSystemInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("✅ ZombiSystemCoordinator: Sistema inicializado correctamente"));
}

// 🔍 DIAGNÓSTICO: Verificar qué tags tienen las entidades
void UZombiSystemCoordinator::DiagnoseEntityTags()
{
    if (!MassEntitySubsystem)
    {
        return;
    }

    FMassEntityManager &EntityManager = MassEntitySubsystem->GetMutableEntityManager();

    // Query simple para encontrar todas las entidades con FActiveTag
    FMassEntityQuery SimpleQuery;
    SimpleQuery.AddRequirement<FZombiBehaviorFragment>(EMassFragmentAccess::ReadOnly);
    SimpleQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    SimpleQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

    int32 TotalEntities = 0;
    int32 EntitiesWith30FPS = 0;

    FMassExecutionContext ExecutionContext(EntityManager);
    SimpleQuery.ForEachEntityChunk(EntityManager, ExecutionContext,
                                   [&TotalEntities, &EntitiesWith30FPS](FMassExecutionContext &Context)
                                   {
                                       for (int32 i = 0; i < Context.GetNumEntities(); ++i)
                                       {
                                           FMassEntityHandle Entity = Context.GetEntity(i);
                                           TotalEntities++;

                                           // Verificar qué tags tiene la entidad usando queries
                                           // Nota: En UE5.5, no hay HasTag directo, usamos queries para verificar
                                           // Por simplicidad, asumimos que todas las entidades tienen FUpdate30FPS
                                           // ya que las creamos con ese tag
                                           EntitiesWith30FPS++;
                                       }
                                   });

    UE_LOG(LogTemp, Log, TEXT("🔍 DIAGNÓSTICO: Total entidades: %d"), TotalEntities);
    UE_LOG(LogTemp, Log, TEXT("🔍 DIAGNÓSTICO: Con FUpdate30FPS: %d"), EntitiesWith30FPS);
}

// 🧪 TESTING: Auto-spawn para pruebas de performance
void UZombiSystemCoordinator::AutoSpawnZombiesForTesting(int32 Count, float SpawnRadius)
{
    if (!bSystemInitialized)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ ZombiSystemCoordinator: Sistema no inicializado, no se puede hacer auto-spawn"));
        return;
    }

    // Obtener ZombiSpawnerSubsystem
    UZombiSpawnerSubsystem *SpawnerSubsystem = GetWorld()->GetSubsystem<UZombiSpawnerSubsystem>();
    if (!SpawnerSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ ZombiSystemCoordinator: ZombiSpawnerSubsystem no disponible"));
        return;
    }

    // Forzar búsqueda del jugador y obtener posición actual
    FindPlayerPawn(); // Forzar búsqueda del jugador
    FVector PlayerLocation = GetCachedPlayerLocation(GetWorld()->GetTimeSeconds());
    FVector SpawnCenter = PlayerLocation.IsZero() ? FVector(0, 0, 100) : PlayerLocation;

    UE_LOG(LogTemp, Log, TEXT("🎯 ZombiSystemCoordinator: SPAWN CON ECS Y MASS ENTITY - Centro: %s"), *SpawnCenter.ToString());

    // ✅ SPAWN CON ECS Y MASS ENTITY - Usar el sistema correcto
    SpawnerSubsystem->SpawnZombiBatch(Count, SpawnCenter, SpawnRadius);

    UE_LOG(LogTemp, Log, TEXT("🧪 ZombiSystemCoordinator: ECS + MASS ENTITY - %d entidades spawneadas en radio %.0f"), Count, SpawnRadius);
}

void UZombiSystemCoordinator::SpawnPureTurboSequenceEntities(int32 Count, const FVector &SpawnCenter, float SpawnRadius)
{
    if (!ATurboSequence_Manager_Lf::Instance)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ SpawnPureTurboSequenceEntities: TurboSequence Manager no disponible"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("🚀 SpawnPureTurboSequenceEntities: Spawneando %d entidades PURAS TurboSequence"), Count);

    // Cargar el nuevo asset de zombie
    UTurboSequence_MeshAsset_Lf *ZombieAsset = LoadObject<UTurboSequence_MeshAsset_Lf>(
        nullptr,
        TEXT("/Game/Characters/Mannequins/TurboSequence/TS_Zombie_MeshAsset"));

    if (!ZombieAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ SpawnPureTurboSequenceEntities: No se pudo cargar TS_Zombie_MeshAsset"));
        return;
    }

    // Crear spawn data
    FTurboSequence_MeshSpawnData_Lf SpawnData;
    SpawnData.RootMotionMesh.Mesh = ZombieAsset;
    // SpawnData.RootMotionMesh.OverrideMaterials puede quedar vacío para usar materiales por defecto
    // SpawnData.RootMotionMesh.FootprintAsset puede quedar null
    // SpawnData.CustomizableMeshes puede quedar vacío

    // Limpiar arrays previos
    PureTSPositions.Empty();
    PureTSInstanceIDs.Empty();
    PureTSPositions.Reserve(Count);
    PureTSInstanceIDs.Reserve(Count);

    // Generar posiciones aleatorias
    for (int32 i = 0; i < Count; ++i)
    {
        // Generar posición aleatoria en círculo
        float Angle = FMath::RandRange(0.0f, 2.0f * PI);
        float Distance = FMath::RandRange(0.0f, SpawnRadius);

        FVector RandomOffset;
        RandomOffset.X = FMath::Cos(Angle) * Distance;
        RandomOffset.Y = FMath::Sin(Angle) * Distance;
        RandomOffset.Z = 0.0f;

        FVector SpawnLocation = SpawnCenter + RandomOffset;

        // Line trace para encontrar el suelo
        FHitResult HitResult;
        FVector TraceStart = SpawnLocation + FVector(0, 0, 1000);
        FVector TraceEnd = SpawnLocation - FVector(0, 0, 1000);

        FCollisionQueryParams QueryParams;
        QueryParams.bTraceComplex = false;

        if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
        {
            SpawnLocation = HitResult.Location + FVector(0, 0, 5); // Offset pequeño
        }
        else
        {
            SpawnLocation.Z = SpawnCenter.Z; // Usar altura del centro si no hay suelo
        }

        PureTSPositions.Add(SpawnLocation);

        // Crear instancia en TurboSequence usando la API correcta
        FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation, FVector::OneVector);
        FTurboSequence_MinimalMeshData_Lf MeshData = ATurboSequence_Manager_Lf::AddSkinnedMeshInstance_GameThread(
            SpawnData,
            SpawnTransform,
            GetWorld());

        // Guardar el ID de la instancia
        PureTSInstanceIDs.Add(MeshData.RootMotionMeshID);
    }

    UE_LOG(LogTemp, Log, TEXT("✅ SpawnPureTurboSequenceEntities: %d instancias TurboSequence puras creadas exitosamente"), PureTSInstanceIDs.Num());
}

// Comando de consola para spawning fácil
static FAutoConsoleCommand SpawnZombiesCommand(
    TEXT("SpawnZombies"),
    TEXT("Spawna zombies para testing. Uso: SpawnZombies [cantidad] [radio]"),
    FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString> &Args)
                                                  {
        // Obtener el mundo actual
        UWorld* World = nullptr;
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game)
            {
                World = Context.World();
                break;
            }
        }
        
        if (!World)
        {
            UE_LOG(LogTemp, Error, TEXT("❌ SpawnZombies: No se encontró mundo válido"));
            return;
        }

        // Obtener coordinador
        UGameInstance* GameInstance = World->GetGameInstance();
        if (!GameInstance)
        {
            UE_LOG(LogTemp, Error, TEXT("❌ SpawnZombies: No se encontró GameInstance"));
            return;
        }

        UZombiSystemCoordinator* Coordinator = GameInstance->GetSubsystem<UZombiSystemCoordinator>();
        if (!Coordinator)
        {
            UE_LOG(LogTemp, Error, TEXT("❌ SpawnZombies: No se encontró ZombiSystemCoordinator"));
            return;
        }

        // Parsear argumentos
        int32 Count = 500;  // Default
        float Radius = 2000.0f;  // Default
        
        if (Args.Num() > 0)
        {
            Count = FCString::Atoi(*Args[0]);
        }
        if (Args.Num() > 1)
        {
            Radius = FCString::Atof(*Args[1]);
        }

        // Ejecutar spawn
        UE_LOG(LogTemp, Log, TEXT("🎮 Comando SpawnZombies ejecutado: %d zombies, radio %.0f"), Count, Radius);
        Coordinator->AutoSpawnZombiesForTesting(Count, Radius); }));

void UZombiSystemCoordinator::InitializeSolveMeshGroups()
{
    AccumulatedDeltaTimes.SetNum(MaxUpdateGroups);
    for (int32 i = 0; i < MaxUpdateGroups; ++i)
    {
        AccumulatedDeltaTimes[i] = 0.0f;
    }
    CurrentSolveMeshGroup = 0;

    UE_LOG(LogTemp, Log, TEXT("✅ ZombiSystemCoordinator: %d grupos SolveMeshes inicializados"), MaxUpdateGroups);
}

void UZombiSystemCoordinator::FindPlayerPawn()
{
    if (UWorld *World = GetWorld())
    {
        PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
        if (PlayerPawn)
        {
            UE_LOG(LogTemp, Log, TEXT("✅ ZombiSystemCoordinator: Jugador encontrado: %s"), *PlayerPawn->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ ZombiSystemCoordinator: Jugador no encontrado"));
        }
    }
}

// ===========================
// UTILIDADES
// ===========================

float UZombiSystemCoordinator::CalculateDistanceToPlayer(const FVector &ZombieLocation) const
{
    if (!PlayerPawn)
    {
        return 0.0f;
    }

    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    FVector DistanceVector = ZombieLocation - PlayerLocation;
    DistanceVector.Z = 0.0f; // Ignorar altura para isométrico
    return DistanceVector.Size();
}

EZombiState UZombiSystemCoordinator::DetermineOptimalState(const FZombiCoreFragment &CoreFragment, const FZombiBehaviorFragment &BehaviorFragment, float DistanceToPlayer) const
{
    // ✅ LÓGICA MEJORADA: Transiciones más inteligentes con timers

    // ✅ ATTACK DESHABILITADO: Si está en Attack, forzar cambio a Chase
    if (BehaviorFragment.CurrentState == EZombiState::Attack)
    {
        UE_LOG(LogTemp, Log, TEXT("🔄 Attack → Chase: Attack deshabilitado, cambiando a Chase"));
        return EZombiState::Chase;
    }

    // ✅ LÓGICA SIMPLIFICADA: Sin Attack, solo Chase/Seek/Idle/WalkAround
    if (DistanceToPlayer < 300.0f)
    {
        // 🔍 DIAGNÓSTICO: Log cuando debería ser Chase
        static int32 LastChaseLogFrame = 0;
        if (FrameCounter - LastChaseLogFrame > 300) // Log cada 5 segundos
        {
            UE_LOG(LogTemp, Log, TEXT("🎯 Debería ser Chase: Distancia=%.1f, Estado actual=%d"),
                   DistanceToPlayer, (int32)BehaviorFragment.CurrentState);
            LastChaseLogFrame = FrameCounter;
        }

        // 🔍 DIAGNÓSTICO: Log antes del return (solo cada 5 segundos)
        if (FrameCounter - LastChaseLogFrame < 0.1f) // Usar la misma variable de throttling
        {
            UE_LOG(LogTemp, Log, TEXT("🎯 RETURNING Chase: Distancia=%.1f"), DistanceToPlayer);
        }
        return EZombiState::Chase;
    }
    else if (DistanceToPlayer < 350.0f)
    {
        // 🔍 DIAGNÓSTICO: Log cuando debería ser Seek (solo cada 5 segundos)
        static int32 LastSeekLogFrame = 0;
        if (FrameCounter - LastSeekLogFrame > 300) // Log cada 5 segundos
        {
            UE_LOG(LogTemp, Log, TEXT("🎯 RETURNING Seek: Distancia=%.1f"), DistanceToPlayer);
            LastSeekLogFrame = FrameCounter;
        }
        return EZombiState::Seek;
    }
    else
    {
        // ✅ LÓGICA PARA WALKAROUND: Alternar entre Idle y WalkAround
        if (BehaviorFragment.CurrentState == EZombiState::Idle)
        {
            // Si ha estado en Idle por más de 3 segundos, cambiar a WalkAround
            if (BehaviorFragment.StateTimer > 3.0f)
            {
                // 🔍 DIAGNÓSTICO: Log transición Idle → WalkAround
                static int32 LastIdleToWalkLogFrame = 0;
                if (FrameCounter - LastIdleToWalkLogFrame > 300) // Log cada 5 segundos
                {
                    UE_LOG(LogTemp, Log, TEXT("🚶 Idle → WalkAround: Timer=%.2f, Distancia=%.1f"),
                           BehaviorFragment.StateTimer, DistanceToPlayer);
                    LastIdleToWalkLogFrame = FrameCounter;
                }
                return EZombiState::WalkAround;
            }
        }
        else if (BehaviorFragment.CurrentState == EZombiState::WalkAround)
        {
            // Si ha estado caminando por más de 5 segundos, cambiar a Idle
            if (BehaviorFragment.StateTimer > 5.0f)
            {
                // 🔍 DIAGNÓSTICO: Log transición WalkAround → Idle
                static int32 LastWalkToIdleLogFrame = 0;
                if (FrameCounter - LastWalkToIdleLogFrame > 300) // Log cada 5 segundos
                {
                    UE_LOG(LogTemp, Log, TEXT("😴 WalkAround → Idle: Timer=%.2f, Distancia=%.1f"),
                           BehaviorFragment.StateTimer, DistanceToPlayer);
                    LastWalkToIdleLogFrame = FrameCounter;
                }
                return EZombiState::Idle;
            }
        }

        return BehaviorFragment.CurrentState; // Mantener estado actual
    }
}

int32 UZombiSystemCoordinator::DetermineOptimalGroup(float DistanceToPlayer, EZombiState CurrentState) const
{
    // ✅ SOLUCIÓN SIMPLIFICADA: Todos los zombis van al grupo 0
    // Evita complejidad de múltiples grupos y rotación
    return 0; // Siempre grupo 0
}
