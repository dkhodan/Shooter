#include "EnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Enemy.h"

AEnemyController::AEnemyController()
{
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("Blackboard Component"));
	check(BlackboardComponent);

	BehaviourTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("Behaviour Tree Component"));
	check(BehaviourTreeComponent);
}

void AEnemyController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!InPawn) return;

	AEnemy* Enemy = Cast<AEnemy>(InPawn);

	if (Enemy)
	{
		if (auto Tree = Enemy->GetBehaviourTree())
		{
			BlackboardComponent->InitializeBlackboard(*(Enemy->GetBehaviourTree()->BlackboardAsset));
		}
	}
}
