#pragma config(Sensor, in2,    liftPot,        sensorPotentiometer)
#pragma config(Sensor, in3,    clawPot,        sensorPotentiometer)
#pragma config(Sensor, in4,    hyro,           sensorGyro)
#pragma config(Sensor, in5,    modePot,        sensorPotentiometer)
#pragma config(Sensor, in6,    sidePot,        sensorPotentiometer)
#pragma config(Sensor, dgtl1,  leftEnc,        sensorQuadEncoder)
#pragma config(Sensor, dgtl3,  rightEnc,       sensorQuadEncoder)
#pragma config(Motor,  port1,           rbd,           tmotorVex393_HBridge, openLoop, reversed)
#pragma config(Motor,  port2,           lbd,           tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port3,           lfd,           tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port4,           rfd,           tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port5,           lift1,         tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port6,           lift2,         tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port7,           lift3,         tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port8,           lift4,         tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port9,           claw1,         tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port10,          claw2,         tmotorVex393_HBridge, openLoop, reversed)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

#pragma platform(VEX2)
#pragma competitionControl(Competition)
#include "Vex_Competition_Includes.c"

#include "buttonTracker.c"
#include "motorGroup.c"
#include "parallelDrive.c"
#include "pd_autoMove.c"

//buttons
#define toggleLiftModeBtn Btn8U
#define openClawBtn Btn6D //claw
#define closeClawBtn Btn6U
#define liftUpBtn Btn5U //lift
#define liftDownBtn Btn5D

//positions
#define liftBottom 355 //lift
#define liftTop 1450
#define liftMiddle 735
#define liftVert 3215
#define clawOpenPos 1130 //claw
#define clawClosedPos 750
#define clawMax 1620

//constants
#define liftStillSpeed 10
#define clawStillSpeed 15

//variables
bool clawOpen = false;
short autoSign; //for autonomous, positive if robot is on on the blue side by the pole or the symmetric tile of red side

motorGroup lift;
motorGroup claw;

void pre_auton() {
	bStopTasksBetweenModes = true;

	initializeDrive(drive);
  setDriveMotors(drive, 4, lfd, lbd, rfd, rbd);
  attachEncoder(drive, leftEnc, LEFT);
  attachEncoder(drive, rightEnc, RIGHT, false, 4);
  attachGyro(drive, hyro);

	initializeGroup(lift, 4, lift1, lift2, lift3, lift4);
  configureButtonInput(lift, liftUpBtn, liftDownBtn, liftStillSpeed);
  addSensor(lift, liftPot);

  initializeGroup(claw, 2, claw1, claw2);
  addSensor(claw, clawPot);
}

//autonomous region
task maneuvers() {
	while (true) {
		executeManeuver(claw);
		executeManeuver(lift);
	}
}

void deployClaw(int waitAtEnd=250) {
	setDrivePower(drive, 127, 127);
	wait1Msec(500);
	setDrivePower(drive, -127, -127);
	wait1Msec(750);
	setDrivePower(drive, 0, 0);
	wait1Msec(waitAtEnd);
}

void setClawStateManeuver(bool open = !clawOpen) { //toggles by default
	if (open) {
		createManeuver(claw, clawOpenPos, clawStillSpeed);
	} else {
		createManeuver(claw, clawClosedPos, -clawStillSpeed);
	}

	clawOpen = open;
}

void openClaw(bool stillSpeed=true) {
	goToPosition(claw, clawOpenPos, (stillSpeed ? clawStillSpeed : 0));
}

void closeClaw(bool stillSpeed=true) {
	goToPosition(claw, clawClosedPos, (stillSpeed ? -clawStillSpeed : 0));
}

void hyperExtendClaw(bool stillSpeed=true) {
	goToPosition(claw, clawMax, (stillSpeed ? clawStillSpeed : 0));
}

void setLiftStateManeuver(bool top = potentiometerVal(lift)<liftMiddle) { //toggles by default
  if (top) {
    createManeuver(lift, liftTop, liftStillSpeed);
  } else {
    createManeuver(lift, liftBottom, liftStillSpeed);
  }
}

task pillowAuton() {
	setClawStateManeuver(true); //open claw
  driveStraight(5, true); //drive away from wall
  while(driveData.isDriving);

  //move toward pillow
  turn(-52, true);
  while(turnData.isTurning || claw.maneuverExecuting);
  driveStraight(12);

  closeClaw(); //clamp pillow

  setLiftStateManeuver(true);
  driveStraight(15, true);
  while (driveData.isDriving);
  turn(33, true, 40, 80, -20); //turn to face fence
  while (turnData.isTurning);
  driveStraight(17, true); // drive up to wall
  while (driveData.isDriving || lift.maneuverExecuting);

  openClaw(); //release pillow
  closeClaw();
  driveStraight(-5); //back up
  hyperExtendClaw();

  //push jacks over
 	driveStraight(10);
 	goToPosition(claw, 900);

 	createManeuver(claw, clawMax, clawStillSpeed); //hyperextend claw

 	//drive to other wall and lift down
 	driveStraight(-5, true);
 	while (driveData.isDriving);
 	turn(63, true);
 	while (turnData.isTurning || claw.maneuverExecuting);
 	createManeuver(lift, 1500, liftStillSpeed);
 	driveStraight(40, true);
 	while (driveData.isDriving || lift.maneuverExecuting);
 	turn(-95);
 	driveStraight(6);

 	goToPosition(lift, 2435); //push jacks over
}

task oneSideAuton() {
	createManeuver(claw, clawMax, clawStillSpeed); //open claw
	createManeuver(lift, liftTop-400, liftStillSpeed); //lift to near top
  driveStraight(5, true); //drive away from wall
  while(driveData.isDriving);

  turn(30, true);
  while (turnData.isTurning || claw.maneuverExecuting);

  driveStraight(10);
  while (driveData.isDriving);

  turn(-27, true); //turn toward wall
  while (turnData.isTurning || lift.maneuverExecuting);

  driveStraight(30);
  goToPosition(lift, 2435);
}

task autonomous() {
	lift.maneuverExecuting = false;
	claw.maneuverExecuting = false;
	startTask(maneuvers);

	//deploy stops
	goToPosition(lift, 725);
	goToPosition(lift, liftBottom);

  deployClaw();

  autoSign = (SensorValue[sidePot] < 1800) ? 1 : -1;

  //start appropriate autonomous task
  if (SensorValue[modePot] > 2540) {
  	startTask(pillowAuton);
  } else if (SensorValue[modePot] > 1320) {
  	startTask(oneSideAuton);
  }
}
//end autonomous region

//user control region
void liftControl() {
	short potPos = potentiometerVal(lift);
	lift.stillSpeed = liftStillSpeed * ((potPos<liftMiddle || potPos>liftVert) ? -1 : 1);
	takeInput(lift);
}

void clawControl() {
	if (vexRT[closeClawBtn] == 1) {
		setPower(claw, 127);
		clawOpen = false;
	} else if (vexRT[openClawBtn] == 1) {
		setPower(claw, -127);
		clawOpen = true;
	} else {
		setPower(claw, clawStillSpeed * (clawOpen ? -1 : 1));
	}
}

task usercontrol() {
	while (true) {
  	driveRuntime(drive);

  	liftControl();

		clawControl();
  }
}
//end user control region
