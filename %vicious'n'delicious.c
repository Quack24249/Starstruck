#pragma config(Sensor, in1,    ClawPot,        sensorPotentiometer)
#pragma config(Sensor, in2,    LeftPot,        sensorPotentiometer)
#pragma config(Sensor, in3,    RightPot,       sensorPotentiometer)
#pragma config(Sensor, dgtl1,  rightEncoder,   sensorQuadEncoder)
#pragma config(Sensor, dgtl3,  leftEncoder,    sensorQuadEncoder)
#pragma config(Sensor, dgtl5,  leftLiftPneu,   sensorDigitalOut)
#pragma config(Sensor, dgtl6,  rightLiftPneu,  sensorDigitalOut)
#pragma config(Sensor, dgtl7,  clawPneu,       sensorDigitalOut)
#pragma config(Motor,  port1,           lbd,           tmotorVex393_HBridge, openLoop)
#pragma config(Motor,  port2,           lfd,           tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port3,           lift1,         tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port4,           lift2,         tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port7,           lift3,         tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port8,           lift4,         tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port9,           rfd,           tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port10,          rbd,           tmotorVex393_HBridge, openLoop)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

#pragma platform(VEX2)
#pragma competitionControl(Competition)
#include "Vex_Competition_Includes.c"

#include "parallelDrive.c"
#include "motorGroup.c"
#include "buttonTracker.c"

#define toggleClawBtn Btn6U
#define toggleLiftBtn Btn7U
#define liftUpBtn Btn5U
#define liftDownBtn Btn5D
#define liftTopBtn Btn7U
#define liftBottomBtn Btn7D

#define liftMax 3750
#define liftMin 2200
//#define clawClosedCutoff 200
#define clawStillSpeed 15

parallel_drive drive;
motorGroup lift;
motorGroup claw;

void pre_auton() {
  bStopTasksBetweenModes = true;

  initializeDrive(drive);
  setLeftMotors(drive, 2, lfd, lbd);
  setRightMotors(drive, 2, rfd, rbd);
  attachEncoderL(drive, leftEncoder);
  attachEncoderR(drive, rightEncoder);

  initializeGroup(lift, 4, lift1, lift2, lift3, lift4);
  configureButtonInput(lift, liftUpBtn, liftDownBtn, 10, 127, -80);
  attachPotentiometer(lift, RightPot);
  createTarget(lift, liftMax, liftTopBtn);
  createTarget(lift, liftMin, liftBottomBtn);

  //initializeGroup(claw, 2, claw1, claw2);
  //attachPotentiometer(claw, ClawPot);
}

task autonomous() {
  AutonomousCodePlaceholderForTesting();
}

task usercontrol() {
  bool clawClosed = false;

  while (true) {
    driveRuntime(drive);

    takeInput(lift);

    if (newlyPressed(toggleClawBtn)) {
    	SensorValue[clawPneu] = 1 - SensorValue[clawPneu];
    }

    if (newlyPressed(toggleLiftBtn)) {
    	SensorValue[leftLiftPneu] = 1 - SensorValue[leftLiftPneu];
    	SensorValue[rightLiftPneu] = SensorValue[leftLiftPneu];
    }
		//claw
    //if (vexRT[clawInBtn] == 1) {
    //  clawClosed = true;
    //  setPower(claw, 127);
    //} else if (vexRT[clawOutBtn] == 1) {
    //  clawClosed = false;
    //  setPower(claw, -127);
    //} else {
    //  setPower(claw, clawClosed/*&&potentiometerVal(claw)<clawClosedCutoff*/ ? clawStillSpeed : 0);
    //}
  }
}
