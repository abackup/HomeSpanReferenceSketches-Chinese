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

////////////////////////////////////////////////////////////////////
//                                                                //
//            HomeSpan 参考草图：加湿器/除湿器服务                 //
//                                                                //
////////////////////////////////////////////////////////////////////

#include "HomeSpan.h"

////////////////////////////////////////////////////////////////////////

// 这里我们创建了一个虚拟湿度传感器，可以在下面的加湿器服务中用作真实传感器。
// 与读取真实湿度传感器不同，此结构允许您通过串行监视器更改当前加湿器

struct DummyHumiditySensor {
  static float relativeHumidity;
  
  DummyHumiditySensor(float rH) {
    relativeHumidity=rH;
    new SpanUserCommand('h',"<humidity> - set the relative humidity in percent [0-100]", [](const char *buf){relativeHumidity=atof(buf+1);});
  }

  float read() {return(relativeHumidity);}
};

float DummyHumiditySensor::relativeHumidity;

////////////////////////////////////////////////////////////////////////

struct Reference_HumidifierDehumidifier : Service::HumidifierDehumidifier {

  // 创建特性、设置初始值，并将 NVS 中的存储设置为 true

  Characteristic::Active active{0,true};
  Characteristic::CurrentRelativeHumidity humidity{70,true};
  Characteristic::CurrentHumidifierDehumidifierState currentState{0,true};
  Characteristic::TargetHumidifierDehumidifierState targetState{0,true};
  Characteristic::RelativeHumidityHumidifierThreshold humidThreshold{40,true};
  Characteristic::RelativeHumidityDehumidifierThreshold dehumidThreshold{80,true};
  Characteristic::SwingMode swing{0,true};
  Characteristic::WaterLevel water{50,true};
  Characteristic::RotationSpeed fan{0,true};

  DummyHumiditySensor humiditySensor{70};                                             // 实例化一个初始湿度为 70% 的虚拟湿度传感器
 
  Reference_HumidifierDehumidifier() : Service::HumidifierDehumidifier() {
    Serial.printf("\n*** Creating HomeSpan Humidifier/DeHumidifer ***\n");
    
//    targetState.setValidValues(1,1);      // 取消注释以将允许的模式限制为仅加湿
//    targetState.setValidValues(1,2);      // 取消注释以将允许的模式限制为仅除湿

  }

  boolean update() override {

    if(active.updated()){
      Serial.printf("Humidifier/DeHumidifier Power is %s\n",active.getNewVal()?"ON":"OFF");      
    }

    if(swing.updated()){
      Serial.printf("Humidifier/DeHumidifier Swing Mode is %s\n",swing.getNewVal()?"ON":"OFF");      
    }    

    if(fan.updated()){
      Serial.printf("Humidifier/DeHumidifier Fan Speed set to %g\n",fan.getNewVal<float>());
    }    

    if(targetState.updated()){
      switch(targetState.getNewVal()){
        case 0:
          Serial.printf("Mode set to AUTO\n");
          break;
        case 1:
          Serial.printf("Mode set to HUMIDIFY\n");
          break;
        case 2:
          Serial.printf("Mode set to DEHUMIDIFY\n");
          break;
      }
    }

    // 注意：即使你只更改一个阈值，HomeKit 也会更新两个阈值
    
    if(humidThreshold.updated() && humidThreshold.getNewVal<float>()!=humidThreshold.getVal<float>())
      Serial.printf("Humidifier Threshold changed to %g\n",humidThreshold.getNewVal<float>());
      
    if(dehumidThreshold.updated() && dehumidThreshold.getNewVal<float>()!=dehumidThreshold.getVal<float>())
      Serial.printf("Dehumidifier Threshold changed to %g\n",dehumidThreshold.getNewVal<float>());
    
    return(true);
  }

  // 这里是所有主要逻辑所在，通过比较当前湿度和设备的当前设置来打开/关闭加湿/除湿

  void loop() override {

    float humid=humiditySensor.read();       // 读取湿度传感器（在此示例中，它只是一个虚拟传感器）
    
    if(humid<0)                   // 限制值保持在 0 到 100 之间
      humid=0;
    if(humid>100)
      humid=100;

    if(humidity.timeVal()>5000 && fabs(humidity.getVal<float>()-humid)>0.25){      // 如果距离上次更新已超过 5 秒，并且湿度已发生变化
      humidity.setVal(humid);                                                       
      Serial.printf("Current humidity is now %g\n",humidity.getVal<float>());
    } 

    if(active.getVal()==0){                                               // 电源关闭
      if(currentState.getVal()!=0){                                       // 如果当前状态不是非活动状态
        Serial.printf("Humidifier/DeHumidifier State: INACTIVE\n");       // 设置为非活动状态
        currentState.setVal(0);
      }
      return;                                                             // 返回，因为当设备关闭时没有其他需要检查的内容
    }

    if(humidity.getVal<float>()<humidThreshold.getVal<float>() && targetState.getVal()!=2){    // 湿度太低，模式允许加湿
      if(currentState.getVal()!=2){                                                          // 如果当前状态不是加湿
        Serial.printf("Humidifier/DeHumidifier State: HUMIDIFYING\n");                       // 设置为加湿
        currentState.setVal(2);
      }
     return;
    }

    if(humidity.getVal<float>()>dehumidThreshold.getVal<float>() && targetState.getVal()!=1){  // 湿度太高，模式允许除湿
      if(currentState.getVal()!=3){                                                          // 如果当前状态不是除湿
        Serial.printf("Humidifier/DeHumidifier State: DE-HUMIDIFYING\n");                    // 设置为除湿
        currentState.setVal(3);
      }
     return;
    }

    if(currentState.getVal()!=1){                                         // 状态应该为空闲，但事实并非如此
       currentState.setVal(1);
       Serial.printf("Humidifier/DeHumidifier State: IDLE\n");            // 设置为空闲 
    }
  }

};

////////////////////////////////////////////////////////////////////////

void setup() {
  
  Serial.begin(115200);

  homeSpan.begin(Category::Humidifiers,"HomeSpan Humidifier");

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();

    new Reference_HumidifierDehumidifier();    
}

////////////////////////////////////////////////////////////////////////

void loop() {
  homeSpan.poll();
}

////////////////////////////////////////////////////////////////////////
