/*-------------------------Code 06/06/2020 -------------------------*/
#include <WiFi.h>
#include <PubSubClient.h> 
#include <HttpFOTA.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "time.h"
#include <RTClib.h>
#include <Adafruit_MCP3008.h>
#include <Machine.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <TridentTD_EasyFreeRTOS32.h>

/*------------GPIO---------------*/
#define LED2 13 //CS2# 
#define RST 12

/*--------------Config Variable---------------------*/
const char* wifi_pwd;
const char* wifi_ssid;
const char* mqtt_server;
int mqttPort;
const char* mqttUser;
const char* mqttPassword;
const char* clientId;
int otatimeout; // OTA Timeout limit 30 sec
const char* sendtopic; // Machine send data Topic
const char* gtopic; //OTA Group Topic 
const char* ctopic; //OTA Sub Companny Topic
const char* stopic; //OTA Self Machine Topic
const char* ackota; //OTA Acknowledge use for Machine confirm received OTA
const char* getconf; // //Topic of this machine subscribe (or listen) for receive command from web socket command getcf(get config)
const char* sendconf; // Topic for Machine send config back to(publish to) web server (use web socket)
const char* dbreply;//Topic for check db active  Server Reply OK if Insert data already  ADD BY RJK 
/*--------------Config Variable---------------------*/

String eachline;// String  Object receiving each line in file conf.txt

char* certssl; // SSL Certification for download firmware OTA Load from certi.txt
String  Certs = "";// String Object for concatination certification from file certi.txt


/* String Object array for keep config each line in file conf.txt in SD Card */
String Line[16];
File iotfmx; //create object root from Class File use SD Card use write new config to SD Card
File root; //create object root from Class File use SD Card read from SD Card and assign to config variable

/*----------TIME NTP Server-------------*/
const char* ntpServer = "time.uni.net.th";
const long  gmtOffset_sec = 7 * 3600;
const int   daylightOffset_sec = 0;

volatile char datareceivedmqtt[2]; // receive OK from DBserv

#define Qos  1 //Quality of Service
uint64_t chipid;  //Declaration for storage ChipID
unsigned long tnow = 0; //Init time for plus current time
unsigned long  startota = 0; //Initial time for counter 
int ota_t; //time difference
int prog; //percent download progress
  
WiFiClient FMXClient;
PubSubClient client(FMXClient);


Machine mac;
RTC_DS3231 RTC;
Adafruit_MCP3008 adc;

/*-----------------------Machine-------------------------*/
#define IO_1 mac.READ_DATASW(sw1)
#define IO_2 mac.READ_DATASW(sw2)
#define IO_3 mac.READ_DATASW(sw3)
#define IO_4 mac.READ_DATASW(sw4)
#define IO_5 mac.READ_DATASW(sw5)
#define IO_6 mac.READ_DATASW(sw6)
#define IO_7 mac.READ_DATASW(sw7)
#define IO_8 mac.READ_DATASW(sw8)
#define writeaddr_eeprom1 32001
#define writeaddr_eeprom2 32002
#define FILE_COUNT_INHISTORYSD 31250 //1GB:1,000,000KB
#define addrsize 128

#define time_limitwifi  5
char DATA_PACKHEADHIS[16];
char DATA_PACKHEAD[21];
char DATA_PACKDATE[6];
char DATA_PACKIO[2];
char Chipid_buf[12];
char filnamechar[12];
char buf_date[12];
char buf_io[4];
char v_fw[4];

unsigned char DATA_PACKPWM1[4];
unsigned char DATA_PACKPWM2[4];
unsigned char DATA_PACKRELAY[1];
unsigned char DATA_PACKPWM3[4];
unsigned char DATA_PACKPWM4[4];
unsigned char DATA_PACKAD1[4];
unsigned char DATA_PACKAD2[4];
unsigned char DATA_PACKAD3[4];
unsigned char DATA_PACKGPS[6];
unsigned char DATA_PACKM1[8];
unsigned char DATA_PACKM2[3];
unsigned char DATA_PACKM3[3];

const char* datasaveram;
const char* datamqtt;
const char* datamqttinsdcard;
const char* filenamesavesd;

unsigned int data_IO;
unsigned int write_addeeprom;
int countfileinsd = 0;
int buf_head = 0;
int bufwrite_eeprom1, bufwrite_eeprom2;
int read_packADD;
int time_outwifi = 0;
int checksettime = 0;
int filename = 0;
int checksendmqtt = 0;

