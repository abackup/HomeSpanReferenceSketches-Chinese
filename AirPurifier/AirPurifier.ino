/*********************************************************************************
 *  MIT License
 *  
 *  Copyright (c) 2024 Gregg E. Berman
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

 
/////////////////////////////////////////////////////////
//                                                     //
//         Homespan参考草图：空气净化器服务             //
//                                                     //
/////////////////////////////////////////////////////////

//从iOS 17.2.1开始，家庭应用程序提供以下空气净化器功能：
//
// *主开关允许您将活动特性设置为活动或无效
// *可以通过在设置屏幕上标记为“模式”的切换按钮获得相同功能的重复，这也使您可以将活动特征设置为活动或无效
//*设置屏幕上的两个选择器按钮允许您将targetAirPurifierState设置为自动或手动的特征
//
//注意：通过主开关或重复的“模式”切换在“设置”屏幕上，将附件的状态从不活动变为活动时，HOME App自动将TargetAirPurifierState设置为自动。 
//如果要在手动模式下操作，则必须在 *首先设置Active的附件后选择该选项 *。 换句话说，家庭应用总是在自动模式下“启动”净化器
//
 
#include "HomeSpan.h"

////////////////////////////////////////////////////////////////////////

struct AirFilter : Service::FilterMaintenance {

  Characteristic::FilterChangeIndication filterChange;
  Characteristic::FilterLifeLevel filterLife;
  Characteristic::ResetFilterIndication filterReset;  
  Characteristic::ConfiguredName filterName;

  AirFilter(const char *name) : Service::FilterMaintenance() {
    filterName.setString(name);
  }

  boolean update() override {
    if(filterReset.updated()){
      filterLife.setVal(100);                               // 将过滤寿命重置为100％
      filterChange.setVal(filterChange.NO_CHANGE_NEEDED);   // 重置过滤器更改指示器
    }

    return(true);
  }
  
};

////////////////////////////////////////////////////////////////////////

struct AirSensor : Service::AirQualitySensor {

  Characteristic::AirQuality airQuality{Characteristic::AirQuality::GOOD};
  Characteristic::OzoneDensity ozoneDensity{100};
  Characteristic::NitrogenDioxideDensity no2Density{200};
  Characteristic::SulphurDioxideDensity so2Density{300};
  Characteristic::PM25Density smallPartDensity{400};
  Characteristic::PM10Density largePartDensity{500};
  Characteristic::VOCDensity vocDensity{600};  
};  

////////////////////////////////////////////////////////////////////////

struct Purifier : Service::AirPurifier {

  Characteristic::Active active;
  Characteristic::CurrentAirPurifierState currentState;
  Characteristic::TargetAirPurifierState targetState;

  AirSensor airSensor;
  AirFilter preFilter{"Pre-Filter"};
  AirFilter hepaFilter{"HEPA Filter"};

  Purifier() : Service::AirPurifier() {

    addLink(&preFilter);          // 空气过滤器需要与空气净化器相关
    addLink(&hepaFilter);
  }

  boolean update() override {

    if(active.updated()){
      if(active.getNewVal()==active.ACTIVE)
        LOG0("Purifier ACTIVE (AUTO)\n");
      else
        LOG0("Purifier INACTIVE\n");
    }

    if(targetState.updated() && active.getVal()==active.ACTIVE){
      if(targetState.getVal()==targetState.AUTO)
        LOG0("Purifier ACTIVE (AUTO)\n");
      else
        LOG0("Purifier ACTIVE (MANUAL)\n");
    }
    
    return(true);
  }

  void loop() override {

    // 如果将主开关设置为非活动性，请确保附件也无效，无论是在手动还是自动模式下
   
    if(active.getVal()==active.INACTIVE && currentState.getVal()!=currentState.INACTIVE){
      LOG0("Purifier is turning OFF.\n");
      currentState.setVal(currentState.INACTIVE);
    }

    // 如果以手动模式和主开关设置为活动，请确保附件正在净化
    
    else if(targetState.getVal()==targetState.MANUAL && active.getVal()==active.ACTIVE && currentState.getVal()!=currentState.PURIFYING){
      LOG0("Purifier is PURIFYING.\n");
      currentState.setVal(currentState.PURIFYING);
    }
    
    // 如果以自动模式和主开关设置为活动，请确保配件要么净化或闲置，具体取决于空气质量传感器

    else if(targetState.getVal()==targetState.AUTO && active.getVal()==active.ACTIVE && currentState.getVal()==currentState.INACTIVE){
      LOG0("Purifier is IDLE.\n");
      currentState.setVal(currentState.IDLE);      
    }
    
  }
};

////////////////////////////////////////////////////////////////////////

void setup() {

  Serial.begin(115200);

  homeSpan.setLogLevel(2);
  
  homeSpan.begin(Category::AirPurifiers,"HomeSpan Purifier");

  new SpanAccessory();                                  
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      
  new SpanAccessory();                                  
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("HEPA Purifier");
                    
    new Purifier;
}

////////////////////////////////////////////////////////////////////////

void loop(){ 
  homeSpan.poll();  
}

////////////////////////////////////////////////////////////////////////
