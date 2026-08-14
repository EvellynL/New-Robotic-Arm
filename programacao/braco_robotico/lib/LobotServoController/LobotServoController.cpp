/******************************************************
 * FileName:      LobotServoController.cpp
 ** Company:       乐幻索尔
 * Date:           2016/07/02  16:53
 * Description:   Lobot servo controller secondary development library, which contains the implementation of the library.(Lobot舵机控制板二次开发库，本文件包含了此库的具体实现)
 *****************************************************/

#include "LobotServoController.h"

#define GET_LOW_BYTE(A) (uint8_t)((A))
//Macro function Get the lower eight bits of A(宏函数 获得A的低八位)
#define GET_HIGH_BYTE(A) (uint8_t)((A) >> 8)
//Macro function Get the high eight bits of A(宏函数 获得A的高八位)
#define BYTE_TO_HW(A, B) ((((uint16_t)(A)) << 8) | (uint8_t)(B))
//Macro function takes A as the upper eight bits and B as the lower eight bits, combined into 16-bit shaping(宏函数 以A为高八位 B为低八位 合并为16位整形)

LobotServoController::LobotServoController()
{
	//Initialize running action group number as 0xFF, run count as 0, running flag as false, battery voltage as 0 (初始化运行中动作组号为0xFF，运行次数为0，运行中标识为假，电池电压为0)
	numOfActinGroupRunning = 0xFF;
	actionGroupRunTimes = 0;
	isRunning = false;
	batteryVoltage = 0;
#if defined(__AVR_ATmega32U4__)  //for Arduino Leonardo，Micro....
	SerialX = &Serial1;
#else
	SerialX = &Serial;
#endif
}

LobotServoController::LobotServoController(HardwareSerial &A)
{
	LobotServoController();
	SerialX = &A;
}
LobotServoController::~LobotServoController()
{
}

/*********************************************************************************
 * Function:  moveServo
 * Description: Control a single servo to rotate (控制单个舵机转动)
 * Parameters:   sevoID: servo ID, Position: target position, Time: rotation time (Parameters:   sevoID:舵机ID，Position:目标位置,Time:转动时间)
                  Servo ID range: 0 <= sevoID <= 31, Time: Time > 0 (舵机ID取值:0<=舵机ID<=31,Time取值: Time > 0)
 * Return:      None (无返回)
 * Others:
 **********************************************************************************/
void LobotServoController::moveServo(uint8_t servoID, uint16_t Position, uint16_t Time)
{
	uint8_t buf[11];
	if (servoID > 31 || !(Time > 0)) { //Servo ID cannot be greater than 31; can be modified based on the control board (舵机ID不能大于31,可根据对应控制板修改)
		return;
	}
	buf[0] = FRAME_HEADER;                   //Fill frame header (填充帧头)
	buf[1] = FRAME_HEADER;
	buf[2] = 8;                              //Data length = number of servos * 3 + 5, here = 1 * 3 + 5 (数据长度=要控制舵机数*3+5，此处=1*3+5)
	buf[3] = CMD_SERVO_MOVE;                 //Fill servo move command (填充舵机移动指令)
	buf[4] = 1;                              //Number of servos to control (要控制的舵机个数)
	buf[5] = GET_LOW_BYTE(Time);             //Fill lower 8 bits of time (填充时间的低八位)
	buf[6] = GET_HIGH_BYTE(Time);            //Fill higher 8 bits of time (填充时间的高八位)
	buf[7] = servoID;                        //Servo ID (舵机ID)
	buf[8] = GET_LOW_BYTE(Position);         //Fill lower 8 bits of target position (填充目标位置的低八位)
	buf[9] = GET_HIGH_BYTE(Position);        //Fill higher 8 bits of target position (填充目标位置的高八位)

	SerialX->write(buf, 10);
}

