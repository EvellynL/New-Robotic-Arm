#include "NewProtocolServo.h"


NewProtocolServo::NewProtocolServo(HardwareSerial &serial)
{
    SerialX = &serial;

    rxIndex = 0;
    expectedLength = 0;
    parserState = 0;

    lastServoID = 0;
    lastStatus = -1;

    for (int i = 0; i < 256; i++)
    {
        servoTemperature[i] = -1;
        temperatureValid[i] = false;
    }
}


void NewProtocolServo::begin(uint32_t baud)
{
    SerialX->begin(baud);
}


/*
 * ============================================================
 * CHECKSUM
 * ============================================================
 *
 * Formato:
 *
 * AA 55 Function Length Data Checksum
 *
 * O checksum é calculado SOMENTE sobre:
 *
 * Function + Length + Data
 *
 * O cálculo é:
 *
 *   checksum = ~(soma dos bytes)
 *
 * mantendo apenas os 8 bits inferiores.
 *
 * Exemplo:
 *
 *   05 02 09 01
 *
 * soma = 0x11
 *
 * checksum = ~0x11 = 0xEE
 */
uint8_t NewProtocolServo::calculateChecksum(const uint8_t *data,
                                            uint8_t length)
{
    uint16_t sum = 0;

    for (uint8_t i = 0; i < length; i++)
    {
        sum += data[i];
    }

    return (uint8_t)(~sum);
}


/*
 * ============================================================
 * LIMPAR BUFFER
 * ============================================================
 */
void NewProtocolServo::clearReceiveBuffer()
{
    while (SerialX->available())
    {
        SerialX->read();
    }

    rxIndex = 0;
    expectedLength = 0;
    parserState = 0;
}


/*
 * ============================================================
 * REQUEST TEMPERATURE
 * ============================================================
 *
 * Pacote:
 *
 *   AA 55 05 02 09 ID CHECKSUM
 *
 * Onde:
 *
 *   AA 55 = cabeçalho
 *   05    = BUS_SERVO
 *   02    = quantidade de bytes de DATA
 *   09    = READ TEMPERATURE
 *   ID    = ID do servo
 */
bool NewProtocolServo::requestTemperature(uint8_t servoID)
{
    /*
     * IDs válidos dos servos:
     * 0x00 até 0xFD.
     *
     * 0xFE é broadcast e não deve ser usado para leitura,
     * pois vários servos poderiam responder simultaneamente.
     */
    if (servoID > 0xFD)
    {
        return false;
    }


    uint8_t packet[7];


    packet[0] = NPS_FRAME_HEADER_1;
    packet[1] = NPS_FRAME_HEADER_2;

    packet[2] = NPS_FUNC_BUS_SERVO;

    /*
     * DATA:
     *
     *   09 = subcomando
     *   ID = servo
     *
     * Portanto Length = 2.
     */
    packet[3] = 0x02;

    packet[4] = NPS_BUS_SERVO_READ_TEMP;

    packet[5] = servoID;


    /*
     * Calcula o checksum sobre:
     *
     *   05 02 09 ID
     */
    packet[6] = calculateChecksum(&packet[2], 4);


    /*
     * Remove respostas antigas para que uma resposta anterior
     * não seja confundida com a resposta desta solicitação.
     */
    clearReceiveBuffer();


    /*
     * Envia o pacote.
     */
    SerialX->write(packet, sizeof(packet));

    SerialX->flush();


    return true;
}


/*
 * ============================================================
 * PARSE BYTE
 * ============================================================
 *
 * Resposta esperada:
 *
 *   AA 55 05 04 ID 09 STATUS TEMP CHECKSUM
 *
 * Campos:
 *
 *   [0] AA
 *   [1] 55
 *   [2] 05
 *   [3] 04
 *   [4] ID
 *   [5] 09
 *   [6] STATUS
 *   [7] TEMP
 *   [8] CHECKSUM
 */
