# STM32 CMSIS Examples

[![STM32F4](https://img.shields.io/badge/STM32-F4%20Discovery-orange.svg)](https://www.st.com/en/evaluation-tools/stm32f4discovery.html)
[![CMSIS](https://img.shields.io/badge/CMSIS-Core%20%26%20Device-green.svg)](https://arm-software.github.io/CMSIS_5/Core/html/index.html)

Bu repository, STM32 mikrodenetleyiciler ile **HAL kütüphanesi kullanmadan**, doğrudan **CMSIS (Cortex Microcontroller Software Interface Standard)** tabanlı programlama örneklerini içerir. 

## 🎯 Amaç

STM32 programlamaya yeni başlayanların donanımın çalışma mantığını **en temel seviyede** öğrenmesini sağlamak:

- ✅ **Donanım kayıt seviyesinde** nasıl çalıştığını anlamak
- ✅ HAL gibi **yüksek seviyeli kütüphanelere bağımlılığı** azaltmak
- ✅ **Gerçek donanım kontrolü** mantığını kavramak
- ✅ **Register manipülasyonu** ile programlama yapmayı öğrenmek

## 📚 Mevcut Örnekler

| Konu | Açıklama | Durum |
|------|----------|-------|
| **GPIO** | Dijital giriş/çıkış kontrolü, LED kontrolü, buton okuma | ✅ Hazır |
| **EXTI** | Harici kesme işlemleri, interrupt handling | ✅ Hazır |
| **ADC** | Analog-Dijital dönüşüm, sensör okuma | ✅ Hazır |
| **DAC** | Dijital-Analog dönüşüm, analog sinyal üretimi | ✅ Hazır |
| **PWM** | Darbe genişlik modülasyonu, motor kontrolü | ✅ Hazır |
| **UART** | Seri haberleşme protokolü | ✅ Hazır |
| **SPI** | Seri Peripheral Interface, ILI9341 TFT sürücüsü | ✅ Hazır |
| **I2C** | Inter-Integrated Circuit, master write/read | ✅ Hazır |
| **Timer** | Zamanlayıcı işlemleri, 1 Hz IRQ | ✅ Hazır |
| **SysTick** | Sistem zamanlayıcısı, ms/us gecikme fonksiyonları | 🔜 Yakında |
| **DMA** | Direct Memory Access, UART ve ADC transferleri | 🔜 Yakında |
| **Watchdog** | IWDG / WWDG bağımsız ve pencere watchdog | 🔜 Yakında |
| **RTC** | Gerçek zamanlı saat, alarm ve zaman damgası | 🔜 Yakında |
| **Flash** | Dahili Flash okuma/yazma, sector silme | 🔜 Yakında |
| **Low Power** | Sleep, Stop ve Standby güç tasarrufu modları | 🔜 Yakında |
| **CAN** | Controller Area Network haberleşme protokolü | 🔜 Yakında |

## 🛠️ Gereksinimler

### Donanım
- **STM32F4 Discovery Kit** (STM32F407VGT6)
- USB Kablo (programlama için)
- Breadboard ve jumper kablolar (opsiyonel)

### Yazılım
- **STM32CubeIDE** (önerilen) veya **Keil µVision**
- **ST-Link Utility** (debug için)
- **Git** (repository klonlama için)

## 🚀 Kurulum ve Çalıştırma

### 1. Repository'yi Klonlayın
```bash
git clone https://github.com/TalhaYaman98/STM32-CMSIS-Examples.git
cd STM32-CMSIS-Examples
```

### 2. STM32CubeIDE'de Açın
1. STM32CubeIDE'yi açın
2. `File -> Import -> Existing Projects into Workspace`
3. Klonladığınız klasörü seçin
4. İstediğiniz örnek projeyi seçin

### 3. CMSIS Kütüphanesini Yapılandırın
CMSIS kütüphanesini projelere dahil etmek için aşağıdaki adımları takip edin:

1. **Drivers klasörünü kopyalayın**
   - Repository'deki `Drivers` klasörünü projenizin kök dizinine kopyalayın

2. **Include Paths Ayarları**
   - Proje üzerine sağ tıklayın ve `Properties` seçin
   - `C/C++ BUILD -> Settings -> Tool settings -> MCU/MPU GCC Compiler -> Include paths`
   - Aşağıdaki yolları ekleyin:
     ```
     ../Drivers/CMSIS/Include
     ../Drivers/CMSIS/Device/ST/STM32F4xx/Include
     ```

3. **Preprocessor Defines Ayarları**
   - `C/C++ BUILD -> Settings -> Tool settings -> MCU/MPU GCC Compiler -> Preprocessor`
   - Aşağıdaki tanımlamaları ekleyin:
     ```
     STM32F407xx
     ARM_MATH_CM4
     ```

### 4. Derleyin ve Yükleyin
1. Projeyi seçin ve `Ctrl+B` ile derleyin
2. STM32F4 Discovery kartınızı USB ile bağlayın
3. `Run -> Debug` ile programı yükleyin ve çalıştırın

## 📚 Faydalı Kaynaklar

- [STM32F4 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0090-stm32f405415-stm32f407417-stm32f427437-and-stm32f429439-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [STM32F407 Datasheet](https://www.st.com/resource/en/datasheet/stm32f407vg.pdf)
- [CMSIS Documentation](https://arm-software.github.io/CMSIS_5/Core/html/index.html)
- [STM32 Programming Manual](https://www.st.com/resource/en/programming_manual/pm0214-stm32-cortexm4-mcus-and-mpus-programming-manual-stmicroelectronics.pdf)

## ⚠️ Önemli Notlar

- Bu örnekler **eğitim amaçlıdır** ve ticari projelerde kullanılmadan önce test edilmelidir
- **Register seviyesinde** programlama yaptığımız için dikkatli olmak gerekir
- Her örnek **standalone** çalışacak şekilde tasarlanmıştır
- **STM32F4 Discovery** kartı için optimize edilmiştir



⭐ Bu projeyi beğendiyseniz **star** vermeyi unutmayın!

🐛 Hata bulursanız veya öneriniz varsa **issue** açmaktan çekinmeyin.

💡 Yeni örnek önerilerinizi **discussions** bölümünde paylaşabilirsiniz.
