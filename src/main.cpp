#include <FEH.h>
#include <Arduino.h>


// Declare things like Motors, Servos, etc. here
// For example:
// FEHMotor leftMotor(FEHMotor::Motor0, 6.0);
// FEHServo servo(FEHServo::Servo0);

#define COUNTS_PER_INCH 33.7408479392
#define COUNTS_PER_DEGREE 2.25 // TEST
#define CDS_THRESHOLD_RED 1.4
#define CDS_THRESHOLD_BLUE 2.2
#define SECONDS_PER_INCH 1.0 // Adjust based on arm speed
FEHMotor leftMotor(FEHMotor::Motor0, 12.0);
FEHMotor rightMotor(FEHMotor::Motor3, 12.0); // BACKWARDS
DigitalEncoder leftEncoder(FEHIO::Pin13);
DigitalEncoder rightEncoder(FEHIO::Pin9);
AnalogInputPin CDSCell(FEHIO::Pin2);
AnalogInputPin leftOpto(FEHIO::Pin0);
AnalogInputPin rightOpto(FEHIO::Pin4);
AnalogInputPin middleOpto(FEHIO::Pin5);
FEHServo smallArm(FEHServo::Servo0); // Continuous
FEHMotor largeArm(FEHMotor::Motor1, 5.0);
FEHMotor compostBinWheel(FEHMotor::Motor2, 5.0);

enum LineState {
    LEFT,
    MIDDLE,
    RIGHT
};
LineState state = MIDDLE;

float getNormalizedRotation(float currentHeading, float targetHeading) {
    float rotation = targetHeading - currentHeading;
    if (rotation < -180) {
        rotation += 360;
    }
    else if (rotation > 180) {
        rotation -= 360;
    }
    return rotation;
}

void testOptosensors() {
    int x, y;
    while(true) {
        while(!LCD.Touch(&x, &y));
        LCD.Clear();
        LCD.WriteLine(leftOpto.Value());
        LCD.WriteLine(middleOpto.Value());
        LCD.WriteLine(rightOpto.Value());
    }
}

