# HomeSpanReferenceSketches

全面的*参考草图*展示了一些更复杂的 HomeKit 服务。这些草图使用 **[HomeSpan HomeKit 库](https://github.com/HomeSpan/HomeSpan)** 构建，旨在在 Arduino IDE 下的 ESP32 设备上运行。有关详细信息，请参阅 [HomeSpan 中文]([https://github.com/HomeSpan/HomeSpan](https://github.com/abackup/HomeSpan-Chinese/tree/master))。

以下参考草图已在 **iOS 16.5** 下测试（见下文注释 2）：

* **[恒温器](Thermostat/Thermostat.ino)**
  * 实现完整的 Homekit 恒温器，提供加热/冷却/自动/关闭模式
  * 包含“模拟”温度传感器，允许您通过串行监视器更改*当前*温度，以观察恒温器在不同模式下的响应情况
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
  * 包括运行“预定程序”（通过串行监视器启动/停止）的功能，使每个*启用*的喷头根据其特定持续时间依次打开/关闭。从一个喷头切换到另一个喷头时，计划会短暂暂停，以考虑喷头速度（在实际系统中，这有助于避免压力突然下降）
  * 注意：用于选择每个喷头持续时间的“家庭”应用下拉菜单仅包含苹果公司确定的固定数量的选项（最短时间为 5 分钟）。Eve 应用提供更精细的选择。您还可以使用任意秒数（从 1 到 3600）直接在草图中设置持续时间，即使这些时间与“家庭”应用中显示的“允许”选项不匹配。为了便于说明，该草图配置为将每个值的头部持续时间初始化为 20 秒

* **[电池检查](BatteryCheck/BatteryCheck.ino)**
  * 实现一个简单的开/关 LED 和电池服务，以检查电池电量、充电状态和低电量警告
  * 包括一个独立的类，用于在使用 [Adafruit Huzzah32 Feather Board](https://www.adafruit.com/product/3405) 时测量 LiPo 电池电压和充电状态

* **[加湿器/除湿器](Humidifier-Dehydrated/Humidifier-Dehydrated.ino)**
  * 实现完整的 Homekit 加湿器/除湿器，提供加湿/除湿/自动/关闭模式
  * 包括一个“模拟”湿度传感器，允许您通过串行监视器更改*当前*湿度，以观察加湿器/除湿器在不同模式下的响应情况
  * 包括可选水位的存根代码，风扇转速和摆动模式
  * 展示如何将允许的模式限制为仅加湿或仅除湿

### 结束注释

1. 这些草图旨在演示各种 HomeKit 服务如何与“家庭”应用实际配合使用。它们不包含与实际硬件（如炉子、水阀等）交互的代码。相反，当发生“模拟”活动（如打开阀门）时，代码会向串行监控报告消息。要与现实世界的应用程序交互，您需要在每个草图的相应部分添加自己的代码。

1. 苹果公司经常在发布新版本的操作系统时更改“家庭”应用界面和底层 HomeKit 架构。这有时会导致特定功能改变其操作方式、显示方式，甚至是否继续运行。因此，上述草图的某些方面可能会或可能不会在苹果 iOS 的未来版本中按预期工作。苹果公司大概会将这些变化告知拥有商业许可的 HomeKit 产品制造商，但自 2019 年发布第 2 版以来，苹果公司尚未更新其非商业 HAP 文档（HomeSpan 使用）。显然，HAP-R2 文档中列出的某些特性不再按指示发挥作用。因此，确保草图继续工作的唯一方法是每当苹果公司发布新版本的 iOS 时进行测试和试验。
