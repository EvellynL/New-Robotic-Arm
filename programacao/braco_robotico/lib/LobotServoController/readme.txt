This library uses the Serial class for serial communication, occupying one hardware serial port on Arduino. (本库使用Serial类进行串口通信，占用Arduino的一个硬件串口)
Wiring: Tx on the servo controller connects to Arduino Rx, Rx on the servo controller connects to Arduino Tx, and GND on the servo controller connects to Arduino GND. (接线：舵机控制板的Tx接Arduino的Rx，舵机控制板的Rx接Arduino的Tx，舵机控制板的GND接Arduino的GND)

Before using the library, please initialize the serial port to 9600 baud rate; other baud rates are not supported. (使用库前请初始化串口为波特率9600  不支持其他波特率)
After instantiating the LobotServoController class, call its functions. (实例化LobotServoController类后调用)
Example usage: (调用举例：)
  LobotServoController Controller(Serial3); //Use Serial3 as the communication interface with the servo board, (使用Serial3作为与舵机板的通信接口，)
                                            //if no parameter is passed, Serial is used by default. (无参数时为Serial)
  Controller.moveServo(0,1200,1000)； //Servo 0 moves to position 1200 in 1000 ms (0号舵机1000ms移动至1200位置)
  
  For controllers like Arduino Leonardo (atmega32u4), the default serial port is Serial1; for others, it is Serial. (对于Arduino Leonardo等 atmega32u4的板子默认串口为Serial1，其他均为Serial)
 For more functions, see the code; detailed comments are provided. (更多函数参看代码，代码有详细注释)

After sending the get battery voltage command, the user needs to manually run receiveHandle to extract the voltage information from the serial data. (发送获取电池电压命令后，需用户自行运行receiveHandle以从串口数据中取出电压信息)
After obtaining the battery voltage, it is stored in the class member batteryVoltage; the user needs to read this variable manually. The unit is millivolts (mV). (获得电池电压后电池电压被保存在类成员batteryVoltage，用户需自行读取该变量，单位为毫伏mv)