/* Drive a set amount of inches with motors at set percent, then stop*/
void driveThenStop(float inches, int percent) {
    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    leftMotor.SetPercent(percent);
    rightMotor.SetPercent(-percent);
    float counts = inches * COUNTS_PER_INCH;
    if (percent > 0) {
        while ((leftEncoder.Counts() + rightEncoder.Counts()) / 2.0 < counts) {
            if (leftEncoder.Counts() > rightEncoder.Counts() + 5) {
                leftMotor.SetPercent(percent);
                rightMotor.SetPercent(-percent - 3);
            } else if (rightEncoder.Counts() > leftEncoder.Counts() + 5) {
                leftMotor.SetPercent(percent + 3);
                rightMotor.SetPercent(-percent);
            } else {
                leftMotor.SetPercent(percent);
                rightMotor.SetPercent(-percent);
            }
        }
        LCD.Clear();
        LCD.WriteLine(leftEncoder.Counts());
        LCD.WriteLine(rightEncoder.Counts());
    } else { // driving backwards
        while ((leftEncoder.Counts() + rightEncoder.Counts()) / 2.0 < counts) {
            if (leftEncoder.Counts() > rightEncoder.Counts() + 5) {
                leftMotor.SetPercent(percent + 3);
                rightMotor.SetPercent(-percent);
            } else if (rightEncoder.Counts() > leftEncoder.Counts() + 5) {
                leftMotor.SetPercent(percent);
                rightMotor.SetPercent(-percent - 3);
            } else {
                leftMotor.SetPercent(percent);
                rightMotor.SetPercent(-percent);
            }
        }
        LCD.Clear();
        LCD.WriteLine(leftEncoder.Counts());
        LCD.WriteLine(rightEncoder.Counts());
    }

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
    while ((leftEncoder.Counts() + rightEncoder.Counts()) / 2.0 < counts) {
        if (leftEncoder.Counts() > rightEncoder.Counts() + 3) {
                leftMotor.SetPercent(percent);
                rightMotor.SetPercent(-percent - 2);
            } else if (rightEncoder.Counts() > leftEncoder.Counts() + 3) {
                leftMotor.SetPercent(percent + 2);
                rightMotor.SetPercent(-percent);
            } else {
                leftMotor.SetPercent(percent);
                rightMotor.SetPercent(-percent);
            }
    }
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

void inversePivotThenStop(float degrees, int percent) {
    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    
    float counts = fabs(degrees) * COUNTS_PER_DEGREE;
    if (degrees < 0) { // LEFT
        leftMotor.SetPercent(-percent);
        rightMotor.SetPercent(0);
    } else { // RIGHT
        leftMotor.SetPercent(0);
        rightMotor.SetPercent(percent);
    }
    
    while ((leftEncoder.Counts() + rightEncoder.Counts()) / 2.0 < counts);
    leftMotor.Stop();
    rightMotor.Stop();
}

void driveUpRamp() {
    driveThenStop(32, 20);
}

void followLineOnce(int straightPercent) {
    float highThreshold = 5;
    float lowThreshold = 4.65;

    bool L= (leftOpto.Value()<highThreshold) && (leftOpto.Value()>lowThreshold);
    bool M= (middleOpto.Value()<highThreshold) && (middleOpto.Value()>lowThreshold);
    bool R= (rightOpto.Value()<highThreshold) && (rightOpto.Value()>lowThreshold);

    if (M) {
        state = MIDDLE;
    }
    else if (L && M) {
         state = MIDDLE;
    }
    else if (L && !R) {
        state = RIGHT;
    }
    else if (R && M) {
        state = MIDDLE;
    }

    switch(state) {

    // If I am in the middle of the line...
    case MIDDLE:
        leftMotor.SetPercent(straightPercent);
        rightMotor.SetPercent(-straightPercent);// drives straight
    break;

    case LEFT:
        leftMotor.SetPercent(straightPercent);
        rightMotor.SetPercent(0); //turns wheels right
    break;

    case RIGHT:
        leftMotor.SetPercent(0);
        rightMotor.SetPercent(-straightPercent); //turns wheels left
    break;

    default: // Error
        leftMotor.SetPercent(0); 
        rightMotor.SetPercent(0);
    break;
    }
}

void activateStartButton() {
    driveThenStop(2, -22);
}

void rotateAfterStartM2() {
    pivotThenStop(80, 16);
    driveThenStop(1, 16);
    pivotThenStop(-29, 16);
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
        followLineOnce(16);
    }
    leftMotor.SetPercent(0);
    rightMotor.SetPercent(0);
    Sleep(.5);
}

void pressLightButton(bool colorIsRed) {
    if (colorIsRed) {
        // drive right to align with red button
        pivotThenStop(45, 16);
        pivotThenStop(-50, 16);
    }
    else {
        //drive left to align with blue button
        pivotThenStop(-45, 20);
        pivotThenStop(50, 20);
    }
    driveThenStop(2, 30);
}

void driveToLight() {
    rotateInPlaceThenStop(-100, 20);
    leftMotor.SetPercent(16);
    rightMotor.SetPercent(-16);
    while (CDSCell.Value() > CDS_THRESHOLD_BLUE);
    leftMotor.Stop();
    rightMotor.Stop();
    Sleep(.5);
}

void followLineToEnd() {
    state = MIDDLE;
    while (true) { // change condition
        followLineOnce(16);
    }
}

void testRCS() {
    int x, y;
    while(!LCD.Touch(&x, &y));
    while(true) {
        while(!LCD.Touch(&x, &y));
        LCD.Clear();
        RCS.RequestPosition(false);

        Sleep(.5);

        
        // Check if position data has arrived
        RCSPose* pose = RCS.Position();
        if (pose != nullptr) {
            LCD.WriteRC(pose->x, 0, 0);
            LCD.WriteRC(pose->y, 1, 0);
            LCD.WriteRC(pose->heading, 2, 0);
        } else {
            LCD.WriteLine("Position not ready");
        }
    }
}

void moveLargeArmInches(float inches) {
    if (inches < 0) {
        largeArm.SetPercent(35);
    } else {
        largeArm.SetPercent(-35);
    }
    Sleep(fabs(inches) * SECONDS_PER_INCH); // Adjust sleep time based on arm speed
    largeArm.Stop();
}

