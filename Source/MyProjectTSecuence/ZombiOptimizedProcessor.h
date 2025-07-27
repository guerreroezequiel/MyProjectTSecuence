#pragma once

#include "MassProcessor.h"
#include "MassEntityTypes.h"
#include "ZombiCoreFragment.h"
#include "ZombiBehaviorFragment.h"
#include "ZombiUpdateFrequencyFragment.h"
#include "ZombiTags.h"
#include "Engine/Engine.h"
#include "ZombiOptimizedProcessor.generated.h"

/**
 * @brief Procesador optimizado para 1000+ zombies con Update Frequency Control y Batch Processing
 *
 * Este procesador implementa:
 * 1. Update Frequency Control: Diferentes frecuencias de update basadas en distancia
 * 2. Batch Processing: Procesamiento en lotes de 250 entidades
 * 3. Spatial Optimization: Solo procesar entidades dentro de 1500 unidades
 * 4. Priority-Based Processing: Diferentes niveles de detalle según prioridad
 *
 * Optimizado para cámara isométrica fija con distancia máxima de 1500 unidades
 */
UCLASS()
class MYPROJECTTSECUENCE_API UZombiOptimizedProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UZombiOptimizedProcessor();

protected:
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    // Query principal para todas las entidades con frecuencia de update
    FMassEntityQuery OptimizedQuery{*this};

    // Timers para controlar diferentes frecuencias de update
    float LastCriticalUpdate = 0.0f;
    float LastHighUpdate = 0.0f;
    float LastNormalUpdate = 0.0f;
    float LastLowUpdate = 0.0f;

    // Tamaño de lotes para procesamiento optimizado
    static constexpr int32 BATCH_SIZE = 250;

    // Distancias de optimización para cámara isométrica fija
    static constexpr float CRITICAL_DISTANCE = 300.0f; // 60 FPS
    static constexpr float HIGH_DISTANCE = 600.0f;     // 30 FPS
    static constexpr float NORMAL_DISTANCE = 1000.0f;  // 15 FPS
    static constexpr float LOW_DISTANCE = 1500.0f;     // 5 FPS

    // Frecuencias de update
    static constexpr float CRITICAL_FREQUENCY = 1.0f / 60.0f; // 60 FPS
    static constexpr float HIGH_FREQUENCY = 1.0f / 30.0f;     // 30 FPS
    static constexpr float NORMAL_FREQUENCY = 1.0f / 15.0f;   // 15 FPS
    static constexpr float LOW_FREQUENCY = 1.0f / 5.0f;       // 5 FPS

    /**
     * @brief Obtiene la posición del jugador para cálculos de distancia
     * @return Posición del jugador en el mundo
     */
    FVector GetPlayerLocation() const;

    /**
     * @brief Procesa zombies con prioridad crítica (60 FPS)
     * @param EntityManager Manager de entidades Mass
     * @param Context Contexto de ejecución
     * @param CurrentTime Tiempo actual del juego
     */
    void ProcessCriticalZombies(FMassEntityManager &EntityManager, FMassExecutionContext &Context, float CurrentTime);

    /**
     * @brief Procesa zombies con prioridad alta (30 FPS)
     * @param EntityManager Manager de entidades Mass
     * @param Context Contexto de ejecución
     * @param CurrentTime Tiempo actual del juego
     */
    void ProcessHighZombies(FMassEntityManager &EntityManager, FMassExecutionContext &Context, float CurrentTime);

    /**
     * @brief Procesa zombies con prioridad normal (15 FPS)
     * @param EntityManager Manager de entidades Mass
     * @param Context Contexto de ejecución
     * @param CurrentTime Tiempo actual del juego
     */
    void ProcessNormalZombies(FMassEntityManager &EntityManager, FMassExecutionContext &Context, float CurrentTime);

    /**
     * @brief Procesa zombies con prioridad baja (5 FPS)
     * @param EntityManager Manager de entidades Mass
     * @param Context Contexto de ejecución
     * @param CurrentTime Tiempo actual del juego
     */
    void ProcessLowZombies(FMassEntityManager &EntityManager, FMassExecutionContext &Context, float CurrentTime);

    /**
     * @brief Procesa movimiento completo para zombies críticos
     * @param CoreFragment Fragmento de datos centrales
     * @param BehaviorFragment Fragmento de comportamiento
     * @param UpdateFragment Fragmento de frecuencia de update
     * @param DeltaTime Tiempo delta para el frame
     */
    void ProcessFullMovement(FZombiCoreFragment &CoreFragment, const FZombiBehaviorFragment &BehaviorFragment,
                             FZombiUpdateFrequencyFragment &UpdateFragment, float DeltaTime);

    /**
     * @brief Procesa movimiento básico para zombies de prioridad alta
     * @param CoreFragment Fragmento de datos centrales
     * @param BehaviorFragment Fragmento de comportamiento
     * @param UpdateFragment Fragmento de frecuencia de update
     * @param DeltaTime Tiempo delta para el frame
     */
    void ProcessBasicMovement(FZombiCoreFragment &CoreFragment, const FZombiBehaviorFragment &BehaviorFragment,
                              FZombiUpdateFrequencyFragment &UpdateFragment, float DeltaTime);

    /**
     * @brief Procesa movimiento simple para zombies de prioridad normal
     * @param CoreFragment Fragmento de datos centrales
     * @param BehaviorFragment Fragmento de comportamiento
     * @param UpdateFragment Fragmento de frecuencia de update
     * @param DeltaTime Tiempo delta para el frame
     */
    void ProcessSimpleMovement(FZombiCoreFragment &CoreFragment, const FZombiBehaviorFragment &BehaviorFragment,
                               FZombiUpdateFrequencyFragment &UpdateFragment, float DeltaTime);

    /**
     * @brief Procesa movimiento mínimo para zombies de prioridad baja
     * @param CoreFragment Fragmento de datos centrales
     * @param BehaviorFragment Fragmento de comportamiento
     * @param UpdateFragment Fragmento de frecuencia de update
     * @param DeltaTime Tiempo delta para el frame
     */
    void ProcessMinimalMovement(FZombiCoreFragment &CoreFragment, const FZombiBehaviorFragment &BehaviorFragment,
                                FZombiUpdateFrequencyFragment &UpdateFragment, float DeltaTime);

    /**
     * @brief Genera dirección aleatoria para movimiento
     * @return Vector de dirección aleatoria normalizado
     */
    FVector GenerateRandomDirection() const;

    /**
     * @brief Confina la posición dentro del área de movimiento
     * @param Position Posición actual
     * @param Center Centro del área de movimiento
     * @param Radius Radio del área de movimiento
     * @return Posición confinada
     */
    FVector ClampToMovementArea(const FVector &Position, const FVector &Center, float Radius) const;

    /**
     * @brief Verifica si una posición está dentro del radio de movimiento
     * @param Position Posición a verificar
     * @param Center Centro del área de movimiento
     * @param Radius Radio del área de movimiento
     * @return true si está dentro del área
     */
    bool IsWithinMovementRadius(const FVector &Position, const FVector &Center, float Radius) const;
};