bool NewProtocolServo::parseByte(uint8_t byteReceived)
{
    switch (parserState)
    {

        /*
         * ----------------------------------------------------
         * ESTADO 0
         * Procurando primeiro AA.
         * ----------------------------------------------------
         */
        case 0:

            if (byteReceived == NPS_FRAME_HEADER_1)
            {
                rxBuffer[0] = byteReceived;
                parserState = 1;
            }

            break;


        /*
         * ----------------------------------------------------
         * ESTADO 1
         * Esperando 55.
         * ----------------------------------------------------
         */
        case 1:

            if (byteReceived == NPS_FRAME_HEADER_2)
            {
                rxBuffer[1] = byteReceived;
                parserState = 2;
            }
            else if (byteReceived == NPS_FRAME_HEADER_1)
            {
                /*
                 * Se chegou outro AA, ele pode ser o começo
                 * de um novo pacote.
                 */
                rxBuffer[0] = byteReceived;
                parserState = 1;
            }
            else
            {
                parserState = 0;
            }

            break;


        /*
         * ----------------------------------------------------
         * ESTADO 2
         * Recebe Function.
         * ----------------------------------------------------
         */
        case 2:

            rxBuffer[2] = byteReceived;

            /*
             * Para nossa biblioteca, queremos somente
             * PACKET_FUNC_BUS_SERVO = 0x05.
             */
            parserState = 3;

            break;


        /*
         * ----------------------------------------------------
         * ESTADO 3
         * Recebe Length.
         * ----------------------------------------------------
         */
        case 3:

            expectedLength = byteReceived;

            /*
             * Limite de segurança.
             */
            if (expectedLength == 0 ||
                expectedLength > 10)
            {
                parserState = 0;
                return false;
            }

            rxBuffer[3] = byteReceived;

            rxIndex = 4;

            parserState = 4;

            break;


        /*
         * ----------------------------------------------------
         * ESTADO 4
         * Recebe DATA.
         * ----------------------------------------------------
         */
        case 4:

            rxBuffer[rxIndex++] = byteReceived;

            /*
             * Já recebemos:
             *
             * 2 headers
             * 1 function
             * 1 length
             * expectedLength bytes de DATA
             *
             * O próximo byte será o checksum.
             */
            if (rxIndex >= (uint8_t)(4 + expectedLength))
            {
                parserState = 5;
            }

            break;


        /*
         * ----------------------------------------------------
         * ESTADO 5
         * Recebe e verifica CHECKSUM.
         * ----------------------------------------------------
         */
        case 5:
        {
            uint8_t receivedChecksum = byteReceived;


            /*
             * O checksum é calculado sobre:
             *
             * Function + Length + Data
             *
             * Começa em rxBuffer[2].
             */
            uint8_t calculatedChecksum =
                calculateChecksum(
                    &rxBuffer[2],
                    2 + expectedLength
                );


            /*
             * Volta para o estado inicial antes de retornar.
             */
            parserState = 0;


            /*
             * Verifica checksum.
             */
            if (receivedChecksum != calculatedChecksum)
            {
                return false;
            }


            /*
             * Verifica se é realmente uma resposta
             * de BUS SERVO.
             */
            if (rxBuffer[2] != NPS_FUNC_BUS_SERVO)
            {
                return false;
            }


            /*
             * Para temperatura:
             *
             * Length = 4
             *
             * ID
             * 09
             * STATUS
             * TEMPERATURE
             */
            if (expectedLength != 4)
            {
                return false;
            }


            /*
             * Verifica o subcomando.
             */
            if (rxBuffer[5] != NPS_BUS_SERVO_READ_TEMP)
            {
                return false;
            }


            /*
             * Extrai os dados.
             */
            uint8_t servoID = rxBuffer[4];

            int8_t status = (int8_t)rxBuffer[6];

            uint8_t temperature = rxBuffer[7];


            lastServoID = servoID;

            lastStatus = status;


            /*
             * STATUS = 0 significa que a leitura foi
             * realizada corretamente.
             */
            if (status == 0)
            {
                servoTemperature[servoID] = temperature;

                temperatureValid[servoID] = true;

                return true;
            }


            /*
             * Se STATUS = -1, a leitura falhou.
             */
            servoTemperature[servoID] = -1;

            temperatureValid[servoID] = false;

            return false;
        }


        default:

            parserState = 0;

            break;
    }


    return false;
}


/*
 * ============================================================
 * RECEIVE
 * ============================================================
 *
 * Processa tudo que já chegou na UART.
 */
bool NewProtocolServo::receive()
{
    bool temperatureReceived = false;


    while (SerialX->available())
    {
        uint8_t byteReceived =
            (uint8_t)SerialX->read();


        if (parseByte(byteReceived))
        {
            temperatureReceived = true;
        }
    }


    return temperatureReceived;
}


/*
 * ============================================================
 * GET TEMPERATURE
 * ============================================================
 *
 * Função bloqueante:
 *
 *   1. limpa buffer
 *   2. envia AA 55 05 02 09 ID CRC
 *   3. espera resposta
 *   4. verifica checksum
 *   5. verifica status
 *   6. retorna temperatura
 */
int NewProtocolServo::getTemperature(uint8_t servoID,
                                     uint32_t timeoutMs)
{
    if (!requestTemperature(servoID))
    {
        return -1;
    }


    uint32_t startTime = millis();


    while (millis() - startTime < timeoutMs)
    {
        if (receive())
        {
            /*
             * Verifica se a resposta recebida pertence
             * ao servo solicitado.
             */
            if (lastServoID == servoID &&
                lastStatus == 0 &&
                temperatureValid[servoID])
            {
                return servoTemperature[servoID];
            }
        }


        delay(1);
    }


    return -1;
}