/*********************************************************************************
 * Function:  moveServos
 * Description： Control multiple servos to rotate (控制多个舵机转动)
 * Parameters:   servos[]: servo structure array, Num: number of servos, Time: rotation time (Parameters:   servos[]:舵机结构数组，Num:舵机个数,Time:转动时间)
                    0 < Num <= 32,Time > 0
 * Return:       None (无返回)
 * Others:
 **********************************************************************************/
void LobotServoController::moveServos(LobotServo servos[], uint8_t Num, uint16_t Time)
{
	uint8_t buf[103];    //Create buffer (建立缓存)
	if (Num < 1 || Num > 32 || !(Time > 0)) {
		return; //Number of servos cannot be zero or greater than 32, time cannot be zero (舵机数不能为零和大于32，时间不能为零)
	}
	buf[0] = FRAME_HEADER;    //Fill frame header (填充帧头)
	buf[1] = FRAME_HEADER;
	buf[2] = Num * 3 + 5;     //Data length = number of servos * 3 + 5 (数据长度 = 要控制舵机数*3+5)
	buf[3] = CMD_SERVO_MOVE;  //Fill servo move command (填充舵机移动指令)
	buf[4] = Num;             //Number of servos to control (要控制的舵机个数)
	buf[5] = GET_LOW_BYTE(Time); //Get lower 8 bits of time (取得时间的低八位)
	buf[6] = GET_HIGH_BYTE(Time); //Get higher 8 bits of time (取得时间的高八位)
	uint8_t index = 7;
	for (uint8_t i = 0; i < Num; i++) { //Loop to fill servo IDs and corresponding target positions (循环填充舵机ID和对应目标位置)
		buf[index++] = servos[i].ID; //Fill servo ID (填充舵机ID)
		buf[index++] = GET_LOW_BYTE(servos[i].Position); //Fill lower 8 bits of target position (填充目标位置低八位)
		buf[index++] = GET_HIGH_BYTE(servos[i].Position);//Fill higher 8 bits of target position (填充目标位置高八位)
	}
	SerialX->write(buf, buf[2] + 2); //Send frame, length = data length + 2 bytes for header (发送帧，长度为数据长度+两个字节的帧头)
}

/*********************************************************************************
 * Function:  moveServos
 * Description： 控制多个舵机转动
 * Parameters:   Num: number of servos, Time: rotation time, ...: servo ID, angle, servo ID, angle, and so on (Num:舵机个数,Time:转动时间,...:舵机ID,转动角，舵机ID,转动度 如此类推)
 * Return:       None (无返回)
 * Others:
 **********************************************************************************/
void LobotServoController::moveServos(uint8_t Num, uint16_t Time, ...)
{
	uint8_t buf[128];

	// Validate fixed arguments before starting the variable-argument list.
	if (Num < 1 || Num > 32 || !(Time > 0)) {
		return;
	}

	va_list arg_ptr;
	va_start(arg_ptr, Time);
	buf[0] = FRAME_HEADER;     //Fill frame header (填充帧头)
	buf[1] = FRAME_HEADER;
	buf[2] = Num * 3 + 5;      //Data length = number of servos * 3 + 5 (数据长度 = 要控制舵机数 * 3 + 5)
	buf[3] = CMD_SERVO_MOVE;   //Servo move command (舵机移动指令)
	buf[4] = Num;              //Number of servos to control (要控制舵机数)
	buf[5] = GET_LOW_BYTE(Time); //Get lower 8 bits of time (取得时间的低八位)
	buf[6] = GET_HIGH_BYTE(Time); //Get higher 8 bits of time (取得时间的高八位)
	uint8_t index = 7;
	for (uint8_t i = 0; i < Num; i++) { //Get and loop to fill servo IDs and corresponding target positions from variable arguments (从可变参数中取得并循环填充舵机ID和对应目标位置)
		// Arguments passed through ... undergo integer promotion on ESP32.
		uint16_t tmp = (uint16_t)va_arg(arg_ptr, int); //Get servo ID
		buf[index++] = GET_LOW_BYTE(tmp); //It seems in avr-gcc, variable integer arguments are all 16 bits (貌似avrgcc中可变参数整形都是十六位)
		                                  //Then get its lower 8 bits (再取其低八位)
		uint16_t pos = (uint16_t)va_arg(arg_ptr, int); //Get corresponding target position
		buf[index++] = GET_LOW_BYTE(pos); //Fill lower 8 bits of target position (填充目标位置低八位)
		buf[index++] = GET_HIGH_BYTE(pos); //Fill higher 8 bits of target position (填充目标位置高八位)
	}
	va_end(arg_ptr);     //Set arg_ptr to null (置空arg_ptr)
	SerialX->write(buf, buf[2] + 2); //Send frame (发送帧)
}


