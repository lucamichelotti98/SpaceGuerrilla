// Fill out your copyright notice in the Description page of Project Settings.

#include "Spaceship.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine.h"
#include "DrawDebugHelpers.h"
#include "UnrealNetwork.h"


// Sets default values
ASpaceship::ASpaceship()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	MaxSpeed = 100000.f;
	TurnSpeed = 80.f;
	Acceleration = 6000;
	MinSpeed = 0.f;
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

// Variables to replicate
void ASpaceship::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpaceship, ReplicatedTransform);
	DOREPLIFETIME(ASpaceship, CurrentYawSpeed);
	DOREPLIFETIME(ASpaceship, CurrentPitchSpeed);
	DOREPLIFETIME(ASpaceship, CurrentRollSpeed);
	DOREPLIFETIME(ASpaceship, CurrentStrafeSpeed);
	DOREPLIFETIME(ASpaceship, RollRoll);
	DOREPLIFETIME(ASpaceship, CurrentForwardSpeed);
}

//Function to get the role in string to debug
FString GetEnumText(ENetRole Role)
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

// Called every frame
void ASpaceship::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FRotator Banana = GetActorRotation();
	UE_LOG(LogTemp, Warning, TEXT("BANANA: %s"), *Banana.ToString());
	//UE_LOG(LogTemp, Warning, TEXT("Rotazione: %s"), *NewRotation.ToString());

//	FRotator NewRotation = OurCameraSpringArm->GetComponentRotation();
//	NewRotation.Yaw = (CameraInput.X * DeltaTime);
//	NewRotation.Pitch = (CameraInput.Y * DeltaTime);
//	OurCameraSpringArm->SetWorldRotation(FQuat(NewRotation), true);

	//TODO Refactoring the code in small functions so it's easier to implement in the server
	UE_LOG(LogTemp, Warning, TEXT("Rotazione: %s"), *CameraInput.ToString());
	// Move forward tick
	{
		const FVector LocalMove = FVector(CurrentForwardSpeed * DeltaTime, 0.f, 0.f);

		// Move plan forwards (with sweep so we stop when we collide with things)
		AddActorLocalOffset(LocalMove, true);
	}
	{
		FRotator rotationDelta(CurrentPitchSpeed *DeltaTime, 0, 0);

		FTransform NewTrasform = GetTransform();
		NewTrasform.ConcatenateRotation(rotationDelta.Quaternion());
		NewTrasform.NormalizeRotation();
		SetActorTransform(NewTrasform);

		// Calculate change in rotation this frame
		FRotator DeltaRotation(0, 0, 0);
		DeltaRotation.Yaw = CurrentYawSpeed * DeltaTime;
		UE_LOG(LogTemp, Warning, TEXT("Rotazione: %s"), *DeltaRotation.ToString());

		// Rotate plane
		AddActorWorldRotation(FQuat(DeltaRotation), true);
	}
	{
		FRotator DeltaRotationRoll(0, 0, 0);
		DeltaRotationRoll.Roll = (CurrentRollSpeed + RollRoll * 2)* DeltaTime;
		AddActorLocalRotation(FQuat(DeltaRotationRoll), true);
		UE_LOG(LogTemp, Warning, TEXT("Rotazione: %s"), *DeltaRotationRoll.ToString());
	}


	// Check if we are the server or the client, so we make sure the replicated variables are the same
	if (HasAuthority())
	{
		ReplicatedTransform = GetActorTransform();
	}

	// Getting the Role and showing it above the actor
	DrawDebugString(GetWorld(), FVector(0, 0, 100), GetEnumText(Role), this, FColor::White, DeltaTime);
}

// Replicated function called when the variable replicated changes
void ASpaceship::OnRep_ReplicatedTransform()
{
	SetActorTransform(ReplicatedTransform);
}


// Called to bind functionality to input
void ASpaceship::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// Respond every frame to the values our movements.
	PlayerInputComponent->BindAxis("ForwardSpaceship", this, &ASpaceship::MoveForwardInput);
	PlayerInputComponent->BindAxis("YawSpaceship", this, &ASpaceship::MoveYawInput);
	//PlayerInputComponent->BindAxis("Strafe", this, &ACorpo::MoveStrafeInput);
	//PlayerInputComponent->BindAction("Boost", this, &ACorpo::BoostInput);
	InputComponent->BindAxis("CameraPitchSpaceship", this, &ASpaceship::PitchCamera);
	InputComponent->BindAxis("CameraYawSpaceship", this, &ASpaceship::YawCamera);

	//TODO Implement projectile
	//InputComponent->BindAction("Fire", IE_Pressed, this, &ASpaceship::OnFire);
}

// Movement on client
void ASpaceship::MoveForwardInput(float Val)
{
	bool bHasInput = !FMath::IsNearlyEqual(Val, 0.f);
	float CurrentAcc = bHasInput ? (Val * Acceleration) : (-0.2f * Acceleration);
	float NewForwardSpeed = CurrentForwardSpeed + (GetWorld()->GetDeltaSeconds() * CurrentAcc);
	CurrentForwardSpeed = FMath::Clamp(NewForwardSpeed, MinSpeed, MaxSpeed);
	const FRotator Rotation = GetActorRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);
	const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	AddMovementInput(Direction, CurrentForwardSpeed);


	Server_MoveForwardInput(Val);
}

