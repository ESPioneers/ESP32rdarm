#include <WiFi.h>         //Bibliotheken einbinden
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo360.h>
#include <ESP32Servo.h>

//================================================================================================================================================
//Variablen/Defines festlegen
//================================================================================================================================================

#define MOT1 12           //Anschlusspins für Servos
#define CONTROL 13        //Controlpin für 360°Servo
#define MOT2 14
#define MOT3 27
#define MOT4 26
#define MOT5 33
#define MOT6 32
#define MOT1_SPEED 120    //Speed für 360° Servo

#define LED_RED 25        //Anschlusspins Für LED
#define LED_GREEN 23
#define LED_BLUE 5

char wifiHostname[ ] = "Roboter 1";       //Name des ESP32 im Netzwerk

const char* ssid = "MyHotSpot";                    //Einlogdaten für Wifi Hotspot
const char* password = "fhbielefeld";

const char* PARAM_INPUT_1 = "Slider1";          //Variablen zum ausleden der Slider values aus Webinterface
const char* PARAM_INPUT_2 = "Slider2";
const char* PARAM_INPUT_3 = "Slider3";
const char* PARAM_INPUT_4 = "Slider4";
const char* PARAM_INPUT_5 = "Slider5";
const char* PARAM_INPUT_6 = "Slider6";

String slider1 = "180";                   //Variable für die Achsen festlegen
String slider2 = "90";                   //werden an Webinterface zurückgesandt!!!!
String slider3 = "45"; 
String slider4 = "90";
String slider5 = "90"; 
String slider6 = "90";  

int pos1;                       //Stelllvariablen für Achsen
int pos2;
int pos3;
int pos4;
int pos5;
int pos6;

//Diese Werte beeinflussen wie genau und welchen Winkel die Selvos exakt fahren
//Die eingestellten Werte ergeben sich durch Trial and Error

const int MG90S_stepMin = 500;       //Werte für Winkel angeben (MG90S)
const int MG90S_stepMax = 2500;
const int MG995_stepMin = 500;       //Werte für Winkel angeben (MG995)
const int MG995_stepMax = 2400;

bool change = true;          //Überprüfungsvariable: Main Loop wird nur ausgeführt wenn change=true
                             // Die Motoren drehen nur, wenn eine Änderung detektiert wird
bool DEMO = false;


//================================================================================================================================================
//Servoobjekte instanziieren
//================================================================================================================================================

ESP32Servo360 Servo1;    //Objekt instanziieren ohne Vorgabewerte

Servo Servo2; //funktioniert nicht, warum auch immer, muss aber bleiben!!!!
Servo Servo3;
Servo Servo4;
Servo Servo5;
Servo Servo6;
Servo Servo7;




//================================================================================================================================================
//HTML Code Webinterface
//================================================================================================================================================

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=yes">   <!--Hier wird die Größe der Seite für Mobilgeräre automatisch angepasst -->
    <style>
      html {text-align: center;}
    </style>
  </head>
  <body>
    
    <h1>Slider Steuerung fuer 6-Achs-Roboter</h1>
    
 
    <form action="/update">
      Achse 1 [0 - 360]<br>
      <input type="range" name="Slider1" id="Slider1" min="0" max="360" value="%VALUEAXIS1%"><br> 
      %VALUEAXIS1% grad<br><br>
      
      Achse 2 [0 - 180]<br>
      <input type="range" name="Slider2" id="Slider2" min="0" max="180" value="%VALUEAXIS2%"><br>
      %VALUEAXIS2% grad<br><br>
      
      Achse 3 [0 - 180]<br> 
      <input type="range" name="Slider3" id="Slider3" min="0" max="180" value="%VALUEAXIS3%"><br>
      %VALUEAXIS3% grad<br><br>
      
      Achse 4 [0 - 180]<br>
      <input type="range" name="Slider4" id="Slider4" min="0" max="180" value="%VALUEAXIS4%"><br>
      %VALUEAXIS4% grad<br><br>
      
      Achse 5 [0 - 180]<br>
      <input type="range" name="Slider5" id="Slider5" min="0" max="180" value="%VALUEAXIS5%"><br>
      %VALUEAXIS5% grad<br><br>
      
      Achse 6 [0 - 180]<br>
      <input type="range" name="Slider6" id="Slider6" min="0" max="180" value="%VALUEAXIS6%"><br>
      %VALUEAXIS6% grad<br><br>
      
      <input type="submit" value="Submit">
    </form>
    <form action="/demo">
    <br>
    --------------------------------------------------<br>
    <br>
      <input type="submit" value="Start Demo">
    </form>
    
    
    
  </body>
<html>

)rawliteral";

