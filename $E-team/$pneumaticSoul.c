#pragma config(Sensor, in2,    hyro,           sensorGyro)
#pragma config(Sensor, in3,    shoulderPot,    sensorPotentiometer)
#pragma config(Sensor, in4,    wristPot,       sensorPotentiometer)
#pragma config(Sensor, in5,    clawPot,        sensorPotentiometer)
#pragma config(Sensor, in6,    modePot,        sensorPotentiometer)
#pragma config(Sensor, in7,    sidePot,        sensorPotentiometer)
#pragma config(Sensor, dgtl1,  leftEnc,        sensorQuadEncoder)
#pragma config(Sensor, dgtl3,  rightEnc,       sensorQuadEncoder)
#pragma config(Motor,  port1,           lbd,           tmotorVex393_HBridge, openLoop)
#pragma config(Motor,  port2,           lfd,           tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port3,           claw1,         tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port4,           claw2,         tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port5,           shoulder1,     tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port6,           shoulder2,     tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port7,           wristMotor,    tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port8,           rfd,           tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port9,           rbd,           tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port10,          wheelieMotor,  tmotorVex393_HBridge, openLoop)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

#pragma platform(VEX2)
#pragma competitionControl(Competition)
#include "Vex_Competition_Includes.c"

#include "..\Includes\buttonTracker.c"
#include "..\Includes\motorGroup.c"
#include "..\Includes\parallelDrive.c"
#include "..\Includes\pd_autoMove.c"
#include "..\Includes\logisticRamp.c"

//buttons
#define openClawBtn Btn6D //claw
#define closeClawBtn Btn6U
#define wheelieOutBtn Btn7U //wheelie bars
#define wheelieInBtn Btn7D
#define toggleLiftModeBtn Btn8L //lift
#define liftUpBtn Btn5U //         -- shoulder/4-bar
#define liftDownBtn Btn5D
#define wristUpBtn Btn8U //        -- wrist
#define wristDownBtn Btn8D

//positions
#define shoulderBottom -60. //shoulder
#define shoulderTop 135.
#define shoulderMiddle -45.
#define shoulderVert 90.
#define clawOpenPos 1130 //claw
#define clawClosedPos 750
#define clawMax 1620

//potentiometer-val/angle regressions
#define potentiometerRegSlope 0.0651
#define shoulderRegInt 20.
#define clawRegInt = 

//constants -- lift
#define fourBarDeadband 50 //allowable deviation from totalTargetPos
#define shoulderFourBarPower 90
#define shoulderStillSpeed 10 //still speeds
#define wristStillSpeed 10
#define clawStillSpeed 15
#define shoulderRegSlope 0.0651 //potentiometer-val/angle regression constants
#define shoulderRegInt 20.
#define logistic_k 0.0005 //wrist pos-matching logistic constants
#define logisticM 127
#define logistic_i 10
#define logistic_s -log((2*logisticM/(logisticM+logistic_i) - 1) / (2*logisticM/logistic_i - 1)) / (2*logistic_k*logisticM) //Shifts logistic function over so f(0) = logistic_i. It's ugly. I know.


//variables
bool fourBar = false;
bool clawOpen = false;
int totalTargetPos; //the target sum of the shoulder and wrist pot values in 4-bar mode
short autoSign; //for autonomous, positive if robot is left of pillow
short clawCounter, driveCounter, liftCounter; //store the progress of each robot component during autonomous

motorGroup shoulder;
motorGroup wrist;
motorGroup claw;
motorGroup wheelieWinch;
logisticRamper wristRamp;

void pre_auton() {
	bStopTasksBetweenModes = true;

	//configure drive
	initializeDrive(drive);
  setDriveMotors(drive, 4, lfd, lbd, rfd, rbd);
  attachEncoder(drive, leftEnc, LEFT);
  attachEncoder(drive, rightEnc, RIGHT, false, 4);
  attachGyro(drive, hyro);

  //configure shoulder
	initializeGroup(shoulder, 2, shoulder1, shoulder2);
  configureButtonInput(shoulder, liftUpBtn, liftDownBtn, shoulderStillSpeed);
  addSensor(shoulder, shoulderPot);

  //configure wrist
	initializeGroup(wrist, 1, wristMotor);
	configureButtonInput(wrist, wristUpBtn, wristDownBtn);
	addSensor(wrist, wristPot);

	//configure wheelie winch
	initializeGroup(wheelieWinch, 1, wheelieMotor);
	configureButtonInput(wheelieWinch, wheelieInBtn, wheelieOutBtn);

	//configure claw
  initializeGroup(claw, 2, claw1, claw2);
  addSensor(claw, clawPot);

	initializeLogisticRamp(wristRamp, logistic_k, 2*logisticM, logistic_i); //used for position matching between lift components
}

