# RedStack
Project RedStack is mailbox iot use arduino mega 2560 and arduino promini RS485 Serial protocol


#fix1
    แก้ให้บอร์ดแม่ส่ง new message ไปที่ mqtt broker ตามจำนวนจดหมาย
#
#
#
### Bug ###
#   ถ้า Mqtt broker ปิดอยู่ จะใช้เวลาในการ reconnect ประมาณ 10 - 20 วิและไม่ทำงานอื่น แต่ถ้า ethernet หลุดจะใช้เวลาคอนเนคแค่ 2 - 3  วิ
#   
#

