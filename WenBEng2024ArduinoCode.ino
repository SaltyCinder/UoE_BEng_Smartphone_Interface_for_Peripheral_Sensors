#include <SoftwareSerial.h>
#include "DHT.h"
#define DHTPIN 3     //定义引脚
#define DHTTYPE DHT11   // 选择传感器为 DHT 11

// Set the digital pin numbers for software serial communication
int rxPin = 2; // RX Pin
int txPin = 3; // TX Pin

// Initialize the software serial port
SoftwareSerial mySerial(rxPin, txPin);
SoftwareSerial BT(13, 12);		//定义蓝牙输入输出引脚, Pin5为RX，接HC05的TX针脚Pin6为TX，接HC05的RX针脚

// 用于存储接收到的数据的数组
byte receivedBytes[7];
DHT dht(DHTPIN, DHTTYPE); //使用引脚和芯片初始化，一个名为dht的DHT对象

int index = 0;// 用于追踪当前存储位置的索引，只在printing received data时使用到
int combinedValue =0; //储存器所储存的值，单位为0.1Hz
int frequency = 0;    //最终输出值，单位为HZ
char firstChar = 'N'; // 初始化indicator变量，N for Nah

void setup() {
  // Start the hardware serial port
  Serial.begin(9600);
  // Start the software serial port
  delay(100);
  mySerial.begin(9600);
  delay(100);
  BT.begin(9600);	
  delay(100);
  Serial.println("Serial monitor started. Awaiting data...");
  dht.begin();// 初始化先前创建的dht对象
/*
//===================================================================================================
    // Command to send to the VM501 chip
    //写入
  //byte commandBytes[] = {0xAA, 0xBB, 0x01, 0x11|0x80, 0x00, 0x0A};
    //读取
  byte commandBytes[] = {0xAA, 0xBB, 0x01, 0x23};

  int length = sizeof(commandBytes); // Number of bytes in the original command
  // Create an array to hold the command with checksum (1 byte larger than original)
  byte commandBytesWithChecksum[length + 1];
  // Call the function
  appendChecksum(commandBytes, length, commandBytesWithChecksum);
  // Send command to the chip
  mySerial.write(commandBytesWithChecksum, sizeof(commandBytesWithChecksum));
  Serial.print("Commend Sent with CRC: ");
  for (unsigned int i = 0; i < length + 1; i++) { // 注意长度加2
    Serial.print(commandBytesWithChecksum[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  while (!mySerial.available()) {
  //delay(5); // 短暂延迟以避免完全占用CPU
  }

 Serial.print("Received Data: ");
  while (mySerial.available()) {
    char inByte = mySerial.read();
    // Do something with the received byte
    Serial.print((unsigned char)inByte, HEX);// Print each byte in HEX format
    Serial.print(" "); // Print space for readability
  }
  
  Serial.println("");
  // Wait some time before sending the command again
  delay(5000); // Adjust as necessary for your application
  */
}

