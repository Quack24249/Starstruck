#pragma config(Sensor, in1,    rightPot,       sensorPotentiometer)
#pragma config(Sensor, in2,    leftPot,        sensorPotentiometer)
#pragma config(Sensor, dgtl1,  hippo,          sensorDigitalOut)
#pragma config(Sensor, dgtl2,  hungryHungry,   sensorDigitalOut)
#pragma config(Motor,  port1,           lbd,           tmotorVex393_HBridge, openLoop)
#pragma config(Motor,  port2,           rbd,           tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port3,           rfd,           tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port4,           lfd,           tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port5,           lift1,         tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port6,           lift2,         tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port7,           lift3,         tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port8,           lift4,         tmotorVex393_MC29, openLoop)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

#pragma platform(VEX2)
#pragma competitionControl(Competition)
#include "Vex_Competition_Includes.c"

#include "buttonTracker.c"
#include "parallelDrive.c"
#include "motorGroup.c"

#define toggleMouthBtn Btn6U
#define toggleScoopBtn Btn6D
#define liftUpBtn Btn5U
#define liftDownBtn Btn5D

parallel_drive drive;
motorGroup lift;

void pre_auton() {
	bStopTasksBetweenModes = true;

	initializeDrive(drive);
  setLeftMotors(drive, 2, lfd, lbd);
  setRightMotors(drive, 2, rfd, rbd);

  initializeGroup(lift, 4, lift1, lift2, lift3, lift4);
  configureButtonInput(lift, liftUpBtn, liftDownBtn, 15);
}

task autonomous() {
  AutonomousCodePlaceholderForTesting();
}

task usercontrol() {
	while (true) {
  	driveRuntime(drive);

  	takeInput(lift);

		if (newlyPressed(toggleMouthBtn)) {
			SensorValue[hungryHungry] = 1 - SensorValue[hungryHungry];
			SensorValue[hippo] = SensorValue[hungryHungry];
		}
  }
}
