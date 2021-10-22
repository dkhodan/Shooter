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
	CharacterYaw(0.f),
	CharacterYawLastFrame(0.f),
	RootYawOffset(0.f),
	PitchAimOffset(0.f),
	bReloading(false)
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
	if (Speed > 0)
	{
		RootYawOffset = 0.f;
		CharacterYaw = ShooterCharacter->GetActorRotation().Yaw;
		CharacterYawLastFrame = CharacterYaw;
		RotationCurve = 0.f;
		RotationCurveLastFrame = 0.f;
	}
	else
	{
		// Calculate root yaw offset [-180, 180]
		CharacterYawLastFrame = CharacterYaw;
		CharacterYaw = ShooterCharacter->GetActorRotation().Yaw;
		const float YawDelta = CharacterYaw - CharacterYawLastFrame;
		RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - YawDelta);

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
	}

	TurnInPlace();
}