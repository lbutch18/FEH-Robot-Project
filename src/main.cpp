#include <FEH.h>
#include <Arduino.h>


// Declare things like Motors, Servos, etc. here
// For example:
// FEHMotor leftMotor(FEHMotor::Motor0, 6.0);
// FEHServo servo(FEHServo::Servo0);

#define COUNTS_PER_INCH 33.7408479392
#define COUNTS_PER_DEGREE 0.88333333333 // TEST
FEHMotor leftMotor(FEHMotor::Motor0, 12.0);
FEHMotor rightMotor(FEHMotor::Motor1, 12.0);
DigitalEncoder leftEncoder(FEHIO::Pin0);
DigitalEncoder rightEncoder(FEHIO::Pin1);

/* Drive a set amount of inches with motors at set percent, then stop*/
void driveThenStop(float inches, int percent) {
    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    leftMotor.SetPercent(percent);
    rightMotor.SetPercent(percent);
    float counts = inches * COUNTS_PER_INCH;
    while (leftEncoder.Counts() + rightEncoder.Counts() < counts * 2);
    leftMotor.Stop();
    rightMotor.Stop();
}

/* Rotate in place a set degree rotation at set motor percent, then stop*/
void rotateInPlaceThenStop(float degrees, int percent) {
    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    leftMotor.SetPercent(percent);
    rightMotor.SetPercent(-percent);
    float counts = degrees * COUNTS_PER_DEGREE;
    while (leftEncoder.Counts() + rightEncoder.Counts() < counts * 2);
    leftMotor.Stop();
    rightMotor.Stop();
}

void ERCMain()
{
    // Shaft encoder testing
    
    int x, y;
    LCD.Clear();
    while(!LCD.Touch(&x, &y));
    driveThenStop(6, 50);
    LCD.WriteLine("Left: " + String(leftEncoder.Counts()));
    LCD.WriteLine("Right: " + String(rightEncoder.Counts()));
    while(!LCD.Touch(&x, &y));
    LCD.Clear();
    rotateInPlaceThenStop(90, 50);
    LCD.WriteLine("Left: " + String(leftEncoder.Counts()));
    LCD.WriteLine("Right: " + String(rightEncoder.Counts()));
    while(!LCD.Touch(&x, &y));
    LCD.Clear();
    rotateInPlaceThenStop(-90, 50);
    LCD.WriteLine("Left: " + String(leftEncoder.Counts()));
    LCD.WriteLine("Right: " + String(rightEncoder.Counts()));

}