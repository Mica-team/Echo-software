#include <Arduino.h>
#include <ESP32Servo.h>
#include "ServoManager.h"

namespace
{
constexpr int SERVO_PIN = 13;
constexpr int SERVO_CENTER = 90;
constexpr int SERVO_LEFT = 40;
constexpr int SERVO_RIGHT = 140;
}

Servo head;

void servoSetup()
{
    head.setPeriodHertz(50);
    head.attach(SERVO_PIN);
    head.write(SERVO_CENTER);
}

void servoStop()
{
    head.detach();
}

void servoCenter()
{
    head.write(SERVO_CENTER);
}

void servoLeft()
{
    head.write(SERVO_LEFT);
}

void servoRight()
{
    head.write(SERVO_RIGHT);
}
