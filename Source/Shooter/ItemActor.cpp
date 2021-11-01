#include "ItemActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ShooterCharacter.h"
#include "Components/WidgetComponent.h"
#include "Sound/SoundCue.h"
#include "Curves/CurveVector.h"

AItemActor::AItemActor() :
	ItemName(FString("Default Name")),
	BulletsAmount(0),
	ItemRarity(EItemRarity::EIR_Common),
	ItemState(EItemState::EIS_Pickup),
	// Item interp variables
	ZCurveTime(0.7f),
	ItemInterpStartLocation(FVector(0.f)),
	CameraTargetLocation(FVector(0.f)),
	bIsInterping(false),
	ItemInterpX(0.f),
	ItemInterpY(0.f),
	InterpInitialYawOffset(0.f),
	ItemType(EItemType::EIT_Ammo),
	InterpLocationIndex(0),
	MaterialIndex(0),
	bCanChangeCustomDepth(true),
	GlowAmmount(150.f),
	FresnelExponent(3.f),
	FresnelReflectFraction(4.f),
	PulseCurveTime(5.f),
	SlotIndex(0)
{
	PrimaryActorTick.bCanEverTick = true;

	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMesh);


	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(GetRootComponent());
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	PickUpWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickUpWidget"));
	PickUpWidget->SetupAttachment(GetRootComponent());

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(GetRootComponent());
}

void AItemActor::BeginPlay()
{
	Super::BeginPlay();
	PickUpWidget->SetVisibility(false);

	// Setup overlap for area sphere

	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AItemActor::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AItemActor::OnSphereEndOverlap);

	// set active stars array based on item rarity
	if (PickUpWidget) SetActiveStars();

	SetItemProperties(ItemState);

	// Set Custom Depth to disabled
	InitializeCustomDepth();

	StartPulseTimer();
}

void AItemActor::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor)
	{
		AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
		if (ShooterCharacter)
		{
			ShooterCharacter->IncrementOverlappedItemCounter(1);
		}
	}
}

void AItemActor::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
		if (ShooterCharacter)
		{
			ShooterCharacter->IncrementOverlappedItemCounter(-1);
		}
	}
}

void AItemActor::SetActiveStars()
{
	for (int32 i = 0; i <= 5; i++)
	{
		ActiveStars.Add(false);
	}

	switch (ItemRarity)
	{
	case EItemRarity::EIR_Damaged:
		ActiveStars[1] = true;
		break;

	case EItemRarity::EIR_Common:
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		break;

	case EItemRarity::EIR_Uncommon:
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		ActiveStars[3] = true;
		break;

	case EItemRarity::EIR_Rare:
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		ActiveStars[3] = true;
		ActiveStars[4] = true;
		break;
	case EItemRarity::EIR_Legendary:
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		ActiveStars[3] = true;
		ActiveStars[4] = true;
		ActiveStars[5] = true;
		break;
	}
}

void AItemActor::SetItemProperties(EItemState State)
{
	switch (State)
	{
	case EItemState::EIS_Pickup:
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// Set AreaSpehere properties
		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;

	case EItemState::EIS_Equipped:
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PickUpWidget->SetVisibility(false);

		// Set AreaSpehere properties
		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;

	case EItemState::EIS_Falling:
		ItemMesh->SetSimulatePhysics(true);
		ItemMesh->SetEnableGravity(true);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);

		// Set AreaSpehere properties
		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EItemState::EIS_EquipInterping:
		PickUpWidget->SetVisibility(false);

		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PickUpWidget->SetVisibility(false);

		// Set AreaSpehere properties
		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;

	case EItemState::EIS_PickedUp:
		PickUpWidget->SetVisibility(false);

		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetVisibility(false);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PickUpWidget->SetVisibility(false);

		// Set AreaSpehere properties
		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	}
}

void AItemActor::FinishInterping()
{
	bIsInterping = false;
	if (Character)
	{
		// some garbage shit that need to be refactored in future. TODO: rid of this shit and refactor
		Character->IncrementInterpLocationItemCount(InterpLocationIndex, -1);
		Character->GetPickUpItem(this);
	}

	// Set scale back to normal
	SetActorScale3D(FVector(1.f));
	
	DisableGlowMaterial();
	bCanChangeCustomDepth = true;
	DisableCustomDepth();
}

void AItemActor::ItemInterp(float DeltaTime)
{
	if (bIsInterping)
	{
		if (Character && ItemZCurve)
		{
			// Elapsed time since we started ItemInterpTimer
			const float ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);

			// Get curve value corresponding to elapsed time
			const float CurveValue = ItemZCurve->GetFloatValue(ElapsedTime);

			// Get the item initial location when curve started
			FVector ItemLocation = ItemInterpStartLocation;
			const FVector CameraInterpLocation = GetInterpLocation();

			// idk what that mean and why we need it yet. TODO: discover why we need it
			// vector from item to camera interp location, X and Y are zeroed out
			const FVector ItemToCamera = FVector(0.f, 0.f, (CameraInterpLocation - ItemLocation).Z);

			// Scale factor to multiply with CurveValue
			const float DeltaZ = ItemToCamera.Size();

			const FVector CurrentLocation = GetActorLocation();

			// Interpolate X and Y values
			const float InterpXValue = FMath::FInterpTo(CurrentLocation.X, CameraInterpLocation.X, DeltaTime, 30.f);
			const float InterpYValue = FMath::FInterpTo(CurrentLocation.Y, CameraInterpLocation.Y, DeltaTime, 30.f);

			// Adding curve value to the Z component of the Initial Location
			ItemLocation.Z += CurveValue * DeltaZ;
			ItemLocation.X = InterpXValue;
			ItemLocation.Y = InterpYValue;

			SetActorLocation(ItemLocation, true, nullptr, ETeleportType::TeleportPhysics);

			// Camera rotation this frame + initial yaw offset
			const FRotator CameraRotation = Character->GetFollowCamera()->GetComponentRotation();
			FRotator ItemRotation(0.f, CameraRotation.Yaw + InterpInitialYawOffset, 0.f);

			SetActorRotation(ItemRotation, ETeleportType::TeleportPhysics);

			if (ItemScaleCurve)
			{
				const float ScaleCurveValue = ItemScaleCurve->GetFloatValue(ElapsedTime);
				SetActorScale3D(FVector(ScaleCurveValue, ScaleCurveValue, ScaleCurveValue));
			}
		}
	}
}

