// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiMovementProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "MyProjectTSecuence/MyProjectTSecuenceCharacter.h"

UZombiMovementProcessor::UZombiMovementProcessor()
{
    // Configuración para registro automático en UE5.5.4 - IDÉNTICA a los otros procesadores
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionOrder.ExecuteInGroup = TEXT("MassBehavior");
    bRequiresGameThreadExecution = false;
    bAutoRegisterWithProcessingPhases = true;

    // Log solo en la primera instancia
    static bool bLoggedConstructor = false;
    if (!bLoggedConstructor)
    {

        bLoggedConstructor = true;
    }
}

void UZombiMovementProcessor::ConfigureQueries()
{
    // 🔥 SIMPLIFICADO: UNA SOLA QUERY - Sin comandos diferidos
    // Elimina las 4 queries por frecuencia que generaban comandos diferidos masivos
    MovementQuery.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadWrite);
    MovementQuery.AddRequirement<FZombiBehaviorFragment>(EMassFragmentAccess::ReadOnly);
    MovementQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    MovementQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);
    MovementQuery.RegisterWithProcessor(*this);

    // ❌ ELIMINADO: Update60FPSQuery, Update30FPSQuery, Update15FPSQuery, Update5FPSQuery
    // RAZÓN: Generaban comandos diferidos masivos (parte de los 3000 por frame)
    // REEMPLAZADO POR: Una sola query con procesamiento simple
}

void UZombiMovementProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // ✅ REACTIVADO: Procesador de movimiento para manejar posiciones y rotaciones
    // Ahora maneja el movimiento físico y actualiza posiciones para TurboSequence

    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        return;
    }

    // 🔍 DIAGNÓSTICO: Log cada 5 segundos para ver si se ejecuta
    static float LastLogTime = 0.0f;
    float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastLogTime > 5.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("🏃 ZombiMovementProcessor: Ejecutándose"));
        LastLogTime = CurrentTime;
    }

    // Obtener referencia al jugador (cached para performance)
    if (!PlayerPawn)
    {
        PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    }

    if (!PlayerPawn)
    {
        return; // No hay jugador, no procesar
    }

    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // ✅ SIMPLIFICADO: UNA SOLA QUERY - Sin comandos diferidos
    // Procesa TODAS las entidades en una sola pasada, sin separación por frecuencia
    MovementQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &Context)
                                     {
        TArrayView<FZombiCoreFragment> CoreFragments = Context.GetMutableFragmentView<FZombiCoreFragment>();
        TArrayView<const FZombiBehaviorFragment> BehaviorFragments = Context.GetFragmentView<FZombiBehaviorFragment>();

        // 🔍 DIAGNÓSTICO: Log cada 5 segundos para ver si procesa entidades
        static float LastEntityLogTime = 0.0f;
        float CurrentTime = GetWorld()->GetTimeSeconds();
        bool bShouldLog = (CurrentTime - LastEntityLogTime > 5.0f);
        if (bShouldLog)
        {
            UE_LOG(LogTemp, Log, TEXT("🏃 ZombiMovementProcessor: Procesando %d entidades"), Context.GetNumEntities());
            LastEntityLogTime = CurrentTime;
        }

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiCoreFragment& CoreFragment = CoreFragments[i];
            const FZombiBehaviorFragment& BehaviorFragment = BehaviorFragments[i];

            EZombiState CurrentState = BehaviorFragment.GetState();

            // 🔍 DIAGNÓSTICO: Log movimiento solo cada 5 segundos (no cada frame)
            if (bShouldLog)
            {
                UE_LOG(LogTemp, Log, TEXT("🏃 Movimiento: Estado=%d, Velocidad=%.1f, Posición=%s"), 
                       (int32)CurrentState, CoreFragment.MovementSpeed, *CoreFragment.Position.ToString());
            }

            // Procesar movimiento según estado
            if (CurrentState == EZombiState::Chase || CurrentState == EZombiState::Seek)
            {
                ProcessPlayerChaseMovement(CoreFragment, DeltaTime);
            }
            else
            {
                ProcessRandomMovement(CoreFragment, DeltaTime);
            }

            // Procesar rotación y movimiento
            ProcessRotationAndMovement(CoreFragment, BehaviorFragment, DeltaTime);
        } });
}

// ===========================
// MÉTODOS AUXILIARES SIMPLIFICADOS
// ===========================

// Genera una dirección aleatoria para el movimiento
FVector UZombiMovementProcessor::GenerateRandomDirection() const
{
    // Genera un ángulo aleatorio en el plano XZ (horizontal)
    float RandomAngle = FMath::RandRange(0.0f, 360.0f);
    float Radians = FMath::DegreesToRadians(RandomAngle);

    return FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.0f).GetSafeNormal();
}

// Ajusta la posición para mantener al zombi dentro del área
FVector UZombiMovementProcessor::ClampToMovementArea(const FVector &Position, const FVector &Center, float Radius) const
{
    FVector Direction = Position - Center;
    Direction.Z = 0.0f; // Mantiene la altura original

    if (Direction.Size2D() > Radius)
    {
        Direction = Direction.GetSafeNormal2D() * Radius;
        return Center + Direction;
    }

    return Position;
}

void UZombiMovementProcessor::ProcessPlayerChaseMovement(FZombiCoreFragment &CoreFragment, float DeltaTime)
{
    if (!PlayerPawn)
    {
        return;
    }

    // Calcular dirección hacia el jugador
    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    FVector DirectionToPlayer = (PlayerLocation - CoreFragment.Position).GetSafeNormal();
    DirectionToPlayer.Z = 0.0f; // Ignorar altura para isométrico

    if (!DirectionToPlayer.IsNearlyZero())
    {
        // Actualizar dirección de movimiento hacia el jugador
        CoreFragment.MovementDirection = DirectionToPlayer;

        // Velocidad alta para persecución
        CoreFragment.MovementSpeed = 120.0f;
    }
}

void UZombiMovementProcessor::ProcessRandomMovement(FZombiCoreFragment &CoreFragment, float DeltaTime)
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
            CoreFragment.MovementSpeed = 0.0f;
        }
        else if (SpeedVariation < 0.7f)
        {
            CoreFragment.MovementSpeed = 25.0f;
        }
        else
        {
            CoreFragment.MovementSpeed = 80.0f;
        }
    }
}

void UZombiMovementProcessor::ProcessRotationAndMovement(FZombiCoreFragment &CoreFragment, const FZombiBehaviorFragment &BehaviorFragment, float DeltaTime)
{
    EZombiState CurrentState = BehaviorFragment.GetState();

    // Procesar rotación hacia la dirección de movimiento
    if (!CoreFragment.MovementDirection.IsNearlyZero())
    {
        if (CoreFragment.MovementSpeed > 1.0f)
        {
            FRotator TargetRotation = CoreFragment.MovementDirection.Rotation();
            FRotator CurrentRotation = CoreFragment.Rotation;

            // Rotación más agresiva si está persiguiendo
            float RotationSpeedMultiplier = (CurrentState == EZombiState::Chase) ? 3.0f : 1.0f;

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
    if (CurrentState != EZombiState::Chase && CurrentState != EZombiState::Seek)
    {
        CoreFragment.Position = ClampToMovementArea(CoreFragment.Position, CoreFragment.MovementCenter, CoreFragment.MovementRadius);
    }
}
