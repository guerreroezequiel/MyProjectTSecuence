#include "Systems/Zombies/ECS/Processors/ZombiOptimizedProcessor.h"
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
 * @brief Constructor del procesador optimizado
 *
 * Configura el procesador para ejecutarse en la fase PrePhysics
 * y registrarse automáticamente con el sistema Mass Entity
 */
UZombiOptimizedProcessor::UZombiOptimizedProcessor()
{
    // Configurar para ejecutarse en la fase PrePhysics (antes de la física)
    ProcessingPhase = EMassProcessingPhase::PrePhysics;

    // Ejecutar en el grupo "MassBehavior" para mantener orden con otros procesadores
    ExecutionOrder.ExecuteInGroup = TEXT("MassBehavior");

    // Configuración para optimización de rendimiento
    bRequiresGameThreadExecution = false;     // Permitir ejecución en threads paralelos
    bAutoRegisterWithProcessingPhases = true; // Registro automático

    // Inicializar timers de update
    LastCriticalUpdate = 0.0f;
    LastHighUpdate = 0.0f;
    LastNormalUpdate = 0.0f;
    LastLowUpdate = 0.0f;
}

/**
 * @brief Configura las queries para el procesador optimizado
 *
 * Define qué fragmentos y tags necesita el procesador para funcionar
 */
void UZombiOptimizedProcessor::ConfigureQueries()
{
    // Query principal para todas las entidades con frecuencia de update
    // Incluye todos los fragmentos necesarios para el procesamiento optimizado
    OptimizedQuery.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadWrite);
    OptimizedQuery.AddRequirement<FZombiBehaviorFragment>(EMassFragmentAccess::ReadOnly);
    OptimizedQuery.AddRequirement<FZombiUpdateFrequencyFragment>(EMassFragmentAccess::ReadWrite);

    // Solo procesar entidades activas y no muertas
    OptimizedQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    OptimizedQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);
}

/**
 * @brief Ejecuta el procesador optimizado con Update Frequency Control y Batch Processing
 *
 * Este método implementa la lógica principal de optimización:
 * 1. Calcula distancias al jugador para todas las entidades
 * 2. Actualiza frecuencias de update basadas en distancia
 * 3. Procesa entidades según su prioridad y frecuencia
 * 4. Aplica Batch Processing para mejor rendimiento
 */
void UZombiOptimizedProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        return;
    }

    const float CurrentTime = GetWorld()->GetTimeSeconds();
    const float DeltaTime = Context.GetDeltaTimeSeconds();
    const FVector PlayerLocation = GetPlayerLocation();

    // PASO 1: Actualizar información de frecuencia para todas las entidades
    // Esto se hace cada frame para mantener distancias actualizadas
    OptimizedQuery.ForEachEntityChunk(EntityManager, Context, [this, CurrentTime, PlayerLocation](FMassExecutionContext &Context)
                                      {
        TArrayView<FZombiCoreFragment> CoreFragments = Context.GetMutableFragmentView<FZombiCoreFragment>();
        TArrayView<FZombiUpdateFrequencyFragment> UpdateFragments = Context.GetMutableFragmentView<FZombiUpdateFrequencyFragment>();

        // Procesar en lotes de BATCH_SIZE para optimizar cache locality
        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiCoreFragment& CoreFragment = CoreFragments[i];
            FZombiUpdateFrequencyFragment& UpdateFragment = UpdateFragments[i];

            // Calcular distancia al jugador
            float DistanceToPlayer = FVector::Dist(CoreFragment.Position, PlayerLocation);
            
            // Actualizar información de frecuencia basada en la nueva distancia
            UpdateFragment.UpdateFrequencyInfo(DistanceToPlayer, CurrentTime);
        } });

    // PASO 2: Procesar entidades según su prioridad y frecuencia de update
    // Cada prioridad se procesa a su propia frecuencia para optimizar rendimiento

    // Procesar zombies críticos (60 FPS) - muy cercanos al jugador
    if (CurrentTime - LastCriticalUpdate >= CRITICAL_FREQUENCY)
    {
        ProcessCriticalZombies(EntityManager, Context, CurrentTime);
        LastCriticalUpdate = CurrentTime;
    }

    // Procesar zombies de alta prioridad (30 FPS) - visibles
    if (CurrentTime - LastHighUpdate >= HIGH_FREQUENCY)
    {
        ProcessHighZombies(EntityManager, Context, CurrentTime);
        LastHighUpdate = CurrentTime;
    }

    // Procesar zombies de prioridad normal (15 FPS) - de fondo
    if (CurrentTime - LastNormalUpdate >= NORMAL_FREQUENCY)
    {
        ProcessNormalZombies(EntityManager, Context, CurrentTime);
        LastNormalUpdate = CurrentTime;
    }

    // Procesar zombies de baja prioridad (5 FPS) - lejanos
    if (CurrentTime - LastLowUpdate >= LOW_FREQUENCY)
    {
        ProcessLowZombies(EntityManager, Context, CurrentTime);
        LastLowUpdate = CurrentTime;
    }
}