FVector AItemActor::GetInterpLocation()
{
	if (!Character) return FVector(0.f);

	switch (ItemType)
	{
	case EItemType::EIT_Ammo:
		return Character->GetInterpLocationByIndex(InterpLocationIndex).SceneComponent->GetComponentLocation();
		break;

	case EItemType::EIT_Weapon:
		return Character->GetInterpLocationByIndex(0).SceneComponent->GetComponentLocation();
		break;
	}
	return FVector();
}

void AItemActor::PlayPickUpSound()
{
	if (Character && Character->ShouldPlayPickUpSound())
	{
		Character->StartPickSoundTime();
		if (PickUpSound)
		{
			UGameplayStatics::PlaySound2D(this, PickUpSound);
		}
	}
}

void AItemActor::EnableCustomDepth()
{
	if (bCanChangeCustomDepth)
	{
		ItemMesh->SetRenderCustomDepth(true);
	}
}

void AItemActor::DisableCustomDepth()
{
	if (bCanChangeCustomDepth)
	{
		ItemMesh->SetRenderCustomDepth(false);
	}
}

void AItemActor::InitializeCustomDepth()
{
	DisableCustomDepth();
}

void AItemActor::OnConstruction(const FTransform& Transform)
{
	if (MaterialInstance)
	{
		DynamicMaterialInstance = UMaterialInstanceDynamic::Create(MaterialInstance, this); 
		ItemMesh->SetMaterial(MaterialIndex, DynamicMaterialInstance);
	}
	EnableGlowMaterial();
}

void AItemActor::UpdateCurvePulse()
{
	float ElapsedTime{};
	FVector CurveValue{};

	switch (ItemState)
	{
	case EItemState::EIS_Pickup:
		if (PulseCurve)
		{
			ElapsedTime = GetWorldTimerManager().GetTimerElapsed(PulseCurveTimer);
			CurveValue = PulseCurve->GetVectorValue(ElapsedTime);
		}
		break;

	case EItemState::EIS_EquipInterping:
		if (InterpMaterialPulseCurve)
		{
			ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);
			CurveValue = InterpMaterialPulseCurve->GetVectorValue(ElapsedTime);
		}
		break;
	}

	if (!DynamicMaterialInstance) return;

	DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowAmount"), CurveValue.X * GlowAmmount);
	DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelExponent"), CurveValue.Y * FresnelExponent);
	DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelReflectFraction"), CurveValue.Z * FresnelReflectFraction);
}

void AItemActor::EnableGlowMaterial()
{
	if (DynamicMaterialInstance)
	{
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 0);
	}
}

void AItemActor::DisableGlowMaterial()
{
	if (DynamicMaterialInstance)
	{
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 1);
	}
}

void AItemActor::PlayEquipSound()
{
	if (Character && Character->ShouldPLayEquipSound())
	{
		Character->StartEquipSoundTimer();
		if (EquipSound)
		{
			UGameplayStatics::PlaySound2D(this, EquipSound);
		}
	}
}

void AItemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ItemInterp(DeltaTime);

	// get curve values from pulse curve and set dynamic material parameters
	UpdateCurvePulse();
}

void AItemActor::ResetPulseTimer()
{
	StartPulseTimer();
}

void AItemActor::StartPulseTimer()
{
	if (ItemState == EItemState::EIS_Pickup)
	{
		GetWorldTimerManager().SetTimer(PulseCurveTimer, this, &AItemActor::ResetPulseTimer, PulseCurveTime);
	}
}

void AItemActor::SetItemState(EItemState State)
{
	ItemState = State;
	SetItemProperties(State);
}

void AItemActor::StartItemCurve(AShooterCharacter* Char)
{
	// Store handle to the Character
	Character = Char;

	// Get array index in interp location with a lowest item count
	InterpLocationIndex = Character->GetInterpLocationIndex();
	Character->IncrementInterpLocationItemCount(InterpLocationIndex, 1);

	PlayPickUpSound();

	ItemInterpStartLocation = GetActorLocation();
	bIsInterping = true;
	SetItemState(EItemState::EIS_EquipInterping);
	GetWorldTimerManager().ClearTimer(PulseCurveTimer);

	GetWorldTimerManager().SetTimer(ItemInterpTimer, this, &AItemActor::FinishInterping, ZCurveTime);

	// Get initial yaw of the camera and item
	const float CameraRotationYaw = Character->GetFollowCamera()->GetComponentRotation().Yaw;
	const float ItemRotationYaw = GetActorRotation().Yaw;
	
	InterpInitialYawOffset = ItemRotationYaw - CameraRotationYaw;

	bCanChangeCustomDepth = false;
}

