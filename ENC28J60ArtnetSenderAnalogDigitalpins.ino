

/*
  This SCRIPT allows you to use arduino with the ENC28J60 ethernet shield and send dmx artnet data.
  Up to you to use logics for channels as you want.

  Orginal Code from:
  https://vvvv.org/contribution/artnet-arduino-set-v3.1-0
  (c)Christoph Guillermet
  http://www.le-chat-noir-numerique.fr
  karistouf@yahoo.fr
  Modified by Johannes Regneri 2020
  https://github.com/JohannesRegneri

  PinMap:

  ENC28J60_____________________________________________________________
  5V      -   5V  //the 3,3V doesnt work for my Board
  GND     -   GND
  INT     -   (D2)
  CLK     -   /
  SO      -   D12
  WOL     -   /
  SCK     -   D13
  SI      -   D11
  RST     -   (RST)
  CS      -   10 or 8 //defined in UIPEthernet
  _____________________________________________________________________

  Analog Input at A0 and 5V GND
  
  Pushbuttona on D3-D6 and GND

*/

#include <SPI.h>         // needed for Arduino versions later than 0018
#include <UIPEthernet.h> // Used for Ethernet https://github.com/UIPEthernet/UIPEthernet

// **** ETHERNET SETTING ****
IPAddress ip(2, 0, 0, 200);
byte destination_Ip[] = {   255, 255, 255, 255 };    // the ip to send data, 255,255,255,255 is broadcast sending
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x78, 0xEE  };

//MAC and IP of the ethernet shield
//MAC adress of the ethershield is stamped down the shield
//to translate it from hexa decimal to decimal, use: http://www.frankdevelopper.com/outils/convertisseur.php

// art net parameters
unsigned int localPort = 6454;      // artnet UDP port is by default 6454
const int DMX_Universe = 2; //universe is from 0 to 15, subnet is not used
const int number_of_channels = 16; //512 for 512 channels, MAX=512

//ART-NET variables
char ArtNetHead[8] = "Art-Net";
const int art_net_header_size = 17;

short OpOutput = 0x5000 ; //output

byte buffer_dmx[number_of_channels]; //buffer used for DMX data
byte neu_buffer_dmx[number_of_channels];

EthernetUDP Udp;

//Artnet PACKET
byte  ArtDmxBuffer[(art_net_header_size + number_of_channels) + 8 + 1];

//TIMER
long Timerpremillis = 0;
long interval = 1000; //send artnet Package every 2 seconds


void setup() {

  //initialise artnet header
  construct_arnet_packet();
  // démarrage ethernet et serveur UDP
  Ethernet.begin(mac, ip);
  Udp.begin(localPort);

  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);

}

void loop() {
  check_arduino_analog_inputs();
  check_arduino_digital_inputs();
  construct_arnet_packet();
  checkchangeandsend_packet();
  send_packet_timer();
}



void check_arduino_analog_inputs()
{
  //data from arduino aquisition

  int temp_val = 0;
  for (int i = 0; i < 1; i++) //reads the 6 analogic inputs and set the data from 1023 steps to 255 steps (dmx)
  {
    temp_val = analogRead(i);
    buffer_dmx[i] = byte(temp_val / 4);
  }
}


void check_arduino_digital_inputs()
{
  // read the state of the pushbutton value:
  int button1State = digitalRead(3);
  int button2State = digitalRead(4);
  int button3State = digitalRead(5);
  int button4State = digitalRead(6);

  if (button1State == LOW) {
    buffer_dmx[10] = byte(255); //Channel 89
  }
  else {
    buffer_dmx[10] = byte(0); //Channel 89
  }
  if (button2State == LOW) {
    buffer_dmx[11] = byte(255); //Channel 90
  }
  else {
    buffer_dmx[11] = byte(0); //Channel 90
  }
  if (button3State == LOW) {
    buffer_dmx[12] = byte(255); //Channel 91
  }
  else {
    buffer_dmx[12] = byte(0); //Channel 91
  }
  if (button4State == LOW) {
    buffer_dmx[13] = byte(255); //Channel 92
  }
  else {
    buffer_dmx[13] = byte(0); //Channel 92
  }

}


void construct_arnet_packet()
{
  //preparation pour tests
  for (int i = 0; i < 7; i++)
  {
    ArtDmxBuffer[i] = ArtNetHead[i];
  }

  //Operator code low byte first
  ArtDmxBuffer[8] = OpOutput;
  ArtDmxBuffer[9] = OpOutput >> 8;
  //protocole
  ArtDmxBuffer[10] = 0;
  ArtDmxBuffer[11] = 14;
  //sequence
  ArtDmxBuffer[12] = 0;
  //physical
  ArtDmxBuffer[13] = 0;
  // universe
  ArtDmxBuffer[14] = DMX_Universe; //or 0
  ArtDmxBuffer[15] = DMX_Universe >> 8;
  //data length
  ArtDmxBuffer[16] = number_of_channels >> 8;
  ArtDmxBuffer[17] = number_of_channels;

  for (int t = 0; t < number_of_channels; t++)
  {
    ArtDmxBuffer[t + art_net_header_size + 1] = buffer_dmx[t];
  }

}


void checkchangeandsend_packet()
{
  for (int s = 0; s < number_of_channels; s++)
  {
    //Compare last Packet with current for sending only changed Data
    if (neu_buffer_dmx[s] !=  buffer_dmx[s]) {
      Udp.beginPacket(destination_Ip, localPort);
      Udp.write(ArtDmxBuffer, (art_net_header_size + number_of_channels + 1)); // was Udp.sendPacket
      Udp.endPacket();
      delay(25);
    }
    //else {
    //}
    //    neu_buffer_dmx[s] = buffer_dmx[s]; //uncomment to only send data when values Changed
  }
}

void send_packet_timer()
{
  //send every intervall seconds
  unsigned long TimercurrentMillis = millis();
  if (TimercurrentMillis - Timerpremillis > interval) {
    Timerpremillis = TimercurrentMillis;
    
    Udp.beginPacket(destination_Ip, localPort);
    Udp.write(ArtDmxBuffer, (art_net_header_size + number_of_channels + 1)); // was Udp.sendPacket
    Udp.endPacket();
  }
}


