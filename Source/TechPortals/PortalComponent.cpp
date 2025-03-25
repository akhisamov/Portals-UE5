// Copyright (c) 2025 Amil Khisamov
// See LICENSE.txt for copyright and licensing details (standard MIT License)

#include "PortalComponent.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetMathLibrary.h"

static FRotator TransformRotation(const FRotator& OriginalRotation, const FTransform& FromTransform, const FTransform& ToTransform)
{
	FVector X;
	FVector Y;
	FVector Z;
	FRotationMatrix(OriginalRotation).GetScaledAxes(X, Y, Z);

	const FVector OriginalForward = FromTransform
		.InverseTransformVectorNoScale(X)
		.MirrorByVector(FVector(1.0, 0.0, 0.0))
		.MirrorByVector(FVector(0.0, 1.0, 0.0));
	const FVector OriginalRight = FromTransform
		.InverseTransformVectorNoScale(Y)
		.MirrorByVector(FVector(1.0, 0.0, 0.0))
		.MirrorByVector(FVector(0.0, 1.0, 0.0));
	const FVector OriginalUp = FromTransform
		.InverseTransformVectorNoScale(Z)
		.MirrorByVector(FVector(1.0, 0.0, 0.0))
		.MirrorByVector(FVector(0.0, 1.0, 0.0));
	
	FVector Forward = ToTransform.TransformVectorNoScale(OriginalForward);
	Forward.Normalize();
	FVector Right = ToTransform.TransformVectorNoScale(OriginalRight);
	Right.Normalize();
	FVector Up = ToTransform.TransformVectorNoScale(OriginalUp);
	Up.Normalize();

	return FMatrix(Forward, Right, Up, FVector::ZeroVector).Rotator();
}

UPortalComponent::UPortalComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(TEXT("OverlapAllDynamic"), true);

	Camera = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Camera"));
	Camera->FOVAngle = 80.0f;
	Camera->CompositeMode = ESceneCaptureCompositeMode::SCCM_Composite;
	Camera->bCaptureEveryFrame = false;
	Camera->bCaptureOnMovement = false;
	Camera->bAlwaysPersistRenderingState = true;

	ForwardDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("ForwardDirection"));
}

void UPortalComponent::BeginPlay()
{
	Super::BeginPlay();

	SetTickGroup(ETickingGroup::TG_PostUpdateWork);

	Material = UMaterialInstanceDynamic::Create(MaterialInterface, this);
	if (Material)
	{
		Mesh->SetMaterial(0, Material);
	}

	if (LinkedPortalActor)
	{
		LinkedPortal = LinkedPortalActor->GetComponentByClass<UPortalComponent>();
	}
	else
	{
		Mesh->SetVisibility(false);
	}

	// TODO Auto detection
	if (SceneCaptureFromPortal)
	{
		if (auto* PortalComponent = SceneCaptureFromPortal->GetComponentByClass<UPortalComponent>())
		{
			SceneCaptureFrom = PortalComponent->GetCamera();
		}
	}

	TryInitRenderTarget();

	Camera->bEnableClipPlane = true;
	Camera->ClipPlaneBase = Mesh->GetComponentLocation() + (ForwardDirection->GetForwardVector() * -3.0);
	Camera->ClipPlaneNormal = ForwardDirection->GetForwardVector();

	const FVector OffsetVector = ForwardDirection->GetForwardVector() * OffsetAmmount;
	Material->SetVectorParameterValue(TEXT("OffsetDistance"), FVector4(OffsetVector));
}

void UPortalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (LinkedPortalActor)
	{
		UpdateSceneCapture();
		if (RenderTarget || TryInitRenderTarget())
		{
			UpdateViewportSize();
		}
		TryTeleport();
	}
}

void UPortalComponent::OnComponentCreated()
{
	Super::OnComponentCreated();

	Mesh->SetupAttachment(GetOwner()->GetRootComponent());
	Camera->SetupAttachment(GetOwner()->GetRootComponent());
	ForwardDirection->SetupAttachment(GetOwner()->GetRootComponent());
}

FVector UPortalComponent::GetForwardVector() const
{
	return ForwardDirection->GetForwardVector();
}

void UPortalComponent::HideMesh()
{
	Mesh->SetVisibility(false);
}

bool UPortalComponent::TryInitRenderTarget()
{
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(GetWorld());
	if (ViewportSize.IsZero())
	{
		return false;
	}
	RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(),
		static_cast<int32>(ViewportSize.X), static_cast<int32>(ViewportSize.Y));
	if (Material)
	{
		Material->SetTextureParameterValue(TEXT("Texture"), RenderTarget);
	}
	if (LinkedPortal)
	{
		LinkedPortal->GetCamera()->TextureTarget = RenderTarget;
	}
	return true;
}

