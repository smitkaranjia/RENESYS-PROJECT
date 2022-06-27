//****INSTALLING THE LIBRARIES***//
#include <IRremoteESP8266.h> 
#include <IRsend.h>  
#include <ESP8266WiFi.h>      
#include<PubSubClient.h>     
#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <Hash.h>
#include <FS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>


String old_ssid = "M40";   //the default ssid given to esp
String old_pwd = "1234678910";           //default password given to esp

//**** DECLARING INPUT PARAMETER FOR WEB SERVER *****// 
const char* input_ssid = "ssid";        //variable to store input data
const char* input_pwd = "pwd";          //variable used to store input password

//**** declaring variable of the code ****//
int number;                  //Define variable 'n' for the available sacnned network number
String wifi_net[100];    //define 'wifi_net' as the string array of maximum lenth which is 100 that will store the nearby ssid names
String new_ssid;        //storing the new ssid name as string
String new_pwd;         //string type for storing entered password


//**** create AsyncWebServer on PORT 80 ****// 
AsyncWebServer server(80);

//**** address of the mqtt broker ****//
const char* mqtt_server = "broker.mqtt-dashboard.com";
            
//**** declare espClient as mqtt client ****//
WiFiClient espClient;
PubSubClient client(espClient);


const uint16_t kIrLed = D2;            //pin num. on which ir transmitter is connected
IRsend irsend(kIrLed);                 //set ir transmitter pin 

 
//*****  HTML code which will take the ssid and password  ******//
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
    <title>HTML Form to Input Data</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        html {
            font-family: Times New Roman;
            display: inline-block;
            text-align: center;
        }

        h2 {
            font-size: 3.0rem;
            color: #535f60;
        }

        body {
            background-image: linear-gradient(to right, red, orange, yellow, green, blue, indigo, violet);
            font-family: 'Times New Roman', Times, serif;
            font-size: 15px;
            line-height: 2;
            color: rgb(40, 37, 35);
        }

        button {
            margin-left: 90px;
        }

        h1 {
            margin-left: 550px;
        }

        .center-align {
            margin-left: 400px;
            margin-bottom: 3px;
        }

        input {
            padding: 4px 4px;
            margin: 0px 0;
            display: inline-block;
            border: 1px solid rgb(245, 190, 190);
            border-radius: 4px;
            box-sizing: border-box;
        }
    </style>
</head>

<body>
    <h2 style=text-align:center>Enter your SSID and Password</h2>
    <form action="/get">
        <label for="ssid"><b>SSID:</b></label>
        %BUTTONPLACEHOLDER%
        <br>
        <br>
        <label for="pwd"><b>PASSWORD :</b></labels>
        <input type="password" placeholder="PASSWORD" id="pwd" name="pwd" required>
        <input type="checkbox" onclick="myFunction()">Show Password
        <script>
            function myFunction() {
                var x = document.getElementById("pwd");
                if (x.type === "password") {
                    x.type = "text";
                } else {
                    x.type = "password";
                }
            }
        </script>


        <hr width="50%">
        <button type="submit">submit</button>
    </form>

</body>

</html>)rawliteral";

//**** this function will replace BUTTONPLACEHOLDER to options of nearby scanned network ****//
String processor(const String& var){
  if(var == "BUTTONPLACEHOLDER"){
    String opts = "";
    opts += "<select name=\"ssid\" id=\"ssid\">" ;
    for(int i = 0; i < number; i++){
      opts += "<option required> " + wifi_net[i] +"</option>";
    }
    opts += "</select>"; 
    return opts;
  }
  return String();
}

//****function for searching nearby networks ****//
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");      //send http status code 404
}

//**** creating the string for reading from the eeprom ****//

String read_file(fs::FS &fs, const char * path){
  Serial.print("");
  Serial.printf("Reading file: %s\r\n", path);      //print "reading file:'name of file'" on serial monitor
  File file = fs.open(path, "r");                   //open the file
  if(!file || file.isDirectory()){                  //if file is not opened then execute this loop
    Serial.println("Empty file/Failed to open file");   //print "empty file/failed to open file"
    return String();                //returns the string                 
  }
  Serial.println("- read from file:");
  String fileContent;           //declare a string variable named 'filecontent'
  while(file.available()){      //execute this loop until file is available
    fileContent+=String((char)file.read());
  }
  file.close();               //close the file
  Serial.println(fileContent);      //print the value of 'filecontent'
  return fileContent;               //return the value of 'filecontent'
}


//**** creating the string for reading from the eeprom ****//
void write_file(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);    //print "writing file:'name of file'"
  File file = fs.open(path, "w");
  if(!file){                        //if file is not available the execute this loop
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("SUCCESS in writing file");
  } else {
    Serial.println("FAILED to write file");
  }
  file.close();             //close the file
}


