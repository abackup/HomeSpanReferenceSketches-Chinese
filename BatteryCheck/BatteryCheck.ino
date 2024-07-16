/*********************************************************************************
 *  MIT License
 *  
 *  Copyright (c) 2023 Gregg E. Berman
 *  
 *  https://github.com/HomeSpan/HomeSpan
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *  
 ********************************************************************************/
 
///////////////////////////////////////////////////////
//                                                   //
//            HomeSpan 参考草图：电池服务            //
//                                                   //
///////////////////////////////////////////////////////

#include "HomeSpan.h" 

////////////////////////////////////////////////////////////////////////
//                            电池类                                  //
////////////////////////////////////////////////////////////////////////

// 这个独立类（与 HomeSpan 分开）旨在测量 Adafruit ESP32 Huzzah Featherboard 上的电池电压，其中 LiPo 硬连线到引脚 35，并且可以通过 analogRead(35) 读取电池电压。

// 这些模拟值的范围从大约 1850（电池刚好耗尽）到 2400 以上（电池充满电）。

// 此外，通过将 ESP32 上的 USB 电压引脚通过由两个 10K 欧姆电阻组成的分压器连接到数字引脚 21，可以通过调用 digitalRead(21) 来确定 ESP 是否插入 USB 电源。这可用于确定电池是否正在充电。

// 此类支持以下两种方法：
//
//  * int getPercentCharged() - 返回 0-100
//  * int getChargingState() - 如果 ESP32 插入 USB 电源则返回 1，否则返回 0

// 注意：此类会在每秒运行的后台任务中自动检查电池电压。

class BATTERY {
  
  int batteryPin;         // 用于模拟读取电池电压的引脚
  int usbPin;             // 用于数字读取 USB 电压的引脚
  int minReading;         // 电池电压的最小预期模拟值（对应 0% 充电）
  int maxReading;         // 电池电压的最大预期模拟值（对应 100% 充电）
  float analogReading;    // 电池电压的模拟读数

  public:

  BATTERY(int batteryPin, int usbPin, int minReading, int maxReading){
    this->batteryPin=batteryPin;
    this->usbPin=usbPin;
    this->minReading=minReading;
    this->maxReading=maxReading;
    analogReading=maxReading;
        
    pinMode(usbPin,INPUT_PULLDOWN);     // 将 usbPin 设置为输入模式
    
    xTaskCreateUniversal(batteryUpdate, "batteryTaskHandle", 4096, this, 1, NULL, 0);   // 启动后台任务来测量模拟读数(35)
  }

  // 返回 0-100 之间的充电百分比
  
  int getPercentCharged(){
    int percentCharged=100.0*(analogReading-minReading)/(maxReading-minReading);

    if(percentCharged>100)
      percentCharged=100;
    else if(percentCharged<0)
      percentCharged=0;

    return(percentCharged);
  }

  // returns 1 if USB is powered, else 0

  int getChargingState(){
    return(digitalRead(usbPin));
  }

  // background task that measures voltage of battery
  
  static void batteryUpdate(void *args){
    BATTERY *b = (BATTERY*)args;
    for(;;){
      b->analogReading*=0.9;
      b->analogReading+=0.1*analogRead(b->batteryPin);      // 使用指数平滑法
      delay(1000);
    }
  }
  
};

BATTERY Battery(35,21,1850,2400);     // 创建 BATTERY 类的全局实例，供下面的电池服务使用

////////////////////////////////////////////////////////////////////////
//                        HomeSpan 代码                               //
////////////////////////////////////////////////////////////////////////

struct SimpleLED : Service::LightBulb {

  Characteristic::On power;
  int ledPin;
 
  SimpleLED(int ledPin) : Service::LightBulb(){
    this->ledPin=ledPin;
    pinMode(ledPin,OUTPUT);    
  }

  boolean update(){            
    digitalWrite(ledPin,power.getNewVal());
    return(true);
  }
};

////////////////////////////////////////////////////////////////////////

struct SimpleBattery : Service::BatteryService{

  SpanCharacteristic *percentCharged;
  SpanCharacteristic *chargingState;
  SpanCharacteristic *lowBattery;
  int lowPercent;

  SimpleBattery(int lowPercent) : Service::BatteryService(){

    this->lowPercent=lowPercent;

    percentCharged = new Characteristic::BatteryLevel(Battery.getPercentCharged());
    chargingState = new Characteristic::ChargingState(Battery.getChargingState());
    lowBattery = new Characteristic::StatusLowBattery(Battery.getPercentCharged()<lowPercent?1:0);
  }
  
  void loop() override {
        
    if(Battery.getChargingState()!=chargingState->getVal())          // 如果发生变化，立即更新充电状态
      chargingState->setVal(Battery.getChargingState());

    if(percentCharged->timeVal()>5000 && Battery.getPercentCharged()!=percentCharged->getVal()){   // 更新百分比收费仅每 5 秒一次，如果更改
      percentCharged->setVal(Battery.getPercentCharged());
      lowBattery->setVal(Battery.getPercentCharged()<lowPercent?1:0);
    }
  }
};

////////////////////////////////////////////////////////////////////////

void setup() {

  Serial.begin(115200);

  homeSpan.begin(Category::Lighting,"HomeSpan LED");
  
  new SpanAccessory(); 
    new Service::AccessoryInformation(); 
      new Characteristic::Identify();                
    new SimpleLED(13);                      // 创建一个 LightBulb 服务，操作一个简单的 LED 引脚 13
    new SimpleBattery(20);                  // 在“家庭”应用中创建电池服务，使用 20% 作为低电量警告的阈值
}

////////////////////////////////////////////////////////////////////////

void loop(){
  homeSpan.poll();  
}

////////////////////////////////////////////////////////////////////////