/**
 * @brief Obtiene la posición del jugador para cálculos de distancia
 *
 * Busca el personaje del jugador en el mundo y retorna su posición
 * Si no encuentra al jugador, retorna el origen (0,0,0)
 */
FVector UZombiOptimizedProcessor::GetPlayerLocation() const
{
    // Buscar el personaje del jugador en el mundo
    AMyProjectTSecuenceCharacter *PlayerCharacter = Cast<AMyProjectTSecuenceCharacter>(
        UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

    if (PlayerCharacter)
    {
        return PlayerCharacter->GetActorLocation();
    }

    // Si no encuentra al jugador, retornar origen
    return FVector::ZeroVector;
}

/**
 * @brief Procesa zombies con prioridad crítica (60 FPS)
 *
 * Zombies muy cercanos al jugador (< 300 unidades)
 * Reciben procesamiento completo: AI compleja, rotación suave, comportamiento detallado
 */
void UZombiOptimizedProcessor::ProcessCriticalZombies(FMassEntityManager &EntityManager, FMassExecutionContext &Context, float CurrentTime)
{
    OptimizedQuery.ForEachEntityChunk(EntityManager, Context, [this, CurrentTime](FMassExecutionContext &Context)
                                      {
        TArrayView<FZombiCoreFragment> CoreFragments = Context.GetMutableFragmentView<FZombiCoreFragment>();
        TArrayView<const FZombiBehaviorFragment> BehaviorFragments = Context.GetFragmentView<FZombiBehaviorFragment>();
        TArrayView<FZombiUpdateFrequencyFragment> UpdateFragments = Context.GetMutableFragmentView<FZombiUpdateFrequencyFragment>();
        const float DeltaTime = Context.GetDeltaTimeSeconds();

        // Procesar en lotes de BATCH_SIZE para optimizar cache locality
        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiUpdateFrequencyFragment& UpdateFragment = UpdateFragments[i];
            
            // Solo procesar si es prioridad crítica y es tiempo de update
            if (UpdateFragment.Priority != EZombiUpdatePriority::Critical || 
                !UpdateFragment.ShouldUpdate(CurrentTime))
            {
                continue;
            }

            FZombiCoreFragment& CoreFragment = CoreFragments[i];
            const FZombiBehaviorFragment& BehaviorFragment = BehaviorFragments[i];

            // Procesamiento completo para zombies críticos
            ProcessFullMovement(CoreFragment, BehaviorFragment, UpdateFragment, DeltaTime);
        } });
}

/**
 * @brief Procesa zombies con prioridad alta (30 FPS)
 *
 * Zombies visibles (< 600 unidades)
 * Reciben procesamiento básico: AI básica, rotación simple
 */
void UZombiOptimizedProcessor::ProcessHighZombies(FMassEntityManager &EntityManager, FMassExecutionContext &Context, float CurrentTime)
{
    OptimizedQuery.ForEachEntityChunk(EntityManager, Context, [this, CurrentTime](FMassExecutionContext &Context)
                                      {
        TArrayView<FZombiCoreFragment> CoreFragments = Context.GetMutableFragmentView<FZombiCoreFragment>();
        TArrayView<const FZombiBehaviorFragment> BehaviorFragments = Context.GetFragmentView<FZombiBehaviorFragment>();
        TArrayView<FZombiUpdateFrequencyFragment> UpdateFragments = Context.GetMutableFragmentView<FZombiUpdateFrequencyFragment>();
        const float DeltaTime = Context.GetDeltaTimeSeconds();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiUpdateFrequencyFragment& UpdateFragment = UpdateFragments[i];
            
            // Solo procesar si es prioridad alta y es tiempo de update
            if (UpdateFragment.Priority != EZombiUpdatePriority::High || 
                !UpdateFragment.ShouldUpdate(CurrentTime))
            {
                continue;
            }

            FZombiCoreFragment& CoreFragment = CoreFragments[i];
            const FZombiBehaviorFragment& BehaviorFragment = BehaviorFragments[i];

            // Procesamiento básico para zombies de alta prioridad
            ProcessBasicMovement(CoreFragment, BehaviorFragment, UpdateFragment, DeltaTime);
        } });
}

