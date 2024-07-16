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
//            HomeSpan 参考草图：恒温器服务           //
//                                                   //
///////////////////////////////////////////////////////

#include "HomeSpan.h"

#define MIN_TEMP  0         // 最低允许温度（摄氏度）
#define MAX_TEMP  40        // 最高允许温度（摄氏度）

////////////////////////////////////////////////////////////////////////

// 这里我们创建了一个虚拟温度传感器，可以在下面的恒温器服务中用作真实传感器。
// 此结构允许您通过串行监视器更改当前温度，而不是读取真正的温度传感器

struct DummyTempSensor {
  static float temp;
  
  DummyTempSensor(float t) {
    temp=t;
    new SpanUserCommand('f',"<temp> - set the temperature, where temp is in degrees F", [](const char *buf){temp=(atof(buf+1)-32.0)/1.8;});
    new SpanUserCommand('c',"<temp> - set the temperature, where temp is in degrees C", [](const char *buf){temp=atof(buf+1);});
  }

  float read() {return(temp);}
};

float DummyTempSensor::temp;

////////////////////////////////////////////////////////////////////////

struct Reference_Thermostat : Service::Thermostat {

  // 创建特性、设置初始值，并将 NVS 中的存储设置为 true

  Characteristic::CurrentHeatingCoolingState currentState{0,true};
  Characteristic::TargetHeatingCoolingState targetState{0,true}; 
  Characteristic::CurrentTemperature currentTemp{22,true};
  Characteristic::TargetTemperature targetTemp{22,true};
  Characteristic::CurrentRelativeHumidity currentHumidity{50,true};
  Characteristic::TargetRelativeHumidity targetHumidity{50,true};
  Characteristic::HeatingThresholdTemperature heatingThreshold{22,true};
  Characteristic::CoolingThresholdTemperature coolingThreshold{22,true};  
  Characteristic::TemperatureDisplayUnits displayUnits{0,true};               // 这是为了更改实际恒温器（如果有）上的显示，而不是在 Home App 中

  DummyTempSensor tempSensor{22};                                             // 实例化一个初始温度为 22 摄氏度的虚拟温度传感器
 
  Reference_Thermostat() : Service::Thermostat() {
    Serial.printf("\n*** Creating HomeSpan Thermostat***\n");

    currentTemp.setRange(MIN_TEMP,MAX_TEMP);                                  // 将所有范围设置为相同，以确保 Home App 在同一表盘上正确显示它们
    targetTemp.setRange(MIN_TEMP,MAX_TEMP);
    heatingThreshold.setRange(MIN_TEMP,MAX_TEMP);
    coolingThreshold.setRange(MIN_TEMP,MAX_TEMP);    
  }

  boolean update() override {

    if(targetState.updated()){
      switch(targetState.getNewVal()){
        case 0:
          Serial.printf("Thermostat turning OFF\n");
          break;
        case 1:
          Serial.printf("Thermostat set to HEAT at %s\n",temp2String(targetTemp.getVal<float>()).c_str());
          break;
        case 2:
          Serial.printf("Thermostat set to COOL at %s\n",temp2String(targetTemp.getVal<float>()).c_str());
          break;
        case 3:
          Serial.printf("Thermostat set to AUTO from %s to %s\n",temp2String(heatingThreshold.getVal<float>()).c_str(),temp2String(coolingThreshold.getVal<float>()).c_str());
          break;
      }
    }

    if(heatingThreshold.updated() || coolingThreshold.updated())
      Serial.printf("Temperature range changed to %s to %s\n",temp2String(heatingThreshold.getNewVal<float>()).c_str(),temp2String(coolingThreshold.getNewVal<float>()).c_str());
      
    else if(targetTemp.updated())
      Serial.printf("Temperature target changed to %s\n",temp2String(targetTemp.getNewVal<float>()).c_str());

    if(displayUnits.updated())
      Serial.printf("Display Units changed to %c\n",displayUnits.getNewVal()?'F':'C');

    if(targetHumidity.updated())
      Serial.printf("Humidity target changed to %d%%\n",targetHumidity.getNewVal());
    
    return(true);
  }

