#pragma once

#include "CoreMinimal.h"
#include "ItemActor.h"
#include "AmmoType.h"
#include "Ammo.generated.h"


UCLASS()
class SHOOTER_API AAmmo : public AItemActor
{
	GENERATED_BODY()
	
public:
	AAmmo();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetItemProperties(EItemState State) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* AmmoMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo", meta = (AllowPrivateAccess = "true"))
	EAmmoType AmmoType;

public:
	FORCEINLINE UStaticMeshComponent* GetAmmoMesh() const { return AmmoMesh; }
	FORCEINLINE EAmmoType GetAmmoType() const { return AmmoType; }
};
