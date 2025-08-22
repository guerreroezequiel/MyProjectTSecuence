#pragma once

#include "CoreMinimal.h"
#include "StimulusTypes.generated.h"

/**
 * Tipos de estímulos que pueden recibir los zombis
 * Optimizado para ECS - usando flags en lugar de enums complejos
 * Escalable para futuros tipos de estímulos
 */
UENUM(BlueprintType)
enum class EStimulusType : uint8
{
    None = 0,
    Visual = 1,        // Estímulo visual (ver algo)
    Auditory = 2,      // Estímulo auditivo (escuchar algo)
    Tactile = 3,       // Estímulo táctil (tocar algo)
    Olfactory = 4,     // Estímulo olfativo (oler algo)
    Environmental = 5, // Estímulo ambiental (temperatura, etc.)
    Custom = 6         // Estímulo personalizado
};

/**
 * Fuentes de estímulos
 * Escalable para futuras fuentes
 */
UENUM(BlueprintType)
enum class EStimulusSource : uint8
{
    None = 0,
    Player = 1,      // Jugador
    Item = 2,        // Item/objeto
    Animal = 3,      // Animal
    Environment = 4, // Entorno
    Zombie = 5,      // Otro zombie
    Explosion = 6,   // Explosión
    Custom = 7       // Fuente personalizada
};

/**
 * Estructura de datos para un estímulo
 * Ultra-optimizada para ECS - 32 bytes máximo
 * Escalable para cualquier tipo de estímulo
 */
USTRUCT(BlueprintType)
struct FStimulusData
{
    GENERATED_BODY()

    // Posición del estímulo (12 bytes)
    UPROPERTY()
    FVector Position = FVector::ZeroVector;

    // Dirección del estímulo (12 bytes)
    UPROPERTY()
    FVector Direction = FVector::ZeroVector;

    // Tipo de estímulo (1 byte)
    UPROPERTY()
    uint8 StimulusType = 0;

    // Fuente del estímulo (1 byte)
    UPROPERTY()
    uint8 StimulusSource = 0;

    // Intensidad del estímulo (1 byte) - 0-255
    UPROPERTY()
    uint8 Intensity = 0;

    // Radio de propagación (4 bytes)
    UPROPERTY()
    float Radius = 0.0f;

    // Timer de decaimiento (1 byte)
    UPROPERTY()
    uint8 DecayTimer = 0;

    // Constructor por defecto
    FStimulusData()
    {
        Position = FVector::ZeroVector;
        Direction = FVector::ZeroVector;
        StimulusType = 0;
        StimulusSource = 0;
        Intensity = 0;
        Radius = 0.0f;
        DecayTimer = 0;
    }

    // Constructor con parámetros
    FStimulusData(const FVector &InPosition, const FVector &InDirection,
                  EStimulusType InType, EStimulusSource InSource,
                  uint8 InIntensity, float InRadius)
    {
        Position = InPosition;
        Direction = InDirection;
        StimulusType = static_cast<uint8>(InType);
        StimulusSource = static_cast<uint8>(InSource);
        Intensity = InIntensity;
        Radius = InRadius;
        DecayTimer = 0;
    }

    // Getters
    EStimulusType GetStimulusType() const { return static_cast<EStimulusType>(StimulusType); }
    EStimulusSource GetStimulusSource() const { return static_cast<EStimulusSource>(StimulusSource); }
    bool IsValid() const { return StimulusType != 0 && Intensity > 0; }
    bool HasExpired() const { return DecayTimer >= 255; }

    // Setters
    void SetStimulusType(EStimulusType Type) { StimulusType = static_cast<uint8>(Type); }
    void SetStimulusSource(EStimulusSource Source) { StimulusSource = static_cast<uint8>(Source); }
    void SetIntensity(uint8 InIntensity) { Intensity = InIntensity; }
    void SetRadius(float InRadius) { Radius = InRadius; }
};

/**
 * Grid espacial para optimizar queries de estímulos
 * Cambia complejidad de O(n·m) a O(n + m)
 */
USTRUCT()
struct FStimulusGrid
{
    GENERATED_BODY()

    // Configuración del grid
    static constexpr int32 GRID_SIZE = 32;                     // 32x32 celdas
    static constexpr float CELL_SIZE = 100.0f;                 // 100 unidades por celda
    static constexpr float WORLD_SIZE = GRID_SIZE * CELL_SIZE; // 3200x3200 unidades

