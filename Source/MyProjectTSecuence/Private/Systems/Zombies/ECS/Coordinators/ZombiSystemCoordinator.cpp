// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Coordinators/ZombiSystemCoordinator.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Systems/Zombies/ECS/Subsystems/ZombiSpawnerSubsystem.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"

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

        // Lógica básica de comportamiento (se expandirá)
        EZombiState OptimalState = DetermineOptimalState(CoreFragment, BehaviorFragment, DistanceToPlayer);
        if (BehaviorFragment.CurrentState != OptimalState)
        {
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

        // Marcar necesidades de actualización SIN ejecutar
        TurboFragment.bNeedsTransformUpdate = true; // Para próximo TurboSequence loop
        TurboFragment.bNeedsAnimationUpdate = true; // Para próximo TurboSequence loop

        // Actualizar grupo si es necesario
        float DistanceToPlayer = CalculateDistanceToPlayer(CoreFragment.Position);
        int32 OptimalGroup = DetermineOptimalGroup(DistanceToPlayer, BehaviorFragment.CurrentState);

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
        CurrentMetrics.SolveMeshesCalls = 1; // Solo una llamada por frame
    }
}

void UZombiSystemCoordinator::CollectAllTurboSequenceOperations(TArray<FTurboSequenceOperation> &AllOperations)
{
    // Implementación básica - se expandirá para recolectar todas las operaciones pendientes
    AllOperations.Reset();

    // Por ahora, implementación mínima para testing
    // Se expandirá en próximas iteraciones
}

void UZombiSystemCoordinator::ApplyAllTurboSequenceOperations(const TArray<FTurboSequenceOperation> &AllOperations)
{
    // Implementación básica - se expandirá para aplicar operaciones masivamente

    // TODO: Implementar llamadas TurboSequence con MeshInstanceID
    // Necesita conversión de MeshInstanceID a FTurboSequence_MinimalMeshData_Lf
    /*
    for (const FTurboSequenceOperation &Operation : AllOperations)
    {
        if (!Operation.IsValid())
        {
            continue;
        }

        switch (Operation.OpType)
        {
        case ETurboSequenceOpType::Animation:
            if (Operation.Animation)
            {
                // TODO: PlayAnimation con MeshInstanceID
                // ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(MeshData, Animation, AnimSettings);
            }
            break;

        case ETurboSequenceOpType::Transform:
            // TODO: SetTransform con MeshInstanceID
            // ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_Concurrent(MeshData, Transform);
            break;

        case ETurboSequenceOpType::GroupChange:
            if (Operation.TargetGroup >= 0)
            {
                // TODO: GroupChange con MeshInstanceID
                // ATurboSequence_Manager_Lf::AddInstanceToUpdateGroup_Concurrent(TargetGroup, MeshData);
            }
            break;
        }
    }
    */

    UE_LOG(LogTemp, VeryVerbose, TEXT("🔄 TurboSequence Operations queued: %d (TODO: implement with MeshInstanceID)"), AllOperations.Num());
}

void UZombiSystemCoordinator::ExecuteSolveMeshesCorrect(float DeltaTime)
{
    // ✅ SOLUCIÓN CRÍTICA: UNA sola llamada SolveMeshes rotativa por frame
    // vs 2 llamadas actuales = 50% reducción

    // Acumular delta para todos los grupos
    for (float &Delta : AccumulatedDeltaTimes)
    {
        Delta += DeltaTime;
    }

    // ✅ PATRÓN OFICIAL: Solo UNA llamada SolveMeshes por frame
    FTurboSequence_UpdateContext_Lf Context;
    Context.GroupIndex = CurrentSolveMeshGroup;

    ATurboSequence_Manager_Lf::SolveMeshes_GameThread(
        AccumulatedDeltaTimes[CurrentSolveMeshGroup], GetWorld(), Context);

    AccumulatedDeltaTimes[CurrentSolveMeshGroup] = 0.0f;
    CurrentSolveMeshGroup = (CurrentSolveMeshGroup + 1) % MaxUpdateGroups; // Rotar grupos 0-4

    if (bEnableDetailedLogs && FrameCounter % 300 == 0) // Log cada 5 segundos aprox
    {
        UE_LOG(LogTemp, Log, TEXT("🎯 SolveMeshes: Grupo %d procesado, Frame %d"),
               CurrentSolveMeshGroup, FrameCounter);
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
            UE_LOG(LogTemp, Log, TEXT("🔄 ZombiSystemCoordinator: Posición jugador actualizada: %s"), *CachedPlayerLocation.ToString());
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

    // Auto-configurar TurboSequence Asset si no está configurado
    UTurboSequence_MeshAsset_Lf *TSAsset = LoadObject<UTurboSequence_MeshAsset_Lf>(nullptr, TEXT("/Game/Characters/Mannequins/TurboSequence/TS_Manny"));
    if (TSAsset)
    {
        SpawnerSubsystem->SetZombiTurboSequenceAsset(TSAsset);
        UE_LOG(LogTemp, Log, TEXT("✅ ZombiSystemCoordinator: TurboSequence Asset TS_Manny configurado automáticamente"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ ZombiSystemCoordinator: No se pudo cargar TS_Manny asset"));
        return;
    }

    // Forzar búsqueda del jugador y obtener posición actual
    FindPlayerPawn(); // Forzar búsqueda del jugador
    FVector PlayerLocation = GetCachedPlayerLocation(GetWorld()->GetTimeSeconds());
    FVector SpawnCenter = PlayerLocation.IsZero() ? FVector(0, 0, 100) : PlayerLocation;

    UE_LOG(LogTemp, Log, TEXT("🎯 ZombiSystemCoordinator: Centro de spawn: %s"), *SpawnCenter.ToString());

    // Spawn del lote
    SpawnerSubsystem->SpawnZombiBatch(Count, SpawnCenter, SpawnRadius);

    UE_LOG(LogTemp, Log, TEXT("🧪 ZombiSystemCoordinator: Auto-spawn solicitado - %d zombies en radio %.0f"), Count, SpawnRadius);
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
    // Lógica básica de estado - se expandirá en próximas iteraciones
    if (DistanceToPlayer < 100.0f)
    {
        return EZombiState::Attack;
    }
    else if (DistanceToPlayer < 300.0f)
    {
        return EZombiState::Chase;
    }
    else if (DistanceToPlayer < 800.0f)
    {
        return EZombiState::Seek;
    }
    else
    {
        return EZombiState::Idle;
    }
}

int32 UZombiSystemCoordinator::DetermineOptimalGroup(float DistanceToPlayer, EZombiState CurrentState) const
{
    // Mapeo distancia/estado → grupo TurboSequence
    if (DistanceToPlayer < 200.0f || CurrentState == EZombiState::Attack)
    {
        return 0; // Grupo 0 - alta calidad
    }
    else if (DistanceToPlayer < 500.0f || CurrentState == EZombiState::Chase)
    {
        return 1; // Grupo 1
    }
    else if (DistanceToPlayer < 800.0f)
    {
        return 2; // Grupo 2
    }
    else
    {
        return 3; // Grupo 3 - baja calidad
    }
}
