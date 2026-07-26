#include <Arduino.h>
#include <ESP32Servo.h>
#include "ServoManager.h"

Servo head;

#define SERVO_PIN 13

void servoSetup()
{
    head.attach(SERVO_PIN);
    head.write(90);
}

void servoCenter()
{
    head.write(90);
}

void servoLeft()
{
    head.write(40);
}

void servoRight()
{
    head.write(140);
}
