#include "ShooterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Animation/AnimInstance.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystemComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Components/WidgetComponent.h"
#include "ItemActor.h"


AShooterCharacter::AShooterCharacter() :
	BaseTurnRate(45.f),
	BaseLookUpRate(45.f),
	bIsAiming(false),
	CameraDefaultFOV(0),  // set in the begin play
	CameraAimingFOV(40.f), // currently default value for ADS FOV
	CameraCurrentFOV(0),
	ZoomInterpSpeed(0),
	HipTurnRate(90.f),
	HipLookUpRate(90.f),
	AimingTurnRate(20.f),
	AimingLookUpRate(20.f),

	// Mouse sensivity rate
	MouseHipTurnRate(1.f),
	MouseHipLookUpRate(1.f),
	MouseAimingTurnRate(0.2f),
	MouseAimingLookUpRate(0.2f),
	ShootTimeDuration(0.05f),
	bIsFire(false),
	AutomaticFireRate(0.1f),
	bShouldFire(true),
	bIsFireButtonPressed(false),
	bShouldTraceForItems(false)
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create CameraBoom (pulls in towards the character)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.f; // the camera follows at this distance behind the character
	CameraBoom->bUsePawnControlRotation = true; // rotate the arm based on the controller
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 50.f);

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Dont rotate character when rotate camera
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Configure character movement
	// Character moves in the direction of controller input
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);

	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	ZoomInterpSpeed = 20.f;
}


void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (FollowCamera)
	{
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}
}

void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateCameraFOV(DeltaTime);
	SetSensitivity();
	CalculateCrosshairSpread(DeltaTime);
	TraceForItems();
}

void AShooterCharacter::MoveForward(float Value)
{
	if (Controller && Value != 0.f)
	{
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 };

		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X) };
		AddMovementInput(Direction, Value);
	}
}

void AShooterCharacter::MoveRight(float Value)
{
	if (Controller && Value != 0.f)
	{
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 };

		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::Y) };
		AddMovementInput(Direction, Value);
	}
}

void AShooterCharacter::TurnAtRate(float Rate)
{
	// deg/sec * sec/frame
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::FireWeapon()
{
	if (FireSound)
	{
		UGameplayStatics::PlaySound2D(this, FireSound);
	}

	const USkeletalMeshSocket* BarrelSocket = GetMesh()->GetSocketByName("BarrelSocket");

	if (BarrelSocket)
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(GetMesh());

		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);
		}

		FVector BeamEnd;
		if (GetBeamEndLocation(SocketTransform.GetLocation(), BeamEnd))
		{
			if (ImpactParticle)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticle, BeamEnd);
			}

			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticle, SocketTransform);

			if (Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	// Get and run hip fire montage that was set from the blueprint
	if (AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}

	// Start bullet fire timer for crosshairs
	StartCrosshairBulletFire();
}

bool AShooterCharacter::GetBeamEndLocation(const FVector& MuzzleSocketLocation, FVector& OutBeamLocation)
{
	FHitResult CrosshairHitResult;
	bool bCrosshairHit = TraceUnderCrosshairs(CrosshairHitResult, OutBeamLocation);

	if (bCrosshairHit)
	{
		OutBeamLocation = CrosshairHitResult.Location;
	}

	FHitResult BarrelTraceHit;
	const FVector Start = MuzzleSocketLocation;
	const FVector StartToEnd = OutBeamLocation - MuzzleSocketLocation;
	const FVector End = MuzzleSocketLocation + StartToEnd * 1.25f;

	// Trace outward from crosshair world location
	GetWorld()->LineTraceSingleByChannel(BarrelTraceHit, Start, End, ECollisionChannel::ECC_Visibility);

	if (BarrelTraceHit.bBlockingHit)
	{
		OutBeamLocation = BarrelTraceHit.Location;
		return true;
	}
	return false;
}

void AShooterCharacter::AimingButtonPressed()
{
	bIsAiming = true;
}

void AShooterCharacter::AimingButtonReleased()
{
	bIsAiming = false;
}

void AShooterCharacter::UpdateCameraFOV(float DeltaTime)
{
	if (bIsAiming)
	{
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraAimingFOV, DeltaTime, ZoomInterpSpeed);
		GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
	}
	else
	{
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpSpeed);
		GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
	}
}

void AShooterCharacter::SetSensitivity()
{
	if (bIsAiming)
	{
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else
	{
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}

void AShooterCharacter::FireButtonPressed()
{
	bIsFireButtonPressed = true;
	StartFireTimer();
}

void AShooterCharacter::FireButtonReleased()
{
	bIsFireButtonPressed = false;
}

void AShooterCharacter::StartFireTimer()
{
	if (bShouldFire)
	{
		FireWeapon();
		bShouldFire = false;
		GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AShooterCharacter::AutoFireReset, AutomaticFireRate);
	}
}

bool AShooterCharacter::TraceUnderCrosshairs(FHitResult& OutHit, FVector& OutHitLocation)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	// Get screenspace location of crosshairs
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);

	FVector CrosshaidWorldPosition;
	FVector CrosshairWorldDirection;

	// Get world position and direction of crosshairs
	bool bScreen2World = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshaidWorldPosition,
		CrosshairWorldDirection);

	if (bScreen2World)
	{
		const FVector Start = CrosshaidWorldPosition;
		const FVector End = CrosshaidWorldPosition + CrosshairWorldDirection * 50'000.f;
		OutHitLocation = End;

		GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECollisionChannel::ECC_Visibility);

		if (OutHit.bBlockingHit)
		{
			OutHitLocation = OutHit.Location;
			return true;
		}
	}
	return false;
}