/*********************************************************************************
 * Function:  runActionGroup
 * Description： Run the specified action group (Description： 运行指定动作组)
 * Parameters:   NumOfAction: action group number, Times: number of repetitions (Parameters:   NumOfAction:动作组序号, Times:执行次数)
 * Return:       None (无返回)
 * Others:       Times = 0 means infinite loop (Times = 0 时无限循环)
 **********************************************************************************/
void LobotServoController::runActionGroup(uint8_t numOfAction, uint16_t Times)
{
	uint8_t buf[7];
	buf[0] = FRAME_HEADER;   //Fill frame header (填充帧头)
	buf[1] = FRAME_HEADER;
	buf[2] = 5;      //Data length, number of data bytes excluding the frame header; fixed as 5 for this command (数据长度，数据帧除帧头部分数据字节数，此命令固定为5)
	buf[3] = CMD_ACTION_GROUP_RUN; //Fill run action group command (填充运行动作组命令)
	buf[4] = numOfAction;      //Fill the action group number to run (填充要运行的动作组号)
	buf[5] = GET_LOW_BYTE(Times); //Get lower 8 bits of the number of repetitions (取得要运行次数的低八位)
	buf[6] = GET_HIGH_BYTE(Times); //Get higher 8 bits of the number of repetitions (取得要运行次数的高八位)
	SerialX->write(buf, 7);      //Send data frame (发送数据帧)
}

/*********************************************************************************
 * Function:  stopActiongGroup
 * Description： Stop running the action group (Description： 停止动作组运行)
 * Parameters:   Speed: target speed (Parameters:   Speed: 目标速度)
 * Return:       None (无返回)
 * Others:
 **********************************************************************************/
void LobotServoController::stopActionGroup(void)
{
	uint8_t buf[4];
	buf[0] = FRAME_HEADER;     //Fill frame header (填充帧头)
	buf[1] = FRAME_HEADER;
	buf[2] = 2;                //Data length, number of data bytes excluding the frame header; fixed as 2 for this command (数据长度，数据帧除帧头部分数据字节数，此命令固定为2)
	buf[3] = CMD_ACTION_GROUP_STOP; //Fill stop action group command (填充停止运行动作组命令)

	SerialX->write(buf, 4);      //Send data frame (发送数据帧)
}

/*********************************************************************************
 * Function:  setActionGroupSpeed
 * Description： Set the running speed of a specified action group (Description： 设定指定动作组的运行速度)
 * Parameters:   NumOfAction: action group number, Speed: target speed (Parameters:   NumOfAction: 动作组序号 , Speed:目标速度)
 * Return:       None (无返回)
 * Others:
 **********************************************************************************/
void LobotServoController::setActionGroupSpeed(uint8_t numOfAction, uint16_t Speed)
{
	uint8_t buf[7];
	buf[0] = FRAME_HEADER;     //Fill frame header (填充帧头)
	buf[1] = FRAME_HEADER;
	buf[2] = 5;                //Data length, number of data bytes excluding the frame header; fixed as 5 for this command (数据长度，数据帧除帧头部分数据字节数，此命令固定为5)
	buf[3] = CMD_ACTION_GROUP_SPEED; //Fill set action group speed command (填充设置动作组速度命令)
	buf[4] = numOfAction;      //Fill action group number to set (填充要设置的动作组号)
	buf[5] = GET_LOW_BYTE(Speed); //Get lower 8 bits of target speed (获得目标速度的低八位)
	buf[6] = GET_HIGH_BYTE(Speed); //Get higher 8 bits of target speed (获得目标速度的高八位)

	SerialX->write(buf, 7);      //Send data frame (发送数据帧)
}