//================================================================================================================================================
//Prozessor für Templates
//================================================================================================================================================
//Der Prozessor sorgt dafür, dass die Varieablen zwischen dem Webinterface und dem ESP kommuniziert werden
String processor(const String& var)
{
  if (var == "VALUEAXIS1")
  {
    return slider1;
  }
  if (var == "VALUEAXIS2")
  {
    return slider2;
  }
  if (var == "VALUEAXIS3")
  {
    return slider3;
  }
  if (var == "VALUEAXIS4")
  {
    return slider4;
  }
  if (var == "VALUEAXIS5")
  {
    return slider5;
  }
  if (var == "VALUEAXIS6")
  {
    return slider6;
  }
}

//================================================================================================================================================
//Webserver port festlegen
//================================================================================================================================================

AsyncWebServer server(80);                      

//================================================================================================================================================
//SETUP
//================================================================================================================================================
void setup() 
{
  pinMode(MOT1, OUTPUT);    //Pins zuweisen
  pinMode(MOT2, OUTPUT);
  pinMode(MOT3, OUTPUT);
  pinMode(MOT4, OUTPUT);
  pinMode(MOT5, OUTPUT);
  pinMode(MOT6, OUTPUT);
  pinMode(CONTROL, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  
  Serial.begin (115200);                        //Serielle Kommunikationsschnittstelle aufbauen

  WiFi.setHostname(wifiHostname);               //Namen des ESP32 setzen

  WiFi.begin(ssid, password);                   //mit Wlan verbinden

  while(WiFi.status() != WL_CONNECTED)            //Solange nach Wlan Verbindung gesucht wird folgendes ausgeben:
  {
    delay(1000);
    Serial.println("Connecting to WiFi ...");
  }

  //Wenn eine Verbindung gefunden wurde, lokale IP des ESP ausgeben

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP adress: ");
  Serial.println(WiFi.localIP());

  server.on("/",HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send_P(200, "text/html", index_html, processor);        //hier wird die Website aufegrufen welche am Anfang unter const char index_html "gebaut" wurde
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)   //hier ist die updatefunktion , welche die slidervalues aktualisiert
  {
   
    if(request->hasParam(PARAM_INPUT_1))
    {
      slider1 = request->getParam(PARAM_INPUT_1)->value();
    }
    if(request->hasParam(PARAM_INPUT_2))
    {
      slider2 = request->getParam(PARAM_INPUT_2)->value();
    }
    if(request->hasParam(PARAM_INPUT_3))
    {
      slider3 = request->getParam(PARAM_INPUT_3)->value();
    }
    if(request->hasParam(PARAM_INPUT_4))
    {
      slider4 = request->getParam(PARAM_INPUT_4)->value();
    }
    if(request->hasParam(PARAM_INPUT_5))
    {
      slider5 = request->getParam(PARAM_INPUT_5)->value();
    }
    if(request->hasParam(PARAM_INPUT_6))
    {
      slider6 = request->getParam(PARAM_INPUT_6)->value();
    }

       
    request->send_P(200, "text/html", index_html, processor);
    change =true;
  });

    server.on("/demo", HTTP_GET, [](AsyncWebServerRequest *request)   //hier ist die demofunktion , welche aufgerufen wird, wenn der Button "Start Demo" gedrückt wird
  {
    DEMO = true;
    request->send_P(200, "text/html", index_html, processor);
  });

  server.begin();

//Servoeinstellungen---------------------------------------------------------------------

  Servo1.attach(MOT1,CONTROL);      //Zweiter Wert für Encoderpin
  Servo1.setSpeed(MOT1_SPEED);              //Speed für 360° Servo
  Servo1.rotateTo(0);               //Nullstellung anfahren

  ESP32PWM::allocateTimer(0);         //Alle Timer für Servos allokieren
  ESP32PWM::allocateTimer(1);         //notwendig!!!nicht hinterfragen!!!
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  Servo2.setPeriodHertz(50);          //Frequenzen der Servos fetslegen
  Servo3.setPeriodHertz(50);
  Servo4.setPeriodHertz(50);
  Servo5.setPeriodHertz(50);
  Servo6.setPeriodHertz(50);
  Servo7.setPeriodHertz(50);
  

  Servo2.attach(18);   //Servo-Objekte an Pins anschließen
                      //nicht hinterfragen Servo2 muss drin bleiben
  Servo7.attach(MOT2,MG995_stepMin,MG995_stepMax);
  Servo3.attach(MOT3,MG90S_stepMin,MG90S_stepMax);
  Servo4.attach(MOT4,MG90S_stepMin,MG90S_stepMax);
  Servo5.attach(MOT5,MG90S_stepMin,MG90S_stepMax);
  Servo6.attach(MOT6,MG90S_stepMin,MG90S_stepMax);
  
  
}

//================================================================================================================================================
//Umrechner-Funktion
//================================================================================================================================================

void Umrechner()          //schreibt die String Werte von der Website in Integer
{
  pos1 = slider1.toInt();
  pos2 = slider2.toInt();
  pos3 = slider3.toInt();
  pos4 = slider4.toInt();
  pos5 = slider5.toInt();
  pos6 = slider6.toInt();
}