    // Grid de estímulos por celda
    TArray<FStimulusData> Grid[GRID_SIZE][GRID_SIZE];

    // Constructor
    FStimulusGrid()
    {
        // Inicializar todas las celdas vacías
        for (int32 X = 0; X < GRID_SIZE; ++X)
        {
            for (int32 Y = 0; Y < GRID_SIZE; ++Y)
            {
                Grid[X][Y].Empty();
            }
        }
    }

    // Convertir posición mundial a coordenadas de grid
    FORCEINLINE FIntPoint WorldToGrid(const FVector &WorldPosition) const
    {
        int32 GridX = FMath::Clamp(FMath::FloorToInt(WorldPosition.X / CELL_SIZE), 0, GRID_SIZE - 1);
        int32 GridY = FMath::Clamp(FMath::FloorToInt(WorldPosition.Y / CELL_SIZE), 0, GRID_SIZE - 1);
        return FIntPoint(GridX, GridY);
    }

    // Obtener celdas vecinas (incluyendo la celda actual)
    FORCEINLINE void GetNeighborCells(const FVector &WorldPosition, TArray<FIntPoint> &OutCells) const
    {
        FIntPoint CenterCell = WorldToGrid(WorldPosition);
        OutCells.Empty();
        OutCells.Reserve(9); // 3x3 = 9 celdas

        for (int32 DX = -1; DX <= 1; ++DX)
        {
            for (int32 DY = -1; DY <= 1; ++DY)
            {
                int32 X = CenterCell.X + DX;
                int32 Y = CenterCell.Y + DY;

                if (X >= 0 && X < GRID_SIZE && Y >= 0 && Y < GRID_SIZE)
                {
                    OutCells.Add(FIntPoint(X, Y));
                }
            }
        }
    }

    // Agregar estímulo al grid
    FORCEINLINE void AddStimulus(const FStimulusData &Stimulus)
    {
        FIntPoint Cell = WorldToGrid(Stimulus.Position);
        if (Cell.X >= 0 && Cell.X < GRID_SIZE && Cell.Y >= 0 && Cell.Y < GRID_SIZE)
        {
            Grid[Cell.X][Cell.Y].Add(Stimulus);
        }
    }

    // Obtener estímulos en rango (optimizado)
    FORCEINLINE void GetStimuliInRange(const FVector &Position, float Range, TArray<FStimulusData> &OutStimuli) const
    {
        OutStimuli.Empty();

        // Obtener celdas vecinas
        TArray<FIntPoint> NeighborCells;
        GetNeighborCells(Position, NeighborCells);

        // Buscar estímulos en celdas vecinas
        for (const FIntPoint &Cell : NeighborCells)
        {
            const TArray<FStimulusData> &CellStimuli = Grid[Cell.X][Cell.Y];

            for (const FStimulusData &Stimulus : CellStimuli)
            {
                float Distance = FVector::Dist(Position, Stimulus.Position);
                if (Distance <= Range && Distance <= Stimulus.Radius)
                {
                    OutStimuli.Add(Stimulus);
                }
            }
        }
    }

    // Limpiar estímulos expirados
    FORCEINLINE void CleanupExpiredStimuli()
    {
        for (int32 X = 0; X < GRID_SIZE; ++X)
        {
            for (int32 Y = 0; Y < GRID_SIZE; ++Y)
            {
                Grid[X][Y].RemoveAll([](const FStimulusData &Stimulus)
                                     { return Stimulus.HasExpired(); });
            }
        }
    }

    // Limitar número de estímulos por celda
    FORCEINLINE void LimitStimuliPerCell(int32 MaxPerCell = 10)
    {
        for (int32 X = 0; X < GRID_SIZE; ++X)
        {
            for (int32 Y = 0; Y < GRID_SIZE; ++Y)
            {
                if (Grid[X][Y].Num() > MaxPerCell)
                {
                    // Mantener solo los estímulos más intensos
                    Grid[X][Y].Sort([](const FStimulusData &A, const FStimulusData &B)
                                    { return A.Intensity > B.Intensity; });
                    Grid[X][Y].SetNum(MaxPerCell);
                }
            }
        }
    }
};