  // 这里是所有主要逻辑所在，通过比较当前温度和恒温器的设置来打开/关闭加热/冷却

  void loop() override {

      float temp=tempSensor.read();       // 读取温度传感器（在此示例中只是一个虚拟传感器）
      
      if(temp<MIN_TEMP)                   // 限制值保持在 MIN_TEMP 和 MAX_TEMP 之间
        temp=MIN_TEMP;
      if(temp>MAX_TEMP)
        temp=MAX_TEMP;

      if(currentTemp.timeVal()>5000 && fabs(currentTemp.getVal<float>()-temp)>0.25){      // 如果距离上次更新已超过 5 秒，并且温度已发生变化
        currentTemp.setVal(temp);                                                       
        Serial.printf("Current Temperature is now %s.\n",temp2String(currentTemp.getNewVal<float>()).c_str());
      } 

      switch(targetState.getVal()){
        
        case 0:
          if(currentState.getVal()!=0){
            Serial.printf("Thermostat OFF\n");
            currentState.setVal(0);
          }
          break;
          
        case 1:
          if(currentTemp.getVal<float>()<targetTemp.getVal<float>() && currentState.getVal()!=1){
            Serial.printf("Turning HEAT ON\n");
            currentState.setVal(1);
          }
          else if(currentTemp.getVal<float>()>=targetTemp.getVal<float>() && currentState.getVal()==1){
            Serial.printf("Turning HEAT OFF\n");
            currentState.setVal(0);
          }
          else if(currentState.getVal()==2){
            Serial.printf("Turning COOL OFF\n");
            currentState.setVal(0);            
          }
          break;
          
        case 2:
          if(currentTemp.getVal<float>()>targetTemp.getVal<float>() && currentState.getVal()!=2){
            Serial.printf("Turning COOL ON\n");
            currentState.setVal(2);
          }
          else if(currentTemp.getVal<float>()<=targetTemp.getVal<float>() && currentState.getVal()==2){
            Serial.printf("Turning COOL OFF\n");
            currentState.setVal(0);
          }
          else if(currentState.getVal()==1){
            Serial.printf("Turning HEAT OFF\n");
            currentState.setVal(0);            
          }
          break;
          
        case 3:
          if(currentTemp.getVal<float>()<heatingThreshold.getVal<float>() && currentState.getVal()!=1){
            Serial.printf("Turning HEAT ON\n");
            currentState.setVal(1);
          }
          else if(currentTemp.getVal<float>()>=heatingThreshold.getVal<float>() && currentState.getVal()==1){
            Serial.printf("Turning HEAT OFF\n");
            currentState.setVal(0);
          }
          
          if(currentTemp.getVal<float>()>coolingThreshold.getVal<float>() && currentState.getVal()!=2){
            Serial.printf("Turning COOL ON\n");
            currentState.setVal(2);
          }
          else if(currentTemp.getVal<float>()<=coolingThreshold.getVal<float>() && currentState.getVal()==2){
            Serial.printf("Turning COOL OFF\n");
            currentState.setVal(0);
          }
          break;
      }
  }

  // 此“辅助”函数可轻松在串行监视器上显示温度（单位为 F 或 C），具体取决于 TemperatureDisplayUnits
  
  String temp2String(float temp){
    String t = displayUnits.getVal()?String(round(temp*1.8+32.0)):String(temp);
    t+=displayUnits.getVal()?" F":" C";
    return(t);    
  }  

};

////////////////////////////////////////////////////////////////////////

void setup() {
  
  Serial.begin(115200);

  homeSpan.begin(Category::Thermostats,"HomeSpan Thermostat");

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();

    new Reference_Thermostat();    
}

////////////////////////////////////////////////////////////////////////

void loop() {
  homeSpan.poll();
}

////////////////////////////////////////////////////////////////////////
