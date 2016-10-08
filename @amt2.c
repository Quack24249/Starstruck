#pragma config(Sensor, in1,    Yaw,            sensorGyro)
#pragma config(Sensor, dgtl3,  leftE,          sensorQuadEncoder)
#pragma config(Sensor, dgtl5,  rightE,         sensorQuadEncoder)
#pragma config(Motor,  port1,           Seymore,       tmotorVex393_HBridge, openLoop, reversed)
#pragma config(Motor,  port2,           lfd,           tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port3,           lbd,           tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port8,           rfd,           tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port9,           rbd,           tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port10,          FeedMe,        tmotorVex393_HBridge, openLoop)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

#define autoDrive drive

#include "coreIncludes.c"
#include "parallelDrive.c"
#include "PID.c"
#include "timer.c"

//turn defaults
#define TURN_BRAKE_DURATION 100 //maximum duration of braking at end of turn
angleType defAngleType = DEGREES;
bool defTurnRunAsTask = false;
int defTurnInts[5] = { 40, 100, -20, 100, 20 }; //initialPower, maxPower, finalPower, waitAtEnd, brakePower
//end turn defaults

typedef enum correctionType { NONE, GYRO, ENCODER, AUTO };

//drive defaults
#define DRIVE_BRAKE_POWER 30 //power used during driveStraight braking
#define DRIVE_BRAKE_DURATION 100 //maximum duration of braking at end of driveStraight
correctionType defCorrectionType = AUTO;
bool defDriveBools[2] = { false, false }; //runAsTask, rawValue
int defDriveInts[6] = { 40, 120, 0, 1000, 100, 50 }; //initialPower, maxPower, finalPower, timeout, waitAtEnd, sampleTime
float defDriveFloats[4] = { 0.25, 0.25, 0.25, 3 }; //kP, kI, kD, minSpeed
//end drive defaults

int debug;

parallel_drive autoDrive;


//turning region
typedef struct {
	float angle; //positive for clockwise, negative for counterclockwise
	int waitAtEnd; //delay after finishing turn (default 100ms for braking)
	int brakePower; //the motor power while braking
	bool isTurning; //whether turn is executing (useful for running as task)
	float a, b, c; //ramping equation constants
	int direction; //sign of angle
} turnStruct;

turnStruct turnData;

bool turnIsComplete() {
	return abs(gyroVal(autoDrive, DEGREES)) >= abs(turnData.angle);
}

void turnRuntime() {
	int gyro = abs(gyroVal(autoDrive, DEGREES));
	int power = turnData.a*pow(gyro, 2) +  turnData.b*gyro + turnData.c;

	setDrivePower(autoDrive, turnData.direction*power, -turnData.direction*power);
}

void turnEnd() {
	//brake
	setDrivePower(autoDrive, -turnData.direction * turnData.brakePower, turnData.direction * turnData.brakePower);
	int brakeDelay = limit(0, TURN_BRAKE_DURATION, turnData.waitAtEnd);
	wait1Msec(brakeDelay);
	setDrivePower(autoDrive, 0, 0);

	wait1Msec(turnData.waitAtEnd>TURN_BRAKE_DURATION ? turnData.waitAtEnd-TURN_BRAKE_DURATION : 0);
	turnData.isTurning = false;
}

task turnTask() {
	while (!turnIsComplete()) {
		turnRuntime();
		EndTimeSlice();
	}
	turnEnd();
}

void _turn_(parallel_drive &drive, float angle, float a, float b, float c, bool runAsTask=false, int waitAtEnd=100, int brakePower=20) { //Internal function. Use at your own risk.
	if (drive.hasGyro) {
		//initialize variables
		autoDrive = drive;
		turnData.angle = abs(angle);
		turnData.a = a;
		turnData.b = b;
		turnData.c = c;
		turnData.direction = sgn(angle);
		turnData.waitAtEnd = waitAtEnd;
		turnData.isTurning = true;

		resetGyro(autoDrive);

		if (runAsTask) {
			startTask(turnTask);
		}
		else {
			while (!turnIsComplete()) { turnRuntime(); }
			debug = gyroVal(drive);
			turnEnd();
		}
	}
}

