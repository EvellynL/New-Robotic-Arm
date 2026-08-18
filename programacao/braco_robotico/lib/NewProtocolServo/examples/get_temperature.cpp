#include "NewProtocolServo.h"

NewProtocolServo TemperatureController(Serial2);


void setup()
{
    Serial.begin(115200);

    /*
     * ESP32 <-> LBSC-V1.5
     *
     * A sua comunicação com a LBSC é 9600 baud.
     */
    TemperatureController.begin(9600);

    delay(500);
}


void loop()
{
    /*
     * Teste com servo ID 1.
     */
    int temperatura =
        TemperatureController.getTemperature(1, 200);


    if (temperatura >= 0)
    {
        Serial.print("Temperatura servo 1: ");
        Serial.print(temperatura);
        Serial.println(" C");
    }
    else
    {
        Serial.println("Erro ao ler temperatura do servo 1");
    }


    delay(1000);
}