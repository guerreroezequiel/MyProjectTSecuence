#pragma once

#include "MassProcessor.h"
#include "MassEntityTypes.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiUpdateFrequencyFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Engine/Engine.h"
#include "ZombiPeriodicChaseProcessor.generated.h"

/**
 * @brief Procesador especializado para persecución periódica de zombies
 *
 * Este procesador maneja la lógica de persecución periódica:
 * - Cada 10 segundos, zombies a 100 unidades del jugador lo persiguen durante 5 segundos
 * - Se integra con el sistema de Update Frequency Control
 * - Optimizado para rendimiento con 1000+ entidades
 */
UCLASS()
class MYPROJECTTSECUENCE_API UZombiPeriodicChaseProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UZombiPeriodicChaseProcessor();

protected:
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    // Query para entidades que pueden participar en persecución periódica
    FMassEntityQuery PeriodicChaseQuery{*this};

    /**
     * @brief Obtiene la posición del jugador para cálculos de distancia
     * @return Posición del jugador en el mundo
     */
    FVector GetPlayerLocation() const;

    /**
     * @brief Procesa la lógica de persecución periódica para una entidad
     * @param CoreFragment Fragmento de datos centrales
     * @param BehaviorFragment Fragmento de comportamiento
     * @param UpdateFragment Fragmento de frecuencia de update
     * @param DeltaTime Tiempo delta para el frame
     * @param PlayerLocation Posición del jugador
     */
    void ProcessPeriodicChase(FZombiCoreFragment &CoreFragment,
                              FZombiBehaviorFragment &BehaviorFragment,
                              const FZombiUpdateFrequencyFragment &UpdateFragment,
                              float DeltaTime,
                              const FVector &PlayerLocation);

    /**
     * @brief Inicia la persecución periódica para una entidad
     * @param CoreFragment Fragmento de datos centrales
     * @param BehaviorFragment Fragmento de comportamiento
     * @param PlayerLocation Posición del jugador
     */
    void StartPeriodicChase(FZombiCoreFragment &CoreFragment,
                            FZombiBehaviorFragment &BehaviorFragment,
                            const FVector &PlayerLocation);

    /**
     * @brief Actualiza la persecución activa para una entidad
     * @param CoreFragment Fragmento de datos centrales
     * @param BehaviorFragment Fragmento de comportamiento
     * @param DeltaTime Tiempo delta para el frame
     * @param PlayerLocation Posición del jugador
     */
    void UpdateActiveChase(FZombiCoreFragment &CoreFragment,
                           FZombiBehaviorFragment &BehaviorFragment,
                           float DeltaTime,
                           const FVector &PlayerLocation);

    /**
     * @brief Termina la persecución periódica para una entidad
     * @param CoreFragment Fragmento de datos centrales
     * @param BehaviorFragment Fragmento de comportamiento
     */
    void EndPeriodicChase(FZombiCoreFragment &CoreFragment,
                          FZombiBehaviorFragment &BehaviorFragment);

    /**
     * @brief Calcula la dirección hacia el jugador
     * @param ZombiePosition Posición del zombie
     * @param PlayerLocation Posición del jugador
     * @return Vector de dirección normalizado hacia el jugador
     */
    FVector CalculateDirectionToPlayer(const FVector &ZombiePosition, const FVector &PlayerLocation) const;
};