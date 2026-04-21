#include <FEH.h>
#include <Arduino.h>


// Declare things like Motors, Servos, etc. here
// For example:
// FEHMotor leftMotor(FEHMotor::Motor0, 6.0);
// FEHServo servo(FEHServo::Servo0);

#define COUNTS_PER_INCH 33.7408479392
#define COUNTS_PER_DEGREE 2.3275
#define CDS_THRESHOLD_RED 1.45
#define CDS_THRESHOLD_BLUE 2.5
#define SECONDS_PER_INCH 1.0 // Adjust based on arm speed
#define INCHES_PER_COORD_X 1
#define INCHES_PER_COORD_Y 1
FEHMotor leftMotor(FEHMotor::Motor0, 9.0);
FEHMotor rightMotor(FEHMotor::Motor3, 9.0); // BACKWARDS
DigitalEncoder leftEncoder(FEHIO::Pin13);
DigitalEncoder rightEncoder(FEHIO::Pin9);
AnalogInputPin CDSCell(FEHIO::Pin2);
AnalogInputPin leftOpto(FEHIO::Pin7);
AnalogInputPin rightOpto(FEHIO::Pin3);
AnalogInputPin middleOpto(FEHIO::Pin5);
FEHServo smallArm(FEHServo::Servo0); // Continuous, 90 deg = 0 percent
FEHMotor largeArm(FEHMotor::Motor2, 5.0);
FEHMotor compostBinWheel(FEHMotor::Motor1, 5.0);

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
            if (leftEncoder.Counts() < 60 && rightEncoder.Counts() < 60) {
                continue;
            }
            if (leftEncoder.Counts() > rightEncoder.Counts() + 5) {
                leftMotor.SetPercent(percent);
                rightMotor.SetPercent(-percent - 4);
            } else if (rightEncoder.Counts() > leftEncoder.Counts() + 5) {
                leftMotor.SetPercent(percent + 4);
                rightMotor.SetPercent(-percent);
            } else {
                leftMotor.SetPercent(percent);
                rightMotor.SetPercent(-percent);
            }
        }
    } else { // driving backwards
        while ((leftEncoder.Counts() + rightEncoder.Counts()) / 2.0 < counts) {
            if (leftEncoder.Counts() < 60 && rightEncoder.Counts() < 60) {
                continue;
            }
            if (leftEncoder.Counts() > rightEncoder.Counts() + 5) {
                leftMotor.SetPercent(percent + 4);
                rightMotor.SetPercent(-percent);
            } else if (rightEncoder.Counts() > leftEncoder.Counts() + 5) {
                leftMotor.SetPercent(percent);
                rightMotor.SetPercent(-percent - 4);
            } else {
                leftMotor.SetPercent(percent);
                rightMotor.SetPercent(-percent);
            }
        }
    }

    leftMotor.Stop();
    rightMotor.Stop();
}

void driveThenStopWithTimeout(float inches, int percent, float timeout) {
    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    leftMotor.SetPercent(percent);
    rightMotor.SetPercent(-percent);
    float counts = inches * COUNTS_PER_INCH;
    float startTime = TimeNow();
    if (percent > 0) {
        while ((leftEncoder.Counts() + rightEncoder.Counts()) / 2.0 < counts && TimeNow() - startTime < timeout) {
            if (leftEncoder.Counts() < 60 && rightEncoder.Counts() < 60) {
                continue;
            }
            if (leftEncoder.Counts() > rightEncoder.Counts() + 5) {
                leftMotor.SetPercent(percent);
                rightMotor.SetPercent(-percent - 4);
            } else if (rightEncoder.Counts() > leftEncoder.Counts() + 5) {
                leftMotor.SetPercent(percent + 4);
                rightMotor.SetPercent(-percent);
            } else {
                leftMotor.SetPercent(percent);
                rightMotor.SetPercent(-percent);
            }
        }
    } else { // driving backwards
        while ((leftEncoder.Counts() + rightEncoder.Counts()) / 2.0 < counts && TimeNow() - startTime < timeout) {
            if (leftEncoder.Counts() < 60 && rightEncoder.Counts() < 60) {
                continue;
            }
            if (leftEncoder.Counts() > rightEncoder.Counts() + 5) {
                leftMotor.SetPercent(percent + 4);
                rightMotor.SetPercent(-percent);
            } else if (rightEncoder.Counts() > leftEncoder.Counts() + 5) {
                leftMotor.SetPercent(percent);
                rightMotor.SetPercent(-percent - 4);
            } else {
                leftMotor.SetPercent(percent);
                rightMotor.SetPercent(-percent);
            }
        }
    }

    leftMotor.Stop();
    rightMotor.Stop();
}

