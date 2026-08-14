#include <LobotServoController.h>

LobotServoController myse;

void setup() {
  pinMode(13,OUTPUT);
  Serial.begin(9600);
  while(!Serial);
  digitalWrite(13,HIGH);

  myse.runActionGroup(100,0);  //Continuously run action group 100 (不断运行100号动作组)
  delay(5000);
  myse.stopActionGroup(); //Stop running the action group (停止动作组运行)
  delay(2000);
  myse.setActionGroupSpeed(100,200); //Set the speed of action group 100 to 200% (设置100号动作组运行速度为200%)
  delay(2000);
  myse.runActionGroup(100,5);  //Run action group 100 five times (运行100号动作组 5次)
  delay(5000);
  myse.stopActionGroup(); //Stop running the action group (停止动作组运行)
  delay(2000);
  myse.moveServo(0,1000,1000); //Servo 0 moves to position 1000 in 1000 ms (0号舵机1000ms移动至1000位置)
  delay(2000);
  myse.moveServo(2,800,1000); //Servo 2 moves to position 800 in 1000 ms (2号舵机1000ms移动至800位置)
  delay(2000);
  myse.moveServos(5,1000,0,1000,2,500,4,600,6,900,8,790);  //
  //控制Control 5 servos, move time 1000 ms, servo 0 to position 1000, servo 2 to position 500, servo 4 to position 600, (控制5个舵机，移动时间1000ms，0号舵机至1000位置，2号舵机至500位置，4号舵机至600位置，)
  //servo 6 to position 900, servo 8 to position 790 (6号舵机至900位置，8号舵机至790位置)
  delay(2000);

  LobotServo servos[2];   //Servo ID and position structure array (舵机ID位置结构数组)
  servos[0].ID = 2;       //Servo 2 (2号舵机)
  servos[0].Position = 1000;  //Position 1000 (1000位置)
  servos[1].ID = 4;       //Servo 4 (4号舵机)
  servos[1].Position = 500;  //Position 500 (500位置)
  myse.moveServos(servos,2,1000);  //Control two servos, move time 1000 ms, IDs and positions specified by servos (控制两个舵机，移动时间1000ms,ID和位置由servos指定)
}

void loop() {
 
}