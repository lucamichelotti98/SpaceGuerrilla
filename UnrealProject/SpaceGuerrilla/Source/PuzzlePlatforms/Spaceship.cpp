// Fill out your copyright notice in the Description page of Project Settings.

#include "Spaceship.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Controller.h"

// Sets default values
ASpaceship::ASpaceship()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bReplicateMovement = false;

	MovementComponent = CreateDefaultSubobject<USpaceshipMovementComponent>(TEXT("MovementComponent"));
	MovementReplicator = CreateDefaultSubobject<USpaceshipMovementReplicator>(TEXT("MovementReplicator"));
}

// Called when the game starts or when spawned
void ASpaceship::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		NetUpdateFrequency = 1;
	}
}

//Function to get the role in string to debug
FString GetEnumNetRoleText(ENetRole Role)
{
	switch (Role)
	{
	case ROLE_None:
		return "None";
	case ROLE_SimulatedProxy:
		return "SimulatedProxy";
	case ROLE_AutonomousProxy:
		return "AutonomousProxy";
	case ROLE_Authority:
		return "Authority";
	default:
		return "ERROR";
	}
}

//Function to get the role in string to debug
FString GetEnumNetModeText(ENetMode Role)
{

	switch (Role)
	{
	case NM_Standalone:
		return "Standalone";
	case NM_DedicatedServer:
		return "DedicatedServer";
	case NM_ListenServer:
		return "ListenServer";
	case NM_Client:
		return "Client";
	case NM_MAX:
		return "MAX";
	default:
		return "ERROR";
	}
}


// Called every frame
void ASpaceship::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	//FRotator Banana = GetActorRotation();// Debug console
	//UE_LOG(LogTemp, Warning, TEXT("BANANA: %s"), *Banana.ToString()); // Debug console

	//UE_LOG(LogTemp, Warning, TEXT("Rotazione: %s"), *CameraInput.ToString());// Debug console

	// Getting the Role and showing it above the actor for debug	
	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumNetRoleText(Role), this, FColor::White, DeltaTime);
	DrawDebugString(GetWorld(), FVector(0, 0, 150), GetEnumNetRoleText(GetRemoteRole()), this, FColor::White, DeltaTime);
	DrawDebugString(GetWorld(), FVector(0, 0, 200), GetEnumNetModeText(GetNetMode()), this, FColor::White, DeltaTime);
}

// Called to bind functionality to input
void ASpaceship::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// Respond every frame to the values our movements.
	PlayerInputComponent->BindAxis("ForwardSpaceship", this, &ASpaceship::MoveForwardInput);
	PlayerInputComponent->BindAxis("YawSpaceship", this, &ASpaceship::MoveYawInput);
	InputComponent->BindAxis("CameraPitchSpaceship", this, &ASpaceship::PitchCamera);
	InputComponent->BindAxis("CameraYawSpaceship", this, &ASpaceship::YawCamera);

	//TODO Implement projectile
	//InputComponent->BindAction("Fire", IE_Pressed, this, &ASpaceship::OnFire);
}


// Movement on client // 
void ASpaceship::MoveForwardInput(float Val)
{
	if (MovementComponent == nullptr) return;

	MovementComponent->SetThrottle(Val);
}

void ASpaceship::MoveYawInput(float Val)
{
	if (MovementComponent == nullptr) return;

	MovementComponent->SetRollRotationRatio(Val);	
}

void ASpaceship::PitchCamera(float Val)
{
	if (MovementComponent == nullptr) return;

	MovementComponent->SetPitchRotationRatio(Val);
	CameraInput.Y = FMath::FInterpTo(CameraInput.Y, Val * MovementComponent->GetTurnSpeed(), GetWorld()->GetDeltaSeconds(), 1.f);
}

void ASpaceship::YawCamera(float Val)
{
	if (MovementComponent == nullptr) return;

	MovementComponent->SetYawRotationRatio(Val);
	CameraInput.X = FMath::FInterpTo(CameraInput.X, Val * MovementComponent->GetTurnSpeed(), GetWorld()->GetDeltaSeconds(), 1.f);
}
// end of movement //