void loop() {
    String receivedCmd; // 修改为String类型
    while (BT.available()) { // 检查是否有数据从端口接收                                                //这里可以更改为硬件/蓝牙串口测试
      //BT.listen();                                                                                 //这里可以更改为硬件/蓝牙串口测试
      receivedCmd = BT.readStringUntil(' '); // 读取一行数据                                      //这里可以更改为硬件/蓝牙串口测试
      Serial.println("Received Command: " + receivedCmd); // 打印接收到的指令
      if (receivedCmd.length() > 0) { // 确保字符串不为空
      firstChar = receivedCmd.charAt(0); // 获取第一个字符
      
      }
    }

    Serial.print("Current firstChar:  ");
    Serial.println(firstChar);
    
    if (firstChar == 'A') {
      float h = dht.readHumidity();  // 摄氏度读取温度 (the default)
      float t = dht.readTemperature();  // 华氏度读取温度 (isFahrenheit = true)
      float f = dht.readTemperature(true);

      // 检查错误信息 (to try again).
      if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println(F("Failed to read from DHT sensor!"));
        return;
      }
      //常规串口输出
      Serial.print(F("Humidity: "));
      BT.print(t);		//蓝牙输出温度值
      BT.print("a");
      Serial.print(h);
      Serial.print(F("%  Temperature: "));
      BT.print(h);			//蓝牙输出湿度值
      BT.println("a");
      Serial.print(t);
      Serial.print(F("°C "));
      Serial.print(f);
      Serial.println(F("°F "));          
    } 
    //=====================B Commend===========================
    if (firstChar == 'B') {
      //------------激励方式改变------------↓↓↓↓↓↓↓↓
      sendSettingCommandWithCRC_bits(0xAA, 0xBB, 0x01, 0x0A | 0x80, 0x00, 0x64);
      unsigned long B_Time = millis();// Implement a break condition or a delay as necessary
      while (!mySerial.available()) {
        mySerial.listen();
        if (millis() - B_Time > 3000) { // 5秒超时
          sendSettingCommandWithCRC_bits(0xAA, 0xBB, 0x01, 0x0A | 0x80, 0x00, 0x64);
          B_Time = millis();
        }
      }
      
      Serial.print("Received Data: ");
      while (mySerial.available()) {
        mySerial.listen();                                                      //极其极其极其关键
        //Serial.println("Enter mySerial Detection Loop");
        char inByte = mySerial.read();
        // Do something with the received byte
        Serial.print((unsigned char)inByte, HEX);// Print each byte in HEX format
        Serial.print(" "); // Print space for readability
      }
      Serial.println(" ");
      Serial.println("Auto Feedback Method Set");
      //------------F_MAX------------↑↑↑↑↑↑↑↑
      Serial.println("B finished. Excitation Method set to auto feedback");     
      firstChar = 'D';    
    } 
    //=====================C Commend===========================
    else if (firstChar == 'C') {
      handleCCommand(receivedCmd);
      Serial.println("C Order handling finished"); 
      firstChar = 'D';           
    } 
    //=====================D Commend===========================
    else if (firstChar == 'D') {
    unsigned long startTime = millis();// Implement a break condition or a delay as necessary
    //以下为发送测量指令+确认接受反馈的完整代码=============↓↓↓↓↓↓↓↓
    sendMeasureCommandWithCRC(0xAA, 0xBB, 0x01, 0x23);
    unsigned long D_Time = millis();// Implement a break condition or a delay as necessary

    while (!mySerial.available()) {
      mySerial.listen();
      //Serial.println("Stuck in ! while loop");
      if (millis() - D_Time > 3000) { // 3秒超时
        sendMeasureCommandWithCRC(0xAA, 0xBB, 0x01, 0x23);
        Serial.println("Measure Request Sent");
        D_Time = millis();
     }
      if(mySerial.available()){
        break;
      }
    }

    Serial.print("Received Data: ");
    while (mySerial.available()) {
      mySerial.listen();                                                      //极其极其极其关键
      //Serial.println("Enter mySerial Detection Loop");
      char inByte = mySerial.read();
      // Do something with the received byte
      receivedBytes[index] = inByte;
      // 更新索引
      index++;
      Serial.print((unsigned char)inByte, HEX);// Print each byte in HEX format
      Serial.print(" "); // Print space for readability
    }
    //以上为发送测量指令+确认接受反馈的完整代码=============↑↑↑↑↑↑↑↑
  

    byte highByte = receivedBytes[4]; // 第五位
    byte lowByte = receivedBytes[5];  // 第六位
    combinedValue = (highByte << 8) | lowByte;
    frequency = combinedValue/10;
    Serial.print("Combined value: ");
    Serial.println(combinedValue);
    Serial.print("Frequency: ");
    Serial.println(frequency);

    if (frequency > 0 & frequency < 5000){
      BT.println(frequency);
      Serial.println("BT write");
    }  
    
    // 重置索引以存储下一个序列
    index = 0;    
    } 
    
    Serial.println("");
    unsigned long Delay = millis();// Implement a break condition or a delay as necessary
    while (!BT.available()) {
      //BT.listen();
      //Serial.println("Stuck in ! while loop");
      if (millis() - Delay > 1000) { // 延迟
        break;
      }
    }

  }
    




void appendChecksum(byte commandBytes[], int length, byte result[]) {
  // Calculate checksum
  byte checksum = 0;
  for (int i = 0; i < length; i++) {
    checksum += commandBytes[i];
  }
  checksum = checksum & 0xFF;
  // Copy original command bytes to result array
  for (int i = 0; i < length; i++) {
    result[i] = commandBytes[i];
  }
  // Append checksum to result array
  result[length] = checksum;
}

