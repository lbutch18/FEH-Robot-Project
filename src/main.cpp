#include <FEH.h>
#include <Arduino.h>


// Declare things like Motors, Servos, etc. here
// For example:
// FEHMotor leftMotor(FEHMotor::Motor0, 6.0);
// FEHServo servo(FEHServo::Servo0);

#define COUNTS_PER_INCH 33.7408479392
#define COUNTS_PER_DEGREE 2.33 // TEST
#define COUNTS_PER_DEGREE_180 2.25 // NEED CORRECTIVE ALGO
FEHMotor leftMotor(FEHMotor::Motor0, 12.0);
FEHMotor rightMotor(FEHMotor::Motor3, 12.0); // BACKWARDS
DigitalEncoder leftEncoder(FEHIO::Pin8);
DigitalEncoder rightEncoder(FEHIO::Pin9);

/* Drive a set amount of inches with motors at set percent, then stop*/
void driveThenStop(float inches, int percent) {
    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    leftMotor.SetPercent(percent);
    rightMotor.SetPercent(-percent);
    float counts = inches * COUNTS_PER_INCH;
    while ((leftEncoder.Counts() + rightEncoder.Counts()) / 2.0 < counts);
    leftMotor.Stop();
    rightMotor.Stop();
}

/* Rotate in place a set degree rotation at set motor percent, then stop*/
void rotateInPlaceThenStop(float degrees, int percent) {
    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    
    float counts = fabs(degrees) * COUNTS_PER_DEGREE;
    if (degrees < 0) { // LEFT
        leftMotor.SetPercent(-percent);
        rightMotor.SetPercent(-percent);
    } else { // RIGHT
        leftMotor.SetPercent(percent);
        rightMotor.SetPercent(percent);
    }
    
    while ((leftEncoder.Counts() + rightEncoder.Counts()) / 2.0 < counts);
    leftMotor.Stop();
    rightMotor.Stop();
}

void testShaftEncoder()
{
    LCD.Clear();
    LCD.WriteLine("Testing shaft encoders");
    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    while(true)
    {
        LCD.Clear();
        LCD.WriteLine(leftEncoder.Counts());
        LCD.WriteLine(rightEncoder.Counts());
    }
}

void driveToAndFromWall() {
    driveThenStop(30, 20);
    driveThenStop(25, -20);
}

void driveUpAndDownRamp() {
    driveThenStop(30, 30);
    driveThenStop(30, -30);
}

void ERCMain()
{
    // Shaft encoder testing
    
    int x, y;
    LCD.Clear();
    while(!LCD.Touch(&x, &y));
    driveToAndFromWall();
    rotateInPlaceThenStop(90, 20);
    driveUpAndDownRamp();
}