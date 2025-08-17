#include "Systems/StimulusSubsystem/StimulusSubsystem.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

UStimulusSubsystem::UStimulusSubsystem()
{
    // Configuración por defecto
    StimulusDecayRate = 1.0f; // Velocidad de decaimiento
    CleanupInterval = 1.0f;   // Intervalo de limpieza
    MaxActiveStimuli = 1000;  // Límite de estímulos activos
}

void UStimulusSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    // Log de inicialización
    UE_LOG(LogTemp, Log, TEXT("🧠 StimulusSubsystem: Inicializando"));
}

void UStimulusSubsystem::Deinitialize()
{
    // Limpiar estímulos activos
    ActiveStimuli.Empty();
    PlayerStimuli.Empty();
    EnvironmentStimuli.Empty();

    UE_LOG(LogTemp, Log, TEXT("🧠 StimulusSubsystem: Desinicializando"));

    Super::Deinitialize();
}

void UStimulusSubsystem::OnWorldBeginPlay(UWorld &InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    // Marcar sistema como inicializado
    bSystemInitialized = true;

    UE_LOG(LogTemp, Log, TEXT("🧠 StimulusSubsystem: Sistema listo para recibir estímulos"));
}

void UStimulusSubsystem::Tick(float DeltaTime)
{
    // Solo ejecutar si el sistema está inicializado
    if (!bSystemInitialized)
    {
        return;
    }

    // Actualizar decaimiento de estímulos
    UpdateStimulusDecay(DeltaTime);

    // Limpiar estímulos expirados periódicamente
    CleanupTimer += DeltaTime;
    if (CleanupTimer >= CleanupInterval)
    {
        CleanupExpiredStimuli();
        OrganizeStimuli();
        LimitActiveStimuli();
        CleanupTimer = 0.0f;
    }
}

void UStimulusSubsystem::AddPlayerStimulus(const FVector &Position, const FVector &Direction,
                                           EStimulusType Type, uint8 Intensity, float Radius)
{
    FStimulusData Stimulus(Position, Direction, Type, EStimulusSource::Player, Intensity, Radius);
    AddStimulus(Stimulus);
}

void UStimulusSubsystem::AddItemStimulus(const FVector &Position, const FVector &Direction,
                                         EStimulusType Type, uint8 Intensity, float Radius)
{
    FStimulusData Stimulus(Position, Direction, Type, EStimulusSource::Item, Intensity, Radius);
    AddStimulus(Stimulus);
}

void UStimulusSubsystem::AddAnimalStimulus(const FVector &Position, const FVector &Direction,
                                           EStimulusType Type, uint8 Intensity, float Radius)
{
    FStimulusData Stimulus(Position, Direction, Type, EStimulusSource::Animal, Intensity, Radius);
    AddStimulus(Stimulus);
}

void UStimulusSubsystem::AddEnvironmentStimulus(const FVector &Position, const FVector &Direction,
                                                EStimulusType Type, uint8 Intensity, float Radius)
{
    FStimulusData Stimulus(Position, Direction, Type, EStimulusSource::Environment, Intensity, Radius);
    AddStimulus(Stimulus);
}

void UStimulusSubsystem::AddCustomStimulus(const FVector &Position, const FVector &Direction,
                                           EStimulusType Type, EStimulusSource Source,
                                           uint8 Intensity, float Radius)
{
    FStimulusData Stimulus(Position, Direction, Type, Source, Intensity, Radius);
    AddStimulus(Stimulus);
}

void UStimulusSubsystem::AddStimulus(const FStimulusData &Stimulus)
{
    // Validar estímulo antes de agregarlo
    if (!IsStimulusValid(Stimulus))
    {
        UE_LOG(LogTemp, Warning, TEXT("🧠 StimulusSubsystem: Estímulo inválido rechazado"));
        return;
    }

    // Agregar a la lista principal
    ActiveStimuli.Add(Stimulus);

    // Organizar por categoría
    switch (Stimulus.GetStimulusSource())
    {
    case EStimulusSource::Player:
        PlayerStimuli.Add(Stimulus);
        break;
    case EStimulusSource::Environment:
    case EStimulusSource::Item:
    case EStimulusSource::Animal:
    case EStimulusSource::Explosion:
        EnvironmentStimuli.Add(Stimulus);
        break;
    default:
        break;
    }

    // Log para debugging (opcional)
    UE_LOG(LogTemp, Verbose, TEXT("🧠 StimulusSubsystem: Estímulo agregado - Tipo: %d, Fuente: %d, Intensidad: %d, Radio: %.1f"),
           Stimulus.GetStimulusType(), Stimulus.GetStimulusSource(), Stimulus.Intensity, Stimulus.Radius);

    // DEBUG: Log para verificar que se están recibiendo estímulos
    static int32 DebugCounter = 0;
    if (++DebugCounter % 50 == 0) // Log cada 5 segundos (50 * 0.1s)
    {
        UE_LOG(LogTemp, Log, TEXT("🧠 StimulusSubsystem: Estímulo recibido - Tipo: %d, Fuente: %d, Pos: %s"),
               Stimulus.GetStimulusType(), Stimulus.GetStimulusSource(), *Stimulus.Position.ToString());
    }
}