void MoveRob(int a1,int a2, int a3,int a4, int a5, int a6)  // bewegt der Roboter zur angegebenen Position. Der Funktion werden die 6 Winkel mitgegeben
{
  int m = 100;      //Teiler für Einzelbewegungen
  double prePos1;
  double prePos2;
  double prePos3;
  double prePos4;
  double prePos5;
  double prePos6;
  
  prePos1=pos1;   //aktuelle Position jeder Achse merken
  prePos2=pos2;
  prePos3=pos3;
  prePos4=pos4;
  prePos5=pos5;
  prePos6=pos6;
  
  double move1;
  double move2;
  double move3;
  double move4;
  double move5;
  double move6;
  
  
  for(int i = 1;i<=m;i++)               //For-Schleife für die Bewegung zum Zielwinkel. Der Winkel wird in m Schritten angefahren, damit alle Achsen gleichzeitig "ankommen"
  {
    move1=((a1-prePos1)/m)*i+prePos1;
    move2=((a2-prePos2)/m)*i+prePos2;
    move3=((a3-prePos3)/m)*i+prePos3;
    move4=((a4-prePos4)/m)*i+prePos4;
    move5=((a5-prePos5)/m)*i+prePos5;
    move6=((a6-prePos6)/m)*i+prePos6;

  if(move1>360)       //Fehlerkorrektur, wenn werte über 180 oder unter 0 errechnet werden
    {
      move1=180;
    }
  if(move1<0)
    {
      move6=0;
    }
    
  if(move2>180)
    {
      move2=180;
    }
  if(move2<0)
    {
      move2=0;
    }
    
  if(move3>180)
    {
      move3=180;
    }
  if(move3<0)
    {
      move3=0;
    }
    
  if(move4>180)
    {
      move4=180;
    }
  if(move4<0)
    {
      move4=0;
    }
    
  if(move5>180)
    {
      move5=180;
    }
  if(move5<0)
    {
      move5=0;
    }

  if(move6>180)
    {
      move6=180;
    }
  if(move6<0)
    {
      move6=0;
    }
    
  
  if(prePos1 != a1)
    {
      Servo1.rotateTo(move1);
    }
  Servo7.write(move2);
  Servo3.write(move3);
  Servo4.write(move4);
  Servo5.write(move5);
  Servo6.write(move6);
  
  delay(20);              //Verzögerung zwischen den Minischritten
  
  }
  pos1=move1;
  pos2=move2;
  pos3=move3;
  pos4=move4;
  pos5=move5;
  pos6=move6;
}


void loop() 
{
  analogWrite(LED_RED,0);           //Standardmäßig leuchtet die LED grün
  analogWrite(LED_GREEN,255);
  analogWrite(LED_BLUE,0);

  
if(change == true)
  {
   analogWrite(LED_RED,255);        //Werden die Achsen über die Slider gesteuert wird die LED Rot
   analogWrite(LED_GREEN,0);
   analogWrite(LED_BLUE,0);
   
   Umrechner();
   
   Serial.print("Achse 2: ");
   Serial.print(pos2);
   Serial.println(" Grad");
   Servo7.write(pos2);
  
   Serial.print("Achse 3: ");
   Serial.print(pos3);
   Serial.println(" Grad");
   Servo3.write(pos3);
  
   Serial.print("Achse 4: ");
   Serial.print(pos4);
   Serial.println(" Grad");
   Servo4.write(pos4);
  
   Serial.print("Achse 5: ");
   Serial.print(pos5);
   Serial.println(" Grad");
   Servo5.write(pos5);
  
   Serial.print("Achse 6: ");
   Serial.print(pos6);
   Serial.println(" Grad");
   Servo6.write(pos6);
   
   Serial.print("Achse 1: ");
   Serial.print(pos1);
   Serial.println(" Grad");
   Servo1.rotateTo(pos1);
  
   
   change = false;
   delay(300);
  }
  
 if(DEMO == true)
  {
  analogWrite(LED_RED,255);     //Wird die Demo gefahren, leuchtet die LED Lila
  analogWrite(LED_GREEN,0);
  analogWrite(LED_BLUE,255);
  
  Serial.println("Demo läuft");
  MoveRob(180,135,45,90,45,90);   //Die verschiedenen Positionen werden angefahren
  MoveRob(120,145,55,150,135,0);
  MoveRob(270,40,35,90,80,0);
  MoveRob(270,40,35,90,80,180);
  MoveRob(270,40,35,90,80,0);
  MoveRob(270,40,35,90,80,180);
  MoveRob(270,118,34,90,51,0);
  MoveRob(270,136,61,90,60,0);
  MoveRob(270,118,34,90,51,0);
  MoveRob(360,118,34,90,51,180);
  MoveRob(360,136,61,90,60,180);
  MoveRob(360,118,34,90,51,180);
  MoveRob(180,90,45,90,90,90);
  Serial.println("Demo beendet");
  DEMO = false;
  }
}
 
