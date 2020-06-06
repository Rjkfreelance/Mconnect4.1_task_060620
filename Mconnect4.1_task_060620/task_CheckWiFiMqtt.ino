void taskCheckWiFi_and_Mqtt(void*){
  int countLoss = 0;
  VOID SETUP(){      
     
   }

  VOID LOOP(){    // Infinite Loop
   if(WiFi.status() != WL_CONNECTED){
       digitalWrite(LED2,LOW);// เมื่อ WiFi หลุดการเชื่อมต่อ LED2 ติดค้าง GPIO Active Low
        while (WiFi.status() != WL_CONNECTED){
         countLoss++;
         DELAY(250); // delay status connect
         Serial.println(String("wifi Loss reconnecting...")+ countLoss);
           if(countLoss >= 199) ESP.restart();
         DELAY(100); //delay count
     }
  Serial.println();
  Serial.println("WiFi Reconnected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Connection ok");
 }
   if(!client.connected()){
      digitalWrite(LED2,LOW);
      Serial.println();
      mqttconnect();
    }
    digitalWrite(LED2,HIGH);
    DELAY(250);
    digitalWrite(LED2,LOW);
    DELAY(125)
    client.loop();
   DELAY(500);
  }
}
