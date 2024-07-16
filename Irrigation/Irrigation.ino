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
//           HomeSpan 参考草图：灌溉服务              //
//                                                   //
///////////////////////////////////////////////////////
 
#include "HomeSpan.h"

#define  HEAD_DURATION    20       // 每个 Head 保持打开的默认持续时间（以秒为单位）（可以在 Home App 中为每个 Head 配置）
#define  HEAD_SPEED       5000     // 阀门打开/关闭以及水完全流动或停止流动所需的时间（以毫秒为单位）

////////////////////////////////////////////////////////////////////////

struct Head : Service::Valve {

  SpanCharacteristic *active=new Characteristic::Active(0);
  SpanCharacteristic *inUse=new Characteristic::InUse(0);
  SpanCharacteristic *enabled = new Characteristic::IsConfigured(1,true);
  SpanCharacteristic *setDuration = new Characteristic::SetDuration(HEAD_DURATION);
  SpanCharacteristic *remainingDuration = new Characteristic::RemainingDuration(0);
  SpanCharacteristic *name;

  Head(const char *headName) : Service::Valve() {
    new Characteristic::ValveType(1);                           // 设置阀门类型 = 灌溉
    name=new Characteristic::ConfiguredName(headName,true);     
    enabled->addPerms(PW);                                      // 将“PW”添加到 IsConfigured 特性可以启用/禁用阀门
  }

  boolean update() override {
    
    if(enabled->updated()){
      if(enabled->getNewVal()){
        Serial.printf("%s value ENABLED\n",name->getString());
      } else {
        Serial.printf("%s value DISABLED\n",name->getString());          
        if(active->getVal()){
          active->setVal(0);
          remainingDuration->setVal(0);
          Serial.printf("%s is CLOSING\n",name->getString());          
        }
      }
    }

    if(active->updated()){
      if(active->getNewVal()){
        Serial.printf("%s valve is OPENING\n",name->getString());
        remainingDuration->setVal(setDuration->getVal());
      } else {
        Serial.printf("%s valve is CLOSING\n",name->getString());
        remainingDuration->setVal(0);
      }
    }
    
    return(true);
  }

  void loop() override {
    if(active->getVal()){
      int remainingTime=setDuration->getVal()-active->timeVal()/1000;
         
      if(remainingTime<=0){
        Serial.printf("%s valve is CLOSING (%d-second timer is complete)\n",name->getString(),setDuration->getVal());
        active->setVal(0);
        remainingDuration->setVal(0);
      } else

      if(remainingTime<remainingDuration->getVal()){
        remainingDuration->setVal(remainingTime,false);
      }
    }

    // 下面我们模拟需要一些固定时间来打开/关闭的阀门，以便 InUse 中的变化滞后于 Active 中的变化。
    // 当最初打开一个值时，Home App 将通过显示“等待...”来准确反映这种中间状态。
    
    if(active->timeVal()>HEAD_SPEED && active->getVal()!=inUse->getVal()){
      inUse->setVal(active->getVal());
      Serial.printf("%s value is %s\n",name->getString(),inUse->getVal()?"OPEN":"CLOSED");
    }
  }

};

////////////////////////////////////////////////////////////////////////

struct Sprinkler : Service::IrrigationSystem {

  // 在此配置中，我们将一个或多个主管（即阀门服务）链接到灌溉服务，而不是将灌溉服务作为具有未链接阀门的独立服务。
  // 这意味着 Home App 将不会为灌溉服务显示单独的“主”控件，而是根据一个或多个值是否处于活动状态来自动确定系统是否处于活动状态。

  SpanCharacteristic *programMode=new Characteristic::ProgramMode(0);   // HomeKit 需要此特性，但它仅用于在 Home App 中显示
  SpanCharacteristic *active=new Characteristic::Active(0);             // 虽然在此配置中 Home App 不会显示“主”控件，但仍然需要 Active Characteristic
  SpanCharacteristic *inUse=new Characteristic::InUse(0);               // 虽然在此配置中 Home App 不会显示“主”控件，但仍然需要 InUse Characteristic
  