//uint64_t chipid;

String sDate;
String filenames;
String datainfilesd;
String Headerhistory = "";
String buffilenamedel;

long time_out = 0;
long time_limit = 100;

typedef enum {
    Runnning_e = 0x01,
   Fota_e  
}SysState;

/*-----------------Firmware Source Download------------------------*/
char url[100];//Url firmware download 
char md5_1[50];// md5 checksum firmware filename .bin

SysState state = Runnning_e;  //Create an instance state



void ChipID(){//Show Chip ID
  chipid=ESP.getEfuseMac();//The chip ID is  MAC address(length: 6 bytes).
  Serial.printf("Machine Board Chip ID = %04X",(uint16_t)(chipid>>32));//print High 2 bytes
  Serial.printf("%08X\n",(uint32_t)chipid);//print Low 4bytes.
 
}

void IOTCallback(char *topic, byte *payload, unsigned int length){
  Serial.println(topic);//Print topic received
  Serial.println((char*)payload);//print payload (or message) in topic received
  if((strncmp(gtopic, topic, strlen(gtopic)) == 0)||(strncmp(ctopic, topic, strlen(ctopic)) == 0)||(strncmp(stopic, topic, strlen(stopic)) == 0)){
    memset(url, 0, 100);
    memset(md5_1, 0, 50);
    char *tmp = strstr((char *)payload, "url:");//Query url: in payload(message received) and assign to pointer tmp
    char *tmp1 = strstr((char *)payload, ",");//Query , in payload(message received) and assign to pointer tmp1
    memcpy(url, tmp+strlen("url:"), tmp1-(tmp+strlen("url:")));
    
    char *tmp2 = strstr((char *)payload, "md5:");//Query md5: in payload(message received) and assign to pointer tmp2
    memcpy(md5_1, tmp2+strlen("md5:"), length-(tmp2+strlen("md5:")-(char *)&payload[0]));

    Serial.printf("started fota url: %s\n", url);
    Serial.printf("started fota md5: %s\n", md5_1);
    client.publish(ackota,stopic);// Publish message OTA TOPIC if acknowledge OTA backward to web server 
    state = Fota_e; //Start state Firmware OTA
   }
   /*  Check topic received and payload (message) is equal command for getconfig (getcf)  */
  if(strncmp(getconf, topic, strlen(getconf)) == 0){//Cmd from php or websocket getcf (Get config)
    if((char)payload[0] == 'g' && (char)payload[1] == 'e' && (char)payload[2] == 't' && (char)payload[3] == 'c' && (char)payload[4] == 'f'){
        sendconfig();
    }
    if((char)payload[0] == 'g' && (char)payload[1] == 'e' && (char)payload[2] == 't' && (char)payload[3] == 'i' && (char)payload[4] == 'd'){
        sendId();
    }
    if((char)payload[0] == 'c' && (char)payload[1] == 'f'){//Change config by https get to server iotfmx.com
        ChangeConfig();
     }
    if((char)payload[0] == 'c' && (char)payload[1] == 'f' && (char)payload[2] == 'b' ){//Change config by https get to server iotfmx.com
        ChangeConfig_Restart();
      } 
    if((char)payload[0] == 'c' && (char)payload[1] == 'c' && (char)payload[2] == 'f'){//Check current config
       currentConfig(SD,"/conf.txt");
     }
    if((char)payload[0] == 'c' && (char)payload[1] == 's'){// Check Cert SSL
       String rep = CheckCertSSL();
       client.publish(sendconf,rep.c_str(),Qos);
     }
   }
   
   
   if (strncmp(dbreply, topic, strlen(dbreply)) == 0) {
     Serial.println((char*)payload);
     
    for (int r = 0; r < length; r++) {
      //Serial.write((char)payload[r]);
        datareceivedmqtt[r] = (char)payload[r];
      //         delay(20);
    }

    Serial.println();

  }

 }
int mqttconnect() {
  /* Loop until reconnected */
  while (1) {
    /* connect now */
    if (client.connect(clientId, mqttUser, mqttPassword)) {
      // if (client.connect(clientId,mqttUser,mqttPassword)) {
      Serial.println("Mqtt....connected");
      
      /* subscribe topic */
       client.subscribe(stopic,Qos);
       client.subscribe(gtopic,Qos);
        client.subscribe(ctopic,Qos);
        client.subscribe(getconf,Qos);
        client.subscribe(dbreply,Qos);
        
        
      }
    return 1;
  }
}

