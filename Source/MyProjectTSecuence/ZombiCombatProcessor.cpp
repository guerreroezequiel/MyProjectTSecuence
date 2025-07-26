// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiCombatProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "ZombiCombatFragment.h"
#include "ZombiBehaviorFragment.h"
#include "ZombiCoreFragment.h"
#include "ZombiTags.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "MyProjectTSecuenceCharacter.h"

UZombiCombatProcessor::UZombiCombatProcessor()
{
    // Configuración para registro automático en UE5.5.4
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionOrder.ExecuteInGroup = TEXT("MassBehavior");
    bRequiresGameThreadExecution = false;
    bAutoRegisterWithProcessingPhases = true;

    // Log solo en la primera instancia
    static bool bLoggedConstructor = false;
    if (!bLoggedConstructor)
    {
        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiCombatProcessor: Procesador de combate inicializado"));
        bLoggedConstructor = true;
    }
}

void UZombiCombatProcessor::ConfigureQueries()
{
    // Query optimizada para combate - solo fragmentos necesarios
    CombatQuery.AddRequirement<FZombiCombatFragment>(EMassFragmentAccess::ReadWrite);
    CombatQuery.AddRequirement<FZombiBehaviorFragment>(EMassFragmentAccess::ReadWrite);
    CombatQuery.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadOnly);
    CombatQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    CombatQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

    UE_LOG(LogTemp, Log, TEXT("🎮 ZombiCombatProcessor: ConfigureQueries completado - optimizado para combate"));
    UE_LOG(LogTemp, Warning, TEXT("🎮 ZombiCombatProcessor: CONFIGURACIÓN COMPLETADA - Procesador debería registrarse automáticamente"));
}

void UZombiCombatProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Log crítico para verificar que el procesador se está ejecutando
    static bool bFirstExecute = true;
    if (bFirstExecute)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 ZombiCombatProcessor: EXECUTE LLAMADO - Procesador está funcionando"));
        bFirstExecute = false;
    }

    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        return;
    }

    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Log para debugging
    static float DebugTimer = 0.0f;
    DebugTimer += DeltaTime;
    if (DebugTimer >= 6.0f) // Log cada 6 segundos
    {
        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiCombatProcessor: Ejecutándose - DeltaTime: %f"), DeltaTime);
        DebugTimer = 0.0f;
    }

    // Procesa entidades activas para combate
    CombatQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext &Context)
                                   {
        TArrayView<FZombiCombatFragment> CombatFragments = Context.GetMutableFragmentView<FZombiCombatFragment>();
        TArrayView<FZombiBehaviorFragment> BehaviorFragments = Context.GetMutableFragmentView<FZombiBehaviorFragment>();
        TArrayView<const FZombiCoreFragment> CoreFragments = Context.GetFragmentView<FZombiCoreFragment>();
        const float DeltaTime = Context.GetDeltaTimeSeconds();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiCombatFragment& CombatFragment = CombatFragments[i];
            FZombiBehaviorFragment& BehaviorFragment = BehaviorFragments[i];
            const FZombiCoreFragment& CoreFragment = CoreFragments[i];

            // Solo procesar si está vivo
            if (!CombatFragment.IsDead())
            {
                // Actualizar cooldowns de combate
                UpdateCombatCooldowns(CombatFragment, DeltaTime);

                // Procesar lógica de ataque
                ProcessAttackLogic(CombatFragment, BehaviorFragment, CoreFragment, DeltaTime);

                // Procesar lógica de daño
                ProcessDamageLogic(CombatFragment, BehaviorFragment, DeltaTime);
            }
        } });
}

void UZombiCombatProcessor::UpdateCombatCooldowns(FZombiCombatFragment &CombatFragment, float DeltaTime)
{
    // Actualizar cooldowns
    CombatFragment.UpdateCooldowns(DeltaTime);
}

void UZombiCombatProcessor::ProcessAttackLogic(FZombiCombatFragment &CombatFragment, FZombiBehaviorFragment &BehaviorFragment, const FZombiCoreFragment &CoreFragment, float DeltaTime)
{
    // Lógica simplificada de ataque
    // TODO: Implementar detección de jugador y lógica de ataque real

    // Por ahora, simular ataques aleatorios
    static float AttackTimer = 0.0f;
    AttackTimer += DeltaTime;

    if (AttackTimer > 5.0f && CombatFragment.CanAttack()) // Ataque cada 5 segundos
    {
        // Simular ataque
        CombatFragment.StartAttackCooldown();
        BehaviorFragment.SetAttackingAction(true);
        BehaviorFragment.ActionTimer = 0.0f; // Reset timer de acción

        AttackTimer = 0.0f;

        // Log para debugging
        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiCombatProcessor: Entidad atacando - Daño: %.1f"), CombatFragment.AttackDamage);
    }
}

void UZombiCombatProcessor::ProcessDamageLogic(FZombiCombatFragment &CombatFragment, FZombiBehaviorFragment &BehaviorFragment, float DeltaTime)
{
    // Lógica simplificada de daño
    // TODO: Implementar detección de daño real

    // Por ahora, simular daño aleatorio
    static float DamageTimer = 0.0f;
    DamageTimer += DeltaTime;

    if (DamageTimer > 10.0f && CombatFragment.CanTakeDamage()) // Daño cada 10 segundos
    {
        // Simular daño
        float DamageAmount = FMath::RandRange(10.0f, 30.0f);
        CombatFragment.RemoveHealth(DamageAmount);
        CombatFragment.StartDamageCooldown();
        BehaviorFragment.SetDamagedAction(true);
        BehaviorFragment.ActionTimer = 0.0f; // Reset timer de acción

        DamageTimer = 0.0f;

        // Log para debugging
        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiCombatProcessor: Entidad recibió %.1f de daño - Salud restante: %.1f"),
               DamageAmount, CombatFragment.GetRemainingHealth());

        // Verificar si murió
        if (CombatFragment.IsDead())
        {
            BehaviorFragment.SetState(EZombiState::Dead);
            BehaviorFragment.ClearAllActions();
            UE_LOG(LogTemp, Warning, TEXT("🎮 ZombiCombatProcessor: Entidad murió"));
        }
    }
}