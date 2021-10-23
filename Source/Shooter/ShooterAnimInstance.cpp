#include "ShooterAnimInstance.h"
#include "ShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

UShooterAnimInstance::UShooterAnimInstance() :
	Speed(0.f),
	bIsInAir(false),
	bIsAccelerating(false),
	MovementOffset(0.f),
	LastMovementOffsetYaw(0.f),
	bIsAiming(false),
	RootYawOffset(0.f),
	PitchAimOffset(0.f),
	bReloading(false),
	OffsetState(EOffsetState::EOS_Hip),
	CharacterRotation(FRotator(0.f)),
	CharacterRotationLastFrame(FRotator(0.f)),
	TIPCharacterYaw(0),
	TIPCharacterYawLastFrame(0),
	YawDelta(0)
{

}

void UShooterAnimInstance::NativeInitializeAnimation()
{
	ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
}

void UShooterAnimInstance::TurnInPlace()
{
	if (!ShooterCharacter) return;

	PitchAimOffset = ShooterCharacter->GetBaseAimRotation().Pitch;

	// Don't want to turn in place where character is moving
	if (Speed > 0 || bIsInAir)
	{
		RootYawOffset = 0.f;
		TIPCharacterYaw = ShooterCharacter->GetActorRotation().Yaw;
		TIPCharacterYawLastFrame = TIPCharacterYaw;
		RotationCurve = 0.f;
		RotationCurveLastFrame = 0.f;
	}
	else
	{
		// Calculate root yaw offset [-180, 180]
		TIPCharacterYawLastFrame = TIPCharacterYaw;
		TIPCharacterYaw = ShooterCharacter->GetActorRotation().Yaw;
		const float TIPYawDelta = TIPCharacterYaw - TIPCharacterYawLastFrame;
		RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - TIPYawDelta);

		// get value of metadata curve which created inside animation IDLE_TURN_LEFT_90
		// Character\Animation\MainCharacterAnimations\Movement
		const float Turning = GetCurveValue(TEXT("Turning"));

		if (Turning > 0)
		{
			RotationCurveLastFrame = RotationCurve;
			RotationCurve = GetCurveValue(TEXT("Rotation"));
			const float DeltaRotation = RotationCurve - RotationCurveLastFrame;

			// RootYawOffset > 0 mean we turning left, if RootYawOffset < 0 mean we turning right
			RootYawOffset > 0 ? RootYawOffset -= DeltaRotation : RootYawOffset += DeltaRotation;

			const float AbsoluteRootYawOffset = FMath::Abs(RootYawOffset);

			if (AbsoluteRootYawOffset > 90.f)
			{
				const float YawExcess = AbsoluteRootYawOffset - 90.f;
				RootYawOffset > 0 ? RootYawOffset -= YawExcess : RootYawOffset += YawExcess;
			}
		}
	}
}

void UShooterAnimInstance::Lean(float DeltaTime)
{
	if (!ShooterCharacter) return;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = ShooterCharacter->GetActorRotation();

	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);

	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(YawDelta, Target, DeltaTime, 6.f);
	YawDelta = FMath::Clamp(Interp, -90.f, 90.f);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, -1, FColor::Green, FString::Printf(TEXT("Delta.Yaw: %f"), Delta.Yaw));
	}
}

void UShooterAnimInstance::CalculateAimOffset()
{
}


void UShooterAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if (!ShooterCharacter) 
	{
		ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
	}

	if (ShooterCharacter)
	{
		bReloading = ShooterCharacter->GetCombatState() == ECombatState::ECS_Reloading;

		// Get the speed of the character from velocity
		FVector Velocity = ShooterCharacter->GetVelocity();
		Velocity.Z = 0;

		Speed = Velocity.Size();

		// Is a character in the air? 
		bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling();

		// Is the character accelerating? 
		if (ShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0)
		{
			bIsAccelerating = true;
		}
		else
		{
			bIsAccelerating = false;
		}

		// TODO: Need to review how actually works here math behind. 
		// How calculations between aim vector and movement vector calculated and why value bounded into -180 to 180
		FRotator AimRotation = ShooterCharacter->GetBaseAimRotation();
		FRotator MovementRotator = UKismetMathLibrary::MakeRotFromX(ShooterCharacter->GetVelocity());

		MovementOffset = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotator, AimRotation).Yaw;
		
		if(ShooterCharacter->GetVelocity().Size() > 0.f) LastMovementOffsetYaw = MovementOffset;

		bIsAiming = ShooterCharacter->GetIsAiming();

		bIsJumping = ShooterCharacter->GetMovementComponent()->IsFalling();

		if (bReloading)
		{
			OffsetState = EOffsetState::EOS_Reloading;
		}
		else if (bIsInAir)
		{
			OffsetState = EOffsetState::EOS_InAir;
		}
		else if (ShooterCharacter->GetIsAiming())
		{
			OffsetState = EOffsetState::EOS_Aimig;
		}
		else
		{
			OffsetState = EOffsetState::EOS_Hip;
		}
	}

	TurnInPlace();
	Lean(DeltaTime);
}