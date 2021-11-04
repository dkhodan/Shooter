#include "Weapon.h"

AWeapon::AWeapon() :
	ThrowWeaponTime(0.7f),
	bIsFalling(false),
	Ammo(30),
	WeaponType(EWeaponType::EWT_SubmachineGun),
	AmmoType(EAmmoType::EAT_9MM),
	ReloadMontageSection(FName("ReloadSMG")),
	MagazineCapacity(30),
	bMovingClip(false),
	ClipBoneName(TEXT("smg_clip"))
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

	EnableGlowMaterial();
}

void AWeapon::StopFalling()
{
	bIsFalling = false;
	SetItemState(EItemState::EIS_Pickup);
	StartPulseTimer();
}

void AWeapon::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const FString WeaponTablePath(TEXT("DataTable'/Game/Game/DataTable/WeaponDataTable.WeaponDataTable'"));
	UDataTable* WeaponTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *WeaponTablePath));

	if (!WeaponTableObject) return;

	FWeaponDataTable* WeaponDataRow = nullptr;

	switch (WeaponType)
	{
	case EWeaponType::EWT_SubmachineGun:
		WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("SubmachineGun"), TEXT(""));
		break;

	case EWeaponType::EWT_AssaultRifle:
		WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("AssaultRifle"), TEXT(""));
		break;
	case EWeaponType::EWT_Pistol:
		WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("Pistol"), TEXT(""));
		break;
	}

	if (WeaponDataRow)
	{
		AmmoType = WeaponDataRow->AmmoType;
		Ammo = WeaponDataRow->WeaponAmmo;
		MagazineCapacity = WeaponDataRow->MagazineCapacity;
		PickUpSound = WeaponDataRow->PickUpSound;
		EquipSound = WeaponDataRow->EquipSound;
		GetItemMesh()->SetSkeletalMesh(WeaponDataRow->ItemMesh);
		ItemName = WeaponDataRow->WeaponName;
		AmmoIcon = WeaponDataRow->AmmoIcon;
		ItemIcon = WeaponDataRow->InventoryIcon;
		ClipBoneName = WeaponDataRow->ClipBoneName;
		ReloadMontageSection = WeaponDataRow->ReloadMontageSection;
		GetItemMesh()->SetAnimInstanceClass(WeaponDataRow->AnimBP);

		// set crosshairs
		CrosshairsMiddle = WeaponDataRow->CrosshairsMiddle;
		CrosshairsLeft = WeaponDataRow->CrosshairsLeft;
		CrosshairsRight = WeaponDataRow->CrosshairsRight;
		CrosshairsBottom = WeaponDataRow->CrosshairsBottom;
		CrosshairsTop = WeaponDataRow->CrosshairsTop;

		// weapon metadata
		AutoFireRate = WeaponDataRow->AutoFireRate;
		MuzzleFlash = WeaponDataRow->MuzzleFlash;
		FireSound = WeaponDataRow->FireSound;

		// pistol
		BoneToHide = WeaponDataRow->BoneToHide;

		MaterialInstance = WeaponDataRow->MaterialInstance;

		// clear previous material index and material itself
		PreviousMaterialIndex = MaterialIndex;
		ItemMesh->SetMaterial(PreviousMaterialIndex, nullptr);

		MaterialIndex = WeaponDataRow->MaterialIndex;
	}

	if (MaterialInstance)
	{
		DynamicMaterialInstance = UMaterialInstanceDynamic::Create(MaterialInstance, this);
		DynamicMaterialInstance->SetVectorParameterValue(TEXT("FresnelColor"), GlowColor);
		ItemMesh->SetMaterial(MaterialIndex, DynamicMaterialInstance);
		EnableGlowMaterial();
	}
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (BoneToHide != FName(""))
	{
		GetItemMesh()->HideBoneByName(BoneToHide, EPhysBodyOp::PBO_None);
	}
}

void AWeapon::ReloadAmmo(int32 Amount)
{
	checkf(Ammo + Amount <= MagazineCapacity, TEXT("Attempted to reload with more then magazine capacity!"));
	Ammo += Amount;
}

bool AWeapon::ClipIsFull()
{
	return Ammo >= MagazineCapacity;
}