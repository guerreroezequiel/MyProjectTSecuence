#include "Systems/Zombies/ECS/Processors/ZombiPeriodicChaseProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiUpdateFrequencyFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "MyProjectTSecuence/MyProjectTSecuenceCharacter.h"
#include "Math/UnrealMathUtility.h"

/**
 * @brief Constructor del procesador de persecución periódica
 *
 * Configura el procesador para ejecutarse en la fase PrePhysics
 * y registrarse automáticamente con el sistema Mass Entity
 */
UZombiPeriodicChaseProcessor::UZombiPeriodicChaseProcessor()
{
    // Configurar para ejecutarse en la fase PrePhysics (antes de la física)
    ProcessingPhase = EMassProcessingPhase::PrePhysics;

    // Ejecutar en el grupo "MassBehavior" para mantener orden con otros procesadores
    ExecutionOrder.ExecuteInGroup = TEXT("MassBehavior");

    // Configuración para optimización de rendimiento
    bRequiresGameThreadExecution = false;     // Permitir ejecución en threads paralelos
    bAutoRegisterWithProcessingPhases = true; // Registro automático
}

/**
 * @brief Configura las queries para el procesador de persecución periódica
 *
 * Define qué fragmentos y tags necesita el procesador para funcionar
 */
void UZombiPeriodicChaseProcessor::ConfigureQueries()
{
    // Query para entidades que pueden participar en persecución periódica
    // Incluye todos los fragmentos necesarios para el procesamiento
    PeriodicChaseQuery.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadWrite);
    PeriodicChaseQuery.AddRequirement<FZombiBehaviorFragment>(EMassFragmentAccess::ReadWrite);
    PeriodicChaseQuery.AddRequirement<FZombiUpdateFrequencyFragment>(EMassFragmentAccess::ReadOnly);

    // Solo procesar entidades activas y no muertas
    PeriodicChaseQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    PeriodicChaseQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);
}

/**
 * @brief Ejecuta el procesador de persecución periódica
 *
 * Este método implementa la lógica principal de persecución periódica:
 * 1. Actualiza timers de persecución periódica
 * 2. Verifica si es tiempo de iniciar persecución
 * 3. Maneja persecuciones activas
 * 4. Termina persecuciones cuando expiran
 */
void UZombiPeriodicChaseProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        return;
    }

    const float DeltaTime = Context.GetDeltaTimeSeconds();
    const FVector PlayerLocation = GetPlayerLocation();

    // Procesar entidades para persecución periódica
    int32 TotalEntitiesProcessed = 0;
    int32 EntitiesInRange = 0;
    int32 TotalChunks = 0;

    PeriodicChaseQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime, PlayerLocation, &TotalEntitiesProcessed, &EntitiesInRange, &TotalChunks](FMassExecutionContext &Context)
                                          {
        TotalChunks++;
        TArrayView<FZombiCoreFragment> CoreFragments = Context.GetMutableFragmentView<FZombiCoreFragment>();
        TArrayView<FZombiBehaviorFragment> BehaviorFragments = Context.GetMutableFragmentView<FZombiBehaviorFragment>();
        TArrayView<const FZombiUpdateFrequencyFragment> UpdateFragments = Context.GetFragmentView<FZombiUpdateFrequencyFragment>();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiCoreFragment& CoreFragment = CoreFragments[i];
            FZombiBehaviorFragment& BehaviorFragment = BehaviorFragments[i];
            const FZombiUpdateFrequencyFragment& UpdateFragment = UpdateFragments[i];

            // Solo procesar si no está muerto
            if (BehaviorFragment.IsDead())
            {
                continue;
            }

            TotalEntitiesProcessed++;
            
            // Calcular distancia al jugador para debug
            float DistanceToPlayer = FVector::Dist(CoreFragment.Position, PlayerLocation);
            if (DistanceToPlayer <= BehaviorFragment.PeriodicChaseDistance)
            {
                EntitiesInRange++;
            }
            
            // Log de distancia real cada 5 segundos
            static float DistanceDebugTimer = 0.0f;
            DistanceDebugTimer += DeltaTime;
            if (DistanceDebugTimer >= 5.0f)
            {
                UE_LOG(LogTemp, Warning, TEXT("🎯 Distancia Debug - Zombi en: %s, Jugador en: %s, Distancia: %.1f, Rango requerido: %.1f"), 
                       *CoreFragment.Position.ToString(), *PlayerLocation.ToString(), DistanceToPlayer, BehaviorFragment.PeriodicChaseDistance);
                DistanceDebugTimer = 0.0f;
            }

            // Procesar persecución periódica
            ProcessPeriodicChase(CoreFragment, BehaviorFragment, UpdateFragment, DeltaTime, PlayerLocation);
        } });

    // Log de debug cada 2 segundos
    static float DebugTimer = 0.0f;
    DebugTimer += DeltaTime;
    if (DebugTimer >= 2.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎯 PeriodicChase Debug - Chunks: %d, Entidades procesadas: %d, En rango (500u): %d, PlayerPos: %s"),
               TotalChunks, TotalEntitiesProcessed, EntitiesInRange, *PlayerLocation.ToString());
        DebugTimer = 0.0f;
    }
}

