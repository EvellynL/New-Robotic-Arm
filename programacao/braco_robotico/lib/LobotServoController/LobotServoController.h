/******************************************************
* FileName:      LobotServoController.h
* Company:       乐幻索尔
* Date:          2016/07/02  16:53
* Description:   Macro definitions, classes, etc. for Lobot servo control board secondary development library (Lobot舵机控制板二次开发库的宏定义、类等)
*****************************************************/

#ifndef LOBOTSERVOCONTROLLER_H
#define LOBOTSERVOCONTROLLER_H

#include <Arduino.h>

//发送部分的指令
#define FRAME_HEADER            0x55   //Frame header (帧头)
#define CMD_SERVO_MOVE          0x03   //Servo move command (舵机移动指令)
#define CMD_ACTION_GROUP_RUN    0x06   //Run action group command (运行动作组指令)
#define CMD_ACTION_GROUP_STOP   0x07   //Stop action group command (停止动作组运行指令)
#define CMD_ACTION_GROUP_SPEED  0x0B   //Set action group speed command (设置动作组运行速度指令)
#define CMD_GET_BATTERY_VOLTAGE 0x0F   //Get battery voltage command (获得电池电压指令)
#define CMD_MULT_SERVO_UNLOAD   0x14   //Multiple servos power-off command (多个舵机掉电指令)
#define CMD_MULT_SERVO_POS_READ 0x15   //Read angles of multiple servos command (读取多个舵机的角度位置指令)

//Commands for receiving part (接收部分的指令)
#define BATTERY_VOLTAGE       0x0F  //Battery voltage (电池电压)
#define ACTION_GROUP_RUNNING  0x06  //Action group is running (动作组被运行)
#define ACTION_GROUP_STOPPED  0x07  //Action group stopped (动作组被停止)
#define ACTION_GROUP_COMPLETE 0x08  //Action group completed (动作组完成)

struct LobotServo {  //Servo ID and position structure (舵机ID和位置结构体)
  uint8_t  ID;       // Servo ID (舵机ID)
  uint16_t Position; //Servo data (舵机数据)
};

class LobotServoController {
  public:
    LobotServoController();
    LobotServoController(HardwareSerial &A);
    ~LobotServoController();

    void moveServo(uint8_t servoID, uint16_t Position, uint16_t Time);
    void moveServos(LobotServo servos[], uint8_t Num, uint16_t Time);
    void moveServos(uint8_t Num,uint16_t Time, ...);
    void runActionGroup(uint8_t NumOfAction, uint16_t Times);
    void stopActionGroup(void);
    void setActionGroupSpeed(uint8_t NumOfAction, uint16_t Speed);
    void setAllActionGroupSpeed(uint16_t Speed);
    void setServoUnload(uint8_t numOfServos, ...);
    byte getServosPos(uint8_t numOfServos, ...);

    int LobotSerialServoReceiveHandle(byte *ret);
    
    void getBatteryVoltage(void);
    void receiveHandle(void);

  public:
    uint8_t  numOfActinGroupRunning; // Currently running action group number (正在运行的动作组序号)
    uint16_t actionGroupRunTimes; //Number of times the action group is running (正在运行的动作组运行次数)
    bool isRunning; //Is an action group running? (有动作组运行？)
    uint16_t batteryVoltage; //Control board battery voltage (控制板电池电压)
    int servosPos[128]; //Servo ID and position (舵机ID和舵机位置)

    HardwareSerial *SerialX;
};
#endif