void wifi_setup()
{ 
  
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid,wifi_pwd); //assign wifi ssid , pass

  while (WiFi.status() != WL_CONNECTED) {//Loop is not connect until connected exit loop
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Connect ok");
}

String DateTimeNOW(){
   DateTime now = RTC.now();
   String DMY = String(now.day())+"/"+String(now.month())+"/"+String(now.year())+" "+String(now.hour())+":"+String(now.minute())+":"+String(now.second());
   return DMY;
}


void data_time()
{
  DateTime now = RTC.now();
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return;
  }
  //  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  //    Serial.printf("NTP TIME : %02d/%02d/%04d ",timeinfo.tm_mday,timeinfo.tm_mon + 1,timeinfo.tm_year + 1900);
  //    Serial.printf("%02d:%02d:%02d \r\n",timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);
  if (checksettime == 0 ) {
    RTC.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
    checksettime = 1;
  }
  else
  {
    if (timeinfo.tm_wday == 0 && timeinfo.tm_hour == 0 && timeinfo.tm_min == 0 && timeinfo.tm_sec <= 60)
    {
      RTC.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
      Serial.println("Update Time Success");
    }
  }
  Serial.printf("%02d", now.day());
  Serial.print('/');
  Serial.printf("%02d", now.month());
  Serial.print('/');
  Serial.printf("%02d", now.year());
  Serial.print(' ');
  Serial.printf("%02d", now.hour());
  Serial.print(':');
  Serial.printf("%02d", now.minute());
  Serial.print(':');
  Serial.printf("%02d", now.second());
  Serial.println();

  //delay(1000);
}


void sendsdcardtomqtt() {
  const char *filenameinsd;
  String buffilename;
  sdbegin();
  /*------------------- List Filename In SDCARD ---------------------*/
  filenameinsd = listDir(SD, "/history", 0);
  buffilename = filenameinsd;
  if (filenameinsd != "0") {
    readFileinSD(SD, filenameinsd);
    packdata_HEADSDCARD();
    delay(100);
    datainfilesd = Headerhistory + datainfilesd;
    datamqttinsdcard = datainfilesd.c_str();
    client.publish(sendtopic,datamqttinsdcard);
    client.setCallback(IOTCallback);
    if((datareceivedmqtt[0] == 'O')&&(datareceivedmqtt[1] == 'K')){ 
      Serial.println("***************** Send File In SDCARD OK *****************");
      const char *delfilename;
      delfilename = buffilename.c_str();
      datainfilesd = "";
      deleteFile(SD, delfilename);
    }
  } else
  {
    Serial.println("**************** No File In SDCARD *****************");
  }
}

/*----------------Declare Tasks---------------------*/
TridentOS   task_CheckWiFiMqtt,
            task_PackdataSend;
            

/*----------------Declare Tasks---------------------*/

/*--------------Var share task------------------*/
String xSdatamqtt;

void setup() {
  pinMode(LED2,OUTPUT);
  digitalWrite(LED2,HIGH);//HIGH Turn off LOW on
  Serial.begin(115200);
  v_fw[0] = 0x30;
  v_fw[1] = 0x31;
  v_fw[2] = 0x31;
  v_fw[3] = 0x30;
  sdbegin();
  assignConfig(SD,"/conf.txt");
  Serial.println(F("Connected SD Card ok."));
    Serial.println(F("Load config from SD card file:conf.txt"));
  ChipID();//Show Chip ID (IMEI)
  certssl = (char*)readcert(SD,"/certi.txt").c_str(); //Load certificate and convert to char* datatype
  //Serial.println(certssl);
  
  wifi_setup();
  delay(100);
  data_time();
  delay(100);
  Wire.begin();
  delay(100);
  mac.begin();
  delay(100);  
  RTC.begin();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(100);
  //Serial.println("NOW: "+DateTimeNOW());
  client.setServer(mqtt_server,mqttPort); 
  client.setCallback(IOTCallback);
   task_CheckWiFiMqtt.start(taskCheckWiFi_and_Mqtt);//Default StackDepth=2048
   task_PackdataSend.start(taskPackdatasend,NULL,61440); 
  
}
void loop() {
   /*-------------------FOR OTA--------------------------*/
   function_ota();

}
