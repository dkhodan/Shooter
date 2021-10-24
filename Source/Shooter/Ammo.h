#pragma once

#include "CoreMinimal.h"
#include "ItemActor.h"
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

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ammo", meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* AmmoMesh;

public:
	FORCEINLINE UStaticMeshComponent* GetAmmoMesh() const { return AmmoMesh; }
};