void UPortalComponent::UpdateSceneCapture()
{
	APlayerCameraManager* PlayerCamera = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	USceneComponent* PlayerCameraTransform = PlayerCamera->GetTransformComponent();
	USceneComponent* CameraTransformComponent = nullptr;

	if (SceneCaptureFrom &&
		FVector::Distance(PlayerCameraTransform->GetComponentLocation(), GetOwner()->GetActorLocation()) > FVector::Distance(SceneCaptureFrom->GetComponentLocation(), GetOwner()->GetActorLocation()))
	{
		CameraTransformComponent = SceneCaptureFrom;
	}
	else
	{
		CameraTransformComponent = PlayerCameraTransform;
	}

	const FVector TempLocation = UpdateLocation(CameraTransformComponent->GetComponentLocation());
	const FRotator TempRotation = UpdateRotation(CameraTransformComponent->GetComponentRotation());

	LinkedPortal->GetCamera()->SetWorldLocationAndRotation(TempLocation, TempRotation);

	Mesh->SetVisibility(false);
	LinkedPortal->GetCamera()->CaptureScene();
	Mesh->SetVisibility(true);
}

FVector UPortalComponent::UpdateLocation(const FVector& OldLocation)
{
	FTransform Transform = GetOwner()->GetActorTransform();
	FVector NewScale = Transform.GetScale3D();
	NewScale.X *= -1;
	NewScale.Y *= -1;
	Transform.SetScale3D(NewScale);

	const FVector InvertedTransformPosition = Transform.InverseTransformPosition(OldLocation);
	return LinkedPortalActor->GetActorTransform().TransformPosition(InvertedTransformPosition);
}

FRotator UPortalComponent::UpdateRotation(const FRotator& OldRotation)
{
	return TransformRotation(OldRotation, GetOwner()->GetActorTransform(), LinkedPortalActor->GetActorTransform());
}

void UPortalComponent::UpdateViewportSize()
{
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(GetWorld());
	if (RenderTarget->SizeX != ViewportSize.X || RenderTarget->SizeY != ViewportSize.Y)
	{
		RenderTarget->ResizeTarget(
			static_cast<uint32>(ViewportSize.X),
			static_cast<uint32>(ViewportSize.Y)
		);
	}
}

void UPortalComponent::TryTeleport()
{
	TArray<AActor*> OverlappingActors;
	Mesh->GetOverlappingActors(OverlappingActors, PlayerBlueprint);
	if (OverlappingActors.IsEmpty())
	{
		return;
	}

	AActor* Actor = OverlappingActors[0];
	if (!IsValid(Actor))
	{
		return;
	}

	if (ForwardDirection->GetForwardVector().Dot(Actor->GetActorLocation() - GetOwner()->GetActorLocation()) < 0.0)
	{
		TeleportPlayerActor(Actor);
	}
}

void UPortalComponent::TeleportPlayerActor(AActor* Actor)
{
	FTransform ActorTransform;

	{
		FTransform Transform = GetOwner()->GetActorTransform();
		FVector NewScale = Transform.GetScale3D();
		NewScale.X *= -1;
		NewScale.Y *= -1;
		Transform.SetScale3D(NewScale);

		const FVector NewLocation = LinkedPortalActor->GetActorTransform().TransformPosition(
			Transform.InverseTransformPosition(Actor->GetActorLocation()));
		ActorTransform.SetLocation(NewLocation);
	}

	{
		const FRotator NewRotation = TransformRotation(
			Actor->GetActorRotation(), GetOwner()->GetActorTransform(), LinkedPortalActor->GetActorTransform());
		ActorTransform.SetRotation(FQuat::MakeFromRotator(NewRotation));
	}

	Actor->SetActorTransform(ActorTransform);

	FRotator NewControlRotation = FRotator::ZeroRotator;
	if (auto* Pawn = Cast<APawn>(Actor))
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController()))
		{
			NewControlRotation = TransformRotation(PlayerController->GetControlRotation(), GetOwner()->GetActorTransform(), LinkedPortalActor->GetActorTransform());
			PlayerController->SetControlRotation(NewControlRotation);
		}
		if (UPawnMovementComponent* MovementComponent = Pawn->GetMovementComponent())
		{
			FVector Velocity = MovementComponent->Velocity;
			Velocity.Normalize();
			const FVector Direction = GetOwner()->GetActorTransform()
				.InverseTransformVectorNoScale(Velocity)
				.MirrorByVector(FVector(1.0, 0.0, 0.0))
				.MirrorByVector(FVector(0.0, 1.0, 0.0));
			const FVector NewVelocity =
				LinkedPortalActor->GetActorTransform().TransformVectorNoScale(Direction)
				* MovementComponent->Velocity.Length();
			MovementComponent->Velocity = NewVelocity;
		}
	}
	OnPlayerTeleported.Broadcast(NewControlRotation);
}
