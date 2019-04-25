#include "RobotController.h"

RobotController::RobotController(PinName leftWheelPwm, PinName leftWheelFwd,
    PinName leftWheelRev, PinName rightWheelPwm, PinName rightWheelFwd,
    PinName rightWheelRev, PinName leftEncoder, PinName rightEncoder,
    PinName imuSda, PinName imuScl) :
    leftWheel(leftWheelPwm, leftWheelFwd, leftWheelRev),
    rightWheel(rightWheelPwm, rightWheelFwd, rightWheelRev),
    _leftEncoder(leftEncoder), _rightEncoder(rightEncoder),
    imu(imuSda, imuScl, 0xD6, 0x3C) {
        imu.begin();
        imu.calibrate();
        w1 = 0.0;
        w2 = 0.0;
        t1 = 0.0;
        t2 = 0.0;
}

RobotController::~RobotController() {
    delete &leftWheel;
    delete &rightWheel;
    delete &_leftEncoder;
    delete &_rightEncoder;
    delete &imu;
    delete &t;
}

void RobotController::detectObstacles() {
    led = 0b0001;
    for (int i = 0; i < 360; i++) {
        _leftEncoder.reset();
        _rightEncoder.reset();
        leftWheel.speed(0.4);
        rightWheel.speed(-0.4);
        while((_leftEncoder.read() < 1) && (_rightEncoder.read() < 1));
        leftWheel.speed(0);
        rightWheel.speed(0);
        obstacles[i] = (int)(lidarDistance / 10);
        pc.printf("Index %d, %d\n", i, lidarDistance);
    }
    led = 0;
}

void RobotController::followTrajectory() {
    led = 0b1000;
    while(!pb);
    for (int i = 0; i < trajectoryLength; i = i + 2) {
        t.reset();
        yaw = 0.0;
        w1 = 0.0;
        w2 = 0.0;
        t1 = 0.0;
        t2 = 0.0;
        int angle = trajectory[i] % 360;
        if ((trajectory[i] >= 0) && (trajectory[i] <= 90)) {
            angle = angle*ROTERRI;
        } else if (trajectory[i] <= 180) {
            angle = angle*ROTERRII;
        } else if (trajectory[i] < 360) {
            angle = angle*ROTERRIII;
        }
        useImu = true;
        t.start();
        leftWheel.speed(0.2);
        rightWheel.speed(-0.2);
        while(yaw > -angle) {
            yaw = yaw + (((w2+w1)/2.0)*(t2-t1));
            while(!imu.gyroAvailable());
            imu.readGyro();
            w1 = w2;
            w2 = imu.calcGyro(imu.gz);
            t1 = t2;
            t2 = t.read();
        }
        leftWheel.speed(0);
        rightWheel.speed(0);
        t.stop();
        useImu = false;
        _leftEncoder.reset();
        _rightEncoder.reset();
        int distance = (int)(trajectory[i + 1]*COUNTPERCM);
        leftWheel.speed(0.2);
        rightWheel.speed(0.2);
        while((_leftEncoder.read() < distance) && (_rightEncoder.read() < distance));
        leftWheel.speed(0);
        rightWheel.speed(0);
    }
    delete []trajectory;
    led = 0;
}