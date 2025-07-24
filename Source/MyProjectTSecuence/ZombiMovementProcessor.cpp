// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiMovementProcessor.h"
#include "MassExecutionContext.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "TurboSequence_Manager_Lf.h"
#include "Engine/Engine.h"

// Constructor del procesador
UZombiMovementProcessor::UZombiMovementProcessor()
{
    // Se ejecuta en el grupo de procesamiento de movimiento por defecto
    ExecutionFlags = (int32)(EProcessorExecutionFlags::All);
    ExecutionOrder.ExecuteInGroup = TEXT("BehaviorBeginFrame");
}

// Configura el query para requerir los fragmentos de movimiento y estado
void UZombiMovementProcessor::ConfigureQueries()
{
    // Evita configurar múltiples veces
    static bool bConfigured = false;
    if (bConfigured)
    {
        return;
    }

    MovementQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadWrite);
    MovementQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadOnly);

    bConfigured = true;
    UE_LOG(LogTemp, Log, TEXT("ZombiMovementProcessor: Query configurado correctamente"));
}

// Ejecuta el procesamiento de movimiento para todas las entidades
void UZombiMovementProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Configura el query si no está configurado
    static bool bQueryConfigured = false;
    if (!bQueryConfigured)
    {
        ConfigureQueries();
        bQueryConfigured = true;
        UE_LOG(LogTemp, Log, TEXT("ZombiMovementProcessor: Query configurado en Execute"));
    }

    // Log para verificar si el query está funcionando
    static float QueryLogTimer = 0.0f;
    QueryLogTimer += DeltaTime;
    if (QueryLogTimer >= 1.0f)
    {
        // Solo ejecuta el query si hay entidades para procesar
        if (Context.GetNumEntities() > 0)
        {
            // Intenta ejecutar el query directamente para ver si encuentra entidades
            int32 FoundEntities = 0;
            MovementQuery.ForEachEntityChunk(EntityManager, Context, [&FoundEntities](FMassExecutionContext &Context)
                                             { FoundEntities += Context.GetNumEntities(); });

            UE_LOG(LogTemp, Log, TEXT("ZombiMovementProcessor: Query encontró %d entidades directamente, Context tiene %d"), FoundEntities, Context.GetNumEntities());
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("ZombiMovementProcessor: Context no tiene entidades para procesar"));
        }
        QueryLogTimer = 0.0f;
    }

    // Log temporal para verificar que el procesador se ejecuta
    static float LogTimer = 0.0f;
    LogTimer += DeltaTime;
    if (LogTimer >= 2.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("ZombiMovementProcessor: Ejecutándose - Entidades: %d"), Context.GetNumEntities());

        // Log adicional para verificar el estado de las entidades
        if (Context.GetNumEntities() > 0)
        {
            MovementQuery.ForEachEntityChunk(EntityManager, Context, [](FMassExecutionContext &Context)
                                             {
                const TArrayView<FZombiMovementFragment> MovementFragments = Context.GetMutableFragmentView<FZombiMovementFragment>();
                const TConstArrayView<FZombiStateFragment> StateFragments = Context.GetFragmentView<FZombiStateFragment>();
                
                // Muestra información de la primera entidad como ejemplo
                if (Context.GetNumEntities() > 0)
                {
                    const FZombiMovementFragment& FirstMovement = MovementFragments[0];
                    const FZombiStateFragment& FirstState = StateFragments[0];
                    
                    UE_LOG(LogTemp, Log, TEXT("ZombiMovementProcessor: Primera entidad - Posición: %s, Estado: %d"), 
                        *FirstMovement.Position.ToString(), 
                        (int32)FirstState.State);
                } });
        }

        LogTimer = 0.0f;
    }

    // Solo ejecuta el query si hay entidades para procesar
    if (Context.GetNumEntities() > 0)
    {
        MovementQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext &Context)
                                         {
		const TArrayView<FZombiMovementFragment> MovementFragments = Context.GetMutableFragmentView<FZombiMovementFragment>();
		const TConstArrayView<FZombiStateFragment> StateFragments = Context.GetFragmentView<FZombiStateFragment>();
		const float DeltaTime = Context.GetDeltaTimeSeconds();

		for (int32 i = 0; i < Context.GetNumEntities(); ++i)
		{
			FZombiMovementFragment& MovementFragment = MovementFragments[i];
			const FZombiStateFragment& StateFragment = StateFragments[i];

			// Solo procesa movimiento si el zombi no está muerto
			if (StateFragment.State != EZombiState::Death)
			{
				// Actualiza el timer de cambio de dirección
				MovementFragment.DirectionChangeTimer += DeltaTime;

				// Cambia dirección aleatoriamente
				if (MovementFragment.DirectionChangeTimer >= MovementFragment.DirectionChangeInterval)
				{
					MovementFragment.MovementDirection = GenerateRandomDirection();
					MovementFragment.DirectionChangeTimer = 0.0f;
				}

				// Calcula el movimiento
				FVector NewPosition = MovementFragment.Position + 
					MovementFragment.MovementDirection * MovementFragment.MovementSpeed * DeltaTime;

				// Mantiene al zombi dentro del área de movimiento
				NewPosition = ClampToMovementArea(NewPosition, MovementFragment.MovementCenter, MovementFragment.MovementRadius);

				// Actualiza la posición
				MovementFragment.Position = NewPosition;

				// Calcula la rotación hacia la dirección de movimiento
				if (!MovementFragment.MovementDirection.IsNearlyZero())
				{
					FRotator TargetRotation = MovementFragment.MovementDirection.Rotation();
					FRotator CurrentRotation = MovementFragment.Rotation;

					// Interpola suavemente la rotación
					MovementFragment.Rotation = FMath::RInterpTo(
						CurrentRotation, 
						TargetRotation, 
						DeltaTime, 
						MovementFragment.RotationSpeed / 90.0f // Normaliza la velocidad de rotación
					);
				}

				// Actualiza la instancia visual de TurboSequence
				UpdateTurboSequenceInstance(MovementFragment);
			}
		} });
    }
}

// Genera una dirección aleatoria para el movimiento
FVector UZombiMovementProcessor::GenerateRandomDirection() const
{
    // Genera un ángulo aleatorio en el plano XZ (horizontal)
    float RandomAngle = FMath::RandRange(0.0f, 360.0f);
    float Radians = FMath::DegreesToRadians(RandomAngle);

    return FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.0f).GetSafeNormal();
}

// Verifica si el zombi está dentro del radio de movimiento
bool UZombiMovementProcessor::IsWithinMovementRadius(const FVector &Position, const FVector &Center, float Radius) const
{
    return FVector::Dist2D(Position, Center) <= Radius;
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

// Crea o actualiza la instancia visual de TurboSequence
void UZombiMovementProcessor::UpdateTurboSequenceInstance(const FZombiMovementFragment &MovementFragment)
{
    // Por ahora, solo actualizamos la transformación si ya existe una instancia
    // La creación de instancias se manejará en un procesador separado o en el spawner
    // Esto evita problemas con la API de TurboSequence y mejora el rendimiento

    // TODO: Implementar la sincronización con TurboSequence cuando sea necesario
    // Por ahora, solo mantenemos los datos de movimiento en el fragmento
}