  vector<Head *> heads;                                                 // 可选：用于存储用于运行预定程序的链接头列表的向量
  vector<Head *>::iterator currentHead;                                 // 可选：指向预定程序中的当前头的指针

  Sprinkler(uint8_t numHeads) : Service::IrrigationSystem() {
    
    for(int i=1;i<=numHeads;i++){              // 创建喷头（阀门）并将每个喷头链接到洒水器对象
      char name[16];
      sprintf(name,"Head-%d",i);               // 每个头部都有唯一名称，可在 Home App 中更改
      addLink(new Head(name));
    }
    
    for(auto s : getLinks())                  // 可选：将每个链接的头部添加到存储向量中，以便于在下面轻松访问
      heads.push_back((Head *)s);
      
    new SpanUserCommand('p', "- starts/stops scheduled program",startProgram,this);     // 可选：允许用户启动计划程序，按顺序打开每个启用的 Head
  }

  static void startProgram(const char *buf, void *arg){     // 可选：启动预定程序
    
    Sprinkler *sprinkler=(Sprinkler *)arg;                      // 将第二个参数重新转换为 Sprinkler
        
    for(auto s : sprinkler->getLinks()) {                       // 循环遍历所有链接的服务
      Head *head = (Head *)s;                                   // 重塑与头部相关的服务
      if(head->enabled->getVal() && head->active->getVal())     // 如果 Head 已启用且处于活动状态
        head->active->setVal(0);                                // 关掉 head
    }

    sprinkler->currentHead=sprinkler->heads.begin();                       // 在预定程序中重置当前头
    sprinkler->active->setVal(0);                                          // 将洒水器活动设置为 false
    sprinkler->programMode->setVal(!sprinkler->programMode->getVal());     // 切换程序模式

    Serial.printf("Scheduled program: %s\n",sprinkler->programMode->getVal()?"STARTED":"STOPPED");
  }

  void loop(){                                              // 可选：仅需要支持预定程序的运行
     
    if(!programMode->getVal())      // 程序模式未启动
      return;

    if(currentHead==heads.end()){
      Serial.printf("Scheduled program: COMPLETED\n");
      programMode->setVal(0);
      active->setVal(0);
      return;
    }

    if(!(*currentHead)->enabled->getVal()){      // 当前 Head 未启用
      currentHead++;                             // 晋级至下一任 Head 
      return;
    }

    if((*currentHead)->active->getVal()){        // 当前负责人处于活动状态
      if(!active->getVal()){                     // 如果洒水器活动仍然为假（因为用户手动打开了喷头）...
        active->setVal(1);                       // ...将洒水器活动设置为 true
        Serial.printf("Scheduled program: %s is ALREADY OPEN\n",(*currentHead)->name->getString());
      }
      return;
      
    } else if((*currentHead)->inUse->getVal()){  // 当前负责人未处于活动状态但仍在使用中
      return;
    }
    
    if(!active->getVal()){                       // 当前喷头未处于活动状态，也未处于使用状态，并且洒水器活动状态为假......
      active->setVal(1);                         // ...将洒水器 Active 设置为 true 并打开头部
      (*currentHead)->active->setVal(1);
      (*currentHead)->remainingDuration->setVal((*currentHead)->setDuration->getVal());
      Serial.printf("Scheduled program: %s is OPENING\n",(*currentHead)->name->getString());
      
    } else if(!(*currentHead)->inUse->getVal()){  // 等待当前水头停止流动，然后移至下一个水头
      active->setVal(0,false);
      currentHead++;      
    }
    
  } // loop()

};

////////////////////////////////////////////////////////////////////////

void setup() {

  Serial.begin(115200);
  
  homeSpan.begin(Category::Sprinklers,"HomeSpan Sprinklers");

  new SpanAccessory();                                  
    new Service::AccessoryInformation();  
      new Characteristic::Identify();                           
                   
    new Sprinkler(4);       // 创建 4 头洒水器
}

////////////////////////////////////////////////////////////////////////

void loop(){ 
  homeSpan.poll();  
}

////////////////////////////////////////////////////////////////////////
