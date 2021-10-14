#include "ShooterPlayerController.h"
#include "Blueprint/UserWidget.h"


AShooterPlayerController::AShooterPlayerController()
{

}

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (HUDOverlayClass_BP)
	{
		HUDOverlay = CreateWidget<UUserWidget>(this, HUDOverlayClass_BP);

		if (HUDOverlay)
		{
			HUDOverlay->AddToViewport();
			HUDOverlay->SetVisibility(ESlateVisibility::Visible);
		}
	}
}