void sendSettingCommandWithCRC_bits(byte cmd1, byte cmd2, byte cmd3, byte cmd4, byte cmd5, byte cmd6) {
  byte commandBytes[] = {cmd1, cmd2, cmd3, cmd4, cmd5, cmd6};
  int length = sizeof(commandBytes);
  byte commandBytesWithChecksum[length + 1];
  appendChecksum(commandBytes, length, commandBytesWithChecksum);
  mySerial.write(commandBytesWithChecksum, sizeof(commandBytesWithChecksum));
  Serial.print("Measure Commend Sent with CRC: ");                                //检查发送指令
  for (unsigned int i = 0; i < length + 1; i++) { // 注意长度加2
    Serial.print(commandBytesWithChecksum[i], HEX);
    Serial.print(" ");
  }
  Serial.println("Setting Order Sent");
}

void sendSettingCommandWithCRC(byte commandBytes[], int length) {
  byte commandBytesWithChecksum[length + 1];
  appendChecksum(commandBytes, length, commandBytesWithChecksum);
  mySerial.write(commandBytesWithChecksum, sizeof(commandBytesWithChecksum));
  Serial.print("Setting Commend Sent with CRC: ");                                //检查发送指令
  for (unsigned int i = 0; i < length + 1; i++) { // 注意长度加2
    Serial.print(commandBytesWithChecksum[i], HEX);
    Serial.print(" ");
  }
  Serial.println("Setting Order Sent");
}

void sendMeasureCommandWithCRC(byte cmd1, byte cmd2, byte cmd3, byte cmd4) {
  byte commandBytes[] = {cmd1, cmd2, cmd3, cmd4};
  int length = sizeof(commandBytes);
  byte commandBytesWithChecksum[length + 1];
  appendChecksum(commandBytes, length, commandBytesWithChecksum);
  mySerial.write(commandBytesWithChecksum, sizeof(commandBytesWithChecksum));
  Serial.print("Measure Commend Sent with CRC: ");                                //检查发送指令
  for (unsigned int i = 0; i < length + 1; i++) { // 注意长度加2
    Serial.print(commandBytesWithChecksum[i], HEX);
    Serial.print(" ");
  }
  Serial.println("Measure Request Sent");
}