/**
 * @brief Procesa zombies con prioridad normal (15 FPS)
 *
 * Zombies de fondo (< 1000 unidades)
 * Reciben procesamiento simple: solo movimiento básico
 */
void UZombiOptimizedProcessor::ProcessNormalZombies(FMassEntityManager &EntityManager, FMassExecutionContext &Context, float CurrentTime)
{
    OptimizedQuery.ForEachEntityChunk(EntityManager, Context, [this, CurrentTime](FMassExecutionContext &Context)
                                      {
        TArrayView<FZombiCoreFragment> CoreFragments = Context.GetMutableFragmentView<FZombiCoreFragment>();
        TArrayView<const FZombiBehaviorFragment> BehaviorFragments = Context.GetFragmentView<FZombiBehaviorFragment>();
        TArrayView<FZombiUpdateFrequencyFragment> UpdateFragments = Context.GetMutableFragmentView<FZombiUpdateFrequencyFragment>();
        const float DeltaTime = Context.GetDeltaTimeSeconds();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiUpdateFrequencyFragment& UpdateFragment = UpdateFragments[i];
            
            // Solo procesar si es prioridad normal y es tiempo de update
            if (UpdateFragment.Priority != EZombiUpdatePriority::Normal || 
                !UpdateFragment.ShouldUpdate(CurrentTime))
            {
                continue;
            }

            FZombiCoreFragment& CoreFragment = CoreFragments[i];
            const FZombiBehaviorFragment& BehaviorFragment = BehaviorFragments[i];

            // Procesamiento simple para zombies de prioridad normal
            ProcessSimpleMovement(CoreFragment, BehaviorFragment, UpdateFragment, DeltaTime);
        } });
}

/**
 * @brief Procesa zombies con prioridad baja (5 FPS)
 *
 * Zombies lejanos (< 1500 unidades)
 * Reciben procesamiento mínimo: solo posición
 */
void UZombiOptimizedProcessor::ProcessLowZombies(FMassEntityManager &EntityManager, FMassExecutionContext &Context, float CurrentTime)
{
    OptimizedQuery.ForEachEntityChunk(EntityManager, Context, [this, CurrentTime](FMassExecutionContext &Context)
                                      {
        TArrayView<FZombiCoreFragment> CoreFragments = Context.GetMutableFragmentView<FZombiCoreFragment>();
        TArrayView<const FZombiBehaviorFragment> BehaviorFragments = Context.GetFragmentView<FZombiBehaviorFragment>();
        TArrayView<FZombiUpdateFrequencyFragment> UpdateFragments = Context.GetMutableFragmentView<FZombiUpdateFrequencyFragment>();
        const float DeltaTime = Context.GetDeltaTimeSeconds();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiUpdateFrequencyFragment& UpdateFragment = UpdateFragments[i];
            
            // Solo procesar si es prioridad baja y es tiempo de update
            if (UpdateFragment.Priority != EZombiUpdatePriority::Low || 
                !UpdateFragment.ShouldUpdate(CurrentTime))
            {
                continue;
            }

            FZombiCoreFragment& CoreFragment = CoreFragments[i];
            const FZombiBehaviorFragment& BehaviorFragment = BehaviorFragments[i];

            // Procesamiento mínimo para zombies de prioridad baja
            ProcessMinimalMovement(CoreFragment, BehaviorFragment, UpdateFragment, DeltaTime);
        } });
}

/**
 * @brief Procesa movimiento completo para zombies críticos
 *
 * Incluye: AI compleja, rotación suave, cambio de dirección aleatorio,
 * confinamiento en área, comportamiento detallado
 */