void UStimulusSubsystem::CleanupExpiredStimuli()
{
    // Remover estímulos expirados de todas las listas
    ActiveStimuli.RemoveAll([](const FStimulusData &Stimulus)
                            { return Stimulus.HasExpired(); });

    PlayerStimuli.RemoveAll([](const FStimulusData &Stimulus)
                            { return Stimulus.HasExpired(); });

    EnvironmentStimuli.RemoveAll([](const FStimulusData &Stimulus)
                                 { return Stimulus.HasExpired(); });
}

TArray<FStimulusData> UStimulusSubsystem::GetStimuliInRange(const FVector &Position, float Range) const
{
    TArray<FStimulusData> StimuliInRange;

    for (const FStimulusData &Stimulus : ActiveStimuli)
    {
        float Distance = FVector::Dist(Position, Stimulus.Position);
        if (Distance <= Range && Distance <= Stimulus.Radius)
        {
            StimuliInRange.Add(Stimulus);
        }
    }

    return StimuliInRange;
}

TArray<FStimulusData> UStimulusSubsystem::GetStimuliByType(EStimulusType Type) const
{
    TArray<FStimulusData> StimuliByType;

    for (const FStimulusData &Stimulus : ActiveStimuli)
    {
        if (Stimulus.GetStimulusType() == Type)
        {
            StimuliByType.Add(Stimulus);
        }
    }

    return StimuliByType;
}

TArray<FStimulusData> UStimulusSubsystem::GetStimuliBySource(EStimulusSource Source) const
{
    TArray<FStimulusData> StimuliBySource;

    for (const FStimulusData &Stimulus : ActiveStimuli)
    {
        if (Stimulus.GetStimulusSource() == Source)
        {
            StimuliBySource.Add(Stimulus);
        }
    }

    return StimuliBySource;
}

void UStimulusSubsystem::UpdateStimulusDecay(float DeltaTime)
{
    // Actualizar decaimiento de todos los estímulos activos
    for (FStimulusData &Stimulus : ActiveStimuli)
    {
        Stimulus.DecayTimer += static_cast<uint8>(DeltaTime * StimulusDecayRate * 100.0f);
    }

    // Actualizar decaimiento en listas organizadas
    for (FStimulusData &Stimulus : PlayerStimuli)
    {
        Stimulus.DecayTimer += static_cast<uint8>(DeltaTime * StimulusDecayRate * 100.0f);
    }

    for (FStimulusData &Stimulus : EnvironmentStimuli)
    {
        Stimulus.DecayTimer += static_cast<uint8>(DeltaTime * StimulusDecayRate * 100.0f);
    }
}

void UStimulusSubsystem::OrganizeStimuli()
{
    // Reorganizar estímulos por categoría (en caso de cambios)
    PlayerStimuli.Empty();
    EnvironmentStimuli.Empty();

    for (const FStimulusData &Stimulus : ActiveStimuli)
    {
        switch (Stimulus.GetStimulusSource())
        {
        case EStimulusSource::Player:
            PlayerStimuli.Add(Stimulus);
            break;
        case EStimulusSource::Environment:
        case EStimulusSource::Item:
        case EStimulusSource::Animal:
        case EStimulusSource::Explosion:
            EnvironmentStimuli.Add(Stimulus);
            break;
        default:
            break;
        }
    }
}

void UStimulusSubsystem::LimitActiveStimuli()
{
    // Limitar número de estímulos activos para performance
    if (ActiveStimuli.Num() > MaxActiveStimuli)
    {
        // Remover estímulos más antiguos (con mayor decaimiento)
        ActiveStimuli.Sort([](const FStimulusData &A, const FStimulusData &B)
                           { return A.DecayTimer > B.DecayTimer; });

        // Mantener solo los más recientes
        ActiveStimuli.SetNum(MaxActiveStimuli);

        // Reorganizar después de la limpieza
        OrganizeStimuli();
    }
}

bool UStimulusSubsystem::IsStimulusValid(const FStimulusData &Stimulus) const
{
    return Stimulus.IsValid() &&
           Stimulus.Position != FVector::ZeroVector &&
           Stimulus.Intensity > 0 &&
           Stimulus.Radius > 0.0f;
}
