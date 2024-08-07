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
//          HomeSpan 参考草图：水龙头服务             //
//                                                   //
///////////////////////////////////////////////////////

#include "HomeSpan.h" 

////////////////////////////////////////////////////////////////////////

struct Sprayer : Service::Valve {

  SpanCharacteristic *active=new Characteristic::Active(0);
  SpanCharacteristic *inUse=new Characteristic::InUse(0);
  SpanCharacteristic *enabled = new Characteristic::IsConfigured(1,true);
  SpanCharacteristic *name;

  Sprayer(const char *sprayerName) : Service::Valve() {
    new Characteristic::ValveType(2);                               // 设置阀门类型 = 淋浴喷头
    name=new Characteristic::ConfiguredName(sprayerName,true);      // 此特性是为电视服务引入的，但在这里运行良好
    enabled->addPerms(PW);                                          // 将“PW”添加到 IsConfigured 特性可以启用/禁用阀门
  }

  boolean update() override {
    
    if(enabled->updated()){
      if(enabled->getNewVal()){
        Serial.printf("Sprayer '%s' enabled\n",name->getString());
      } else {
        Serial.printf("Sprayer '%s' disabled\n",name->getString());          
        if(active->getVal()){
          active->setVal(0);
          Serial.printf("Sprayer '%s' valve is closing\n",name->getString());          
        }
      }
    }

    if(active->updated()){
      if(active->getNewVal())
        Serial.printf("Sprayer '%s' valve is opening\n",name->getString());
      else
        Serial.printf("Sprayer '%s' valve is closing\n",name->getString());
    }
    
    return(true);
  }

};

////////////////////////////////////////////////////////////////////////

struct Shower : Service::Faucet {

  SpanCharacteristic *active=new Characteristic::Active(0);
  
  boolean update() override {
    if(active->getNewVal())
      Serial.printf("Shower is turning ON\n");
    else
      Serial.printf("Shower is turning OFF\n");

    return(true);
  }

  void loop() override {
    for(auto s : getLinks()){
      Sprayer *sprayer=(Sprayer *)s;
      boolean shouldBeOn=active->getVal() && sprayer->enabled->getVal() && sprayer->active->getVal();
      
      if(shouldBeOn && !sprayer->inUse->getVal()){
        Serial.printf("Sprayer '%s' is turning ON\n",sprayer->name->getString());
        sprayer->inUse->setVal(1);
      } else
      
      if(!shouldBeOn && sprayer->inUse->getVal()){
        Serial.printf("Sprayer '%s' is turning OFF\n",sprayer->name->getString());
        sprayer->inUse->setVal(0);
      }
      
    }
    
  }
   
};

////////////////////////////////////////////////////////////////////////

void setup() {

  Serial.begin(115200);
  
  homeSpan.begin(Category::ShowerSystems,"HomeSpan Faucet");

  new SpanAccessory();                                  
    new Service::AccessoryInformation();
      new Characteristic::Identify();                           
       
    (new Shower())
      ->addLink(new Sprayer("Rain Sprayer"))
      ->addLink(new Sprayer("Hand Sprayer"))
      ->addLink(new Sprayer("Jet 1"))
      ->addLink(new Sprayer("Jet 2"))
      ;

} // end of setup()

////////////////////////////////////////////////////////////////////////

void loop(){  
  homeSpan.poll(); 
}

////////////////////////////////////////////////////////////////////////
