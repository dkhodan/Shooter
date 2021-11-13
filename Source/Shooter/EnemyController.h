#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyController.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER_API AEnemyController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyController();

	virtual void OnPossess(APawn* InPawn) override;

private:
	UPROPERTY(BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	class UBlackboardComponent* BlackboardComponent;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	class UBehaviorTreeComponent* BehaviourTreeComponent;

public:

	FORCEINLINE UBlackboardComponent* GetBlackboardComponent() const { return BlackboardComponent; }

};