void setup() {
  Serial.begin(9600);                  //Initialize Serial Monitor and set the Baud Rate of 9600
  if(!SPIFFS.begin()){
      Serial.println("An Error has occurred while mounting SPIFFS");
      return;
    }

  // Connect to Wi-Fi
  setup_wifi();
  number = WiFi.scanNetworks();                          //TO STORE THE NUMBER OF NEARBY NETWORKS IN 'number'               
  for (int i = 0 ; i < number; i++) {                    
    wifi_net[i] = WiFi.SSID(i).c_str();              //STORES THE NAME OF ith NETWORK IN ith POSITION OF 'network' VARIABLE IN STRING FORM  
  }

   // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {

    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(input_ssid) && request->hasParam(input_pwd)) {
      new_ssid = request->getParam(input_ssid)->value();
      new_pwd = request->getParam(input_pwd)->value();
      write_file(SPIFFS, "/ssid.txt", new_ssid.c_str());
      write_file(SPIFFS, "/pwd.txt", new_pwd.c_str());
      Serial.println("    New ssid is :");
      Serial.print(new_ssid);             //print the value of 'new_ssid'
      Serial.println("New password is :");  
      Serial.print(new_pwd);              //print the value of 'new_pwd'
      
    }
    else {                  //if get request does not have the value of both parameter then execute this loop
      new_ssid = "No message sent";
      new_pwd = "No message sent";
      Serial.println("ssid : ");               //print "ssid : "
      Serial.println(new_ssid);                //print the value of 'new_ssid' on 
      Serial.println("pwd :");                 //print "pwd : "
      Serial.println(new_pwd);                 //print the value of 'new_pwd'
    }

    request->send(200, "text/html", "<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);  //if the web page was not found then call the notfound() function
  // Start server
  server.begin();      
  
  client.setServer(mqtt_server, 1883); //Provides MQTT server name and Port number to ESP client 
  client.setCallback(callback);        //Calling the callback function 
  irsend.begin();                      //Initialize IR Transmitter for transmitting IR signal 
}

void loop() {
if(WiFi.status() == WL_CONNECTED){
  if (!client.connected())            // Check if ESP client is connected with mqtt Server
    {           
      reconnect();                      //If the ESP client is not connected with mqtt server then call the reconnect function
    }
    client.loop();
  delay(1000);        //delay of 1 seconds
 }                         
}

//**** create a function to setup the wifi of esp ****//
void setup_wifi(){
 String myssid = read_file(SPIFFS, "/ssid.txt");      //the content of 'ssid.txt' file is given to 'myssid'
  Serial.print("SSID : ");    //print "ssid : "
  Serial.println(myssid);     //print the value of myssid
  
  String mypwd = read_file(SPIFFS, "/pwd.txt");
  Serial.print("PWD : ");
  Serial.println(mypwd);
df:  while(WiFi.status() != WL_CONNECTED){     //this loop will execute until esp not connect
      Serial.println("");
      Serial.print("Configuring access point : ");
      for(int i = 0; i < 1; i++){
       WiFi.begin(old_ssid,old_pwd);        //esp starts connecting with old_ssid and old password
       Serial.print(old_ssid);
       for(int i = 0; i < 20; i++){
         Serial.print(".");
         if(WiFi.status() == WL_CONNECTED){   //if esp is connected with wifi then execute it.
          Serial.println("");
          Serial.print("Connected with : ");
          Serial.print(old_ssid);         //print old ssid on serial monitor
          Serial.println("");             //blank line
          Serial.print("IP Address: ");   //print "ip address: "
          Serial.print(WiFi.localIP());   //print ip address of connected wifi
          goto df;     //go to df line
       }
       delay(500);   //delay of 500 miliseconds
       }
     }
      for(int j = 0; j < 1 ;j++){
        Serial.println("");
        Serial.print("Configuring access point : ");
        WiFi.begin(myssid,mypwd);  //esp start to connect with myssid
        Serial.print(myssid);
        for(int j = 0; j < 20 ;j++){
          Serial.print(".");
          if(WiFi.status() == WL_CONNECTED){ //if esp is connected with wifi then execute it.
            Serial.println("");
            Serial.print("Connected with : ");
            Serial.print(myssid);          //print myssid on serial monitor 
            Serial.println("");            //blank line
            Serial.print("IP Address: ");  //print "ip address: "
            Serial.print(WiFi.localIP());  //print ip address of connected wifi
            goto df;         //go to df
          }
        delay(500);         //delay of 500 miliseconds
        } 
    }
   delay(1000);     
  }
}


//Defines a function for ESP Client to subscribe to a given topic
void subscriber()
{
  client.subscribe("SMIT/mqtt");       //ESP client will subscribe to a given topic
}


// Defines a function to Recieve Messages Which are published on the given topic 
void callback(char* topic,byte* message, unsigned int length) 
{
  Serial.print("Message arrived ");   //Prints "Message arrived [" on Serial Monitor
  Serial.print(topic);                 //Prints Topic name on Serial Monitor
  Serial.print("] ");                  //Print "]" on Serial Monitor           
  for (int i = 0; i < length; i++) 
  {
    Serial.print((char)message[i]);   //Prints the message on Serial monitor which is recieved by ESP client
    irsend.sendNEC((char)message[i]); //Sends the IR Signal from IR Transmitter
  }
  Serial.println();
}

//Declare a function to connect with mqtt client 
//And if not connected then try to connect in every 5 seconds
void reconnect()
{
  int counter = 0;      //Sets the counter value zero 
  while (!client.connected()) //If the ESP client is not connected with mqtt server then this loop will execute until the connection establishes
  {
    if (counter==5)    //if the counter value equals to 5 then this loop will be executed
    {
      ESP.restart(); //Restarts esp module
    }
    counter+=1;       //Increments Counter value 
    Serial.print("try to connect");    //Prints "Attempting MQTT connection..." on Serial monitor
    // Attempting to connect ESP with mqtt server
   
    if (client.connect("7cf38fe7-7a58-4ce0-8dc7-2e0377635c8d","RENESYS" , "123456")) //check if client is connected 
    {
      Serial.println("");
      Serial.print("");      //If the ESP is connected  with mqtt server then prints "connected" on Serial Monitor
      subscriber();                     //Calling a subscriber function 
    } 
    else                                //if client not connected then this loop will be executed
    {
      Serial.print("failed, rc=");      
      Serial.print(client.state());    //Prints the status of ESP and mqtt server connection
      Serial.println("trying again in next 5 sec");  //Prints "try again in 5 seconds" on Serial Monitor
      // Waits for 5 seconds before retrying
      delay(5000);    //Provides delay of 5 seconds
    }
  }
}
