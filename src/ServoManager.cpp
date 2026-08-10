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

// Release the PWM peripheral while OTA is running.
// This is safe even when no physical servo is connected.
void servoStop()
{
    head.detach();
}
