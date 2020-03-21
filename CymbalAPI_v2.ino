#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

//voor wifi
#include "wificredentials.h"
const char *ssid     = mySSID;
const char *password = myPASSWORD;

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output2State = "on";

// Assign output variables to GPIO pins
const int output2 = 2;

// Processing helpers
int flagDemo = 0;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

//prototypes
void processIncoming(WiFiClient &client);
void sendResponseObject(WiFiClient &client);
  
void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(output2, OUTPUT);
  // Set outputs to LOW
  Serial.println("runDemo: output2, HIGH" );
   digitalWrite(output2, HIGH);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void runDemo() {
  Serial.println("runDemo()" );
  Serial.println("runDemo: output2, LOW" );
  digitalWrite(output2, LOW); //Pin 2 is omgekeerd: LOW is aan.
  delay(4000); //2 seconden wachten
  Serial.println("runDemo: output2, HIGH" );
  digitalWrite(output2, HIGH);
  flagDemo = 0;
}

void sendResponseObject(WiFiClient &client){ 
  Serial.println(F("sendResponseObject"));
  // Read the request (we ignore the content in this example)
  while (client.available()) client.read();

  // Allocate JsonBuffer
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonBuffer<207> jsonBuffer;

  // Create the root object
  JsonObject& root = jsonBuffer.createObject();

  // Create the "analog" array
  JsonArray& analogValues = root.createNestedArray("analog");
  for (int pin = 0; pin < 1; pin++) {
    // Read the analog input
    int value = analogRead(pin);

    // Add the value at the end of the array
    analogValues.add(value);
  }

  // Create the "digital" array
  JsonArray& digitalValues = root.createNestedArray("digital");
  for (int pin = 0; pin < 9; pin++) {
    // Read the digital input
    int value = digitalRead(pin);

    // Add the value at the end of the array
    digitalValues.add(value);
  }

  Serial.print(F("Sending: "));
  root.printTo(Serial);
  Serial.println();

  // Write response headers
  client.println("HTTP/1.0 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();

  // Write JSON document
  Serial.println("send JSON to client.");
  root.prettyPrintTo(client);

  // Disconnect
  Serial.println("Disconnect client.");
  client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
}

void processIncoming(WiFiClient &client) {

  int endpos;
  String tempstring;
  String theline;
  String commandstring ="";
  String demostring = "demo";

  String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      //Serial.println("while (client.connected())");
      if (client.available()) {             // if there's bytes to read from the client, in deze loop wordt de header gelezen
        //Serial.println("bytes uit de header lezen");
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        theline += c;
        if (c == '\n') {                    // if the byte is a newline character     
          //regel compleet    
          if (theline.indexOf("hh:") == 0) {
             endpos = theline.indexOf("\n");
             Serial.println("hh waarde: >" + theline.substring(4,endpos) + "<");
          }
          if (theline.indexOf("mm:") == 0) {
             endpos = theline.indexOf("\n");
             Serial.println("mm waarde: >" + theline.substring(4,endpos) + "<");
          }
          if (theline.indexOf("cmd:") == 0) {
             endpos = theline.indexOf("\n");
             Serial.println("cmd waarde: >" + theline.substring(5,endpos) + "<");
             commandstring = commandstring + theline.substring(5,endpos);
             commandstring.trim();
             Serial.println("commandstring: >" + commandstring + "<");
             
             if (commandstring == demostring) {
                flagDemo = 1;
                Serial.println("commandstring wel demo");
             } else {
                Serial.println("commandstring niet demo");
             }
          }
          
          //regel weer leegmaken
          theline = "";
           
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, 
          if (currentLine.length() == 0) {
            Serial.println(header);
 
            // bypass to turn the GPIOs on and off
            if (header.indexOf("GET /2/on") >= 0) {
              Serial.println("GPIO 2 on");
              output2State = "on";
              digitalWrite(output2, HIGH);
            } else if (header.indexOf("GET /2/off") >= 0) {
              Serial.println("GPIO 2 off");
              output2State = "off";
              digitalWrite(output2, LOW);
            }
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }

    // Clear the header variable
    header = "";
}


void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          
    Serial.println("call processIncoming"); 
    processIncoming(client);                //process GET request from the http header
    //bij een demo doen we alleen demo, geen json response
    if (flagDemo == 1){
      runDemo();
      // Write response headers
      client.println("HTTP/1.0 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();      
    } else {
      Serial.println("call sendResponseObject"); 
      sendResponseObject(client);             //return JSON object to requestor
      if (flagDemo == 1){
        runDemo();
      }
    }
  }
}