/*********************************************************************************
 * Function:  setAllActionGroupSpeed
 * Description： Set the running speed of all action groups (Description： 设置所有动作组的运行速度)
 * Parameters:   Speed: target speed (Parameters:   Speed: 目标速度)
 * Return:       None (无返回)
 * Others:
 **********************************************************************************/
void LobotServoController::setAllActionGroupSpeed(uint16_t Speed)
{
	setActionGroupSpeed(0xFF, Speed); //Call the action group speed setting function; when group number is 0xFF, all groups are set (调用动作组速度设定，组号为0xFF时设置所有组的速度)
}

/*********************************************************************************
 * Function:  getBatteryVoltage
 * Description： Send command to get battery voltage (Description： 发送获取电池电压命令)
 * Parameters:   No input parameters (Parameters:   无输入参数)
 * Return:       None (无返回)
 * Others:
 **********************************************************************************/
void LobotServoController::getBatteryVoltage()
{
	uint8_t buf[4];
 uint8_t recvBuf[32];
  uint8_t count = 10000; 
  byte i = 0;
  int ret = 1;
  
	buf[0] = FRAME_HEADER;         //Fill frame header (填充帧头)
	buf[1] = FRAME_HEADER;
	buf[2] = 2;                   //Data length, number of data bytes excluding the frame header; fixed as 2 for this command (数据长度，数据帧除帧头部分数据字节数，此命令固定为2)
	buf[3] = CMD_GET_BATTERY_VOLTAGE; //Fill get battery voltage command (填充获取电池电压命令)

	SerialX->write(buf, 4);        //Send data frame (发送数据帧)
}

/*********************************************************************************
 * Function:  setServoUnload
 * Description： Set servo power off (Description： 设置舵机掉电)
 * Parameters:   numOfServos: number of servos to control, ID: servo ID (Parameters:   numOfServos:要控制舵机的个数，ID:舵机的ID)
 * Return:       None (无返回)
 * Others:
 **********************************************************************************/
void LobotServoController::setServoUnload(uint8_t numOfServos, ...)
{
  uint8_t buf[128];

  if (numOfServos < 1 || numOfServos > 32) {
    return;
  }

  va_list arg_ptr;
  va_start(arg_ptr, numOfServos);
  buf[0] = FRAME_HEADER;     //Fill frame header (填充帧头)
  buf[1] = FRAME_HEADER;
  buf[2] = numOfServos + 3;      //Data length = number of servos * 3 + 5 (数据长度 = 要控制舵机数 * 3 + 5)
  buf[3] = CMD_MULT_SERVO_UNLOAD;   //Servo move command (舵机移动指令)
  buf[4] = numOfServos;              //Number of servos to control (要控制舵机数)

  uint8_t index = 5;
  for (uint8_t i = 0; i < numOfServos; i++) { //Get and loop to fill servo IDs and corresponding target positions from variable arguments (从可变参数中取得并循环填充舵机ID和对应目标位置)
     uint16_t getId = (uint16_t)va_arg(arg_ptr, int); //Get servo ID
     buf[index++] = GET_LOW_BYTE(getId);
  }
  va_end(arg_ptr);     //Set arg_ptr to null (置空arg_ptr)
  SerialX->write(buf, buf[2] + 2); //Send frame (发送帧)
}

/*********************************************************************************
 * Function:  getServosPos
 * Description： Read positions of multiple servos (Description： 读取多个舵机指令)
 * Parameters:   numServos: number of servos, id: IDs of the servos to read (Parameters:   numServos:舵机的个数，id:要读取舵机的ID)
 * Return:       
 * Others:
 **********************************************************************************/