void handleCCommand(String receivedCmd) {
  // Wait and read the full command, then parse and send commands as required
  // This is a placeholder for logic to read and parse the incoming command
    // 分割字符串
    int firstHash = receivedCmd.indexOf('#');
    int secondHash = receivedCmd.indexOf('#', firstHash + 1);
    int thirdHash = receivedCmd.indexOf('#', secondHash + 1);

    // 提取字符串并转换为整数
    int F_MIN = receivedCmd.substring(firstHash + 1, secondHash).toInt();
    int F_STEP = receivedCmd.substring(secondHash + 1, thirdHash).toInt();
    int F_MAX = receivedCmd.substring(thirdHash + 1).toInt();

    // 将整数拆分为高位和低位
    byte F_MIN_high = (F_MIN >> 8) & 0xFF;
    byte F_MIN_low = F_MIN & 0xFF;
    byte F_STEP_high = (F_STEP >> 8) & 0xFF;
    byte F_STEP_low = F_STEP & 0xFF;
    byte F_MAX_high = (F_MAX >> 8) & 0xFF;
    byte F_MAX_low = F_MAX & 0xFF;

    // 创建数组并添加相应的值
    byte F_MIN_array[] = {0xAA, 0xBB, 0x01, 0x0F|0x80, F_MIN_high, F_MIN_low};
    byte F_STEP_array[] = {0xAA, 0xBB, 0x01, 0x11|0x80, F_STEP_high, F_STEP_low};
    byte F_MAX_array[] = {0xAA, 0xBB, 0x01, 0x10|0x80, F_MAX_high, F_MAX_low};

    // 打印数组内容，用于调试
    Serial.print("F_MIN_array: ");
    for (byte i : F_MIN_array) {
      Serial.print(i, HEX);
      Serial.print(" ");
    }
    Serial.println();

    Serial.print("F_STEP_array: ");
    for (byte i : F_STEP_array) {
      Serial.print(i, HEX);
      Serial.print(" ");
    }
    Serial.println();

    Serial.print("F_MAX_array: ");
    for (byte i : F_MAX_array) {
      Serial.print(i, HEX);
      Serial.print(" ");
    }
    Serial.println();
    //===============设置寄存器值===============

    //以下为发送一条指令包括确认接受反馈的完整代码=============F_MIN_______↓↓↓↓↓↓↓↓
    sendSettingCommandWithCRC(F_MIN_array, sizeof(F_MIN_array));
    unsigned long C_Time = millis();// Implement a break condition or a delay as necessary

    while (!mySerial.available()) {
      mySerial.listen();
      //Serial.println("Stuck in ! while loop");
      if (millis() - C_Time > 3000) { // 3秒超时
        sendSettingCommandWithCRC(F_MIN_array, sizeof(F_MIN_array));
        C_Time = millis();
     }
    }

    Serial.print("Received Data: ");
    while (mySerial.available()) {
      mySerial.listen();                                                      //极其极其极其关键
      //Serial.println("Enter mySerial Detection Loop");
      char inByte = mySerial.read();
      // Do something with the received byte
      Serial.print((unsigned char)inByte, HEX);// Print each byte in HEX format
      Serial.print(" "); // Print space for readability
    }
    Serial.println(" ");
    Serial.println("MIN Frequency Set");
    //以上为发送一条指令包括确认接受反馈的完整代码=============F_MIN_______↑↑↑↑↑↑↑↑
    //------------F_STEP------------↓↓↓↓↓↓↓↓
    sendSettingCommandWithCRC(F_STEP_array, sizeof(F_STEP_array));
    C_Time = millis();// Implement a break condition or a delay as necessary

    while (!mySerial.available()) {
      mySerial.listen();
      //Serial.println("Stuck in ! while loop");
      if (millis() - C_Time > 3000) { // 3秒超时
        sendSettingCommandWithCRC(F_STEP_array, sizeof(F_STEP_array));
        C_Time = millis();
     }
    }

    Serial.print("Received Data: ");
    while (mySerial.available()) {
      mySerial.listen();                                                      //极其极其极其关键
      //Serial.println("Enter mySerial Detection Loop");
      char inByte = mySerial.read();
      // Do something with the received byte
      Serial.print((unsigned char)inByte, HEX);// Print each byte in HEX format
      Serial.print(" "); // Print space for readability
    }
    Serial.println(" ");
    Serial.println("STEP Frequency Set");
    //------------F_STEP------------↑↑↑↑↑↑↑↑
    //------------F_MAX------------↓↓↓↓↓↓↓↓
    sendSettingCommandWithCRC(F_MAX_array, sizeof(F_MAX_array));
    C_Time = millis();// Implement a break condition or a delay as necessary

    while (!mySerial.available()) {
      mySerial.listen();
      //Serial.println("Stuck in ! while loop");
      if (millis() - C_Time > 3000) { // 3秒超时
        sendSettingCommandWithCRC(F_MAX_array, sizeof(F_MAX_array));
        C_Time = millis();
     }
    }

    Serial.print("Received Data: ");
    while (mySerial.available()) {
      mySerial.listen();                                                      //极其极其极其关键
      //Serial.println("Enter mySerial Detection Loop");
      char inByte = mySerial.read();
      // Do something with the received byte
      Serial.print((unsigned char)inByte, HEX);// Print each byte in HEX format
      Serial.print(" "); // Print space for readability
    }
    Serial.println(" ");
    Serial.println("MAX Frequency Set");
    //------------F_MAX------------↑↑↑↑↑↑↑↑
    //------------Excitation Change------------↓↓↓↓↓↓↓↓
    sendSettingCommandWithCRC_bits(0xAA, 0xBB, 0x01, 0x0A | 0x80, 0x00, 0x68);
    C_Time = millis();// Implement a break condition or a delay as necessary

    while (!mySerial.available()) {
      mySerial.listen();
      //Serial.println("Stuck in ! while loop");
      if (millis() - C_Time > 3000) { // 3秒超时
        sendSettingCommandWithCRC_bits(0xAA, 0xBB, 0x01, 0x0A | 0x80, 0x00, 0x68);
        C_Time = millis();
     }
    }

    Serial.print("Received Data: ");
    while (mySerial.available()) {
      mySerial.listen();                                                      //极其极其极其关键
      //Serial.println("Enter mySerial Detection Loop");
      char inByte = mySerial.read();
      // Do something with the received byte
      Serial.print((unsigned char)inByte, HEX);// Print each byte in HEX format
      Serial.print(" "); // Print space for readability
    }
    Serial.println(" ");
    Serial.println("Exciation Method changed to Sweeping, with desired sweeping range");
    //------------Excitation Change------------↑↑↑↑↑↑↑↑
}

void printCommand(byte *command, int length) {
  Serial.print("Command Sent with CRC: ");
  for (int i = 0; i < length; i++) {
    Serial.print(command[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}
