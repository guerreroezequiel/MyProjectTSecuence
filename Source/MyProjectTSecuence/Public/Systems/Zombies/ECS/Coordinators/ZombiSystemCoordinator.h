// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "Systems/Zombies/ECS/Common/ZombiSystemTypes.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTurboSequenceFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiLODFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "TurboSequence_Manager_Lf.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "ZombiSystemCoordinator.generated.h"

/**
 * 🚀 COORDINADOR PRINCIPAL OPTIMIZADO
 *
 * Implementa arquitectura híbrida que respeta 100% principios TurboSequence:
 * - Separación TOTAL ECS ↔ TurboSequence loops
 * - Patrón "Big Loop" oficial
 * - Una sola llamada SolveMeshes por grupo por frame
 * - LOD discriminativo temporal para escalabilidad a 5000 entidades
 *
 * SOLUCIONA PROBLEMAS CRÍTICOS:
 * ❌ 16 queries por frame → ✅ 4 queries máximo con LOD
 * ❌ 2 llamadas SolveMeshes → ✅ 1 llamada rotativa por frame
 * ❌ 3000 comandos diferidos → ✅ 0 comandos diferidos
 * ❌ Mezcla ECS-TS → ✅ Separación total de responsabilidades
 */
UCLASS(BlueprintType, Blueprintable)
class MYPROJECTTSECUENCE_API UZombiSystemCoordinator : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    UZombiSystemCoordinator();

    // USubsystem interface
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;

    // FTickableGameObject interface
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return bSystemActive; }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UZombiSystemCoordinator, STATGROUP_Tickables); }
    virtual UWorld *GetTickableGameObjectWorld() const override { return GetWorld(); }

    // Control del sistema
    UFUNCTION(BlueprintCallable, Category = "Zombi System")
    void StartSystem();

    UFUNCTION(BlueprintCallable, Category = "Zombi System")
    void StopSystem();

    UFUNCTION(BlueprintCallable, Category = "Zombi System")
    bool IsSystemActive() const { return bSystemActive; }

    // Métricas de rendimiento
    UFUNCTION(BlueprintCallable, Category = "Zombi System")
    FZombiSystemMetrics GetCurrentMetrics() const { return CurrentMetrics; }

    UFUNCTION(BlueprintCallable, Category = "Zombi System")
    void LogPerformanceMetrics() const { CurrentMetrics.LogMetrics(); }

    /**
     * 🧪 TESTING: Auto-spawn zombies para pruebas de performance
     * Spawna entidades automáticamente para testing sin necesidad de controller
     */
    UFUNCTION(BlueprintCallable, Category = "Zombi System")
    void AutoSpawnZombiesForTesting(int32 Count = 500, float SpawnRadius = 2000.0f);