/**
 * @brief Obtiene la posición del jugador para cálculos de distancia
 *
 * Busca el personaje del jugador en el mundo y retorna su posición
 * Si no encuentra al jugador, retorna el origen (0,0,0)
 */
FVector UZombiPeriodicChaseProcessor::GetPlayerLocation() const
{
    // Buscar el personaje del jugador en el mundo
    AMyProjectTSecuenceCharacter *PlayerCharacter = Cast<AMyProjectTSecuenceCharacter>(
        UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

    if (PlayerCharacter)
    {
        FVector PlayerPos = PlayerCharacter->GetActorLocation();

        // Log de debug cada 10 segundos para verificar que encuentra al jugador
        static float PlayerDebugTimer = 0.0f;
        PlayerDebugTimer += GetWorld()->GetDeltaSeconds();
        if (PlayerDebugTimer >= 10.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("🎮 Player Debug - Jugador encontrado en: %s"), *PlayerPos.ToString());
            PlayerDebugTimer = 0.0f;
        }

        return PlayerPos;
    }

    // Si no encuentra al jugador, log de error
    UE_LOG(LogTemp, Error, TEXT("🎮 Player Debug - NO se encontró al jugador!"));
    return FVector::ZeroVector;
}

/**
 * @brief Procesa la lógica de persecución periódica para una entidad
 *
 * Maneja toda la lógica de persecución periódica:
 * - Actualiza timers
 * - Verifica condiciones para iniciar persecución
 * - Maneja persecuciones activas
 * - Termina persecuciones cuando expiran
 */
void UZombiPeriodicChaseProcessor::ProcessPeriodicChase(FZombiCoreFragment &CoreFragment,
                                                        FZombiBehaviorFragment &BehaviorFragment,
                                                        const FZombiUpdateFrequencyFragment &UpdateFragment,
                                                        float DeltaTime,
                                                        const FVector &PlayerLocation)
{
    // Calcular distancia al jugador
    float DistanceToPlayer = FVector::Dist(CoreFragment.Position, PlayerLocation);

    // Si hay una persecución activa, actualizarla
    if (BehaviorFragment.IsPeriodicChaseActive())
    {
        // Actualizar timer de duración de persecución
        BehaviorFragment.ChaseDurationTimer -= DeltaTime;

        // Si la persecución ha expirado, terminarla
        if (BehaviorFragment.ChaseDurationTimer <= 0.0f)
        {
            EndPeriodicChase(CoreFragment, BehaviorFragment);
        }
        else
        {
            // Actualizar persecución activa
            UpdateActiveChase(CoreFragment, BehaviorFragment, DeltaTime, PlayerLocation);
        }
    }
    else
    {
        // No hay persecución activa, verificar si es tiempo de iniciar una nueva
        // Actualizar timer de persecución periódica
        BehaviorFragment.ChasePeriodicTimer += DeltaTime;

        // Debug: Log cada 5 segundos para entidades en rango
        static float DebugTimer = 0.0f;
        DebugTimer += DeltaTime;
        if (DebugTimer >= 5.0f && DistanceToPlayer <= BehaviorFragment.PeriodicChaseDistance)
        {
            UE_LOG(LogTemp, Warning, TEXT("🎯 Debug Chase - Distancia: %.1f, Timer: %.1f/%.1f, EnRango: %s, EsTiempo: %s"),
                   DistanceToPlayer, BehaviorFragment.ChasePeriodicTimer, BehaviorFragment.PeriodicChaseInterval,
                   BehaviorFragment.IsInPeriodicChaseDistance(DistanceToPlayer) ? TEXT("SI") : TEXT("NO"),
                   BehaviorFragment.IsPeriodicChaseTime() ? TEXT("SI") : TEXT("NO"));
            DebugTimer = 0.0f;
        }

        // Verificar si es tiempo de persecución periódica y está en rango
        if (BehaviorFragment.IsPeriodicChaseTime() &&
            BehaviorFragment.IsInPeriodicChaseDistance(DistanceToPlayer))
        {
            // Iniciar persecución periódica
            StartPeriodicChase(CoreFragment, BehaviorFragment, PlayerLocation);

            // Resetear timer de persecución periódica
            BehaviorFragment.ResetPeriodicChaseTimer();
        }
    }
}

