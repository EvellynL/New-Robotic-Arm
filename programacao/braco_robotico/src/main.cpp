// #include <Arduino.h>
// #include <WiFi.h>
// #include <WebServer.h>
// #include <LobotServoController.h>

// // ---------- Configurações da rede WiFi (modo Access Point) ----------
// const char* AP_SSID = "ESP32_Joystick";
// const char* AP_PASSWORD = "12345678"; // mínimo 8 caracteres

// WebServer server(80);

// LobotServoController Controller(Serial2);

// LobotServo servos[5];

// // ---------- Variáveis de estado dos joysticks/botão ----------
// bool estadoServo5 = false;
// bool ultimoEstado = HIGH;

// int joy1X = 500, joy1Y = 500;
// int joy2X = 500, joy2Y = 500;
// bool botaoPressionado = false;

// int posX1 = 500;
// int posY1 = 500;
// int posX2 = 500;
// int posY2 = 500;
// int posW = 0;

// const int DEADZONE = 30;
// const int MAX_STEP = 8;

// unsigned long tempoControle = 0;
// unsigned long tempoSerial = 0;

// const int intervaloControle = 20;
// const int intervaloSerial = 200;

// // ---------- Página HTML (com CSS e JavaScript embutidos) ----------
// const char index_html[] PROGMEM = R"rawliteral(
// <!DOCTYPE html>
// <html lang="pt-br">
// <head>
// <meta charset="UTF-8">
// <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
// <title>ESP32 Joystick Control</title>
// <style>
//   html, body {
//     margin: 0; padding: 0;
//     background: #101418;
//     color: #eee;
//     font-family: Arial, Helvetica, sans-serif;
//     overflow: hidden;
//     height: 100%;
//     -webkit-user-select: none;
//     user-select: none;
//     touch-action: none;
//   }
//   h1 {
//     text-align: center;
//     font-size: 18px;
//     padding: 10px 0 0 0;
//     margin: 0;
//     color: #883BFF;
//   }
//   .painel {
//     display: flex;
//     align-items: center;
//     justify-content: space-evenly;
//     height: 90vh;
//   }
//   .joystick-container {
//     position: relative;
//     width: 150px;
//     height: 150px;
//     background: radial-gradient(circle, #1c2733 0%, #10151b 100%);
//     border: 2px solid #2f4a63;
//     border-radius: 50%;
//     box-shadow: inset 0 0 15px #000;
//   }
//   .stick {
//     position: absolute;
//     width: 60px;
//     height: 60px;
//     left: 45px;
//     top: 45px;
//     border-radius: 50%;
//     background: radial-gradient(circle at 30% 30%, #883BFF, #1573b5);
//     box-shadow: 0 0 10px #1573b5aa;
//     cursor: grab;
//   }
//   .label {
//     text-align: center;
//     margin-top: 8px;
//     font-size: 12px;
//     color: #9fd6ff;
//   }
//   .valores {
//     text-align: center;
//     font-size: 11px;
//     color: #7d93a6;
//     margin-top: 4px;
//   }
//   .botao-wrap {
//     display: flex;
//     flex-direction: column;
//     align-items: center;
//   }
//   #botao {
//     width: 80px;
//     height: 80px;
//     border-radius: 50%;
//     background: radial-gradient(circle at 30% 30%, #ff6f6f, #b51515);
//     border: 2px solid #7a0f0f;
//     box-shadow: 0 0 10px #b5151566;
//     color: #fff;
//     font-weight: bold;
//     font-size: 13px;
//   }
//   #botao.pressionado {
//     background: radial-gradient(circle at 30% 30%, #7dff7d, #159b15);
//     border-color: #0f7a0f;
//   }
// </style>
// </head>
// <body>

// <h1>STEM CRIAR</h1>

// <div class="painel">

//   <div class="joystick-wrap">
//     <div class="joystick-container" id="joy1">
//       <div class="stick" id="stick1"></div>
//     </div>
//     <div class="label">Joystick 1</div>
//     <div class="valores" id="valores1">X: 500 | Y: 500</div>
//   </div>

//   <div class="botao-wrap">
//     <button id="botao">GARRA</button>
//     <div class="valores" id="valoresBtn">Solto</div>
//   </div>

