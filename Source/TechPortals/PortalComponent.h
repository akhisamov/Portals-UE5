// Copyright (c) 2025 Amil Khisamov
// See LICENSE.txt for copyright and licensing details (standard MIT License)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PortalComponent.generated.h"

class USceneCaptureComponent2D;
class UArrowComponent;

DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FPortalComponentPlayerTeleportedSignature, UPortalComponent, OnPlayerTeleported, const FRotator&, NewControlRotation);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TECHPORTALS_API UPortalComponent : public UActorComponent
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	UMaterialInterface* MaterialInterface = nullptr;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Collision, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AActor> PlayerBlueprint;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Collision, meta = (AllowPrivateAccess = "true"))
	float OffsetAmmount = -4.0;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Portal, meta = (AllowPrivateAccess = "true"))
	AActor* LinkedPortalActor = nullptr;

	UPROPERTY(EditAnywhere, Category = Portal, meta = (AllowPrivateAccess = "true"))
	AActor* SceneCaptureFromPortal = nullptr;
	USceneComponent* SceneCaptureFrom = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneCaptureComponent2D* Camera;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UArrowComponent* ForwardDirection;

	UMaterialInstanceDynamic* Material = nullptr;
	UTextureRenderTarget2D* RenderTarget = nullptr;
	UPortalComponent* LinkedPortal = nullptr;

public:
	// Sets default values for this component's properties
	UPortalComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnComponentCreated() override;

	USceneCaptureComponent2D* GetCamera() const { return Camera; }
	FVector GetForwardVector() const;
	void HideMesh();

	UPROPERTY(BlueprintAssignable)
	FPortalComponentPlayerTeleportedSignature OnPlayerTeleported;

private:
	bool TryInitRenderTarget();
	void UpdateSceneCapture();
	FVector UpdateLocation(const FVector& OldLocation);
	FRotator UpdateRotation(const FRotator& OldRotation);

	void UpdateViewportSize();

	void TryTeleport();
	void TeleportPlayerActor(AActor* Actor);
};
