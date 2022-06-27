//Installing the Libraries which will be used in our program 
#include <IRremoteESP8266.h> 
#include <IRsend.h>  
#include <ESP8266WiFi.h>      //ESP8266 WiFi library provides ESP8266 specific WiFi routines and we are calling it to connect to the network
#include<PubSubClient.h>      //PubSubClient library allows a client to publish/subscribe messaging with a MQTT support server



//Setting WiFi Credentials which will be used by ESP to connect with WiFi network 
const char* ssid1 = "Redmi Note 7 Pro";    //Provide ssid to which ESP will try to connect 
const char* password1 = "123451234";       //Provide password of the ssid network
const char* ssid2 = "M40";
const char* password2 = "1234678910";

//Providing MQTT server name 
const char* mqtt_server = "broker.mqtt-dashboard.com";
            
//Assign the MQTT as a client to ESP8266
WiFiClient espClient;
PubSubClient client(espClient);

const uint16_t kIrLed = D2;            //Define pin number to which IR transmitter is connected
IRsend irsend(kIrLed);                 //Sets IR transmitting pin as a IR Transmitter to Send the IR signal
 
void setup() {
  Serial.begin(9600);                  //Initialize Serial Monitor and set the Baud Rate of 9600
  client.setServer(mqtt_server, 1883); //Provides MQTT server name and Port number to ESP client 
  client.setCallback(callback);        //Calling the callback function 
  irsend.begin();                      //Initialize IR Transmitter for transmitting IR signal 
}

void loop() {
  if(WiFi.status() != WL_CONNECTED)     //Check if WiFi is not connected with ESP then Execute the instrunction which is inside loop 
  {
    myNetwork();                        // if not connected with wifi then call the myNetwork function
  }
  if(WiFi.status() == WL_CONNECTED)     //Check if WiFi is connected with ESP then Execute the instruction which is inside loop
  {
    Serial.println("Connected");        //Print "Connected" on Serial Monitor 
    if (!client.connected())            // Check if ESP client is connected with mqtt Server
    {           
      reconnect();                      //If the ESP client is not connected with mqtt server then call the reconnect function
    }
    client.loop();
  }
  delay(1000);                          
}

//Defining a function for continuosly check for wifi network
//And If the available network matches with our network then ESP connects with that network 
void myNetwork() 
{
  int flag = 1;
  Serial.println("Searching Wifi......");     //Prints "Searching WiFi...." on Serial Monitor
  int network = WiFi.scanNetworks();          //Scans the nearby available WiFi networks
  for (int i = 0 ; i < network; i++)          
  {
    switch (flag)
    {
      case 1:  
        flag =2;
        if (WiFi.SSID(i) == ssid1)           //If the available WiFi ssid match with given ssid then execute the block
        {
          WiFi.begin(ssid1, password1);     //It is used to connect ESP with given WiFi network
          Serial.println("/n Wifi Found");  //Prints "WiFi found" on Serial Monitor
          delay(2000);                      //Provides dalay of 2 seconds
          Serial.println("Connecting with Kirtan Please Wait ");    //Prints "Connecting with smit Please Wait " on Serial Monitor  
          delay(8000);                      //Provides delay of 8 seconds
          break;
        }
        case 2:
           flag =1;
           if (WiFi.SSID(i) == ssid2)         //If the available WiFi ssid match with given ssid then execute the if block
           {
              WiFi.begin(ssid2, password2);   //It is used to connect ESP with given WiFi network
              Serial.println("/n Wifi Found");//Prints "WiFi found" on Serial Monitor
              delay(2000);                    //Provides delay of 2 seconds
              Serial.println("Connecting with Smit Please Wait ");
              delay(8000);                    //Provides delay of 8 seconds
              break;
           }
    }
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
  Serial.print("Message arrived [");   //Prints "Message arrived [" on Serial Monitor
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
    Serial.print("Attempting MQTT connection...");    //Prints "Attempting MQTT connection..." on Serial monitor
    // Attempting to connect ESP with mqtt server
   
    if (client.connect("7cf38fe7-7a58-4ce0-8dc7-2e0377635c8d","RENESYS" , "123456")) //check if client is connected 
    {
      Serial.println("connected");      //If the ESP is connected  with mqtt server then prints "connected" on Serial Monitor
      subscriber();                     //Calling a subscriber function 
    } 
    else                                //if client not connected then this loop will be executed
    {
      Serial.print("failed, rc=");      
      Serial.print(client.state());    //Prints the status of ESP and mqtt server connection
      Serial.println(" try again in 5 seconds");  //Prints "try again in 5 seconds" on Serial Monitor
      // Waits for 5 seconds before retrying
      delay(5000);    //Provides delay of 5 seconds
    }
  }
}
