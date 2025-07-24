// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiTurboSequenceProcessor.h"
#include "MassExecutionContext.h"
#include "TurboSequence_Manager_Lf.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "Engine/Engine.h"

UZombiTurboSequenceProcessor::UZombiTurboSequenceProcessor()
{
    // Se ejecuta en el grupo de procesamiento de renderizado por defecto
    ExecutionFlags = (int32)(EProcessorExecutionFlags::All);
    ExecutionOrder.ExecuteInGroup = TEXT("BehaviorBeginFrame");
}

void UZombiTurboSequenceProcessor::ConfigureQueries()
{
    // Evita configurar múltiples veces
    static bool bConfigured = false;
    if (bConfigured)
    {
        return;
    }

    // Query para entidades que necesitan instancias visuales creadas
    VisualInstanceQuery.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
    VisualInstanceQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadOnly);
    VisualInstanceQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadOnly);

    // Query para entidades que necesitan sincronización de transformación
    TransformSyncQuery.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadOnly);
    TransformSyncQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadOnly);

    bConfigured = true;
    UE_LOG(LogTemp, Log, TEXT("ZombiTurboSequenceProcessor: Queries configurados correctamente"));
}

void UZombiTurboSequenceProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Los procesadores Mass Entity no pueden obtener el mundo directamente
    // Por ahora, deshabilitamos la creación de instancias visuales hasta que resolvamos esto
    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Configura el query si no está configurado
    static bool bQueryConfigured = false;
    if (!bQueryConfigured)
    {
        ConfigureQueries();
        bQueryConfigured = true;
        UE_LOG(LogTemp, Log, TEXT("ZombiTurboSequenceProcessor: Query configurado en Execute"));
    }

    // Log temporal para verificar que el procesador se ejecuta
    static float LogTimer = 0.0f;
    LogTimer += DeltaTime;
    if (LogTimer >= 3.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("ZombiTurboSequenceProcessor: Ejecutándose - Entidades: %d"), Context.GetNumEntities());

        // Log adicional para verificar el estado de las instancias visuales
        if (Context.GetNumEntities() > 0)
        {
            TransformSyncQuery.ForEachEntityChunk(EntityManager, Context, [](FMassExecutionContext &Context)
                                                  {
                const TConstArrayView<FZombiTurboSequenceFragment> TurboSequenceFragments = Context.GetFragmentView<FZombiTurboSequenceFragment>();
                const TConstArrayView<FZombiMovementFragment> MovementFragments = Context.GetFragmentView<FZombiMovementFragment>();
                
                // Cuenta instancias válidas
                int32 ValidInstances = 0;
                for (int32 i = 0; i < Context.GetNumEntities(); ++i)
                {
                    if (TurboSequenceFragments[i].bIsVisualInstanceValid)
                    {
                        ValidInstances++;
                    }
                }
                
                UE_LOG(LogTemp, Log, TEXT("ZombiTurboSequenceProcessor: Instancias válidas: %d/%d"), ValidInstances, Context.GetNumEntities());
                
                // Muestra información de la primera entidad como ejemplo
                if (Context.GetNumEntities() > 0)
                {
                    const FZombiTurboSequenceFragment& FirstTurbo = TurboSequenceFragments[0];
                    const FZombiMovementFragment& FirstMovement = MovementFragments[0];
                    
                    UE_LOG(LogTemp, Log, TEXT("ZombiTurboSequenceProcessor: Primera entidad - Instancia válida: %s, Posición: %s"), 
                        FirstTurbo.bIsVisualInstanceValid ? TEXT("Sí") : TEXT("No"),
                        *FirstMovement.Position.ToString());
                } });
        }

        LogTimer = 0.0f;
    }

    // Por ahora, solo procesamos la sincronización de transformación
    // La creación de instancias se manejará en el spawner

    // Procesa entidades que necesitan instancias visuales creadas
    // TEMPORALMENTE DESHABILITADO - Problema con obtención del mundo en procesadores Mass Entity
    /*
    VisualInstanceQuery.ForEachEntityChunk(EntityManager, Context, [this, World](FMassExecutionContext &Context)
                                           {
        const TArrayView<FZombiTurboSequenceFragment> TurboSequenceFragments = Context.GetMutableFragmentView<FZombiTurboSequenceFragment>();
        const TConstArrayView<FZombiMovementFragment> MovementFragments = Context.GetFragmentView<FZombiMovementFragment>();
        const TConstArrayView<FZombiStateFragment> StateFragments = Context.GetFragmentView<FZombiStateFragment>();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiTurboSequenceFragment& TurboSequenceFragment = TurboSequenceFragments[i];
            const FZombiMovementFragment& MovementFragment = MovementFragments[i];
            const FZombiStateFragment& StateFragment = StateFragments[i];

            // Solo procesa si el zombi no está muerto
            if (StateFragment.State != EZombiState::Death)
            {
                // Crea instancia visual si no existe
                if (!TurboSequenceFragment.bIsVisualInstanceValid)
                {
                    CreateTurboSequenceInstance(TurboSequenceFragment, MovementFragment, World);
                }
            }
            else
            {
                // Destruye instancia visual si el zombi está muerto
                if (TurboSequenceFragment.bIsVisualInstanceValid)
                {
                    DestroyTurboSequenceInstance(TurboSequenceFragment, World);
                }
            }
        } });
    */

    // Solo ejecuta el query si hay entidades para procesar
    if (Context.GetNumEntities() > 0)
    {
        // Procesa entidades que necesitan sincronización de transformación
        TransformSyncQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext &Context)
                                              {
		const TConstArrayView<FZombiTurboSequenceFragment> TurboSequenceFragments = Context.GetFragmentView<FZombiTurboSequenceFragment>();
		const TConstArrayView<FZombiMovementFragment> MovementFragments = Context.GetFragmentView<FZombiMovementFragment>();

		for (int32 i = 0; i < Context.GetNumEntities(); ++i)
		{
			const FZombiTurboSequenceFragment& TurboSequenceFragment = TurboSequenceFragments[i];
			const FZombiMovementFragment& MovementFragment = MovementFragments[i];

			// Actualiza transformación si la instancia visual es válida
			if (TurboSequenceFragment.bIsVisualInstanceValid)
			{
				UpdateTurboSequenceTransform(TurboSequenceFragment, MovementFragment);
			}
		} });
    }
}

