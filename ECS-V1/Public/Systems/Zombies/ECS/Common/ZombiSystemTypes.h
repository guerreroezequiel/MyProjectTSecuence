// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "Engine/Engine.h"
#include "ZombiSystemTypes.generated.h"

// Forward declarations para evitar dependencias complejas en header
struct FTurboSequence_MinimalMeshData_Lf;
struct FTurboSequence_AnimPlaySettings_Lf;
class UAnimSequence;

/**
 * Niveles LOD para discriminación temporal en el coordinador
 * Optimizado para juegos isométricos con 5000 entidades
 */
UENUM(BlueprintType)
enum class ELODLevel : uint8
{
    Critical UMETA(DisplayName = "Critical"), // ~100 entidades - cada frame (Attack, TakeDamage)
    High UMETA(DisplayName = "High"),         // ~400 entidades - cada 2 frames (Chase)
    Normal UMETA(DisplayName = "Normal"),     // ~1500 entidades - cada 4 frames (Seek, WalkAround)
    Low UMETA(DisplayName = "Low")            // ~3000 entidades - cada 12 frames (Idle, Dead)
};

/**
 * Tipos de operaciones TurboSequence para el "Big Loop"
 * Según documentación oficial: "update all instances at once, in a big loop"
 */
UENUM(BlueprintType)
enum class ETurboSequenceOpType : uint8
{
    Animation UMETA(DisplayName = "Animation"),    // PlayAnimation_Concurrent
    Transform UMETA(DisplayName = "Transform"),    // SetMeshWorldSpaceTransform_Concurrent
    GroupChange UMETA(DisplayName = "GroupChange") // AddInstanceToUpdateGroup_Concurrent
};

/**
 * Operación TurboSequence para recolección masiva
 * Implementa patrón "Big Loop" oficial
 */
USTRUCT(BlueprintType)
struct MYPROJECTTSECUENCE_API FTurboSequenceOperation
{
    GENERATED_BODY()

    // ID de la instancia mesh (evitamos dependencias complejas en header)
    UPROPERTY()
    int32 MeshInstanceID = -1;

    // Datos de animación (si aplica)
    UPROPERTY()
    UAnimSequence *Animation = nullptr;

    // Datos de configuración de animación (simple)
    UPROPERTY()
    float AnimationSpeed = 1.0f;

    UPROPERTY()
    bool bLoopAnimation = true;

    // Datos de transform (si aplica)
    UPROPERTY()
    FTransform Transform;

    // Datos de grupo (si aplica)
    UPROPERTY()
    int32 TargetGroup = -1;

    // Tipo de operación
    UPROPERTY()
    ETurboSequenceOpType OpType = ETurboSequenceOpType::Animation;

    // Flags de control
    UPROPERTY()
    bool bNeedsOperation = false;

    // Constructor por defecto
    FTurboSequenceOperation()
    {
        Animation = nullptr;
        TargetGroup = -1;
        OpType = ETurboSequenceOpType::Animation;
        bNeedsOperation = false;
    }

    // Constructor para operación de animación
    FTurboSequenceOperation(int32 InMeshInstanceID,
                            UAnimSequence *InAnimation,
                            float InAnimationSpeed = 1.0f,
                            bool bInLoopAnimation = true)
        : MeshInstanceID(InMeshInstanceID), Animation(InAnimation), AnimationSpeed(InAnimationSpeed), bLoopAnimation(bInLoopAnimation), OpType(ETurboSequenceOpType::Animation), bNeedsOperation(true)
    {
        TargetGroup = -1;
    }

    // Constructor para operación de transform
    FTurboSequenceOperation(int32 InMeshInstanceID,
                            const FTransform &InTransform)
        : MeshInstanceID(InMeshInstanceID), Transform(InTransform), OpType(ETurboSequenceOpType::Transform), bNeedsOperation(true)
    {
        Animation = nullptr;
        TargetGroup = -1;
    }

    // Constructor para operación de cambio de grupo
    FTurboSequenceOperation(int32 InMeshInstanceID,
                            int32 InTargetGroup)
        : MeshInstanceID(InMeshInstanceID), TargetGroup(InTargetGroup), OpType(ETurboSequenceOpType::GroupChange), bNeedsOperation(true)
    {
        Animation = nullptr;
    }

    // Validación
    bool IsValid() const
    {
        if (!bNeedsOperation)
            return false;
        if (MeshInstanceID < 0)
            return false;

        switch (OpType)
        {
        case ETurboSequenceOpType::Animation:
            return Animation != nullptr;
        case ETurboSequenceOpType::Transform:
            return true; // Transform siempre es válido
        case ETurboSequenceOpType::GroupChange:
            return TargetGroup >= 0;
        default:
            return false;
        }
    }
};

/**
 * Métricas de rendimiento para monitoreo del coordinador
 * Útil para debugging y optimización
 */
USTRUCT(BlueprintType)
struct MYPROJECTTSECUENCE_API FZombiSystemMetrics
{
    GENERATED_BODY()

    // Contadores por frame
    UPROPERTY(BlueprintReadOnly)
    int32 QueriesExecuted = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 EntitiesProcessed = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 TurboSequenceOperations = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 SolveMeshesCalls = 0;

    // Tiempos por frame (en milisegundos)
    UPROPERTY(BlueprintReadOnly)
    float ECSLoopTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float TurboSequenceLoopTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float TotalFrameTime = 0.0f;

    // Distribución por LOD
    UPROPERTY(BlueprintReadOnly)
    int32 CriticalEntities = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 HighEntities = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 NormalEntities = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 LowEntities = 0;

    // Reset para nuevo frame
    void Reset()
    {
        QueriesExecuted = 0;
        EntitiesProcessed = 0;
        TurboSequenceOperations = 0;
        SolveMeshesCalls = 0;
        ECSLoopTime = 0.0f;
        TurboSequenceLoopTime = 0.0f;
        TotalFrameTime = 0.0f;
        CriticalEntities = 0;
        HighEntities = 0;
        NormalEntities = 0;
        LowEntities = 0;
    }

    // Log de métricas para debugging
    void LogMetrics() const
    {
        UE_LOG(LogTemp, Log, TEXT("🎯 ZombiSystem Metrics:"));
        UE_LOG(LogTemp, Log, TEXT("  Queries: %d | Entities: %d | TS Operations: %d"),
               QueriesExecuted, EntitiesProcessed, TurboSequenceOperations);
        UE_LOG(LogTemp, Log, TEXT("  ECS: %.2fms | TS: %.2fms | Total: %.2fms"),
               ECSLoopTime, TurboSequenceLoopTime, TotalFrameTime);
        UE_LOG(LogTemp, Log, TEXT("  LOD Distribution - Critical: %d | High: %d | Normal: %d | Low: %d"),
               CriticalEntities, HighEntities, NormalEntities, LowEntities);
    }
};
