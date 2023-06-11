#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>


#define AP_MODE 1
#define DEBUG 0


uint8_t left_dir = LOW;
int left_speed = LOW;
uint8_t right_dir = LOW;
int right_speed = LOW;


const int pwmMotorA = D1;
const int pwmMotorB = D2;
const int dirMotorA = D3;
const int dirMotorB = D4;


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>Joystick Remote</title>
<style>
body {
    text-align: center;
    font-size: 40px;
    height: 100vh;
    padding: 0;
    margin-top: 40px;
}

input.large {
transform : scale(5);
}

container {
    display: flex;
    justify-content: center;
    align-items: center;
    height: 100%;
    width: 100%;
}

label {
  margin: 20px;
}

img {
    width: 50vw;
}

</style>
</head>
<body>
<container>
<div id="stick">
<img src='data:image/svg+xml;charset=utf-8,<svg version="1.0" xmlns:svg="http://www.w3.org/2000/svg" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0" y="0" width="500" height="500"><defs><linearGradient id="linearGradient"><stop style="stop-color:%23ff0000;stop-opacity:1" offset="0" id="stop6455"/><stop style="stop-color:%23ff6666;stop-opacity:1" offset="1" id="stop6457"/></linearGradient><radialGradient cx="171.20810" cy="196.85463" r="200" fx="171.20810" fy="196.85463" id="radialGradient" xlink:href="%23linearGradient" gradientUnits="userSpaceOnUse" gradientTransform="matrix(1.040418,0.796229,-0.814518,1.064316,153.4218,-150.4353)"/></defs><circle cx="250" cy="250" r="200" style="fill:url(%23radialGradient)" /></svg>'/>    
</div>
</container>

<script>
class JoystickController {
  constructor( stickID, maxDistance, deadzone ){
    this.id = stickID;
    let stick = document.getElementById(stickID);
    this.dragStart = null;
    this.touchId = null;    
    this.active = false;
    this.value = { left_dir: 0, left_speed: 0, right_dir:0, right_speed: 0 };

        let self = this;

    function handleDown(event){
        self.active = true;
      stick.style.transition = '0s';

      event.preventDefault();
        if (event.changedTouches)
          self.dragStart = { x: event.changedTouches[0].clientX, y: event.changedTouches[0].clientY };
        else
          self.dragStart = { x: event.clientX, y: event.clientY };
        if (event.changedTouches)
          self.touchId = event.changedTouches[0].identifier;
    }


    function setMovement(angle, speed, xPercent, yPercent) {
     /* speed = 5*Math.round(2*speed)
      speed = (speed > 9)?9:speed*/
      speed = 9
      /*

       Implement a discrete rotation only, based on 45deg angles

      */

      let rot_angle  = Math.round(angle*180/Math.PI)
      console.log(rot_angle)
      let l = 9
      let r = 9
      let ld = 0
      let rd = 0
      if ((rot_angle > -112.5) && (rot_angle < -67.5)) { // UP
        l = 9
        r = 9
        rd = 1
        ld = 0
      } else if ((rot_angle > -22.5) && (rot_angle < 22.5)) { // LEFT
        l = 9
        r = 9
        rd = 0
        ld = 0
      } else if ((rot_angle > 67.5) && (rot_angle < 112.5)) { // DOWN
        l = 9
        r = 9
        rd = 0
        ld = 1
      } else if ( ( (rot_angle > 157.5) && (rot_angle <= 180) ) || 
                  ( (rot_angle >= -180) && (rot_angle < -157.5))) { // Left
        l = 9
        r = 9
        rd = 1
        ld = 1
      }

      self.value = { left_dir: ld, left_speed: l, right_dir: rd, right_speed: r}

       //console.log(self.value)
   }
    function handleMove(event) {
        if ( !self.active ) return;
           let touchmoveId = null;
        if (event.changedTouches) {
          for (let i = 0; i < event.changedTouches.length; i++) {
            if (self.touchId == event.changedTouches[i].identifier) {
              touchmoveId = i;
              event.clientX = event.changedTouches[i].clientX;
              event.clientY = event.changedTouches[i].clientY;
            }
          }
          if (touchmoveId == null) return;
        }
        const xDiff = event.clientX - self.dragStart.x;
        const yDiff = event.clientY - self.dragStart.y;
        const angle = Math.atan2(yDiff, xDiff);
        const distance = Math.min(maxDistance, Math.hypot(xDiff, yDiff));
        const xPosition = distance * Math.cos(angle);
        const yPosition = distance * Math.sin(angle);

        stick.style.transform = `translate3d(${xPosition}px, ${yPosition}px, 0px)`;
        const distance2 = (distance < deadzone) ? 0 : maxDistance / (maxDistance - deadzone) * (distance - deadzone);
        const xPosition2 = distance2 * Math.cos(angle);
        const yPosition2 = distance2 * Math.sin(angle);
        const xPercent = parseFloat((xPosition2 / maxDistance).toFixed(2));
        const yPercent = parseFloat((yPosition2 / maxDistance).toFixed(2));
        setMovement(angle, distance2/maxDistance, xPercent, yPercent)
    }

    function handleUp(event) {
        if ( !self.active ) return;
        if (event.changedTouches && self.touchId != event.changedTouches[0].identifier) return;
        stick.style.transition = '.2s';

        stick.style.transform = `translate3d(0px, 0px, 0px)`;

        self.value = {  left_dir: 0, left_speed: 0, right_dir:0, right_speed: 0 };

        self.touchId = null;
        self.active = false;

    }

    stick.addEventListener('mousedown', handleDown);
    stick.addEventListener('touchstart', handleDown);
    document.addEventListener('mousemove', handleMove, {passive: false});
    document.addEventListener('touchmove', handleMove, {passive: false});
    document.addEventListener('mouseup', handleUp);
    document.addEventListener('touchend', handleUp);
  }
}