uint8_t LobotServoController::getServosPos(uint8_t numOfServos, ...)
{
  int count = 10000;
  int ret;
  int i = 0;
  uint8_t buf[128];

  if (numOfServos < 1 || numOfServos > 32) {
    return 0;
  }

  va_list arg_ptr;
  va_start(arg_ptr, numOfServos);
  buf[0] = FRAME_HEADER;     //Fill frame header (填充帧头)
  buf[1] = FRAME_HEADER;
  buf[2] = numOfServos  + 3;      //Data length = number of servos * 3 + 3 (数据长度 = 要控制舵机数 * 3 + 3)
  buf[3] = CMD_MULT_SERVO_POS_READ;   //Servo move command (舵机移动指令)
  buf[4] = numOfServos;              //Number of servos to control (要控制舵机数)

  uint8_t index = 5;
  for (uint8_t i = 0; i < numOfServos; i++) { //Get and loop to fill servo IDs and corresponding target positions from variable arguments (从可变参数中取得并循环填充舵机ID和对应目标位置)
     uint16_t getId = (uint16_t)va_arg(arg_ptr, int); //Get servo ID
     buf[index++] = GET_LOW_BYTE(getId);
  }
  va_end(arg_ptr);     //Set arg_ptr to null (置空arg_ptr)
  SerialX->write(buf, buf[2] + 2); //Send frame (发送帧)
}

/*********************************************************************************
 * Function:  receiveHandle
 * Description： Handle received serial data (Description： 处理串口接收数据)
 * Parameters:   No input parameters (Parameters:   无输入参数)
 * Return:       None (无返回)
 * Others:
 **********************************************************************************/
void LobotServoController::receiveHandle()
{
	uint8_t buf[16];
	static uint8_t len = 0;
	static uint8_t getHeader = 0;
	if (!SerialX->available())
		return;
	//Return if there is no data (如果没有数据则返回)
	do {
		switch (getHeader) {
		case 0:
			if (SerialX->read() == FRAME_HEADER)
				getHeader = 1;
			break;
		case 1:
			if (SerialX->read() == FRAME_HEADER)
				getHeader = 2;
			else
				getHeader = 0;
			break;
		case 2:
			len = SerialX->read();
			getHeader = 3;
			break;
		case 3:
			if (SerialX->readBytes(buf, len - 1) > 0)
				getHeader = 4;
			else{
				len = 0;
				getHeader = 0;
				break;
			}
		case 4:
			switch (buf[0]) {
			case BATTERY_VOLTAGE: //Battery voltage command (电池电压指令)
				batteryVoltage = BYTE_TO_HW(buf[2], buf[1]); //Combine high and low 8 bits to get battery voltage (高低八位组合成电池电压)
				break;
			case ACTION_GROUP_RUNNING:         //An action group is running (有动作组被运行)
				numOfActinGroupRunning = buf[1]; //Get running action group number (获得运行中动作组号)
				actionGroupRunTimes = BYTE_TO_HW(buf[3], buf[2]); //Combine high and low 8 bits to get run count (高低八位组合成运行次数)
				isRunning = true; //Set running flag to true (设置运行中标识为真)
				break;
			case ACTION_GROUP_STOPPED: //Action group stopped (动作组停止)
			case  ACTION_GROUP_COMPLETE://Action group run completed (动作组运行完成)
				isRunning = false; //Set running flag to false (设置运行中标识为假)
				numOfActinGroupRunning = 0xFF; //Set running action group number to 0xFF (设置运行中动作组号为0xFF)
				actionGroupRunTimes = 0; //Set number of runs to 0 (设置运行次数为0)
				break;
      case CMD_MULT_SERVO_POS_READ:
        for(int i=2,j=0;i<buf[1]*3+2;i+=3)
        {
            servosPos[j++] = buf[i];
            servosPos[j++] = BYTE_TO_HW(buf[i+2], buf[i+1]);
        }
			default:
				break;
			}
		default:
			len = 0;
			getHeader = 0;
			break;
		}
	} while (SerialX->available());
}