//   <div class="joystick-wrap">
//     <div class="joystick-container" id="joy2">
//       <div class="stick" id="stick2"></div>
//     </div>
//     <div class="label">Joystick 2</div>
//     <div class="valores" id="valores2">X: 500 | Y: 500</div>
//   </div>

// </div>

// <script>
// // Estado global enviado ao ESP32
// let estado = { x1: 500, y1: 500, x2: 500, y2: 500, btn: 0 };

// function criarJoystick(containerId, stickId, valoresId, onChange) {
//   const container = document.getElementById(containerId);
//   const stick = document.getElementById(stickId);
//   const valoresEl = document.getElementById(valoresId);

//   let ativo = false;
//   let raioMax = 45; // limite de deslocamento do manípulo (px)

//   function centro() {
//     const rect = container.getBoundingClientRect();
//     return { cx: rect.left + rect.width / 2, cy: rect.top + rect.height / 2 };
//   }

//   function mover(clientX, clientY) {
//     const { cx, cy } = centro();
//     let dx = clientX - cx;
//     let dy = clientY - cy;
//     const dist = Math.sqrt(dx * dx + dy * dy);
//     if (dist > raioMax) {
//       dx = (dx / dist) * raioMax;
//       dy = (dy / dist) * raioMax;
//     }
//     stick.style.left = (45 + dx) + "px";
//     stick.style.top = (45 + dy) + "px";

//     // normaliza para -1..1 (Y invertido: para cima = valor maior)
//     const normX = dx / raioMax;
//     const normY = -dy / raioMax;

//     const valX = Math.round(500 + normX * 500);
//     const valY = Math.round(500 + normY * 500);
//     valoresEl.textContent = "X: " + valX + " | Y: " + valY;
//     onChange(valX, valY);
//   }

//   function soltar() {
//     ativo = false;
//     stick.style.left = "45px";
//     stick.style.top = "45px";
//     valoresEl.textContent = "X: 500 | Y: 500";
//     onChange(500, 500);
//   }

//   stick.addEventListener("pointerdown", (e) => {
//     ativo = true;
//     stick.setPointerCapture(e.pointerId);
//   });
//   stick.addEventListener("pointermove", (e) => {
//     if (ativo) mover(e.clientX, e.clientY);
//   });
//   stick.addEventListener("pointerup", soltar);
//   stick.addEventListener("pointercancel", soltar);
// }

// criarJoystick("joy1", "stick1", "valores1", (x, y) => { estado.x1 = x; estado.y1 = y; });
// criarJoystick("joy2", "stick2", "valores2", (x, y) => { estado.x2 = x; estado.y2 = y; });

// // Botão
// const botao = document.getElementById("botao");
// const valoresBtn = document.getElementById("valoresBtn");

// function botaoPress(e) {
//   e.preventDefault();
//   estado.btn = 1;
//   botao.classList.add("pressionado");
//   valoresBtn.textContent = "Pressionado";
// }
// function botaoSolta(e) {
//   e.preventDefault();
//   estado.btn = 0;
//   botao.classList.remove("pressionado");
//   valoresBtn.textContent = "Solto";
// }

// botao.addEventListener("pointerdown", botaoPress);
// botao.addEventListener("pointerup", botaoSolta);
// botao.addEventListener("pointercancel", botaoSolta);
// botao.addEventListener("pointerleave", botaoSolta);

// // Envia o estado atual ao ESP32 periodicamente
// setInterval(() => {
//   const url = `/update?x1=${estado.x1}&y1=${estado.y1}&x2=${estado.x2}&y2=${estado.y2}&btn=${estado.btn}`;
//   fetch(url).catch(() => {}); // ignora falhas silenciosamente
// }, 120);
// </script>

// </body>
// </html>
// )rawliteral";

// // ---------- Handlers do servidor ----------

// void handleRoot() {
//   server.send_P(200, "text/html", index_html);
// }

// void handleUpdate() {
//   if (server.hasArg("x1")) joy1X = server.arg("x1").toInt();
//   if (server.hasArg("y1")) joy1Y = server.arg("y1").toInt();
//   if (server.hasArg("x2")) joy2X = server.arg("x2").toInt();
//   if (server.hasArg("y2")) joy2Y = server.arg("y2").toInt();
//   if (server.hasArg("btn")) botaoPressionado = server.arg("btn").toInt() == 1;