void driveToPosition(float targetX, float targetY, float targetHeading) {
    // Get current position and heading
    float currentX;
    float currentY;
    float currentHeading;
    RCSPose* pose = RCS.RequestPosition();
    if (pose != nullptr) {
        currentX = pose->x;
        currentY = pose->y;
        currentHeading = pose->heading;
    } else {
        LCD.WriteLine("No RCS response");
        Sleep(5);
    }

    // rotate to FACE target -- targetheading needs updating with trig
    float faceTargetHeading = atan2(targetY - currentY, targetX - currentX) * 180 / M_PI;
    float rotation = getNormalizedRotation(currentHeading, faceTargetHeading);
    rotateInPlaceThenStop(rotation, 16);
}

void driveToCompostBin() {
    driveThenStop(3.35, 16);
    pivotThenStop(-45, 16);
    driveThenStop(11.75, 16);
    pivotThenStop(-15, 16);
}

void spinCompostBin() {
    rightMotor.SetPercent(-12);
    Sleep(.25);
    compostBinWheel.SetPercent(-80);
    Sleep(1.85);
    compostBinWheel.Stop();
    Sleep(0.25);
    compostBinWheel.SetPercent(80);
    Sleep(1.7);
    compostBinWheel.Stop();
    rightMotor.Stop();
    rightEncoder.ResetCounts();
    leftEncoder.ResetCounts();
}

void driveBackToStartFromBin() {
    driveThenStop(2, -26);
    rotateInPlaceThenStop(30, 16);
    driveThenStop(8, -16);
    rotateInPlaceThenStop(45, 16);
    driveThenStop(12, -16);
}

void ERCMain()
{
    // testOptosensors();
    // testGUI();
    // RCS.InitializeTouchMenu("D4");
    // RCS.DisableRateLimit();
    int x, y;
    while(!LCD.Touch(&x, &y)); // Wait for initial touch to start program
    // while(CDSCell.Value() > 2.5); // Wait for start light
    activateStartButton();
    driveToCompostBin();
    spinCompostBin();
    driveBackToStartFromBin();


    /* MILESTONE 4 */
    // driveThenStop(22, 16);
    // rotateInPlaceThenStop(-48, 16);
    // //correctWithRCS(12.23, 20.59, 90);
    // Sleep(.75);
    // // pick up apple bucket 
    // driveThenStop(1.25, 16);
    // Sleep(.5);
    // moveLargeArmInches(2.25);
    // Sleep(.5);
    // // RCS
    // rotateInPlaceThenStop(20, 16);
    // driveThenStop(19.5, -16);
    // rotateInPlaceThenStop(58, 16);
    // Sleep(.5);
    // //correctWithRCS(32.13, 13.77, 0);
    // Sleep(.5);
    // driveThenStop(31, 25); 
    // Sleep(.5);
    // // RCS
    // rotateInPlaceThenStop(-45, 16);
    // driveThenStop(7.5, 16);
    // rotateInPlaceThenStop(48, 16);
    // driveThenStop(9.5, 25);
    // Sleep(.75);
    // moveLargeArmInches(-2.25);
    // driveThenStop(2, -20);
    // Sleep(.5);
    // moveLargeArmInches(2.25);
    // rotateInPlaceThenStop(-40, 16);
    // driveThenStop(15, 20);



    // RCS
    // drop apple bucket
    // RCS
    // follow line to correct lever
    // move lever down, wait 5, move lever up
    // drive back to top of ramp
    // RCS
    
    /* MILESTONE 2 */
    // rotateAfterStartM2();
    // driveUpRamp();
    // driveToLight();
    // bool colorIsRed = getCDSValueAndDisplayColor();
    // pressLightButton(colorIsRed);
    // driveThenStop(36, -20);
    // pivotThenStop(-90, 16);
    // driveThenStop(30, 16);

    /* MILESTONE 3 */
    // rotateAfterStartM2();
    // driveUpRamp();
    // rotateInPlaceThenStop(-90, 16);
    // driveThenStop(8, -22);
    // pivotThenStop(-15, 16);
    // driveThenStop(10, 16);
    // rotateInPlaceThenStop(15, 16);
    // driveThenStop(10, 16);
    // driveThenStop(4, -16);
    // pivotThenStop(30, 16);
    // driveThenStop(1, 16);
    // pivotThenStop(-30, 16);
    // driveThenStop(1, 16);
    // pivotThenStop(-30, 16);
    // driveThenStop(1, 16);
    // inversePivotThenStop(30, 16);
    // driveThenStop(20, -16);

}