/**
 * @brief Inicia la persecución periódica para una entidad
 *
 * Configura la entidad para perseguir al jugador durante 5 segundos
 */
void UZombiPeriodicChaseProcessor::StartPeriodicChase(FZombiCoreFragment &CoreFragment,
                                                      FZombiBehaviorFragment &BehaviorFragment,
                                                      const FVector &PlayerLocation)
{
    // Cambiar estado a persecución
    BehaviorFragment.SetState(EZombiState::Chase);

    // Iniciar timer de duración de persecución
    BehaviorFragment.StartPeriodicChase();

    // Calcular dirección hacia el jugador
    FVector DirectionToPlayer = CalculateDirectionToPlayer(CoreFragment.Position, PlayerLocation);

    // Configurar movimiento hacia el jugador
    CoreFragment.MovementDirection = DirectionToPlayer;
    CoreFragment.MovementSpeed = BehaviorFragment.ChaseSpeed; // Usar velocidad de persecución

    // Configurar rotación hacia el jugador
    CoreFragment.Rotation = DirectionToPlayer.Rotation();

    // Log de debug (opcional)
    UE_LOG(LogTemp, Log, TEXT("🎯 Zombi inició persecución periódica - Posición: %s, Dirección: %s"),
           *CoreFragment.Position.ToString(), *DirectionToPlayer.ToString());
}

/**
 * @brief Actualiza la persecución activa para una entidad
 *
 * Mantiene al zombie persiguiendo al jugador durante la persecución activa
 */
void UZombiPeriodicChaseProcessor::UpdateActiveChase(FZombiCoreFragment &CoreFragment,
                                                     FZombiBehaviorFragment &BehaviorFragment,
                                                     float DeltaTime,
                                                     const FVector &PlayerLocation)
{
    // Calcular nueva dirección hacia el jugador
    FVector DirectionToPlayer = CalculateDirectionToPlayer(CoreFragment.Position, PlayerLocation);

    // Actualizar dirección de movimiento
    CoreFragment.MovementDirection = DirectionToPlayer;

    // Mantener velocidad de persecución
    CoreFragment.MovementSpeed = BehaviorFragment.ChaseSpeed;

    // Actualizar rotación hacia el jugador (rotación suave)
    FRotator TargetRotation = DirectionToPlayer.Rotation();
    CoreFragment.Rotation = FMath::RInterpTo(
        CoreFragment.Rotation,
        TargetRotation,
        DeltaTime,
        CoreFragment.RotationSpeed / 180.0f);

    // Mover hacia el jugador
    if (CoreFragment.MovementSpeed > 0.0f)
    {
        FVector ForwardDirection = CoreFragment.Rotation.Vector();
        CoreFragment.Position += ForwardDirection * CoreFragment.MovementSpeed * DeltaTime;
    }
}

/**
 * @brief Termina la persecución periódica para una entidad
 *
 * Restaura el comportamiento normal del zombie después de la persecución
 */
void UZombiPeriodicChaseProcessor::EndPeriodicChase(FZombiCoreFragment &CoreFragment,
                                                    FZombiBehaviorFragment &BehaviorFragment)
{
    // Cambiar estado de vuelta a caminar
    BehaviorFragment.SetState(EZombiState::WalkAround);

    // Resetear timer de duración de persecución
    BehaviorFragment.ChaseDurationTimer = 0.0f;

    // Generar nueva dirección aleatoria para movimiento normal
    float RandomAngle = FMath::RandRange(0.0f, 360.0f);
    FVector RandomDirection = FVector(
        FMath::Cos(FMath::DegreesToRadians(RandomAngle)),
        FMath::Sin(FMath::DegreesToRadians(RandomAngle)),
        0.0f);

    CoreFragment.MovementDirection = RandomDirection.GetSafeNormal();
    CoreFragment.MovementSpeed = 50.0f; // Velocidad normal de caminar

    // Log de debug (opcional)
    UE_LOG(LogTemp, Log, TEXT("🎯 Zombi terminó persecución periódica - Nueva dirección: %s"),
           *CoreFragment.MovementDirection.ToString());
}

/**
 * @brief Calcula la dirección hacia el jugador
 *
 * Retorna un vector normalizado que apunta desde el zombie hacia el jugador
 */
FVector UZombiPeriodicChaseProcessor::CalculateDirectionToPlayer(const FVector &ZombiePosition, const FVector &PlayerLocation) const
{
    // Calcular vector de dirección hacia el jugador
    FVector Direction = PlayerLocation - ZombiePosition;

    // Ignorar componente Y (altura) para movimiento en plano XZ
    Direction.Z = 0.0f;

    // Normalizar y retornar
    return Direction.GetSafeNormal();
}