protected:
    // ===========================
    // FASE 1: ECS "BIG LOOP" PURO
    // ===========================

    /**
     * Loop ECS principal - SOLO lógica ECS, SIN llamadas TurboSequence
     * Implementa discriminación LOD temporal para escalabilidad
     */
    void ExecuteECSBigLoop(float DeltaTime);

    /**
     * Ejecuta ECS unificado para un nivel LOD específico
     * UNA query unificada vs 4 queries separadas por procesador
     */
    void ExecuteECSForLOD(ELODLevel TargetLOD, float DeltaTime);

    /**
     * Crea query unificada optimizada para cache locality
     * Incluye todos los fragments necesarios en una sola pasada
     */
    FMassEntityQuery CreateUnifiedQueryForLOD(ELODLevel TargetLOD);

    /**
     * Procesa comportamiento inline para máximo rendimiento
     * Reemplaza ZombiBehaviorProcessor separado
     */
    void ProcessBehaviorInline(FMassExecutionContext &Context, ELODLevel LOD, float DeltaTime);

    /**
     * Procesa movimiento inline para máximo rendimiento
     * Reemplaza ZombiMovementProcessor separado
     */
    void ProcessMovementInline(FMassExecutionContext &Context, ELODLevel LOD, float DeltaTime);

    /**
     * Procesa LOD inline SIN comandos diferidos
     * Reemplaza ZombiLODProcessor problemático
     */
    void ProcessLODInline(FMassExecutionContext &Context, ELODLevel LOD, float DeltaTime);

    /**
     * Marca operaciones TurboSequence pendientes SIN ejecutarlas
     * Recolección para el "Big Loop" posterior
     */
    void MarkTurboSequenceOperations(FMassExecutionContext &Context, ELODLevel LOD);

    // ===============================
    // FASE 2: TURBOSEQUENCE "BIG LOOP" PURO
    // ===============================

    /**
     * Loop TurboSequence principal - SOLO operaciones visuales, SIN ECS
     * Implementa patrón "Big Loop" oficial de documentación
     */
    void ExecuteTurboSequenceBigLoop(float DeltaTime);

    /**
     * Recolecta TODAS las operaciones TurboSequence pendientes
     * Patrón "update all instances at once, in a big loop"
     */
    void CollectAllTurboSequenceOperations(TArray<FTurboSequenceOperation> &AllOperations);

    /**
     * Aplica TODAS las operaciones masivamente según tipo
     * Patrón "update all at once" de documentación oficial
     */
    void ApplyAllTurboSequenceOperations(const TArray<FTurboSequenceOperation> &AllOperations);

    /**
     * UNA sola llamada SolveMeshes rotativa por frame
     * Patrón oficial: "call SolveMeshes_GameThread one time per update group, one time a frame"
     */
    void ExecuteSolveMeshesCorrect(float DeltaTime);

    // ===============================
    // DISCRIMINACIÓN LOD TEMPORAL
    // ===============================

    /**
     * Determina si debe ejecutar LOD crítico (cada frame)
     * ~100 entidades: Attack, TakeDamage
     */
    bool ShouldExecuteCritical(float DeltaTime) const { return true; }

    /**
     * Determina si debe ejecutar LOD alto (cada 2 frames)
     * ~400 entidades: Chase
     */
    bool ShouldExecuteHigh(float DeltaTime) const { return FrameCounter % 2 == 0; }

    /**
     * Determina si debe ejecutar LOD normal (cada 4 frames)
     * ~1500 entidades: Seek, WalkAround
     */
    bool ShouldExecuteNormal(float DeltaTime) const { return FrameCounter % 4 == 0; }

    /**
     * Determina si debe ejecutar LOD bajo (cada 12 frames)
     * ~3000 entidades: Idle, Dead
     */
    bool ShouldExecuteLow(float DeltaTime) const { return FrameCounter % 12 == 0; }

    // ===============================
    // CACHE Y OPTIMIZACIONES
    // ===============================

    /**
     * Actualiza estado solo si cambió realmente
     * Evita 3000 comandos diferidos innecesarios por frame
     */
    void UpdateStateIfChanged(FMassEntityHandle Entity, EZombiState NewState);

    /**
     * Actualiza grupo TurboSequence solo si cambió
     * Evita 500 llamadas AddInstanceToUpdateGroup innecesarias
     */
    void UpdateGroupIfChanged(FMassEntityHandle Entity, int32 NewGroup, const FTurboSequence_MinimalMeshData_Lf &MeshData);

    /**
     * Obtiene ubicación del jugador cached
     * Evita 1500 cálculos redundantes por frame
     */
    FVector GetCachedPlayerLocation(float CurrentTime);

    /**
     * Sincroniza estado a frecuencia LOD sin comandos diferidos
     * Reemplaza lógica problemática de ZombiLODProcessor
     */
    void SynchronizeStateToFrequency(FMassEntityHandle Entity, EZombiState NewState);

private:
    // ===============================
    // REFERENCIAS DEL SISTEMA
    // ===============================

    UPROPERTY()
    UMassEntitySubsystem *MassEntitySubsystem = nullptr;

    UPROPERTY()
    APawn *PlayerPawn = nullptr;

    // ===============================
    // CACHE DE OPTIMIZACIÓN
    // ===============================

    // Cache de estados para evitar comandos diferidos innecesarios
    TMap<FMassEntityHandle, EZombiState> PreviousStates;

    // Cache de grupos para evitar llamadas AddInstanceToUpdateGroup innecesarias
    TMap<FMassEntityHandle, int32> PreviousGroups;

    // Cache de ubicación del jugador
    FVector CachedPlayerLocation = FVector::ZeroVector;
    float LastPlayerLocationUpdate = 0.0f;
    const float PlayerLocationCacheTime = 0.1f; // 100ms cache

    // ===============================
    // ESTADO DEL SISTEMA
    // ===============================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System Control", meta = (AllowPrivateAccess = "true"))
    bool bSystemActive = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System Control", meta = (AllowPrivateAccess = "true"))
    bool bEnableDetailedLogs = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System Control", meta = (AllowPrivateAccess = "true"))
    bool bEnablePerformanceMetrics = true;

    // Contador de frames para discriminación LOD
    int32 FrameCounter = 0;

    // Métricas de rendimiento
    mutable FZombiSystemMetrics CurrentMetrics;

    // Timer para logs periódicos
    float LogTimer = 0.0f;
    const float LogInterval = 5.0f; // Log cada 5 segundos

    // ===============================
    // SOLVEMESHES ROTATIVO
    // ===============================

    // Estado para SolveMeshes rotativo
    int32 CurrentSolveMeshGroup = 0;
    TArray<float> AccumulatedDeltaTimes;
    const int32 MaxUpdateGroups = 5; // Grupos 0-4

    // ===============================
    // INICIALIZACIÓN
    // ===============================

    bool bSystemInitialized = false;

    void InitializeSystem();
    void InitializeSolveMeshGroups();
    void FindPlayerPawn();

    // ===============================
    // UTILIDADES
    // ===============================

    float CalculateDistanceToPlayer(const FVector &ZombieLocation) const;
    EZombiState DetermineOptimalState(const FZombiCoreFragment &CoreFragment, const FZombiBehaviorFragment &BehaviorFragment, float DistanceToPlayer) const;
    int32 DetermineOptimalGroup(float DistanceToPlayer, EZombiState CurrentState) const;
};