void ASpaceship::MoveYawInput(float Val)
{
	RollRoll = FMath::FInterpTo(RollRoll, Val * TurnSpeed * 3, GetWorld()->GetDeltaSeconds(), 1.f);

	Server_MoveYawInput(Val);
}

void ASpaceship::PitchCamera(float Val)
{
	float Rotation = GetActorRotation().Pitch;
	CameraInput.Y = FMath::FInterpTo(CameraInput.Y, Val * TurnSpeed, GetWorld()->GetDeltaSeconds(), 1.f);
	CurrentPitchSpeed = FMath::FInterpTo(CurrentPitchSpeed, Val * TurnSpeed*0.5, GetWorld()->GetDeltaSeconds(), 1.f);  // Val * TurnSpeed;

	Server_PitchCamera(Val);
}

void ASpaceship::YawCamera(float Val)
{

	CameraInput.X = FMath::FInterpTo(CameraInput.X, Val * TurnSpeed, GetWorld()->GetDeltaSeconds(), 1.f);
	float Rotation = GetActorRotation().Roll;
	bool bIsTurning = (FMath::Abs(Val) > 0.2f) && (FMath::IsWithin(Rotation, -90.f, 90.f));
	if (bIsTurning)
	{
		CurrentRollSpeed = FMath::FInterpTo(CurrentRollSpeed, Val * TurnSpeed, GetWorld()->GetDeltaSeconds(), 1.f);//TurnSpeed * Val;
	}
	else
	{
		CurrentRollSpeed = FMath::FInterpTo(CurrentRollSpeed, GetActorRotation().Roll * -1.f, GetWorld()->GetDeltaSeconds(), 5.f);//GetActorRotation().Roll * -0.5;
	}
	CurrentYawSpeed = FMath::FInterpTo(CurrentYawSpeed, Val * TurnSpeed, GetWorld()->GetDeltaSeconds(), 1.f); //Val*TurnSpeed;

	Server_YawCamera(Val);
}

// Movement Server Implementation and validation. Validation is for anticheat
void ASpaceship::Server_MoveForwardInput_Implementation(float Val)
{
	bool bHasInput = !FMath::IsNearlyEqual(Val, 0.f);
	float CurrentAcc = bHasInput ? (Val * Acceleration) : (-0.2f * Acceleration);
	float NewForwardSpeed = CurrentForwardSpeed + (GetWorld()->GetDeltaSeconds() * CurrentAcc);
	CurrentForwardSpeed = FMath::Clamp(NewForwardSpeed, MinSpeed, MaxSpeed);
	const FRotator Rotation = GetActorRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);
	const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	AddMovementInput(Direction, CurrentForwardSpeed);
}

bool ASpaceship::Server_MoveForwardInput_Validate(float Val)
{
	return true;
}

void ASpaceship::Server_MoveYawInput_Implementation(float Val)
{
	RollRoll = FMath::FInterpTo(RollRoll, Val * TurnSpeed * 3, GetWorld()->GetDeltaSeconds(), 1.f);

}

bool ASpaceship::Server_MoveYawInput_Validate(float Val)
{
	return true;
}

void ASpaceship::Server_PitchCamera_Implementation(float Val)
{
	float Rotation = GetActorRotation().Pitch;
	CameraInput.Y = FMath::FInterpTo(CameraInput.Y, Val * TurnSpeed, GetWorld()->GetDeltaSeconds(), 1.f);
	CurrentPitchSpeed = FMath::FInterpTo(CurrentPitchSpeed, Val * TurnSpeed*0.5, GetWorld()->GetDeltaSeconds(), 1.f);  // Val * TurnSpeed;

}

bool ASpaceship::Server_PitchCamera_Validate(float Val)
{
	return true;
}

void ASpaceship::Server_YawCamera_Implementation(float Val)
{
	CameraInput.X = FMath::FInterpTo(CameraInput.X, Val * TurnSpeed, GetWorld()->GetDeltaSeconds(), 1.f);
	float Rotation = GetActorRotation().Roll;
	bool bIsTurning = (FMath::Abs(Val) > 0.2f) && (FMath::IsWithin(Rotation, -90.f, 90.f));
	if (bIsTurning)
	{
		CurrentRollSpeed = FMath::FInterpTo(CurrentRollSpeed, Val * TurnSpeed, GetWorld()->GetDeltaSeconds(), 1.f);//TurnSpeed * Val;
	}
	else
	{
		CurrentRollSpeed = FMath::FInterpTo(CurrentRollSpeed, GetActorRotation().Roll * -1.f, GetWorld()->GetDeltaSeconds(), 5.f);//GetActorRotation().Roll * -0.5;
	}
	CurrentYawSpeed = FMath::FInterpTo(CurrentYawSpeed, Val * TurnSpeed, GetWorld()->GetDeltaSeconds(), 1.f); //Val*TurnSpeed;
}

bool ASpaceship::Server_YawCamera_Validate(float Val)
{
	return true;
}
