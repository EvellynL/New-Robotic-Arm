#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LobotServoController.h>
#include <esp_system.h>   // para esp_reset_reason()

// ---------- Configurações da rede WiFi (modo Access Point) ----------
const char* AP_SSID = "ESP32_Joystick";
const char* AP_PASSWORD = "12345678"; // mínimo 8 caracteres

WebServer server(80);

LobotServoController Controller(Serial2);

LobotServo servos[5];

// ---------- Variáveis de estado dos joysticks/botão ----------
bool estadoServo5 = false;
bool ultimoEstado = false;   // botão começa solto

int joy1X = 500, joy1Y = 500;
int joy2X = 500, joy2Y = 500;
bool botaoPressionado = false;

int posX1 = 700;
int posY1 = 550;
int posX2 = 900;
int posY2 = 50;
int posW = 0;

const int DEADZONE = 30;
// CORREÇÃO: intervalo passou de 20 ms para 50 ms (2,5x), então o passo
// máximo subiu de 8 para 20 para manter a mesma velocidade de movimento.
const int MAX_STEP = 20;

unsigned long tempoControle = 0;
unsigned long tempoSerial = 0;

// CORREÇÃO: a 9600 baud um pacote de 5 servos leva ~23 ms para ser enviado.
// Com 20 ms a serial ficava sempre saturada; 50 ms dá folga.
const int intervaloControle = 50;
const int intervaloSerial = 200;