/* Drive a set amount of inches with motors at set percent, then stop*/
void correctionDriveThenStop(float inches, int percent) {
    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    leftMotor.SetPercent(percent);
    rightMotor.SetPercent(-percent);
    float counts = inches * COUNTS_PER_INCH;
    leftMotor.SetPercent(percent);
    rightMotor.SetPercent(-percent);
    while ((leftEncoder.Counts() + rightEncoder.Counts()) / 2.0 < counts);
    leftMotor.Stop();
    rightMotor.Stop();
}

/* Rotate in place a set degree rotation at set motor percent, then stop*/
void rotateInPlaceThenStop(float degrees, int percent) {
    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    
    float counts = fabs(degrees) * COUNTS_PER_DEGREE;
    
    if (degrees > 0) { // RIGHT (clockwise)
        leftMotor.SetPercent(percent);
        rightMotor.SetPercent(percent);
    } else { // LEFT (counter-clockwise)
        leftMotor.SetPercent(-percent);
        rightMotor.SetPercent(-percent);
    }
    
    while ((leftEncoder.Counts() + rightEncoder.Counts()) / 2.0 < counts) {
        if (degrees > 0) { // RIGHT (clockwise)
            if (leftEncoder.Counts() > rightEncoder.Counts() + 3) {
                leftMotor.SetPercent(percent - 2);
                rightMotor.SetPercent(percent);
            } else if (rightEncoder.Counts() > leftEncoder.Counts() + 3) {
                leftMotor.SetPercent(percent);
                rightMotor.SetPercent(percent - 2);
            } else {
                leftMotor.SetPercent(percent);
                rightMotor.SetPercent(percent);
            }
        } else { // LEFT (counter-clockwise)
            if (leftEncoder.Counts() > rightEncoder.Counts() + 3) {
                leftMotor.SetPercent(-percent + 2);
                rightMotor.SetPercent(-percent);
            } else if (rightEncoder.Counts() > leftEncoder.Counts() + 3) {
                leftMotor.SetPercent(-percent);
                rightMotor.SetPercent(-percent + 2);
            } else {
                leftMotor.SetPercent(-percent);
                rightMotor.SetPercent(-percent);
            }
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
    
    float startTime = TimeNow();
    while ((leftEncoder.Counts() + rightEncoder.Counts()) / 2.0 < counts && TimeNow() - startTime < 4.0) {
        LCD.Clear();
        LCD.WriteLine(leftEncoder.Counts());
        LCD.WriteLine(rightEncoder.Counts());
        Sleep(.1);
    }
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
    driveThenStop(28, 35);
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
    float time = TimeNow();
    leftMotor.SetPercent(-20);
    rightMotor.SetPercent(20);
    while(TimeNow() - time < .75);
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
    leftMotor.SetPercent(0);
    rightMotor.SetPercent(0);
    Sleep(.5);
}

void pressLightButton(bool colorIsRed) {
    if (colorIsRed) {
        // drive right to align with red button
        pivotThenStop(45, 25);
        pivotThenStop(-50, 25);
    }
    else {
        //drive left to align with blue button
        pivotThenStop(-45, 25);
        pivotThenStop(50, 25);
    }
    driveThenStopWithTimeout(2, 30, 2);
}

void followLineToEnd() {
    state = MIDDLE;
    while (true) { // change condition
        followLineOnce(25);
    }
}

void testRCS() {
    int x, y;
    while(!LCD.Touch(&x, &y));
    while(true) {
        while(!LCD.Touch(&x, &y));
        LCD.Clear();
        RCS.RequestPosition();
        
        // Check if position data has arrived
        RCSPose* pose = RCS.Position();
        if (pose != nullptr && pose->heading >= 0) {
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
        largeArm.SetPercent(-35);
    } else {
        largeArm.SetPercent(35);
    }
    Sleep(fabs(inches) * SECONDS_PER_INCH); // Adjust sleep time based on arm speed
    largeArm.Stop();
}


// void driveToPosition(float targetX, float targetY, float targetHeading) {
//     const float ANGLE_TOLERANCE = 1.5;
//     const float DISTANCE_TOLERANCE = 0.25;
    
//     driveToTarget(targetX, targetY, ANGLE_TOLERANCE, DISTANCE_TOLERANCE);
    
//     // Turn to target heading
//     Sleep(.25);
//     RCSPose* pose = RCS.RequestPosition();
//     float time = TimeNow();
//     while (RCS.Position() == nullptr && TimeNow() - time < 2);
//     if (pose != nullptr && pose->heading >= 0) {
//         float rotation = -getNormalizedRotation(pose->heading, targetHeading);
//         if (fabs(rotation) >= ANGLE_TOLERANCE) {
//             rotateInPlaceThenStop(rotation, 16);
//         }
//     }
// }

void correctHeading(float targetHeading) {
    Sleep(.35);
    const float ANGLE_TOLERANCE = .65;
    
    for (int i = 0; i < 3; i++) {
        RCSPose* pose = RCS.RequestPosition();
        float time = TimeNow();
        while (RCS.Position() == nullptr && TimeNow() - time < 2);
        if (pose != nullptr && pose->heading >= 0) {
            float rotation = -getNormalizedRotation(pose->heading, targetHeading);
            if (fabs(rotation) >= ANGLE_TOLERANCE) {
                rotateInPlaceThenStop(rotation, 16);
                LCD.WriteLine("Current heading: ");
                LCD.WriteLine(pose->heading);
                LCD.WriteLine("Correcting heading by: ");
                LCD.WriteLine(rotation);
            } else {
                return;
            }
        }
        Sleep(.35);
    }
}

void driveToTargetAfterCompost(float targetX, float targetY, float angleTolerance, float distanceTolerance) {
    Sleep(.25);
    LCD.Clear();
        
    // Get position and check distance
    RCSPose* pose = RCS.RequestPosition();
    for (int i = 0; i < 2; i++) {
        float time = TimeNow();
        while (RCS.Position() == nullptr && TimeNow() - time < 2);
        if (pose != nullptr || pose->x < 0) {
            break;
        }
        pose = RCS.RequestPosition();
        Sleep(.25);
    }
    if (pose == nullptr || pose->x < 0) {
        LCD.WriteLine("Position not ready");
        rotateInPlaceThenStop(140, 20);
        driveThenStop(13, 30);
        return;
    }
        
    float currentX = pose->x;
    float currentY = pose->y;
    float currentHeading = pose->heading;
        
    LCD.WriteLine("Current pos: ");
    LCD.WriteLine(currentX);
    LCD.WriteLine(currentY);
    LCD.WriteLine(currentHeading);
        
    float distance = sqrt(pow(targetX - currentX, 2) + pow(targetY - currentY, 2));
    if (distance < distanceTolerance) {
        return;
    }
        
    float faceTargetHeading = atan2(currentX - targetX, targetY - currentY) * 180 / M_PI;
    if (faceTargetHeading < 0) {
        faceTargetHeading += 360;
    }
    float rotation = -getNormalizedRotation(currentHeading, faceTargetHeading);
    LCD.WriteLine("Rotation: ");
    LCD.WriteLine(rotation);
            
    rotateInPlaceThenStop(rotation, 16);
    correctHeading(faceTargetHeading);
    
    driveThenStop(distance - .35, 28);
    Sleep(.25);
}

// Precondition: heading about 0 or 180
void correctY(float targetY) {
    Sleep(.35);
    const float DISTANCE_TOLERANCE = 0.25;

    for (int i = 0; i < 3; i++) {
        RCSPose* pose = RCS.RequestPosition();
        float time = TimeNow();
        while (RCS.Position() == nullptr && TimeNow() - time < 2);
        if (pose != nullptr && pose->heading >= 0) {
            float distance = targetY - pose->y;
            float percent = 12;
            if (pose-> heading > 90 && pose->heading < 270) { // facing south, backwards is positive
                percent = -percent;
            }
            if (fabs(distance) >= DISTANCE_TOLERANCE) {
                LCD.WriteLine("Current Y: ");
                LCD.WriteLine(pose->y);
                if (distance > 0) {
                    correctionDriveThenStop(.3, 12);
                } else {
                    correctionDriveThenStop(.3, -12);
                }
                LCD.WriteLine("Correcting Y by: ");
                LCD.WriteLine(distance);
            } else {
                return;
            }
        }
        Sleep(.35);
    }
}

// Precondition: heading about 90 or 270
void correctX(float targetX) {
    Sleep(.35);
    const float DISTANCE_TOLERANCE = 0.25;

    for (int i = 0; i < 3; i++) {
        RCSPose* pose = RCS.RequestPosition();
        float time = TimeNow();
        while (RCS.Position() == nullptr && TimeNow() - time < 2);
        if (pose != nullptr && pose->x >= 0) {
            float distance = targetX - pose->x;
            float percent = 12;
            if (pose-> heading > 180) { // facing left, backwards is positive
                percent = -percent;
            }
            if (fabs(distance) >= DISTANCE_TOLERANCE) {
                LCD.WriteLine("Current X: ");
                LCD.WriteLine(pose->x);
                if (distance > 0) {
                    correctionDriveThenStop(.35, -percent);
                } else {
                    correctionDriveThenStop(.35, percent);
                }
                LCD.WriteLine("Correcting X by: ");
                LCD.WriteLine(distance);
            } else {
                return;
            }
        }
        Sleep(.35);
    }
}

void driveToCompostBin() {
    driveThenStop(5.5, 25);
    pivotThenStop(-37.75, 16);
    correctHeading(90);
    leftMotor.SetPercent(25);
    rightMotor.SetPercent(-20);
    Sleep(.1);
    driveThenStopWithTimeout(12.8, 20, 25);
    pivotThenStop(-13.5, 16);
}

void driveToLight(int percent) {
    rotateInPlaceThenStop(-90, percent);
    correctHeading(90);

    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    leftMotor.SetPercent(percent);
    rightMotor.SetPercent(-percent);
    float start = TimeNow();
    while (CDSCell.Value() > CDS_THRESHOLD_BLUE && TimeNow() - start < 6.0) {
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
    leftMotor.Stop();
    rightMotor.Stop();
    driveThenStop(.25, -percent);
    Sleep(.35);
}

void spinCompostBin() {
    rightMotor.SetPercent(-12);
    Sleep(.25);
    compostBinWheel.SetPercent(80);
    Sleep(2.0);
    compostBinWheel.Stop();
    Sleep(0.25);
    compostBinWheel.SetPercent(-80);
    Sleep(1.7);
    compostBinWheel.Stop();
    rightMotor.Stop();
    rightEncoder.ResetCounts();
    leftEncoder.ResetCounts();
}

void driveToAppleBucket() {
    driveToTargetAfterCompost(17.25, 19.1, .67, .25);
    // rotateInPlaceThenStop(130, 16);
    // driveThenStop(12.35, 30);
    rotateInPlaceThenStop(-35, 16);
    correctHeading(0);
    correctY(19.1);
    rotateInPlaceThenStop(-90, 16);
    correctHeading(90);
}

void pickUpAppleBucket() {
    moveLargeArmInches(1.175);
    driveThenStop(4, 20);
    correctX(13.25);
    correctHeading(90);
    driveThenStopWithTimeout(3.5, 20, 3);
    Sleep(.35);
    moveLargeArmInches(2.25);
    Sleep(.35);
}

void driveToBottomOfRamp() {
    driveThenStop(1, -25);
    rotateInPlaceThenStop(45, 20);
    driveThenStop(6, -25);
    rotateInPlaceThenStop(-45, 20);
    correctHeading(90);
    driveThenStop(15.5, -30);
    rotateInPlaceThenStop(90, 20);
}

void driveToTableAndDropAppleBucket() {
    moveLargeArmInches(3.75);
    rotateInPlaceThenStop(90, 16);
    driveThenStopWithTimeout(6, 28, 3.5);
    driveThenStop(1.5, -16);
    rotateInPlaceThenStop(-90, 16);
    correctHeading(0);
    driveThenStopWithTimeout(8, 25, 3);
    moveLargeArmInches(-1.1);
    Sleep(.25);
    driveThenStop(3.15, -25);
}

void driveToWindow() {
    /* Goal is to run into window / flowerbox to align Y coord since we 
    just came from button press which is inexact */
    driveThenStop(2, -25); // Test this
    rotateInPlaceThenStop(90, 20);
    correctHeading(0);
    driveThenStopWithTimeout(15, -25, 7); // Test this (increase/decrease distance and/or timeout)
    driveThenStop(4.5, 25); // Test this -- backs up from wall to reasonable spot for arm
    rotateInPlaceThenStop(90, 20);
    correctHeading(270);
    driveThenStop(6.5, 25); // test this -- drives just past window handle
}

void moveSmallArm(float seconds) {
    if (seconds > 0) {
        smallArm.SetDegree(65);
    } else {
        smallArm.SetDegree(100);
    }
    Sleep(fabs(seconds));
    smallArm.SetDegree(83);
}

void openAndCloseWindow() {
    /* CLOSE WINDOW */
    moveSmallArm(.55); // moves small arm for one second, see above -- may need to reverse direction and also need to make sure degree values are correct in the actual function (continuous servo uses degrees instead of percents, 0% should be 90 deg theoretically)

    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    leftMotor.SetPercent(-25);
    rightMotor.SetPercent(32);
    while ((leftEncoder.Counts() + rightEncoder.Counts()) / 2.0 < COUNTS_PER_INCH * 5); // drive backward with counterrotation until window sensor is triggered
    leftMotor.Stop();
    rightMotor.Stop();
    Sleep(.2);

    /* OPEN WINDOW */
    driveThenStop(1, 25);
    moveSmallArm(-.55); // move small arm back to original position -- again may need to reverse direction and adjust degree values
    driveThenStop(3, -25);
    moveSmallArm(.55); // reopen arm

    leftEncoder.ResetCounts();
    rightEncoder.ResetCounts();
    leftMotor.SetPercent(25);
    rightMotor.SetPercent(-32);
    while((leftEncoder.Counts() + rightEncoder.Counts()) / 2.0 < COUNTS_PER_INCH * 6);
    leftMotor.Stop();
    rightMotor.Stop();
    Sleep(.2);
    driveThenStop(2, -25);
    moveSmallArm(-.55);
    correctHeading(270);

    // See how thrown off heading is if we get stuck on the end here, could rotate, drive rerotate, check heading
}

void driveToTopOfRamp() {
    // literally just ram ts into the wall and back up/rotate
    driveThenStop(10, 30);
    driveThenStopWithTimeout(6, 25, 3); // adjust distance and timeout as needed
    driveThenStop(1.75, -25); // back up to reasonable spot to go down ramp -- adjust distance if needed since we'll just drive straight down
    rotateInPlaceThenStop(90, 25); // rotate to face the ramp
}

void driveDownRampAndEnd() {
    driveThenStop(30, 30); // adjust distance and speed as needed -- drives straight into end button ideally
}

void ERCMain()
{
    // testOptosensors();
    // TestGUI();

    RCS.InitializeTouchMenu("1130D4YKU");
    // RCS readings, light readings, etc.
    // RCS.DisableRateLimit();
    // int x, y;
    // 30 SECOND TIMEOUT MAX
    // while(!LCD.Touch(&x, &y)); // Wait for initial touch to start program
    while(CDSCell.Value() > CDS_THRESHOLD_RED); // Wait for start light
    activateStartButton();
    driveToCompostBin();
    spinCompostBin();
    rotateInPlaceThenStop(26, 16);
    driveToAppleBucket();
    pickUpAppleBucket();
    driveToBottomOfRamp();
    driveUpRamp();
    driveToTableAndDropAppleBucket();
    driveToLight(25);
    bool colorIsRed = getCDSValueAndDisplayColor();
    pressLightButton(colorIsRed);
    /* SKELETON CODE BELOW: UNTESTED CODE */
    // later -- go to correct lever conditionally w/ RCS
    // driveToLeftLever();
    // pushLeftLeverDown();
    driveToWindow();
    openAndCloseWindow();
    // move more?
    driveToTopOfRamp();
    driveDownRampAndEnd();


    /* MILESTONE 4 */
    // driveThenStop(22, 16);
    // rotateInPlaceThenStop(-48, 16);
    // //correctWithRCS(12.23, 20.59, 90);
    // Sleep(.75);
    // // pick up apple bucket 
    // driveThenStop(1.25, 16);
    // Sleep(.35);
    // moveLargeArmInches(2.25);
    // Sleep(.35);
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