void UZombiOptimizedProcessor::ProcessFullMovement(FZombiCoreFragment &CoreFragment, const FZombiBehaviorFragment &BehaviorFragment,
                                                   FZombiUpdateFrequencyFragment &UpdateFragment, float DeltaTime)
{
    // Solo procesar si no está muerto
    if (BehaviorFragment.IsDead())
    {
        return;
    }

    // Procesar movimiento aleatorio solo si no está persiguiendo
    if (!BehaviorFragment.IsChasing())
    {
        // Actualizar timer de cambio de dirección
        CoreFragment.BehaviorTimer += DeltaTime;

        // Cambiar dirección aleatoriamente
        if (CoreFragment.BehaviorTimer >= CoreFragment.DirectionChangeInterval)
        {
            CoreFragment.MovementDirection = GenerateRandomDirection();
            CoreFragment.BehaviorTimer = 0.0f;

            // Cambiar velocidad aleatoriamente
            float SpeedVariation = FMath::RandRange(0.0f, 1.0f);
            if (SpeedVariation < 0.3f)
            {
                CoreFragment.MovementSpeed = 0.0f; // IDLE
            }
            else if (SpeedVariation < 0.7f)
            {
                CoreFragment.MovementSpeed = 25.0f; // WALK
            }
            else
            {
                CoreFragment.MovementSpeed = 80.0f; // RUN
            }
        }
    }

    // Procesar rotación suave hacia la dirección de movimiento
    if (!CoreFragment.MovementDirection.IsNearlyZero())
    {
        if (CoreFragment.MovementSpeed > 1.0f)
        {
            FRotator TargetRotation = CoreFragment.MovementDirection.Rotation();
            FRotator CurrentRotation = CoreFragment.Rotation;

            // Rotación más agresiva si está persiguiendo o acaba de cambiar dirección
            float RotationSpeedMultiplier = (BehaviorFragment.IsChasing() || CoreFragment.BehaviorTimer < 0.5f) ? 3.0f : 1.0f;

            // Interpola suavemente la rotación
            CoreFragment.Rotation = FMath::RInterpTo(
                CurrentRotation,
                TargetRotation,
                DeltaTime,
                (CoreFragment.RotationSpeed * RotationSpeedMultiplier) / 180.0f);
        }
    }

    // Procesar movimiento hacia adelante
    if (CoreFragment.MovementSpeed > 0.0f)
    {
        FVector ForwardDirection = CoreFragment.Rotation.Vector();
        CoreFragment.Position += ForwardDirection * CoreFragment.MovementSpeed * DeltaTime;
    }

    // Aplicar restricciones de área solo si no está persiguiendo
    if (!BehaviorFragment.IsChasing())
    {
        CoreFragment.Position = ClampToMovementArea(CoreFragment.Position, CoreFragment.MovementCenter, CoreFragment.MovementRadius);
    }
}

/**
 * @brief Procesa movimiento básico para zombies de prioridad alta
 *
 * Incluye: AI básica, rotación simple, movimiento básico
 */
void UZombiOptimizedProcessor::ProcessBasicMovement(FZombiCoreFragment &CoreFragment, const FZombiBehaviorFragment &BehaviorFragment,
                                                    FZombiUpdateFrequencyFragment &UpdateFragment, float DeltaTime)
{
    // Solo procesar si no está muerto
    if (BehaviorFragment.IsDead())
    {
        return;
    }

    // Procesar movimiento aleatorio simplificado
    if (!BehaviorFragment.IsChasing())
    {
        CoreFragment.BehaviorTimer += DeltaTime;

        // Cambiar dirección menos frecuentemente
        if (CoreFragment.BehaviorTimer >= CoreFragment.DirectionChangeInterval * 1.5f)
        {
            CoreFragment.MovementDirection = GenerateRandomDirection();
            CoreFragment.BehaviorTimer = 0.0f;

            // Velocidades simplificadas
            float SpeedVariation = FMath::RandRange(0.0f, 1.0f);
            if (SpeedVariation < 0.5f)
            {
                CoreFragment.MovementSpeed = 0.0f; // IDLE
            }
            else
            {
                CoreFragment.MovementSpeed = 50.0f; // WALK
            }
        }
    }

    // Rotación simplificada
    if (!CoreFragment.MovementDirection.IsNearlyZero() && CoreFragment.MovementSpeed > 1.0f)
    {
        FRotator TargetRotation = CoreFragment.MovementDirection.Rotation();
        CoreFragment.Rotation = FMath::RInterpTo(
            CoreFragment.Rotation,
            TargetRotation,
            DeltaTime,
            CoreFragment.RotationSpeed / 180.0f);
    }

    // Movimiento básico
    if (CoreFragment.MovementSpeed > 0.0f)
    {
        FVector ForwardDirection = CoreFragment.Rotation.Vector();
        CoreFragment.Position += ForwardDirection * CoreFragment.MovementSpeed * DeltaTime;
    }

    // Confinamiento básico
    if (!BehaviorFragment.IsChasing())
    {
        CoreFragment.Position = ClampToMovementArea(CoreFragment.Position, CoreFragment.MovementCenter, CoreFragment.MovementRadius);
    }
}