//lift region
float shoulderAngle() {
	return shoulderRegSlope*potentiometerVal(shoulder) + shoulderRegInt;
}

int totalLiftPotVal() {
	return potentiometerVal(wrist) + potentiometerVal(shoulder);
}

void toggleLiftMode() {
	fourBar = !fourBar;

	if (fourBar) {
		totalTargetPos = totalLiftPotVal();
		shoulder.upPower = shoulderFourBarPower;
		shoulder.downPower = shoulderFourBarPower;
	} else {
		shoulder.upPower = 127;
		shoulder.downPower = 127;
	}
}

void liftControl() {
	if (newlyPressed(toggleLiftModeBtn))
		toggleLiftMode();

	short shoulderPos = potentiometerVal(shoulder);

	shoulder.stillSpeed = shoulderStillSpeed * ((shoulderPos<shoulderMiddle || shoulderPos>shoulderVert) ? 1 : -1);
	short shoulderPower = takeInput(shoulder);
	short wristPower = takeInput(wrist, false);

	int totalPos = totalLiftPotVal();

	if (wristPower != 0) {
		setPower(wrist, wristPower);
		totalTargetPos = totalPos;
	}	else if (fourBar && (abs(totalTargetPos - totalPos) > fourBarDeadband)) {
		setPower(wrist, logisticRampRuntime(wristRamp, totalTargetPos - totalPos + logistic_s)); //moves wrist toward shoulder position
	} else {
		setPower(wrist, wristStillSpeed);
	}
}
	//autonomous subregion
void goToShoulderAngle(float angle) {

}

void createShoulderAngleManeuver(float angle) {

}

void goToWristAngle(float angle) {

}

void createWristAngleManeuver(float angle) {

}
	//end autonomous subregion
//end lift region


//claw region
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

 //autonomous subregion
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
 //end autonomous subregion
//end claw region


//autonomous region
void stopManeuvers() {
	claw.maneuverExecuting = false;
	shoulder.maneuverExecuting = false;
	wrist.maneuverExecuting = false;
}

void deploy(int waitAtEnd=250) {
	//deploy stops
	goToPosition(shoulder, 1200);
	goToPosition(shoulder, shoulderBottom);
}

	//pillowAuton subregion
task pillowAutonClaw() {

}

task pillowAutonLift() {

}

task pillowAutonDrive() {
	setClawStateManeuver(true); //open claw
  driveStraight(5, true); //drive away from wall
  while(driveData.isDriving);

  //move toward pillow
  turn(-52, true);
  while(turnData.isTurning || claw.maneuverExecuting);
  driveStraight(12);

  closeClaw(); //clamp pillow

  //--lift up--//
  driveStraight(13, true);
  while (driveData.isDriving);
  turn(33, true, 40, 80, -20); //turn to face fence
  while (turnData.isTurning);
  driveStraight(17, true); // drive up to wall
  while (driveData.isDriving || shoulder.maneuverExecuting);

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
 	createManeuver(shoulder, 1500, shoulderStillSpeed);
 	driveStraight(43, true);
 	while (driveData.isDriving || shoulder.maneuverExecuting);
 	turn(-80, false, 40, 120, -40);
 	driveStraight(7);

 	goToPosition(shoulder, 2435); //push jacks over
}

void pillowAuton() {
	startTask(pillowAutonDrive);
	startTask(pillowAutonLift);
	startTask(pillowAutonClaw);
}
	//end pillowAuton subregion

	//oneSideAuton subregion
task oneSideAutonDrive() {
	createManeuver(claw, clawMax, clawStillSpeed); //open claw
	createManeuver(shoulder, shoulderTop-400, shoulderStillSpeed); //lift to near top
  driveStraight(5, true); //drive away from wall
  while(driveData.isDriving);

  turn(30, true);
  while (turnData.isTurning || claw.maneuverExecuting);

  driveStraight(10);
  while (driveData.isDriving);

  turn(-40, true); //turn toward wall
  while (turnData.isTurning || shoulder.maneuverExecuting);

  driveStraight(25);
  goToPosition(shoulder, 2435);
}

task oneSideAutonLift() {

}

task oneSideAutonClaw() {

}

void oneSideAuton() {
	startTask(oneSideAutonDrive);
	startTask(oneSideAutonLift);
	startTask(oneSideAutonClaw);
}
	//end oneSideAuton subregion

task autonomous() {
	stopManeuvers();

  deploy();

  autoSign = (SensorValue[sidePot] < 1800) ? 1 : -1;

  //start appropriate autonomous task
  if (SensorValue[modePot] > 2540) {
  	pillowAuton();
  } else if (SensorValue[modePot] > 1320) {
  	oneSideAuton();
  }
}
//end autonomous region

//user control region
task usercontrol() {
	while (true) {
  	driveRuntime(drive);

  	takeInput(wheelieWinch);

		clawControl();

		liftControl();
  }
}
//end user control region
