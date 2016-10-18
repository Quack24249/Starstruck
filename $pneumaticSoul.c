#pragma config(Sensor, in1,    hyro,           sensorGyro)
#pragma config(Sensor, in2,    lowLiftPot,     sensorPotentiometer)
#pragma config(Sensor, in3,    highLiftPot,    sensorPotentiometer)
#pragma config(Sensor, dgtl1,  leftEnc,        sensorQuadEncoder)
#pragma config(Sensor, dgtl3,  rightEnc,       sensorQuadEncoder)
#pragma config(Motor,  port1,           rbd,           tmotorVex393_HBridge, openLoop, reversed)
#pragma config(Motor,  port2,           lbd,           tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port3,           lfd,           tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port4,           rfd,           tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port5,           lowLift1,      tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port6,           lowLift2,      tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port7,           highLift1,     tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port8,           highLift2,     tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port9,           claw1,         tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port10,          claw2,         tmotorVex393_HBridge, openLoop)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

#pragma platform(VEX2)
#pragma competitionControl(Competition)
#include "Vex_Competition_Includes.c"

int wristPower = 100; //<--- Change this

#include "buttonTracker.c"
#include "parallelDrive.c"
#include "pd_autoMove.c"
#include "motorGroup.c"

//buttons
#define toggleLiftModeBtn Btn8U
#define openClawBtn Btn6D //claw
#define closeClawBtn Btn6U
#define liftUpBtn Btn5U //lift
#define liftDownBtn Btn5D

//positions
#define liftMiddle 1010

//constants
#define liftStillSpeed 10
#define clawStillSpeed 15

short clawSign = -1; //Sign of still speed. Positive if closed, negative if open
bool fourBar = false; //whether lift is in dumper of four bar mode

motorGroup lowLift;
motorGroup highLift;
motorGroup claw;

//autonomous region
void pre_auton() {
	bStopTasksBetweenModes = true;

	initializeDrive(drive);
  setDriveMotors(drive, 4, lfd, lbd, rfd, rbd);
  attachEncoder(drive, leftEnc, LEFT);
  attachEncoder(drive, rightEnc, RIGHT, true, 4);
  attachGyro(drive, hyro);

  initializeGroup(lowLift, 2, lowLift1, lowLift2);
  configureButtonInput(lowLift, liftUpBtn, liftDownBtn, liftStillSpeed);
  addSensor(lowLift, lowLiftPot, true);

	initializeGroup(highLift, 2, highLift1, highLift2);
	configureButtonInput(highLift, Btn7U, Btn7D);
	addSensor(highLift, highLiftPot);

  initializeGroup(claw, 2, claw1, claw2);
}

task autonomous() {
  turn(-45);
	driveStraight(3*12*sqrt(2));
	turn(-135, defTurnInts[0], defTurnInts[1], -40);
	driveStraight(3*12);
}
//end autonomous region

//user control region
void liftControl() {
	if (newlyPressed(toggleLiftModeBtn))
		fourBar = !fourBar;

	lowLift.stillSpeed = liftStillSpeed * (potentiometerVal(lowLift)>liftMiddle ? 1 : -1);
	short lowLiftPower = takeInput(lowLift);
	short highLiftPower = takeInput(highLift, false);

	if (highLiftPower != 0) {
		setPower(highLift, highLiftPower);
	}	else if (lowLiftPower == liftStillSpeed) {
		setPower(highLift, 5);
	} else {
		setPower(highLift, (fourBar ? lowLiftPower*wristPower/127 : -5);
	}
}

void clawControl() {
	if (vexRT[closeClawBtn] == 1) {
		setPower(claw, 127);
		clawSign = 1;
	} else if (vexRT[openClawBtn] == 1) {
		setPower(claw, -127);
		clawSign = -1;
	} else {
		setPower(claw, clawStillSpeed * clawSign);
	}
}

task usercontrol() {
	while (true) {
  	driveRuntime(drive);

  	liftControl();

		clawControl();

		if (vexRT[Btn7U] == 1) autoTest();
  }
}
//end user control region