//   // Imprime no Monitor Serial
//   Serial.print("Joystick1 -> X: ");
//   Serial.print(joy1X);
//   Serial.print(" | Y: ");
//   Serial.print(joy1Y);
//   Serial.print("   Joystick2 -> X: ");
//   Serial.print(joy2X);
//   Serial.print(" | Y: ");
//   Serial.print(joy2Y);
//   Serial.print("   Botao: ");
//   Serial.println(botaoPressionado ? "PRESSIONADO" : "solto");

//   server.send(200, "text/plain", "OK");
// }

// void handleNotFound() {
//   server.send(404, "text/plain", "Pagina nao encontrada");
// }

// float posicaoParaAngulo(int posicao){
//   return posicao * 240.0 / 1000.0;
// }

// // ---------- Setup e Loop ----------

// void setup() {
//   Serial.begin(115200);
//   Serial2.begin(9600);
//   delay(500);

//   // Cria o Access Point
//   WiFi.softAP(AP_SSID, AP_PASSWORD);
//   IPAddress ip = WiFi.softAPIP();

//   Serial.println();
//   Serial.println("=== ESP32 Joystick Server ===");
//   Serial.print("Rede WiFi criada: ");
//   Serial.println(AP_SSID);
//   Serial.print("Senha: ");
//   Serial.println(AP_PASSWORD);
//   Serial.print("Acesse no navegador: http://");
//   Serial.println(ip);

//   server.on("/", handleRoot);
//   server.on("/update", handleUpdate);
//   server.onNotFound(handleNotFound);

//   server.begin();
//   Serial.println("Servidor HTTP iniciado.");

//   servos[0].ID = 1;
//   servos[1].ID = 2;
//   servos[2].ID = 3;
//   servos[3].ID = 4;
//   servos[4].ID = 5;
  
//   servos[0].Position = posX1;
//   servos[1].Position = posY2;
//   servos[2].Position = posX2;
//   servos[3].Position = posY2;
//   servos[4].Position = posW;

//   Controller.moveServos(servos, 5, 500);
// }

// void loop() {
//   server.handleClient();

//   if(millis() - tempoControle >= intervaloControle){

//     tempoControle = millis();

//     if(botaoPressionado && !ultimoEstado){
//     estadoServo5 = !estadoServo5;
    
//       if(estadoServo5){
//         posW = 1000;
//       }
//       else{
//         posW = 0;
//       }
//     }

//     ultimoEstado = botaoPressionado;

//     //servo 1 

//     int erroX1 = joy1X - 500;

//     if(abs(erroX1) > DEADZONE){

//       int passo = map(abs(erroX1), DEADZONE, 511, 1, MAX_STEP);

//       if(erroX1 > 0){
//         posX1 += passo;
//       }
//       else{
//         posX1 -= passo;
//       }
//     }

//     //servo 2

//     int erroY1 = joy1Y - 500;

//     if(abs(erroY1) > DEADZONE){

//       int passo = map(abs(erroY1), DEADZONE, 511, 1, MAX_STEP);

//       if(erroY1 > 0){
//         posY1 += passo;
//       }
//       else{
//         posY1 -= passo;
//       }
//     }

//     //servo 3

//     int erroX2 = joy2X - 500;

//     if(abs(erroX2), DEADZONE, 511, 1, MAX_STEP){

//       int passo = map(abs(erroX2), DEADZONE, 511, 1, MAX_STEP);

//       if(erroX2 > 0){
//         posX2 += passo;
//       }
//       else{
//         posX2 -= passo;
//       }
//     }

//     //servo 4

//     int erroY2 = joy2Y - 500;

//     if(abs(erroY2) > DEADZONE){
      
//       int passo = map(abs(erroY2), DEADZONE, 511, 1, MAX_STEP);

//       if(erroY2 > 0){
//         posY2 += passo;
//       }
//       else{
//         posY2 -= passo;
//       }
//     }

//     posX1 = constrain(posX1, 0, 1000);
//     posY1 = constrain(posY1, 0 , 1000);
//     posX2 = constrain(posX2, 0, 1000);
//     posY2 = constrain(posY2, 0, 1000);

//     servos[0].Position = posX1;
//     servos[1].Position = posY1;
//     servos[2].Position = posX2;
//     servos[3].Position = posY2;
//     servos[4].Position = posW;

//     Controller.moveServos(servos, 5, 20);

//   }

// }

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