void turn(parallel_drive &drive, float angle, angleType angleType=defAngleType, bool runAsTask=defTurnRunAsTask, int initialPower=defTurnInts[0], int maxPower=defTurnInts[1], int finalPower=defTurnInts[2], int waitAtEnd=defTurnInts[3], int brakePower=defTurnInts[4]) {
	angle = convertAngle(angle, DEGREES, angleType);
	float a = (pow(angle, 2) * (finalPower+initialPower-2*maxPower) - 2*sqrt(pow(angle, 4) * (finalPower-maxPower) * (initialPower-maxPower))) / pow(angle, 4);
	float b = ((finalPower-initialPower)/angle - a*angle) * sgn(angle);

	_turn_(drive, angle, a, b, initialPower, runAsTask, waitAtEnd, brakePower);
}

void setTurnDefaults(angleType angleType, bool runAsTask=defTurnRunAsTask, int initialPower=defTurnInts[0], int maxPower=defTurnInts[1], int finalPower=defTurnInts[2], int waitAtEnd=defTurnInts[3], int brakePower=defTurnInts[4]) {
	defAngleType = angleType;
	defTurnRunAsTask = runAsTask;

	int defInts[5] = { initialPower, maxPower, finalPower, waitAtEnd, brakePower };

	defTurnInts = defInts;
}
//end turning region


//driveStraight region
typedef struct {
	float distance;
	bool rawValue; //whether distance is measured in encoder clicks or inches
	float minSpeed; //minimum speed during maneuver to prevent timeout (distance per 100ms)
	int timeout; //amount of time after which a drive action sensing lower speed than minSpeed ceases (ms)
	int sampleTime; //time between motor power adjustments
	int waitAtEnd; //duration of pause at end of driving
	int brakePower; // motor power during braking
	correctionType correctionType; //which sensor inputs are used for correction
	bool isDriving; //whether driving action is being executed (true if driving, false othrewise)
	//interal variables
	PID pid;
	float a, b, c; //constants in equation for determining motor power
	float totalDist; //distance traveled so far
	int direction; //sign of distance
	float error; //calculated from gyro or encoders
	long timer; //for tracking timeout
} driveStruct;

driveStruct driveData;

bool drivingComplete() {
	return abs(driveData.totalDist)>driveData.distance  || time(driveData.timer)>driveData.timeout;
}

void driveStraightRuntime() {
	int power = driveData.a*pow(driveData.totalDist, 2) + driveData.b*driveData.totalDist + driveData.c;
	float slaveCoeff = 1 + PID_runtime(driveData.pid);

	setDrivePower(autoDrive, slaveCoeff*driveData.direction*power, driveData.direction*power);

	float leftDist = driveEncoderVal(autoDrive, LEFT, driveData.rawValue);
	float rightDist = driveEncoderVal(autoDrive, RIGHT, driveData.rawValue);

	//calculate error value
	if (driveData.correctionType == GYRO) {
		driveData.error = sin(gyroVal(autoDrive, RADIANS));
	} else if (driveData.correctionType == ENCODER) {
		driveData.error = rightDist - leftDist;
	} else {
		driveData.error = 0;
	}

	driveData.totalDist += (leftDist + rightDist) / 2;
	if (driveEncoderVal(autoDrive) > driveData.minSpeed) driveData.timer = resetTimer(); //track timeout state
	resetDriveEncoders(autoDrive);
}

void driveStraightEnd() {
	//brake
	setDrivePower(autoDrive, -driveData.direction * DRIVE_BRAKE_POWER, -driveData.direction * DRIVE_BRAKE_POWER);
	int brakeDelay = limit(0, DRIVE_BRAKE_DURATION, driveData.waitAtEnd);
	wait1Msec(brakeDelay);
	setDrivePower(autoDrive, 0, 0);

	wait1Msec(driveData.waitAtEnd>DRIVE_BRAKE_DURATION ? driveData.waitAtEnd-DRIVE_BRAKE_DURATION : 0);
	driveData.isDriving = false;
}

task driveStraightTask() {
	while (!drivingComplete()) {
		driveStraightRuntime();

		wait1Msec(driveData.sampleTime);
	}
	driveStraightEnd();
}

void setCorrectionType(correctionType type) {
	if (type==GYRO && autoDrive.hasGyro) {
		driveData.correctionType = GYRO;
	} else if (type==ENCODER && autoDrive.leftDrive.hasEncoder && autoDrive.rightDrive.hasEncoder) {
		driveData.correctionType = ENCODER;
	} else {
		driveData.correctionType = NONE;
	}
}