// ---------- Página HTML (com CSS e JavaScript embutidos) ----------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
<title>ESP32 Joystick Control</title>
<style>
  html, body {
    margin: 0; padding: 0;
    background: #101418;
    color: #eee;
    font-family: Arial, Helvetica, sans-serif;
    overflow: hidden;
    height: 100%;
    -webkit-user-select: none;
    user-select: none;
    touch-action: none;
  }
  h1 {
    text-align: center;
    font-size: 18px;
    padding: 10px 0 0 0;
    margin: 0;
    color: #883BFF;
  }
  .painel {
    display: flex;
    align-items: center;
    justify-content: space-evenly;
    height: 90vh;
  }
  .joystick-container {
    position: relative;
    width: 150px;
    height: 150px;
    background: radial-gradient(circle, #1c2733 0%, #10151b 100%);
    border: 2px solid #2f4a63;
    border-radius: 50%;
    box-shadow: inset 0 0 15px #000;
  }
  .stick {
    position: absolute;
    width: 60px;
    height: 60px;
    left: 45px;
    top: 45px;
    border-radius: 50%;
    background: radial-gradient(circle at 30% 30%, #883BFF, #1573b5);
    box-shadow: 0 0 10px #1573b5aa;
    cursor: grab;
  }
  .label {
    text-align: center;
    margin-top: 8px;
    font-size: 12px;
    color: #9fd6ff;
  }
  .valores {
    text-align: center;
    font-size: 11px;
    color: #7d93a6;
    margin-top: 4px;
  }
  .botao-wrap {
    display: flex;
    flex-direction: column;
    align-items: center;
  }
  #botao {
    width: 80px;
    height: 80px;
    border-radius: 50%;
    background: radial-gradient(circle at 30% 30%, #ff6f6f, #b51515);
    border: 2px solid #7a0f0f;
    box-shadow: 0 0 10px #b5151566;
    color: #fff;
    font-weight: bold;
    font-size: 13px;
  }
  #botao.pressionado {
    background: radial-gradient(circle at 30% 30%, #7dff7d, #159b15);
    border-color: #0f7a0f;
  }
</style>
</head>
<body>

<h1>STEM CRIAR</h1>

<div class="painel">

  <div class="joystick-wrap">
    <div class="joystick-container" id="joy1">
      <div class="stick" id="stick1"></div>
    </div>
    <div class="label">Joystick 1</div>
    <div class="valores" id="valores1">X: 500 | Y: 500</div>
  </div>

  <div class="botao-wrap">
    <button id="botao">GARRA</button>
    <div class="valores" id="valoresBtn">Solto</div>
  </div>

  <div class="joystick-wrap">
    <div class="joystick-container" id="joy2">
      <div class="stick" id="stick2"></div>
    </div>
    <div class="label">Joystick 2</div>
    <div class="valores" id="valores2">X: 500 | Y: 500</div>
  </div>

</div>

<script>
// Estado global enviado ao ESP32
let estado = { x1: 500, y1: 500, x2: 500, y2: 500, btn: 0 };

function criarJoystick(containerId, stickId, valoresId, onChange) {
  const container = document.getElementById(containerId);
  const stick = document.getElementById(stickId);
  const valoresEl = document.getElementById(valoresId);

  let ativo = false;
  let raioMax = 45; // limite de deslocamento do manípulo (px)

  function centro() {
    const rect = container.getBoundingClientRect();
    return { cx: rect.left + rect.width / 2, cy: rect.top + rect.height / 2 };
  }

  function mover(clientX, clientY) {
    const { cx, cy } = centro();
    let dx = clientX - cx;
    let dy = clientY - cy;
    const dist = Math.sqrt(dx * dx + dy * dy);
    if (dist > raioMax) {
      dx = (dx / dist) * raioMax;
      dy = (dy / dist) * raioMax;
    }
    stick.style.left = (45 + dx) + "px";
    stick.style.top = (45 + dy) + "px";

    // normaliza para -1..1 (Y invertido: para cima = valor maior)
    const normX = dx / raioMax;
    const normY = -dy / raioMax;

    const valX = Math.round(500 + normX * 500);
    const valY = Math.round(500 + normY * 500);
    valoresEl.textContent = "X: " + valX + " | Y: " + valY;
    onChange(valX, valY);
  }

  function soltar() {
    ativo = false;
    stick.style.left = "45px";
    stick.style.top = "45px";
    valoresEl.textContent = "X: 500 | Y: 500";
    onChange(500, 500);
  }

  stick.addEventListener("pointerdown", (e) => {
    ativo = true;
    stick.setPointerCapture(e.pointerId);
  });
  stick.addEventListener("pointermove", (e) => {
    if (ativo) mover(e.clientX, e.clientY);
  });
  stick.addEventListener("pointerup", soltar);
  stick.addEventListener("pointercancel", soltar);
}

criarJoystick("joy1", "stick1", "valores1", (x, y) => { estado.x1 = x; estado.y1 = y; });
criarJoystick("joy2", "stick2", "valores2", (x, y) => { estado.x2 = x; estado.y2 = y; });

// Botão
const botao = document.getElementById("botao");
const valoresBtn = document.getElementById("valoresBtn");

function botaoPress(e) {
  e.preventDefault();
  estado.btn = 1;
  botao.classList.add("pressionado");
  valoresBtn.textContent = "Pressionado";
}
function botaoSolta(e) {
  e.preventDefault();
  estado.btn = 0;
  botao.classList.remove("pressionado");
  valoresBtn.textContent = "Solto";
}

botao.addEventListener("pointerdown", botaoPress);
botao.addEventListener("pointerup", botaoSolta);
botao.addEventListener("pointercancel", botaoSolta);
botao.addEventListener("pointerleave", botaoSolta);

// Envia o estado atual ao ESP32 periodicamente
setInterval(() => {
  const url = `/update?x1=${estado.x1}&y1=${estado.y1}&x2=${estado.x2}&y2=${estado.y2}&btn=${estado.btn}`;
  fetch(url).catch(() => {}); // ignora falhas silenciosamente
}, 120);
</script>

</body>
</html>
)rawliteral";

// ---------- Handlers do servidor ----------

void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

void handleUpdate() {
  if (server.hasArg("x1")) joy1X = server.arg("x1").toInt();
  if (server.hasArg("y1")) joy1Y = server.arg("y1").toInt();
  if (server.hasArg("x2")) joy2X = server.arg("x2").toInt();
  if (server.hasArg("y2")) joy2Y = server.arg("y2").toInt();
  if (server.hasArg("btn")) botaoPressionado = server.arg("btn").toInt() == 1;

  // Os prints que ficavam aqui (a cada 120 ms) foram movidos para o loop(),
  // a cada 200 ms, para não lotar o Monitor Serial.

  server.send(200, "text/plain", "OK");
}

void handleNotFound() {
  server.send(404, "text/plain", "Pagina nao encontrada");
}

float posicaoParaAngulo(int posicao){
  return posicao * 240.0 / 1000.0;
}

// Calcula o novo valor de posição a partir do joystick (com zona morta)
int atualizarPosicao(int pos, int valorJoystick){
  int erro = valorJoystick - 500;

  if(abs(erro) > DEADZONE){
    int passo = map(abs(erro), DEADZONE, 500, 1, MAX_STEP);
    if(erro > 0){
      pos += passo;
    }
    else{
      pos -= passo;
    }
  }

  return constrain(pos, 0, 1000);
}

// ---------- Setup e Loop ----------

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  delay(500);

  Serial.println();
  Serial.print("Motivo do reset: ");
  Serial.println(esp_reset_reason()); // 1 = ligou normal, 9 = brownout

  // Cria o Access Point
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress ip = WiFi.softAPIP();

  Serial.println("=== ESP32 Joystick Server ===");
  Serial.print("Rede WiFi criada: ");
  Serial.println(AP_SSID);
  Serial.print("Senha: ");
  Serial.println(AP_PASSWORD);
  Serial.print("Acesse no navegador: http://");
  Serial.println(ip);

  server.on("/", handleRoot);
  server.on("/update", handleUpdate);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Servidor HTTP iniciado.");

  servos[0].ID = 1;
  servos[1].ID = 2;
  servos[2].ID = 3;
  servos[3].ID = 4;
  servos[4].ID = 5;

  servos[0].Position = posX1;
  servos[1].Position = posY1;   // CORREÇÃO: antes estava posY2
  servos[2].Position = posX2;
  servos[3].Position = posY2;
  servos[4].Position = posW;

  Controller.moveServos(servos, 5, 500);
}

void loop() {
  server.handleClient();

  if(millis() - tempoControle >= intervaloControle){

    tempoControle = millis();

    // Garra (servo 5): alterna a cada toque no botão
    if(botaoPressionado && !ultimoEstado){
      estadoServo5 = !estadoServo5;
      posW = estadoServo5 ? 400 : 0;
    }
    ultimoEstado = botaoPressionado;

    // CORREÇÃO: os 4 servos agora usam a mesma função, com a zona morta
    // aplicada corretamente. Antes, o servo 3 tinha
    //   if(abs(erroX2), DEADZONE, 511, 1, MAX_STEP)
    // que é sempre verdadeiro e fazia o servo 3 andar sozinho até 0.
    posX1 = atualizarPosicao(posX1, joy1X); // servo 1
    posY1 = atualizarPosicao(posY1, joy1Y); // servo 2
    posX2 = atualizarPosicao(posX2, joy2X); // servo 3
    posY2 = atualizarPosicao(posY2, joy2Y); // servo 4

    servos[0].Position = posX1;
    servos[1].Position = posY1;
    servos[2].Position = posX2;
    servos[3].Position = posY2;
    servos[4].Position = posW;

    Controller.moveServos(servos, 5, intervaloControle);
  }
}
