#include "Systems/StimulusSubsystem/PlayerSignalSubsystem.h"
#include "Systems/StimulusSubsystem/StimulusSubsystem.h"
#include "MyProjectTSecuence/MyProjectTSecuenceCharacter.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

UPlayerSignalSubsystem::UPlayerSignalSubsystem()
{
    // Configuración por defecto
    PositionUpdateInterval = 0.1f; // 10 FPS para posición
    MovementThreshold = 10.0f;     // Umbral para detectar movimiento
}

void UPlayerSignalSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    // Log de inicialización
    UE_LOG(LogTemp, Log, TEXT("🎮 PlayerSignalSubsystem: Inicializando"));
}

void UPlayerSignalSubsystem::Deinitialize()
{
    UE_LOG(LogTemp, Log, TEXT("🎮 PlayerSignalSubsystem: Desinicializando"));

    Super::Deinitialize();
}

void UPlayerSignalSubsystem::OnWorldBeginPlay(UWorld &InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    // Obtener referencia al personaje del jugador
    PlayerCharacter = Cast<AMyProjectTSecuenceCharacter>(UGameplayStatics::GetPlayerCharacter(&InWorld, 0));

    // Obtener referencia al StimulusSubsystem
    StimulusSubsystem = GetWorld()->GetSubsystem<UStimulusSubsystem>();

    if (PlayerCharacter && StimulusSubsystem)
    {
        // Inicializar posición del jugador
        CurrentPlayerPosition = PlayerCharacter->GetActorLocation();
        LastPlayerPosition = CurrentPlayerPosition;
        bSystemInitialized = true;

        UE_LOG(LogTemp, Log, TEXT("🎮 PlayerSignalSubsystem: Jugador y StimulusSubsystem encontrados en %s"), *CurrentPlayerPosition.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("🎮 PlayerSignalSubsystem: No se encontró al jugador o StimulusSubsystem"));
    }
}

void UPlayerSignalSubsystem::Tick(float DeltaTime)
{
    // Solo ejecutar si el sistema está inicializado
    if (!bSystemInitialized || !IsPlayerValid() || !IsStimulusSubsystemValid())
    {
        return;
    }

    // Actualizar posición del jugador
    UpdatePlayerPosition();

    // Detectar movimiento del jugador
    DetectPlayerMovement();

    // Emitir señal de posición periódicamente
    PositionUpdateTimer += DeltaTime;
    if (PositionUpdateTimer >= PositionUpdateInterval)
    {
        EmitPositionSignal();
        PositionUpdateTimer = 0.0f;
    }
}

void UPlayerSignalSubsystem::EmitPositionSignal()
{
    if (!IsPlayerValid() || !IsStimulusSubsystemValid())
    {
        return;
    }

    // Emitir estímulo de posición del jugador
    StimulusSubsystem->AddPlayerStimulus(
        CurrentPlayerPosition,
        FVector::ZeroVector, // Sin dirección específica
        EStimulusType::Visual,
        50,     // Intensidad baja para posición
        1000.0f // Radio grande para posición
    );
}

void UPlayerSignalSubsystem::EmitMovementSignal()
{
    if (!IsPlayerValid() || !IsStimulusSubsystemValid() || !bPlayerHasMoved)
    {
        return;
    }

    // Calcular dirección del movimiento
    FVector MovementDirection = (CurrentPlayerPosition - LastPlayerPosition).GetSafeNormal();

    // Emitir estímulo de movimiento del jugador
    StimulusSubsystem->AddPlayerStimulus(
        CurrentPlayerPosition,
        MovementDirection,
        EStimulusType::Visual,
        75,    // Intensidad media para movimiento
        600.0f // Radio medio para movimiento
    );

    // Resetear flag de movimiento
    bPlayerHasMoved = false;
}

void UPlayerSignalSubsystem::EmitSoundSignal(EStimulusType SoundType, uint8 Intensity, float Radius)
{
    if (!IsPlayerValid() || !IsStimulusSubsystemValid())
    {
        return;
    }

    // Emitir estímulo de sonido del jugador
    StimulusSubsystem->AddPlayerStimulus(
        CurrentPlayerPosition,
        FVector::ZeroVector, // Sin dirección específica
        SoundType,
        Intensity,
        Radius);
}

void UPlayerSignalSubsystem::EmitActionSignal(EStimulusType ActionType, uint8 Intensity, float Radius)
{
    if (!IsPlayerValid() || !IsStimulusSubsystemValid())
    {
        return;
    }

    // Emitir estímulo de acción del jugador
    StimulusSubsystem->AddPlayerStimulus(
        CurrentPlayerPosition,
        FVector::ZeroVector, // Sin dirección específica
        ActionType,
        Intensity,
        Radius);
}

void UPlayerSignalSubsystem::UpdatePlayerPosition()
{
    if (!IsPlayerValid())
    {
        return;
    }

    // Actualizar posición anterior
    LastPlayerPosition = CurrentPlayerPosition;

    // Obtener posición actual
    CurrentPlayerPosition = PlayerCharacter->GetActorLocation();
}

void UPlayerSignalSubsystem::DetectPlayerMovement()
{
    if (!IsPlayerValid())
    {
        return;
    }

    // Calcular distancia movida
    float DistanceMoved = FVector::Dist(CurrentPlayerPosition, LastPlayerPosition);

    // Detectar movimiento significativo
    if (DistanceMoved > MovementThreshold)
    {
        bPlayerHasMoved = true;

        // Emitir señal de movimiento automáticamente
        EmitMovementSignal();
    }
}

bool UPlayerSignalSubsystem::IsPlayerValid() const
{
    return PlayerCharacter != nullptr && PlayerCharacter->IsValidLowLevel();
}

bool UPlayerSignalSubsystem::IsStimulusSubsystemValid() const
{
    return StimulusSubsystem != nullptr && StimulusSubsystem->IsValidLowLevel();
}
