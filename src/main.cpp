#include <FEH.h>
#include <Arduino.h>


// Declare things like Motors, Servos, etc. here
// For example:
// FEHMotor leftMotor(FEHMotor::Motor0, 6.0);
// FEHServo servo(FEHServo::Servo0);

#define COUNTS_PER_INCH 33.7408479392
#define COUNTS_PER_DEGREE 2.1111111111 // TEST
#define CDS_THRESHOLD_RED 2
#define CDS_THRESHOLD_BLUE 4 
FEHMotor leftMotor(FEHMotor::Motor0, 12.0);
FEHMotor rightMotor(FEHMotor::Motor3, 12.0); // PORT 1 IS BACKWARDS
DigitalEncoder leftEncoder(FEHIO::Pin8);
DigitalEncoder rightEncoder(FEHIO::Pin9);
AnalogInputPin CDSCell(FEHIO::Pin2);
AnalogInputPin leftOpto(FEHIO::Pin3);
AnalogInputPin rightOpto(FEHIO::Pin4);
AnalogInputPin middleOpto(FEHIO::Pin5);

enum LineState {
    LEFT,
    MIDDLE,
    RIGHT
};
LineState state = MIDDLE;

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

void pivotThenStop(float degrees, int percent) {
    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    
    float counts = fabs(degrees) * COUNTS_PER_DEGREE;
    if (degrees < 0) { // LEFT
        leftMotor.SetPercent(0);
        rightMotor.SetPercent(-percent);
    } else { // RIGHT
        leftMotor.SetPercent(percent);
        rightMotor.SetPercent(0);
    }
    
    while ((leftEncoder.Counts() + rightEncoder.Counts()) / 2.0 < counts);
    leftMotor.Stop();
    rightMotor.Stop();
}

void driveUpRamp() {
    driveThenStop(30, 20);
}

void followLineOnce(int straightPercent) {
    float highThreshold = 5;
    float lowThreshold = 3.3;

    bool L= (leftOpto.Value()<highThreshold) && (leftOpto.Value()>lowThreshold);
    bool M= (middleOpto.Value()<highThreshold) && (middleOpto.Value()>lowThreshold);
    bool R= (rightOpto.Value()<highThreshold) && (rightOpto.Value()>lowThreshold);

    if(L) {
        state=LEFT;
    }
    else if(M) {
        state=MIDDLE;
    }
    else if (R) {
        state= RIGHT;
    }

    switch(state) {

    // If I am in the middle of the line...
    case MIDDLE:
        leftMotor.SetPercent(-straightPercent);
        rightMotor.SetPercent(-straightPercent);// drives straight
    break;

    case LEFT:
        leftMotor.SetPercent(-straightPercent);
        rightMotor.SetPercent(0); //turns wheels right
    break;

    case RIGHT:
        leftMotor.SetPercent(0);
        rightMotor.SetPercent(-straightPercent); //turns wheels left
    break;

    default: // Error
        leftMotor.SetPercent(0); rightMotor.SetPercent(0);
    break;
    }
    Sleep(.005);
}

void activateStartButton() {
    driveThenStop(5, -30);
}

void rotateAfterStart() {
    driveThenStop(5, 30);
    pivotThenStop(90, 20);
    pivotThenStop(-45, 20); // will likely need to adjust
}

// Returns true if red light, false if blue light, and displays color on LCD
bool getCDSValueAndDisplayColor() {
    float value = CDSCell.Value();
    if (value > CDS_THRESHOLD_RED) {
        LCD.WriteLine("Light color: Blue");
        return false;
    } else {
        LCD.WriteLine("Light color: Red");
        return true;
    }
}

void followLineToLight() {
    state = MIDDLE;
    while (CDSCell.Value() > CDS_THRESHOLD_BLUE) { // change condition
        followLineOnce(25);
    }
}

void pressLightButton(bool colorIsRed) {
    if (colorIsRed) {
        // drive right to align with red button
        pivotThenStop(45, 20);
        pivotThenStop(-45, 20);
    }
    else {
        //drive left to align with blue button
        pivotThenStop(-45, 20);
        pivotThenStop(45, 20);
    }
    driveThenStop(5, 20);
}

void ERCMain()
{
    RCS.InitializeTouchMenu("D4");
    while(CDSCell.Value() > CDS_THRESHOLD_RED); // Wait for start light
    activateStartButton();
    rotateAfterStart();
    driveUpRamp();
    followLineToLight();
    bool colorIsRed = getCDSValueAndDisplayColor();
    pressLightButton(colorIsRed);

}