void UZombiTurboSequenceProcessor::CreateTurboSequenceInstance(FZombiTurboSequenceFragment &TurboSequenceFragment,
                                                               const FZombiMovementFragment &MovementFragment,
                                                               UWorld *World)
{
    if (!TurboSequenceFragment.TurboSequenceAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("TurboSequence Asset no asignado para crear instancia visual"));
        return;
    }

    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("World es null al crear instancia visual de TurboSequence"));
        return;
    }

    if (!World->IsValidLowLevel() || World->IsUnreachable())
    {
        UE_LOG(LogTemp, Error, TEXT("World no es válido al crear instancia visual de TurboSequence"));
        return;
    }

    // Crea la transformación de spawn
    FTransform SpawnTransform(MovementFragment.Rotation, MovementFragment.Position, FVector::OneVector);

    // Crea la instancia visual usando las mejores prácticas de TurboSequence
    FTurboSequence_MeshSpawnData_Lf SpawnData;
    SpawnData.RootMotionMesh.Mesh = TurboSequenceFragment.TurboSequenceAsset;

    // Verifica que el asset sea válido antes de crear la instancia
    if (!TurboSequenceFragment.TurboSequenceAsset->IsValidLowLevel())
    {
        UE_LOG(LogTemp, Error, TEXT("TurboSequence Asset no es válido"));
        return;
    }

    TurboSequenceFragment.MeshData = ATurboSequence_Manager_Lf::AddSkinnedMeshInstance_GameThread(
        SpawnData,
        SpawnTransform,
        World);

    // Agrega a un grupo de actualización para optimización
    if (TurboSequenceFragment.MeshData.IsMeshDataValid())
    {
        ATurboSequence_Manager_Lf::AddInstanceToUpdateGroup_Concurrent(
            TurboSequenceFragment.UpdateGroupIndex,
            TurboSequenceFragment.MeshData);

        TurboSequenceFragment.bIsVisualInstanceValid = true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Error al crear instancia visual de TurboSequence"));
    }
}

void UZombiTurboSequenceProcessor::UpdateTurboSequenceTransform(const FZombiTurboSequenceFragment &TurboSequenceFragment,
                                                                const FZombiMovementFragment &MovementFragment)
{
    if (!TurboSequenceFragment.MeshData.IsMeshDataValid())
    {
        return;
    }

    // Actualiza la transformación de la instancia visual
    FTransform NewTransform(MovementFragment.Rotation, MovementFragment.Position, FVector::OneVector);

    ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_Concurrent(
        TurboSequenceFragment.MeshData,
        NewTransform,
        false // No forzar actualización
    );
}

void UZombiTurboSequenceProcessor::DestroyTurboSequenceInstance(const FZombiTurboSequenceFragment &TurboSequenceFragment,
                                                                UWorld *World)
{
    if (!TurboSequenceFragment.MeshData.IsMeshDataValid())
    {
        return;
    }

    // Remueve del grupo de actualización
    ATurboSequence_Manager_Lf::RemoveInstanceFromUpdateGroup_Concurrent(
        TurboSequenceFragment.UpdateGroupIndex,
        TurboSequenceFragment.MeshData);

    // Destruye la instancia visual
    ATurboSequence_Manager_Lf::RemoveSkinnedMeshInstance_GameThread(
        TurboSequenceFragment.MeshData,
        World);
}