void _driveStraight_(parallel_drive &drive, float distance, float a, float b, float c, bool runAsTask=false, float kP=0.25, float kI=0.25, float kD=0.25, correctionType correctionType=AUTO, bool rawValue=false, float minSpeed=3, int timeout=800, int waitAtEnd=250, int sampleTime=50) {
	//initialize variables
	autoDrive = drive;
	driveData.distance = abs(distance);
	driveData.a = a;
	driveData.b = b;
	driveData.c = c;
	driveData.direction = sgn(distance);
	driveData.rawValue = rawValue;
	driveData.minSpeed = minSpeed * sampleTime / 1000;
	driveData.timeout = timeout;
	driveData.waitAtEnd = waitAtEnd;
	driveData.sampleTime = sampleTime;
	driveData.isDriving = true;
	//configure PID
	driveData.pid.input = &(driveData.error);
	driveData.pid.kP = kP;
	driveData.pid.kI = kI;
	driveData.pid.kD = kD;
	driveData.pid.target = 0;

	driveData.totalDist = 0;
	driveData.error = 0;

	if (correctionType == AUTO) {
		setCorrectionType(ENCODER);

		if (driveData.correctionType == NONE) {
			setCorrectionType(GYRO);
		}
	} else {
		setCorrectionType(correctionType);
	}

	//initialize sensors
	resetDriveEncoders(autoDrive);
	resetGyro(autoDrive);

	driveData.timer = resetTimer();

	if (runAsTask) {
		startTask(driveStraightTask);
	} else { //runs as function
		while (!drivingComplete()) {
			driveStraightRuntime();
			wait1Msec(driveData.sampleTime);
		}
		driveStraightEnd();
	}
}

void driveStraight(parallel_drive &drive, float distance, bool runAsTask=defDriveBools[0], int initialPower=defDriveInts[0], int maxPower=defDriveInts[1], int finalPower=defDriveInts[2], float kP=defDriveFloats[0], float kI=defDriveFloats[1], float kD=defDriveFloats[2], correctionType correctionType=defCorrectionType, bool rawValue=defDriveBools[1], float minSpeed=defDriveFloats[3], int timeout=defDriveInts[3], int waitAtEnd=defDriveInts[4], int sampleTime=defDriveInts[5]) {
	float a = (pow(distance, 2) * (finalPower+initialPower-2*maxPower) - 2*sqrt(pow(distance, 4) * (finalPower-maxPower) * (initialPower-maxPower))) / pow(distance, 4);
	float b = ((finalPower-initialPower)/distance - a*distance) * sgn(distance);

	_driveStraight_(drive, distance, a, b, initialPower, runAsTask, kP, kI, kD, correctionType, rawValue, minSpeed, timeout, waitAtEnd, sampleTime);
}

void setDriveDefaults(bool runAsTask, int initialPower=defDriveInts[0], int maxPower=defDriveInts[1], int finalPower=defDriveInts[2], float kP=defDriveFloats[0], float kI=defDriveFloats[1], float kD=defDriveFloats[2], correctionType correctionType=defCorrectionType, bool rawValue=defDriveBools[1], float minSpeed=defDriveFloats[3], int timeout=defDriveInts[3], int waitAtEnd=defDriveInts[4], int sampleTime=defDriveInts[5]) {
	defCorrectionType = correctionType;

	bool defBools[2] = { runAsTask, rawValue };
	int defInts[6] = { initialPower, maxPower, finalPower, timeout, waitAtEnd, sampleTime };
	float defFloats[4] = { kP, kI, kD, minSpeed };

	defDriveBools = defBools;
	defDriveInts = defInts;
	defDriveFloats = defFloats;
}
//end driveStraight region


///////////////////////////////////////
//////////////    amt    //////////////
///////////////////////////////////////

#include "parallelDrive.c"
#include "motorGroup.c"

motorGroup feed;

void autonomous() {
	driveStraight(drive, 24);
	//debug = gyroVal(drive);
	turn(drive, -90);
	//debug = gyroVal(drive) - debug;
	driveStraight(drive, 48);
}

task main() {
	initializeDrive(drive);
	setDriveMotors(drive, 4, lfd, lbd, rfd, rbd);
	attachGyro(drive, Yaw);
	attachEncoder(drive, leftE, LEFT);
	attachEncoder(drive, rightE, RIGHT, true);

	initializeGroup(feed, 2, Seymore, FeedMe);
	configureButtonInput(feed, Btn7U, Btn7D);

	while (true) {
		driveRuntime(drive);
		updatePosition(drive);

		takeInput(feed);

		if (vexRT[Btn5U] == 1) autonomous();

		if (vexRT[Btn5D] == 1) drive.width = calculateWidth(drive);

		if (vexRT[Btn6D] == 1) {
			setRobotPosition(drive, 0, 0, 0);
			resetGyro(drive);
		}
	}
}