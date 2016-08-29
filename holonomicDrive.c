#include "coreIncludes.c"
#include "motorGroup.c"

typedef struct {
	tMotor motors[4]; //left front, right front, left back, right back
	TVexJoysticks xInput, yInput, turnInput;
	int deadBand;
} holonomicDrive;

void initializeDrive(holonomicDrive *drive, tMotor lf_motor, tMotor rf_motor, tMotor lb_motor, tMotor rb_motor, int deadBand=15, TVexJoysticks xInput=Ch4, TVexJoysticks yInput=Ch3) {
	tMotor tempMotors[4] = { lf_motor, rf_motor, lb_motor, rb_motor };

	for (int i=0; i<4; i++)
		drive->motors[i] = tempMotors[i];

	drive->xInput = xInput;
	drive->yInput = yInput;
	drive->deadBand = deadBand;
}

void setDrivePower(holonomicDrive *drive, int leftPower, int rightPower, bool diagonal=true) {
	motor[ drive->motors[0] ] = leftPower;
	motor[ drive->motors[1] ] = rightPower;
	motor[ drive->motors[2] ] = (diagonal ? rightPower : leftPower);
	motor[ drive->motors[3] ] = (diagonal ? leftPower : rightPower);
}

void setDrivePowerByVector(holonomicDrive *drive, float x, float y) { //sets motor powers so that drive exerts a force along <x, y> with magnitude proportional to its length
	setDrivePower(drive, (x+y)/sqrt(2), (y-x)/sqrt(2));
}

void setDrivePowerByAngle(holonomicDrive *drive, float angle, float magnitude=0, angleType inputType=DEGREES) { //sets motor powers to exert force in a specified direction with a specified magnitude
	angle = convertAngle(angle, RADIANS, inputType);

	if (magnitude == 0) //calculate maximum magnitude in specified direction
		magnitude = 127*sqrt(2) / ((abs(tan(angle)) + 1) * cos(angle));

	setDrivePowerByVector(drive, magnitude*cos(angle), magnitude*sin(angle));
}

void driveRuntime(holonomicDrive *drive) {
	int inX = vexRT[ drive->xInput ];
	int inY = vexRT[ drive->yInput ];

	float squareX, squareY;

	if (abs(inX) > drive->deadBand || abs(inY) > drive->deadBand) {
		if (inX != 0) {
			//Input transformed from joystick circle to velocity square. If you want more detail ask Tynan. Sorry.
			squareX = sgn(inX)*sqrt(2) / (abs(inY/inX) + 1);
			squareY = squareX * inY/inX;
		} else {
			squareX = 0;
			squareY = sqrt(2)*sgn(inY);
		}

		float magnitude = (inX*inX + inY*inY) / 127.0;

		setDrivePowerByVector(drive, squareX*magnitude, squareY*magnitude);
	} else {
		int turnPower = vexRT[ drive->turnInput ];

		setDrivePower(drive, -turnPower, turnPower, false);
	}
}
