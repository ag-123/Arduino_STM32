/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the Uno and
  Leonardo, it is attached to digital pin 13. If you're unsure what
  pin the on-board LED is connected to on your Arduino model, check
  the documentation at http://arduino.cc

  This example code is in the public domain.

  modified 8 May 2014
  by Scott Fitzgerald
  
  modified for generic_f303cc 
  2019/06 by stevestrong
 */

 
// the setup function runs once when you press reset or power the board  
void setup() {
  // initialize pins PE8, PE9 as an output.
  pinMode(PC13, OUTPUT);
  Serial.begin(9600);
  //while (!Serial); delay(10);
  Serial.println("Hello world");
}

int cnt = 0;
// the loop function runs over and over again forever
void loop() {
  digitalWrite(PC13, LOW);    // turn the LED off by making the voltage LOW
  delay(100);                // wait for a second
  digitalWrite(PC13, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(300);                // wait for a second
  Serial.println(cnt++);
}