let joystick = new JoystickController("stick", 128, 32);

cached_value = joystick.value


let update = () => {
  if (websocket.readyState !== WebSocket.OPEN) return;
  if ((joystick.value.left_dir !== cached_value.left_dir) ||
      (joystick.value.left_speed !== cached_value.left_speed) ||
      (joystick.value.right_dir !== cached_value.right_dir) ||
      (joystick.value.right_speed !== cached_value.right_speed)) {
        cached_value = joystick.value;
        //console.log(joystick.value)
        websocket.send(joystick.value.left_dir + "," + joystick.value.left_speed + "," + joystick.value.right_dir + "," + joystick.value.right_speed + "\n")
      }           
}


let loop = () => {
  requestAnimationFrame(loop);
  update();
}

var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);

  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }

  function onOpen(event) {
    console.log('Connection opened');
  }

  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }

  function onMessage(event) {
    console.log(event)
  }

  function onLoad(event) {
    initWebSocket();
    loop();
  }

</script>
</body>
</html>
)rawliteral";




void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;

  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    left_dir = (data[0] == '0')?LOW:HIGH;
    left_speed = 1000*(data[2]-'0')/9;
    right_dir = (data[4] == '0')?LOW:HIGH;
    right_speed = 1000*(data[6]-'0')/9;
    #if (DEBUG)
    Serial.printf("%s\nLeft dir: %d  Left: %d   Right dir: %d Right %d\n", (char*)data, left_dir, left_speed, right_dir, right_speed);

    #endif
  }
}



void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {

  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;

    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;

    case WS_EVT_DATA:

    if (len > 6) // Messages must be 5 bytes long at least
        handleWebSocketMessage(arg, data, len);
      break;

    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}


void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);

}


void setup(){
  // Serial port for debugging purposes
  Serial.begin(9600);

  pinMode(pwmMotorA, OUTPUT);
  pinMode(pwmMotorB, OUTPUT);
  pinMode(dirMotorA, OUTPUT);
  pinMode(dirMotorB, OUTPUT);


  // Connect to Wi-Fi
  const char* ssid = "IARA-BOT";
  const char* password = "compiladores";

  #if (AP_MODE)
  WiFi.softAP(ssid, password);
  Serial.printf("Connect to %s with IP-Address: %s\n", ssid, WiFi.softAPIP().toString().c_str());

  #else

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)  {
      delay(500);
      Serial.print(".");
    }
  Serial.printf("Connect to %s with IP-Address: %s\n", ssid, WiFi.localIP().toString().c_str());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  #endif

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Start server
  server.begin();
  initWebSocket();
}


void loop() {
  digitalWrite(dirMotorA, left_dir);
  digitalWrite(dirMotorB, right_dir);
  digitalWrite(pwmMotorA, left_speed);
  digitalWrite(pwmMotorB, right_speed);
  ws.cleanupClients();
}