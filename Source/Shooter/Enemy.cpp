#include "Enemy.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnemyController.h"

AEnemy::AEnemy() :
	Health(100.f),
	MaxHealth(100.f),
	HealthBarDisplayTime(4.f),
	bCanHitReact(true),
	HitReactTimeMin(0.5f),
	HitReactTimeMax(0.75f),
	HitNumberDestroyTime(1.5f)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	// get the AI controller
	EnemyController = Cast<AEnemyController>(GetController());

	const FVector WorldPatrolPoint = UKismetMathLibrary::TransformLocation(GetActorTransform(), PatrolPoint);
	DrawDebugSphere(GetWorld(), WorldPatrolPoint, 25.f, 12, FColor::Red, true);

	if (EnemyController)
	{
		EnemyController->GetBlackboardComponent()->SetValueAsVector("PatrolPoint", WorldPatrolPoint);
		EnemyController->RunBehaviorTree(BehaviourTree);
	}
}

void AEnemy::ShowHealthBar_Implementation()
{
	GetWorldTimerManager().ClearTimer(HealthBarTimer);
	GetWorldTimerManager().SetTimer(HealthBarTimer, this, &AEnemy::HideHealthBar, HealthBarDisplayTime);
}

void AEnemy::Die()
{
	HideHealthBar();
}

void AEnemy::PlayHitMontage(FName MontageSection, float PlayRate /*= 1.f*/)
{
	if (!bCanHitReact) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance)
	{
		AnimInstance->Montage_Play(HitMontage, PlayRate);
		AnimInstance->Montage_JumpToSection(MontageSection, HitMontage);
	}

	bCanHitReact = false;
	const float HitReactTime = FMath::FRandRange(HitReactTimeMin, HitReactTimeMax);
	GetWorldTimerManager().SetTimer(HitReactTimer, this, &AEnemy::ResetHitReactTimer, HitReactTime);
}

void AEnemy::ResetHitReactTimer()
{
	bCanHitReact = true;
}

void AEnemy::StoreHitNumber(UUserWidget* HitNumber, FVector Location)
{
	HitNumbers.Add(HitNumber, Location);

	FTimerHandle HitNumberTimer;
	FTimerDelegate HitNumberDelegate;

	// call timer delegate to call function by name and pass parameter HitNumber
	HitNumberDelegate.BindUFunction(this, FName("DestroyHitNumber"), HitNumber);
	
	GetWorld()->GetTimerManager().SetTimer(HitNumberTimer, HitNumberDelegate, HitNumberDestroyTime, false);
}

void AEnemy::DestroyHitNumber(UUserWidget* Widget)
{
	HitNumbers.Remove(Widget);
	Widget->RemoveFromParent();
}

void AEnemy::UpdateHitNumber()
{
	for (auto& Hit : HitNumbers)
	{
		UUserWidget* HitNumber = Hit.Key;
		const FVector Location = Hit.Value;
		FVector2D ScreenPosition;

		UGameplayStatics::ProjectWorldToScreen(GetWorld()->GetFirstPlayerController(), Location, ScreenPosition);
		HitNumber->SetPositionInViewport(ScreenPosition);
	}
}

// Called every frame
void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateHitNumber();
}

float AEnemy::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Health - DamageAmount <= 0)
	{
		Health = 0;
		Die();
	}
	else
	{
		Health -= DamageAmount;
	}
	return DamageAmount;
}

// Called to bind functionality to input
void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemy::BulletHit_Implementation(FHitResult& HitResult)
{
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}

	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, HitResult.Location, FRotator(0.f), true);
	}

	ShowHealthBar();
	PlayHitMontage(FName("HitReactFront"));
}

