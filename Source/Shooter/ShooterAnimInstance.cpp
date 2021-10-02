#include "ShooterAnimInstance.h"
#include "ShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UShooterAnimInstance::NativeInitializeAnimation()
{
	ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
}


void UShooterAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if (!ShooterCharacter) 
	{
		ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
	}

	if (ShooterCharacter)
	{
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
}