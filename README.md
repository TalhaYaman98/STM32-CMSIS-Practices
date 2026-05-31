# STM32 CMSIS Examples

[![STM32F4](https://img.shields.io/badge/STM32-F4%20Discovery-orange.svg)](https://www.st.com/en/evaluation-tools/stm32f4discovery.html)
[![CMSIS](https://img.shields.io/badge/CMSIS-Core%20%26%20Device-green.svg)](https://arm-software.github.io/CMSIS_5/Core/html/index.html)

Bu repository, STM32 mikrodenetleyiciler ile **HAL kütüphanesine bağımlılığı minimuma indirerek**, doğrudan **CMSIS (Cortex Microcontroller Software Interface Standard)** tabanlı programlama örneklerini içerir.

## 🎯 Amaç

STM32 programlamaya yeni başlayanların donanımın çalışma mantığını **en temel seviyede** öğrenmesini sağlamak:

- ✅ **Donanım kayıt seviyesinde** nasıl çalıştığını anlamak
- ✅ HAL gibi **yüksek seviyeli kütüphanelere bağımlılığı** azaltmak
- ✅ **Gerçek donanım kontrolü** mantığını kavramak
- ✅ **Register manipülasyonu** ile programlama yapmayı öğrenmek

## 📚 Mevcut Örnekler

| Konu | Dosyalar | Açıklama | Durum |
|------|----------|----------|-------|
| **Clock** | `Clock_CMSIS.c/h` | HSE + PLL ile 168 MHz sistem saati | ✅ Hazır |
| **GPIO** | `GPIO_Interrupt_CMSIS.c/h` | Dijital giriş/çıkış, LED kontrolü, buton okuma | ✅ Hazır |
| **EXTI** | `GPIO_Interrupt_CMSIS.c/h` | Harici kesme işlemleri, interrupt handling | ✅ Hazır |
| **ADC** | `ADC_CMSIS.c/h` | Analog-Dijital dönüşüm, PA0 kanal okuma | ✅ Hazır |
| **DAC** | `DAC_CMSIS.c/h` | Dijital-Analog dönüşüm, analog sinyal üretimi | ✅ Hazır |
| **PWM** | `PWM_CMSIS.c/h` | TIM4 ile darbe genişlik modülasyonu | ✅ Hazır |
| **Timer** | `Timer_CMSIS.c/h` | TIM2 ile 1 Hz güncelleme kesmesi | ✅ Hazır |
| **UART** | `UART_CMSIS.c/h` | USART2 seri haberleşme, TX/RX | ✅ Hazır |
| **I2C** | `I2C_CMSIS.c/h` | I2C1 master write/read (tek ve çok byte) | ✅ Hazır |
| **SPI** | `SPI_CMSIS.c/h` | SPI1 master full-duplex TX/RX (PA4–PA7) | ✅ Hazır |
| **SysTick** | — | Sistem zamanlayıcısı, ms/us gecikme fonksiyonları | 🔜 Yakında |
| **DMA** | — | Direct Memory Access, UART ve ADC transferleri | 🔜 Yakında |
| **Watchdog** | — | IWDG / WWDG bağımsız ve pencere watchdog | 🔜 Yakında |
| **RTC** | — | Gerçek zamanlı saat, alarm ve zaman damgası | 🔜 Yakında |
| **Flash** | — | Dahili Flash okuma/yazma, sector silme | 🔜 Yakında |
| **Low Power** | — | Sleep, Stop ve Standby güç tasarrufu modları | 🔜 Yakında |
| **CAN** | — | Controller Area Network haberleşme protokolü | 🔜 Yakında |

## 🗂️ Proje Yapısı

```
STM32-CMSIS-Examples/
├── main.c                        # Ana program girişi
├── HeaderForAll.h                # Modül seçim başlığı (merkezi include)
├── Clock_CMSIS.c / .h            # Sistem saati (HSE + PLL, 168 MHz)
├── ADC_CMSIS.c / .h              # ADC — PA0 analog okuma
├── DAC_CMSIS.c / .h              # DAC — PA4 analog çıkış
├── PWM_CMSIS.c / .h              # PWM — TIM4 CH1 (PD12)
├── Timer_CMSIS.c / .h            # Timer — TIM2 1 Hz IRQ (PD12 LED)
├── UART_CMSIS.c / .h             # UART — USART2 (PA2/PA3)
├── GPIO_Interrupt_CMSIS.c / .h   # GPIO + EXTI0 (PA0 buton, PD12 LED)
├── I2C_CMSIS.c / .h              # I2C1 master (PB6/PB7)
└── SPI_CMSIS.c / .h              # SPI1 master full-duplex (PA4/PA5/PA6/PA7)
```

### HeaderForAll.h — Modül Seçimi

Tüm modüller `HeaderForAll.h` içinden merkezi olarak yönetilir. Kullanmak istediğiniz modülü `1` yapın, diğerlerini `0` bırakın:

```c
#define ADC_CMSIS             0
#define DAC_CMSIS             0
#define PWM_CMSIS             0
#define GPIO_Interrupt_CMSIS  0
#define TIMER_CMSIS           0
#define UART_CMSIS            0
#define I2C_CMSIS             0
#define SPI_CMSIS             0
```

`main.c` içindeki başlatma ve döngü kodları bu makrolara göre koşullu derleme (`#if`) ile aktif hale gelir. Clock modülü her zaman dahildir.

## 📌 Pin Haritası

| Modül | Pin | Fonksiyon |
|-------|-----|-----------|
| ADC | PA0 | Analog giriş (Kanal 0) |
| DAC | PA4 | Analog çıkış (Kanal 1) |
| PWM | PD12 | TIM4 CH1 PWM çıkışı |
| Timer | PD12 | LED (1 Hz toggle) |
| UART TX | PA2 | USART2 TX (AF7) |
| UART RX | PA3 | USART2 RX (AF7) |
| I2C SCL | PB6 | I2C1 SCL (AF4) |
| I2C SDA | PB7 | I2C1 SDA (AF4) |
| EXTI | PA0 | Buton girişi (rising edge) |
| LED | PD12 | Çıkış (EXTI / Timer) |
| SPI CS | PA4 | SPI1 Chip Select (Software) |
| SPI SCK | PA5 | SPI1 Saat (AF5) |
| SPI MISO | PA6 | SPI1 Master In Slave Out (AF5) |
| SPI MOSI | PA7 | SPI1 Master Out Slave In (AF5) |

## ⏱️ Saat Konfigürasyonu

Tüm örnekler aşağıdaki saat hiyerarşisini kullanır:

| Bus | Frekans | Notlar |
|-----|---------|--------|
| SYSCLK | 168 MHz | HSE (8 MHz) + PLL |
| AHB (HCLK) | 168 MHz | GPIO, DMA |
| APB2 (PCLK2) | 84 MHz | USART1, ADC, SPI1, TIM1 |
| APB1 (PCLK1) | 42 MHz | I2C, USART2, TIM2/4 |
| Timer clock (APB1 × 2) | 84 MHz | TIM2, TIM4 |

## ⚠️ Önemli Notlar

- Bu örnekler **eğitim amaçlıdır** ve ticari projelerde kullanılmadan önce test edilmelidir
- **Register seviyesinde** programlama yaptığımız için dikkatli olmak gerekir
- `main.c` HAL ile oluşturulmuş bir proje iskeletini temel alır; çevresel birim başlatmaları CMSIS ile yapılmaktadır
- Her modül **bağımsız** çalışacak şekilde tasarlanmıştır
- **STM32F4 Discovery** kartı için optimize edilmiştir
- SPI modülünde PA4 hem DAC çıkışı hem CS pini olarak atanmıştır; DAC ve SPI aynı anda kullanılmamalıdır

## 📚 Faydalı Kaynaklar

- [STM32F4 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0090-stm32f405415-stm32f407417-stm32f427437-and-stm32f429439-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [STM32F407 Datasheet](https://www.st.com/resource/en/datasheet/stm32f407vg.pdf)
- [CMSIS Documentation](https://arm-software.github.io/CMSIS_5/Core/html/index.html)
- [STM32 Programming Manual](https://www.st.com/resource/en/programming_manual/pm0214-stm32-cortexm4-mcus-and-mpus-programming-manual-stmicroelectronics.pdf)

---

⭐ Bu projeyi beğendiyseniz **star** vermeyi unutmayın!

🐛 Hata bulursanız veya öneriniz varsa **issue** açmaktan çekinmeyin.

💡 Yeni örnek önerilerinizi **discussions** bölümünde paylaşabilirsiniz.
