# Mconnect4.1_task_060620
version Mconnect4.1_task_060620 นี้ มี 2 Tasks 
 task_CheckWiFiMqtt,
 task_PackdataSend
 run บน core1 ทั้ง 2 tasks

ใน task library TridentTD_EasyFreeRTOS32.h 
 default stack 2048 core1

ปรับ tune stack task_PackdataSend.start(taskPackdatasend,NULL,61440); 
NULL ไม่มี parameter,stack 60KB, ส่งเข้า function   default Core1

(*** เคยแยก task packdata อย่างดียว ใน version4 แล้วมีปัญหา address ram กระโดด 655329 )

ทดสอบแล้ว ทำงานปรกติ  
  - เมื่อ เปิด server database อ่าน ram ส่ง realtime clear ram
  -เมื่อ ปิด server database packdata เขียนลง ram ไปเรื่อยๆ จนถึง 32KB write ลง SDcard
   ยังไม่ได้ทดสอบ อ่านส่งจาก SDcard (history)

***task ที่จะทำต่อไป คือ เมื่อขณะ write sdcard  การ packdata จะหยุด เขียน ram 
   task นี้ จะ Start ตอน เริ่ม write sdcard ทำงาน packdata I/O ปรกติ ที่จะทดลองคือ ไปเก็บ data
       ไว้ในตัวแปร array ก่อน ช่วงที่ เขียน sdcard ใช้เวลาประมาณ 22 วินาที ช่วงนี้  task packdata_continue
       จะทำงาน และ Stop เมื่อ write sdcard เสร็จ  

ใน version นี้  void loop() ใน main program จะ run เพียง function_ota()
เพื่อรอรับ คำสั่ง OTA จากหน้า web 