/**
 * @brief Procesa movimiento simple para zombies de prioridad normal
 *
 * Incluye: solo movimiento básico, sin AI compleja
 */
void UZombiOptimizedProcessor::ProcessSimpleMovement(FZombiCoreFragment &CoreFragment, const FZombiBehaviorFragment &BehaviorFragment,
                                                     FZombiUpdateFrequencyFragment &UpdateFragment, float DeltaTime)
{
    // Solo procesar si no está muerto
    if (BehaviorFragment.IsDead())
    {
        return;
    }

    // Movimiento muy simplificado
    if (CoreFragment.MovementSpeed > 0.0f)
    {
        FVector ForwardDirection = CoreFragment.Rotation.Vector();
        CoreFragment.Position += ForwardDirection * CoreFragment.MovementSpeed * DeltaTime;
    }

    // Confinamiento básico
    if (!BehaviorFragment.IsChasing())
    {
        CoreFragment.Position = ClampToMovementArea(CoreFragment.Position, CoreFragment.MovementCenter, CoreFragment.MovementRadius);
    }
}

/**
 * @brief Procesa movimiento mínimo para zombies de prioridad baja
 *
 * Solo actualiza posición, sin AI ni rotación
 */
void UZombiOptimizedProcessor::ProcessMinimalMovement(FZombiCoreFragment &CoreFragment, const FZombiBehaviorFragment &BehaviorFragment,
                                                      FZombiUpdateFrequencyFragment &UpdateFragment, float DeltaTime)
{
    // Solo procesar si no está muerto
    if (BehaviorFragment.IsDead())
    {
        return;
    }

    // Movimiento mínimo - solo posición
    if (CoreFragment.MovementSpeed > 0.0f)
    {
        FVector ForwardDirection = CoreFragment.Rotation.Vector();
        CoreFragment.Position += ForwardDirection * CoreFragment.MovementSpeed * DeltaTime;
    }
}

/**
 * @brief Genera dirección aleatoria para movimiento
 *
 * Retorna un vector normalizado en una dirección aleatoria
 */
FVector UZombiOptimizedProcessor::GenerateRandomDirection() const
{
    // Generar dirección aleatoria en el plano XZ (para cámara isométrica)
    float RandomAngle = FMath::RandRange(0.0f, 360.0f);
    FVector RandomDirection = FVector(
        FMath::Cos(FMath::DegreesToRadians(RandomAngle)),
        FMath::Sin(FMath::DegreesToRadians(RandomAngle)),
        0.0f);

    return RandomDirection.GetSafeNormal();
}

/**
 * @brief Confina la posición dentro del área de movimiento
 *
 * Si la posición está fuera del radio, la mueve hacia el centro
 */
FVector UZombiOptimizedProcessor::ClampToMovementArea(const FVector &Position, const FVector &Center, float Radius) const
{
    if (IsWithinMovementRadius(Position, Center, Radius))
    {
        return Position;
    }

    // Calcular dirección hacia el centro
    FVector DirectionToCenter = (Center - Position).GetSafeNormal();

    // Mover hacia el centro hasta estar dentro del radio
    return Center - (DirectionToCenter * Radius);
}

/**
 * @brief Verifica si una posición está dentro del radio de movimiento
 *
 * Compara la distancia al centro con el radio especificado
 */
bool UZombiOptimizedProcessor::IsWithinMovementRadius(const FVector &Position, const FVector &Center, float Radius) const
{
    float DistanceSquared = FVector::DistSquared(Position, Center);
    return DistanceSquared <= (Radius * Radius);
}