void AShooterCharacter::AutoFireReset()
{
	bShouldFire = true;
	if (bIsFireButtonPressed) StartFireTimer();
}

//void AShooterCharacter::Jump()
//{
//	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
//
//	UE_LOG(LogTemp, Warning, TEXT("We are in Jump()"));
//
//	// Get and run hip fire montage that was set from the blueprint
//	if (AnimInstance && JumpMontage)
//	{
//		UE_LOG(LogTemp, Warning, TEXT("Statement is true"));
//		AnimInstance->Montage_Play(JumpMontage);
//		AnimInstance->Montage_JumpToSection(FName("JumpStart"));
//	}
//
//	//ACharacter::Jump();
//}

void AShooterCharacter::Turn(float Value)
{
	float TurnScaleFactor;
	if (bIsAiming)
	{
		TurnScaleFactor = MouseAimingTurnRate;
	}
	else
	{
		TurnScaleFactor = MouseHipTurnRate;
	}
	APawn::AddControllerYawInput(Value* TurnScaleFactor);
}

void AShooterCharacter::LookUp(float Value)
{
	float LookUpScaleRate;

	if (bIsAiming)
	{
		LookUpScaleRate = MouseAimingLookUpRate;
	}
	else
	{
		LookUpScaleRate = MouseHipLookUpRate;
	}
	APawn::AddControllerPitchInput(Value * LookUpScaleRate);
}

void AShooterCharacter::CalculateCrosshairSpread(float DeltaTime)
{
	FVector2D WalkSpeedRange(0, 600.f);
	FVector2D VelocityMultiplierRange(0.f, 1.f);
	FVector Velocity = GetVelocity();
	Velocity.Z = 0;

	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

	// Spread crosshairs slowly while in air
	if(GetCharacterMovement()->IsFalling())
	{
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
	}
	// Character is on the ground
	else
	{
		// Shrink crosshair repidly while on the ground
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
	}

	if (bIsAiming)
	{
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, -0.4f, DeltaTime, 30.f);
	}
	else
	{
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
	}

	// True 0.05f second after fire
	if (bIsFire)
	{
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.3f, DeltaTime, 60.f);
	}
	else
	{
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 60.f);
	}

	CrosshairSpreadMultiplier = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor + CrosshairAimFactor + CrosshairShootingFactor;
}

void AShooterCharacter::FinishCrosshairBulletFire()
{
	bIsFire = false;
}

void AShooterCharacter::StartCrosshairBulletFire()
{
	bIsFire = true;
	GetWorldTimerManager().SetTimer(CrosshairShootTimer, this, &AShooterCharacter::FinishCrosshairBulletFire, ShootTimeDuration);
}

void AShooterCharacter::TraceForItems()
{
	if (bShouldTraceForItems)
	{
		FHitResult ItemTraceResult;
		FVector Empty;
		if (TraceUnderCrosshairs(ItemTraceResult, Empty))
		{
			AItemActor* HitItem = Cast<AItemActor>(ItemTraceResult.Actor);
			if (HitItem)
			{
				HitItem->GetPickUpWidget()->SetVisibility(true);
			}

			if (LastOverlappedItem)
			{
				if (HitItem != LastOverlappedItem)
				{
					LastOverlappedItem->GetPickUpWidget()->SetVisibility(false);
				}
			}

			LastOverlappedItem = HitItem;
		}
	}
	else if (LastOverlappedItem)
	{
		if (LastOverlappedItem)
		{
			LastOverlappedItem->GetPickUpWidget()->SetVisibility(false);
		}
	}
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpAtRate);
	PlayerInputComponent->BindAxis("Turn", this, &AShooterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AShooterCharacter::LookUp);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AShooterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("FireButton", IE_Released, this, &AShooterCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("Aiming", IE_Pressed, this, &AShooterCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction("Aiming", IE_Released, this, &AShooterCharacter::AimingButtonReleased);
}

float AShooterCharacter::GetCrosshairSpreadMultiplier() const
{
	return CrosshairSpreadMultiplier;
}

void AShooterCharacter::IncrementOverlappedItemCounter(int8 Amount)
{
	if (OverlappedItemCounter + Amount <= 0)
	{
		OverlappedItemCounter = 0;
		bShouldTraceForItems = false;
	}
	else
	{
		OverlappedItemCounter += Amount;
		bShouldTraceForItems = true;
	}
}

