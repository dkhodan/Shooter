#include "Weapon.h"

AWeapon::AWeapon() :
	ThrowWeaponTime(0.7f),
	bIsFalling(false),
	Ammo(30),
	WeaponType(EWeaponType::EWT_SubmachineGun),
	AmmoType(EAmmoType::EAT_9MM),
	ReloadMontageSection(FName("ReloadSMG")),
	MagazineCapacity(30)
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetItemState() == EItemState::EIS_Falling && bIsFalling)
	{
		const FRotator MeshRotation(0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f);
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void AWeapon::DecrementAmmo()
{
	if (Ammo - 1 <= 0)
	{
		Ammo = 0;
	}
	else
	{
		--Ammo;
	}
}

void AWeapon::ThrowWeapon()
{
	FRotator MeshRotation(0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f);
	GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);

	// Direction in which we throw the weapon
	const FVector MeshForward = GetItemMesh()->GetForwardVector();
	FVector MeshRight = GetItemMesh()->GetRightVector();
	FVector ImpulseDirection = MeshRight.RotateAngleAxis(-20.f, MeshForward);

	float RandomRotation = 30.f;
	ImpulseDirection = ImpulseDirection.RotateAngleAxis(RandomRotation, FVector(0.f, 0.f, 1.f));
	ImpulseDirection *= 20'000.f;

	GetItemMesh()->AddImpulse(ImpulseDirection);
	bIsFalling = true;

	GetWorldTimerManager().SetTimer(ThrowWeaponTimer, this, &AWeapon::StopFalling, ThrowWeaponTime);
}

void AWeapon::StopFalling()
{
	bIsFalling = false;
	SetItemState(EItemState::EIS_Pickup);
}

void AWeapon::ReloadAmmo(int32 Amount)
{
	checkf(Ammo + Amount <= MagazineCapacity, TEXT("Attempted to reload with more then magazine capacity!"));
	Ammo += Amount;
}
