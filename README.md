# HomeSpanReferenceSketches

全面的*参考草图*展示了一些更复杂的 HomeKit 服务。这些草图使用 **[HomeSpan HomeKit 库](https://github.com/HomeSpan/HomeSpan)** 构建，旨在在 Arduino IDE 下的 ESP32 设备上运行。有关详细信息，请参阅 [HomeSpan 中文]([https://github.com/HomeSpan/HomeSpan](https://github.com/abackup/HomeSpan-Chinese/tree/master))。

以下参考草图已在 **iOS 16.5** 下测试（见下文注释 2）：

* **[恒温器](Thermostat/Thermostat.ino)**
  * 实现完整的 Homekit 恒温器，提供加热/冷却/自动/关闭模式
  * 包含“模拟”温度传感器，允许您通过串口监视器更改*当前*温度，以观察恒温器在不同模式下的响应情况
  * 包含用于监视和设置相对湿度的存根代码

* **[水龙头](Faucet/Faucet.ino)**
  * 实现多喷头淋浴系统
  * 提供对单个淋浴喷头的中央控制
  * 喷雾器使用链接的 HomeKit 阀门服务实现
  * 允许您在“家庭”应用中操作、启用/禁用和重命名每个淋浴喷头
  * 注意：“家庭”应用就此服务可能存在一些问题，有时会显示喷雾器已打开，但实际上并未打开

* **[灌溉系统](Irrigation/Irrigation.ino)**
  * 实现多头喷水系统
  * 提供对每个喷头的完全控制，包括从“家庭”应用中设置自动关闭时间的功能
  * 使用链接的 HomeKit 阀门服务实现喷头
  * 包括可配置的“喷头速度”设置，可模拟喷头打开/关闭时水实际开始/停止流动所需的滞后时间
  * 允许您从“家庭”应用中启用/禁用和重命名每个喷头
  * 包括运行“预定程序”（通过串口监视器启动/停止）的功能，使每个*启用*的喷头根据其特定持续时间依次打开/关闭。从一个喷头切换到另一个喷头时，计划会短暂暂停，以考虑喷头速度（在实际系统中，这有助于避免压力突然下降）
  * 注意：用于选择每个喷头持续时间的“家庭”应用下拉菜单仅包含苹果公司确定的固定数量的选项（最短时间为 5 分钟）。Eve 应用提供更精细的选择。您还可以使用任意秒数（从 1 到 3600）直接在草图中设置持续时间，即使这些时间与“家庭”应用中显示的“允许”选项不匹配。为了便于说明，该草图配置为将每个值的头部持续时间初始化为 20 秒

* **[电池检查](BatteryCheck/BatteryCheck.ino)**
  * 实现一个简单的开/关 LED 和电池服务，以检查电池电量、充电状态和低电量警告
  * 包括一个独立的类，用于在使用 [Adafruit Huzzah32 Feather Board](https://www.adafruit.com/product/3405) 时测量 LiPo 电池电压和充电状态

* **[加湿器/除湿器](Humidifier-Dehydrated/Humidifier-Dehydrated.ino)**
  * 实现完整的 Homekit 加湿器/除湿器，提供加湿/除湿/自动/关闭模式
  * 包括一个“模拟”湿度传感器，允许您通过串口监视器更改*当前*湿度，以观察加湿器/除湿器在不同模式下的响应情况
  * 包括可选水位的存根代码，风扇转速和摆动模式
  * 展示如何将允许的模式限制为仅加湿或仅除湿

### 结束注释

1. 这些草图旨在演示各种 HomeKit 服务如何与“家庭”应用实际配合使用。它们不包含与实际硬件（如炉子、水阀等）交互的代码。相反，当发生“模拟”活动（如打开阀门）时，代码会向串口监控报告消息。要与现实世界的应用程序交互，您需要在每个草图的相应部分添加自己的代码。

1. 苹果公司经常在发布新版本的操作系统时更改“家庭”应用界面和底层 HomeKit 架构。这有时会导致特定功能改变其操作方式、显示方式，甚至是否继续运行。因此，上述草图的某些方面可能会或可能不会在苹果 iOS 的未来版本中按预期工作。苹果公司大概会将这些变化告知拥有商业许可的 HomeKit 产品制造商，但自 2019 年发布第 2 版以来，苹果公司尚未更新其非商业 HAP 文档（HomeSpan 使用）。显然，HAP-R2 文档中列出的某些特性不再按指示发挥作用。因此，确保草图继续工作的唯一方法是每当苹果公司发布新版本的 iOS 时进行测试和试验。


# HomeSpanReferenceSketches

Comprehensive *Reference Sketches* showcasing some of the more complex HomeKit Services.  Built using the **[HomeSpan HomeKit Library](https://github.com/HomeSpan/HomeSpan)**, these sketches are designed to run on ESP32 devices under the Arduino IDE.  See [HomeSpan](https://github.com/HomeSpan/HomeSpan) for details.

The following References Sketches have been tested under **iOS 17** (see note 2 below):  

* **[Thermostat](Thermostat/Thermostat.ino)**
  * Implements a complete Homekit Thermostat providing heating/cooling/auto/off modes
  * Includes a "simulated" temperature sensor allowing you to change the *current* temperature via the Serial Monitor to observe how the Thermostat responds in different modes
  * Includes stub code for monitoring and setting relative humidity

* **[Faucet](Faucet/Faucet.ino)**
  * Implements a multi-sprayer Shower System
  * Provides for central control of individual Shower Sprayers
  * Sprayers are implemented using linked HomeKit Valve Services
  * Allows you to operate, enable/disable and rename each Shower Sprayer from within the Home App
  * Note: The Home App can be a bit buggy with regards to this Service and sometimes shows a Sprayer is turned on when it is not

* **[Irrigation System](Irrigation/Irrigation.ino)**
  * Implements a multi-headed Sprinkler System
  * Provides full control of each Sprinkler Head, including the ability to set auto-off times from within the Home App
  * Heads are implemented using linked HomeKit Valve Services
  * Includes a configurable "Head Speed" setting that simulates the lag time it takes for water to actually start/stop flowing when a Head is opening/closing
  * Allows you to enable/disable and rename each Sprinkler Head from within the Home App
  * Includes the ability to run a "scheduled program" (which you start/stop via the Serial Monitor) causing each *enabled* Head to sequentially open/close  based on its specific duration time.  The schedule briefly pauses when switching from one Head to another to account for the Head Speed (in a real system this helps avoid sudden pressure drops)
  * Note: the Home App drop-down menu for selecting the duration time of each Sprinkler Head includes only a fixed number of choices determined by Apple (with the minimum time being 5 minutes).  The Eve for HomeKit App provides for more granular choices.  You can also set the duration time directly in the sketch using any number of seconds, from 1 to 3600, even if those times do not match an "allowed" choice shown in the Home App.  For illustration purposes the sketch is configured to initialize the Head Duration time for each value to be 20 seconds

* **[Battery Check](BatteryCheck/BatteryCheck.ino)**
  * Implements a simple on/off LED with a Battery Service to check battery level, charging status, and low-battery warning
  * Includes a stand-alone class to measure LiPo battery voltage and charging status when using an [Adafruit Huzzah32 Feather Board](https://www.adafruit.com/product/3405)

* **[Humidifier/Dehumidifier](Humidifier-Dehumidifier/Humidifier-Dehumidifier.ino)**
  * Implements a complete Homekit Humidifier/Dehumidifier providing humidify/dehumidify/auto/off modes
  * Includes a "simulated" humidity sensor allowing you to change the *current* humidity via the Serial Monitor to observe how the Humidifier/Dehumidifier responds in different modes
  * Includes stub code for optional water level, fan rotation speed, and swing modes
  * Shows how to restrict allowed modes to Humidify-only or Dehumidify-only
 
* **[Television](Television/Television.ino)**
  * Implements an advanced Television control framework including:
    * Power On/Off
    * Input Source Selection, Enabling, and Renaming
    * Input Source Ordering (including automatic alphabetizing)
    * Volume Control (from iPhone Remote Control Widget)
    * Remote Keys (from iPhone Remote Control Widget)  
  
### End Notes

1. These sketches are designed to demonstrate how various HomeKit Services work in practice with the Home App.  They do not include code that interfaces with actual hardware, such as a furnace, water valve, etc.  Instead, the code outputs messages to the Serial Monitoring reporting when a "simulated" activity occurs (such as a valve being turned on).  To interface with real-world applicances you will need to add your own code in the appropriate sections of each sketch.

1. Apple frequently changes the Home App interface and underlying HomeKit architecture as it releases new versions of its operating system.  This sometimes causes specific functions to change the way they operate, how they are displayed, and even whether or not they continue to function at all.  As a result, aspects of the sketches above may, or may not, work as expected in future releases of Apple iOS.  Apple presumably informs manufactures of HomeKit products with commercial licenses of these changes, but Apple has not updated its non-commercial HAP documentation (which is used by HomeSpan) since version 2 was published in 2019.  It is already apparent that some Characteristics listed in the HAP-R2 documentation no longer function as indicated.  As a result, the only way to ensure sketches continue to work is by testing and experimentation whenever Apple releases new version of iOS.

