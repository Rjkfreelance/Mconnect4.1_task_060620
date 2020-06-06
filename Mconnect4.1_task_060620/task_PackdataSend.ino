void taskPackdatasend(void*){
      
      VOID SETUP()
      {  
         chipid = ESP.getEfuseMac();
         data_time();
      }
          
 VOID LOOP()
 {   
    
    sprintf(Chipid_buf, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)(chipid));
    DELAY(50);

           

  /*-------------------Start PackData--------------------------*/
  packdata_HEAD();
  packdata_DATE();
  packdata_IO();
  packdata_PWM();
  packdata_RELAY();
  packdata_PWM2();
  packdata_AD();
  packdata_GPS();
  packdata_M1();
  packdata_M2();
  packdata_M3();
  DELAY(100);
  bufwrite_eeprom1 = mac.readAddress(writeaddr_eeprom1);
  bufwrite_eeprom2 = mac.readAddress(writeaddr_eeprom2);
  write_addeeprom = (bufwrite_eeprom2 << 8) + bufwrite_eeprom1;
  String sText; 

  sText += sDate;
  for (int i = 0; i < sizeof(buf_io); i++)
  {
    sText += buf_io[i];
  }
  sText += ";" ;
  for (int i = 0; i < sizeof(DATA_PACKPWM1); i++)
  {
    sText += DATA_PACKPWM1[i];
  }
  sText += ";" ;
  for (int i = 0; i < sizeof(DATA_PACKPWM2); i++)
  {
    sText += DATA_PACKPWM2[i];
  }
  sText += ";" ;
  for (int i = 0; i < sizeof(DATA_PACKRELAY); i++)
  {
    sText += DATA_PACKRELAY[i];
  }
  sText += ";" ;
  for (int i = 0; i < sizeof(DATA_PACKPWM3); i++)
  {
    sText += DATA_PACKPWM3[i];
  }
  sText += ";" ;
  for (int i = 0; i < sizeof(DATA_PACKPWM4); i++)
  {
    sText += DATA_PACKPWM4[i];
  }
  sText += ";" ;
  for (int i = 0; i < sizeof(DATA_PACKAD1); i++)
  {
    sText += DATA_PACKAD1[i];
  }
  sText += ";" ;
  for (int i = 0; i < sizeof(DATA_PACKAD2); i++)
  {
    sText += DATA_PACKAD2[i];
  }
  sText += ";" ;
  for (int i = 0; i < sizeof(DATA_PACKAD3); i++)
  {
    sText += DATA_PACKAD3[i];
  }
  sText += ";" ;
  for (int i = 0; i < sizeof(DATA_PACKGPS); i++)
  {
    sText += DATA_PACKGPS[i];
  }
  sText += ";" ;
  for (int i = 0; i < sizeof(DATA_PACKM1); i++)
  {
    sText += DATA_PACKM1[i];
  }
  sText += ";" ;
  for (int i = 0; i < sizeof(DATA_PACKM2); i++)
  {
    sText += DATA_PACKM2[i];
  }
  sText += ";" ;
  for (int i = 0; i < sizeof(DATA_PACKM3); i++)
  {
    sText += DATA_PACKM3[i];
  }
  sText += ";" ;

  datasaveram = sText.c_str();
  xSdatamqtt = sText;
  DELAY(50);
  Serial.print("Data For PackSendMQTT : ");
  Serial.println(sText);
  Serial.print("EEPROM ADDR : ");
  Serial.println(write_addeeprom);
  DELAY(50);   
  if (write_addeeprom >= 32000) //32000  //ถ้าเขียนถึง address ที่ 32000 ให้เอาข้อมูลทั้งหมดใส่ใน file sdcard
   {
    Serial.println("Please wait for read RAM To SDCARD");
    String datab;
    const char * datasavesdcard;
    datab = mac.read_all();
    
    datasavesdcard = datab.c_str();
    listcountfileindir(SD, "/history");
    DELAY(100);
    if (countfileinsd >= FILE_COUNT_INHISTORYSD) { //FILE_COUNT_INHISTORYSD
      const char * delfile;
      delfile = buffilenamedel.c_str();
      deleteFile(SD, delfile);
      countfileinsd = 0;
    }
    String a = "/history/" + filenames + ".txt";
    File file = SD.open("/history");
    if (!file)
    {
      Serial.println("Create Directory");
      SD.mkdir("/history");
    }
    filenamesavesd = a.c_str() ;
    //    writeFile(SD, filenamesavesd , datasavesdcard);
    if (!writeFile(SD, filenamesavesd , datasavesdcard)) {
      Serial.println("******** Write DATA TO SDCARD Success ********");
      //Create file in sd card success update address eeprom = 0
      mac.writeAddress(writeaddr_eeprom1, 0); 
      mac.writeAddress(writeaddr_eeprom2, 0);
      write_addeeprom = 0;
      filename++;
    } else {
      Serial.println("Can't Save SD Card To RAM");
    }
  }
  
  for (int i = 0; i < addrsize; i++)
  {
    mac.writeAddress(write_addeeprom, datasaveram[i]);
    write_addeeprom++;
  }

  mac.writeAddress(writeaddr_eeprom1, write_addeeprom & 0xFF); //ระบุ ADDRESS
  mac.writeAddress(writeaddr_eeprom2, (write_addeeprom >> 8) & 0xFF);  
 
  int buf_lasteeporm = write_addeeprom;
   if (client.connected())
    {
      
     if (write_addeeprom > 0)
      {
        String datahistory;
        String datamakemqtt;
        mac.Scan_data_sstv(write_addeeprom);
        datamakemqtt = mac.make_send_string(xSdatamqtt);
        datamqtt = datamakemqtt.c_str();
        
        client.publish(sendtopic,datamqtt);
        if((datareceivedmqtt[0] == 'O')&&(datareceivedmqtt[1] == 'K')){ 
          Serial.println("*********Send Mqtt Data Realtime Success********");
          Serial.print("write_addr : ");
          Serial.println(write_addeeprom);
          Serial.print("Response : ");
          Serial.write(datareceivedmqtt[0]);
          Serial.write(datareceivedmqtt[1]);
          Serial.println();
          mac.Check_senddata_fram(write_addeeprom);
          datareceivedmqtt[0] = '\0';
          datareceivedmqtt[1] = '\0';
          //checksendmqtt = 1;
         
        } else {
          
          Serial.println("******** Can't Send Server no reply OK (can't insert to database) ********");
        }
        DELAY(100);
        
        if (checksendmqtt == 1) {
          sendsdcardtomqtt();
          checksendmqtt = 0;
        }
        client.loop();
      }
    }

  
  DELAY(1000);   
 }


}
