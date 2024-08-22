
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>
//#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
//------- VARIABLES-----------
HardwareSerial MySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&MySerial);
int mode = 0;
String header;
uint8_t id;
uint8_t lastId;
uint8_t numberfingers;
String ParameterID;
AsyncWebServer server(80);
WebSocketsServer websockets(81);
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

//-------VARIABLES------------










//---------SETUP -------------
void setup()
{
  MySerial.begin(115200, SERIAL_8N1, 4, 5);
  Serial.begin(115200);
  const char* ssid     = "The Promised LAN 2.4G";
  const char* password = "123nuevop24";
  WiFi.disconnect();
  Serial.print("Conectando a  ");
  Serial.println(ssid);
  //Conectamos el esp a la red wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  //Intentamos conectarnos a la red
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  //Si logramos conectarnos mostramos la ip a la que nos conectamos
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
 
    int paramsNr = request->params();
    Serial.println(paramsNr);
 
    for(int i=0;i<paramsNr;i++){
 
        AsyncWebParameter* p = request->getParam(i);
        Serial.print("Param name: ");
        Serial.println(p->name());
        Serial.print("Param value: ");
        Serial.println(p->value());
        ParameterID = p->value();
        Serial.println("------");
    }
    DynamicJsonDocument doc(2048);
    doc["msg"] = "Mensaje recibido";
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json",json);
    mode = 1;
  });

  server.on("/findSensor", HTTP_GET, [](AsyncWebServerRequest *request){
 
   
    DynamicJsonDocument doc(2048);
    doc["msg"] = "Lector Encontrado";
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json",json);

    
  });
  
  server.begin();
  websockets.begin();
 
  finger.begin(57600);
  
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }
  finger.getTemplateCount();
    Serial.println("Waiting for valid finger...");
      Serial.print("Sensor contains "); Serial.print(finger.templateCount); 
      Serial.println(" templates");
      numberfingers = finger.templateCount;
}
//---------SETUP -------------




//-----VOID LOOP---------
void loop()      // run over and over again
{
  websockets.loop();
  websockets.broadcastTXT("Probando");
  if(mode==0){
    getFingerprintID();
    delay(50); 
    }else{
  id = numberfingers + 1;
  getFingerprintEnroll();
      }      
}

//--------VOID LOOP-----------


uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  if(WiFi.status()== WL_CONNECTED){ 
    HTTPClient http;
    DynamicJsonDocument doc(2048);
    doc["idHuella"] = finger.fingerID;
    String json;
    serializeJson(doc, json);
    http.begin("http://192.168.1.104:3000/NewAttendance");        
    http.addHeader("Content-Type", "application/json");

    int codigo_respuesta = http.POST(json);

    if(codigo_respuesta>0){
      Serial.println("Código HTTP ► " + String(codigo_respuesta));

      if(codigo_respuesta == 400){
        String cuerpo_respuesta = http.getString();
        Serial.println("El servidor respondió ▼ ");
        Serial.println(cuerpo_respuesta);
        }
      if(codigo_respuesta == 200){
        String cuerpo_respuesta = http.getString();
        Serial.println("El servidor respondió ▼ ");
        Serial.println(cuerpo_respuesta);
      }

    }else{

     Serial.print("Error enviando POST, código: ");
     Serial.println(codigo_respuesta);

    }

    http.end();  //libero recursos

  }else{

     Serial.println("Error en la conexión WIFI");

  }

  

  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}






uint8_t getFingerprintEnroll() {

  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      websockets.broadcastTXT("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  websockets.broadcastTXT("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    }
  }

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      websockets.broadcastTXT("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
    websockets.broadcastTXT("Prints matched!");
  } 
  
  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    websockets.broadcastTXT("Stored!");
    if(WiFi.status()== WL_CONNECTED){ 
    HTTPClient http;
    DynamicJsonDocument doc(2048);
    doc["id"] = ParameterID;
    doc["huella"] = id;
    String json;
    serializeJson(doc, json);
    http.begin("http://192.168.1.104:3000/addFingerPrint");        
    http.addHeader("Content-Type", "application/json");

    int codigo_respuesta = http.PUT(json);

    if(codigo_respuesta>0){
      Serial.println("Código HTTP ► " + String(codigo_respuesta));

      if(codigo_respuesta == 400){
        String cuerpo_respuesta = http.getString();
        Serial.println("El servidor respondió ▼ ");
        Serial.println(cuerpo_respuesta);
        }
      if(codigo_respuesta == 200){
        String cuerpo_respuesta = http.getString();
        Serial.println("El servidor respondió ▼ ");
        Serial.println(cuerpo_respuesta);
      }

    }else{

     Serial.print("Error enviando POST, código: ");
     Serial.println(codigo_respuesta);

    }

    http.end();  //libero recursos

  }else{

     Serial.println("Error en la conexión WIFI");

  }

  
    mode = 0;
    numberfingers = numberfingers + 1;
  } 

  return true;
}
