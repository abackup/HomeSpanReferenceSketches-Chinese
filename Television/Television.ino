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

 ///////////////////////////////////////////////////////
//                                                   //
//             HomeSpan 参考草图：电视服务            //
//                                                   //
///////////////////////////////////////////////////////

#define REQUIRED VERSION(1,9,1)

#include "HomeSpan.h"

struct sourceData_t {
  int ID;
  const char *name;
};

#define   NUM_SOURCES     5

sourceData_t sourceData[NUM_SOURCES]={
  30,"HDMI-1",
  40,"HDMI-2",
  10,"Component-1",
  25,"Component-2",
  15,"DVI"
};

////////////////////////////////////////////////////////////////////////
/////           TvInput 服务 - 由下面的电视服务使用               ///////
////////////////////////////////////////////////////////////////////////

struct TvInput : Service::InputSource {

  SpanCharacteristic *sourceName;                                    // 输入源名称
  SpanCharacteristic *sourceID;                                      // 输入源ID
  Characteristic::IsConfigured configured{1,true};                   // 表示是否配置了输入源
  Characteristic::CurrentVisibilityState currentVis{0,true};         // 当前输入源可见性（0=VISIBLE）
  Characteristic::TargetVisibilityState targetVis{0,true};           // 目标输入源可见性

  static int numSources;                                             // 输入源数量
  
  TvInput(int id, const char *name) : Service::InputSource() {
    sourceName = new Characteristic::ConfiguredName(name,true);
    sourceID = new Characteristic::Identifier(id);
    Serial.printf("Creating Input Source %d: %s\n",sourceID->getVal(),sourceName->getString());
  }

  boolean update() override {

    if(targetVis.updated()){
      currentVis.setVal(targetVis.getNewVal());
      Serial.printf("Input Source %s is now %s\n",sourceName->getString(),currentVis.getVal()?"HIDDEN":"VISIBLE");
    }
    
    return(true);
  }
  
};

////////////////////////////////////////////////////////////////////////
/////           TvSpeaker 服务 - 由下面的电视服务使用               /////
////////////////////////////////////////////////////////////////////////

struct TvSpeaker : Service::TelevisionSpeaker {

  Characteristic::VolumeSelector volumeChange;

  TvSpeaker() : Service::TelevisionSpeaker() {
    new Characteristic::VolumeControlType(3);
    Serial.printf("Adding Volume Control\n");
  }

  boolean update() override {
    if(volumeChange.updated())
      Serial.printf("Volume %s\n",volumeChange.getNewVal()?"DECREASE":"INCREASE");

    return(true);
  }

};

////////////////////////////////////////////////////////////////////////
/////               HomeSpanTV Television Service                ///////
////////////////////////////////////////////////////////////////////////

struct HomeSpanTV : Service::Television {

  Characteristic::Active power{0,true};                    // 电视电源
  Characteristic::ActiveIdentifier inputSource{1,true};    // 当前电视输入源
  Characteristic::RemoteKey remoteKey;                     // 用于接收来自远程控制小部件的按钮按下操作
  Characteristic::PowerModeSelection settingsKey;          // 在选择屏幕中添加“查看电视设置”选项
  
  SpanCharacteristic *tvName;                                   // 电视名称（将在下面的构造函数中实例化）
  Characteristic::DisplayOrder displayOrder{NULL_TLV,true};     // 设置输入源在 Home App 中的显示顺序

  HomeSpanTV(const char *name) : Service::Television() {
    tvName = new Characteristic::ConfiguredName(name,true);
    
    Serial.printf("Creating Television Service '%s'\n",tvName->getString());

    TLV8 orderTLV;                                         // 创建一个临时的 TLV8 对象来存储输入源在 Home App 中的显示顺序

    for(int i=0;i<NUM_SOURCES;i++){
      orderTLV.add(1,sourceData[i].ID);                    // 将输入源的 ID 添加到用于 displayOrder 的 TLV8 记录中
      orderTLV.add(0);
      
      addLink(new TvInput(sourceData[i].ID,sourceData[i].name));    // 添加此输入源的链接
    }

    displayOrder.setTLV(orderTLV);                         // 使用已完成的 TLV8 记录更新 displayOrder

    addLink(new TvSpeaker());                              // 添加电视扬声器的链接
  }

  boolean update() override {

    if(power.updated()){
      Serial.printf("Set TV Power to: %s\n",power.getNewVal()?"ON":"OFF");
    }

    if(inputSource.updated()){                                  // 请求新的输入源
      for(auto src : getLinks<TvInput *>("InputSource"))        // 循环遍历所有链接的 TvInput 源并找到具有匹配 ID 的源
        if(inputSource.getNewVal()==src->sourceID->getVal())
          Serial.printf("Set to Input Source %d: %s\n",src->sourceID->getVal(),src->sourceName->getString());          
    }

    // 为了好玩，使用“查看电视设置”来触发 HomeSpan 按字母顺序重新排列输入源
    
    if(settingsKey.updated()){
      Serial.printf("Received request to \"View TV Settings\"\n");
      
      // 为了好玩，使用“查看电视设置”来触发 HomeSpan 按字母顺序重新排列输入源
      
      Serial.printf("Alphabetizing Input Sources...\n");
      auto inputs = getLinks<TvInput *>("InputSource");         // 创建链接的输入源向量的副本，以便可以按字母顺序排序
                    
      std::sort(inputs.begin(),inputs.end(),[](TvInput *i, TvInput *j)->boolean{return(strcmp(i->sourceName->getString(),j->sourceName->getString())<0);});

      TLV8 orderTLV;                                            // 创建一个临时的 TLV8 对象来存储输入源在 Home App 中的显示顺序

      for(auto src : inputs){
        orderTLV.add(1,src->sourceID->getVal());
        orderTLV.add(0);
      }

    displayOrder.setTLV(orderTLV);                              // 使用 TLV8 记录更新 displayOrder 特性
    }
    
    if(remoteKey.updated()){
      Serial.printf("Remote Control key pressed: ");
      switch(remoteKey.getNewVal()){
        case 4:
          Serial.printf("UP ARROW\n");
          break;
        case 5:
          Serial.printf("DOWN ARROW\n");
          break;
        case 6:
          Serial.printf("LEFT ARROW\n");
          break;
        case 7:
          Serial.printf("RIGHT ARROW\n");
          break;
        case 8:
          Serial.printf("SELECT\n");
          break;
        case 9:
          Serial.printf("BACK\n");
          break;
        case 11:
          Serial.printf("PLAY/PAUSE\n");
          break;
        case 15:
          Serial.printf("INFO\n");
          break;
        default:
          Serial.print("UNKNOWN KEY\n");
      }
    }

    return(true);
  }
};

////////////////////////////////////////////////////////////////////////

void setup() {
  
  Serial.begin(115200);

  homeSpan.enableWebLog(0);
  homeSpan.begin(Category::Television,"HomeSpan Television");

  new SpanAccessory();   
    new Service::AccessoryInformation(); 
      new Characteristic::Identify();
    new HomeSpanTV("Test TV");              // 实例化名称为“Test TV”的电视
}

///////////////////////////////

void loop() {
  homeSpan.poll();
}
