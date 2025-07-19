// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "MyTurboSequenceAnimComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECTTSECUENCE_API UMyTurboSequenceAnimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Aquí van las funciones y variables públicas (accesibles desde fuera)
	UMyTurboSequenceAnimComponent();
	UFUNCTION(BlueprintCallable, Category = "TurboSequence")
	void SetTurboSequenceAnimation(class UAnimSequence *NewAnimation);

protected:
	// Aquí van funciones/variables protegidas (accesibles desde clases hijas)
	virtual void BeginPlay() override;

private:
	// Aquí van las variables privadas (solo accesibles dentro de esta clase)
	class ATurboSequence_Manager_Lf *TurboSequenceManager = nullptr;
	FTurboSequence_MinimalMeshData_Lf MeshData;
};
