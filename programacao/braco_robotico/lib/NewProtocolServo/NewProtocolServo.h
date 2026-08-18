#ifndef NEW_PROTOCOL_SERVO_H
#define NEW_PROTOCOL_SERVO_H

#include <Arduino.h>

/*
 * NewProtocolServo
 *
 * Protocolo da interface da HiWonder/LBSC para funções de BUS SERVO:
 *
 *   AA 55 | Function | Length | Data | Checksum
 *
 * Function:
 *   0x05 = PACKET_FUNC_BUS_SERVO
 *
 * Leitura de temperatura:
 *   Data = 09 ID
 *
 * Solicitação:
 *   AA 55 05 02 09 ID CHECKSUM
 *
 * Resposta:
 *   AA 55 05 04 ID 09 STATUS TEMPERATURE CHECKSUM
 *
 * STATUS:
 *   0  = leitura OK
 *   -1 = falha
 *
 * IMPORTANTE:
 * Esta biblioteca é independente da LobotServoController.
 * A LobotServoController continua responsável pelo protocolo
 * de controle usado pelos comandos 55 55 da LBSC.
 */

#define NPS_FRAME_HEADER_1       0xAA
#define NPS_FRAME_HEADER_2       0x55

#define NPS_FUNC_BUS_SERVO       0x05
#define NPS_BUS_SERVO_READ_TEMP  0x09


class NewProtocolServo
{
public:

    /*
     * Cria o objeto usando a UART ligada à LBSC.
     *
     * Exemplo:
     *   NewProtocolServo TemperatureController(Serial2);
     */
    NewProtocolServo(HardwareSerial &serial);


    /*
     * Inicializa a UART.
     *
     * Para sua LBSC, use normalmente 9600 baud,
     * pois é a mesma UART usada pela LobotServoController.
     */
    void begin(uint32_t baud = 9600);


    /*
     * Envia uma solicitação de temperatura para um servo.
     *
     * Retorna true se o pacote foi enviado.
     */
    bool requestTemperature(uint8_t servoID);


    /*
     * Processa os bytes disponíveis na UART.
     *
     * Retorna true quando uma resposta válida de temperatura
     * foi recebida.
     */
    bool receive();


    /*
     * Envia a solicitação e espera a resposta.
     *
     * Retorna:
     *   >= 0 : temperatura em °C
     *   -1   : timeout ou erro
     */
    int getTemperature(uint8_t servoID,
                       uint32_t timeoutMs = 100);


    /*
     * Armazena a última temperatura válida de cada servo.
     *
     * Exemplo:
     *   TemperatureController.servoTemperature[1]
     */
    int16_t servoTemperature[256];


    /*
     * Indica se já existe uma leitura válida para o ID.
     */
    bool temperatureValid[256];


    /*
     * ID do último servo que respondeu.
     */
    uint8_t lastServoID;


    /*
     * Status da última resposta:
     *
     *   0  = sucesso
     *   -1 = falha
     */
    int8_t lastStatus;


    /*
     * UART utilizada.
     */
    HardwareSerial *SerialX;


private:

    /*
     * Calcula o checksum do protocolo AA 55.
     *
     * Para este protocolo da HiWonder:
     * checksum = complemento da soma de
     * Function + Length + Data
     */
    uint8_t calculateChecksum(const uint8_t *data,
                              uint8_t length);


    /*
     * Limpa dados antigos da UART.
     */
    void clearReceiveBuffer();


    /*
     * Máquina de estados para receber uma resposta.
     */
    bool parseByte(uint8_t byteReceived);


    uint8_t rxBuffer[16];

    uint8_t rxIndex;

    uint8_t expectedLength;

    uint8_t parserState;
};

#endif