#include <CLOCK_CMSIS.h>

void Clock_Init(void) {
    // HSE başlat
    RCC->CR |= RCC_CR_HSEON;                          // HSE osilatörünü aktif et
    while (!(RCC->CR & RCC_CR_HSERDY));               // HSE hazır olana kadar bekle

    // PLL konfigürasyonu (HSE = 8 MHz, SYSCLK = 168 MHz)
    RCC->PLLCFGR = (8 << RCC_PLLCFGR_PLLM_Pos) |       // PLLM = 8
                   (336 << RCC_PLLCFGR_PLLN_Pos) |     // PLLN = 336
                   (0 << RCC_PLLCFGR_PLLP_Pos) |       // PLLP = 2 (00)
                   (RCC_PLLCFGR_PLLSRC_HSE) |          // PLL kaynağı = HSE
                   (7 << RCC_PLLCFGR_PLLQ_Pos);        // PLLQ = 7 (USB için 48 MHz)

    RCC->CR |= RCC_CR_PLLON;                           // PLL'i aktif et
    while (!(RCC->CR & RCC_CR_PLLRDY));                // PLL hazır olana kadar bekle

    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN |     // Flash önbellekleri aktif et
                 FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_5WS; // 5 wait state

    RCC->CFGR |= RCC_CFGR_SW_PLL;                      // PLL'i sistem clock olarak seç
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL); // PLL'e geçişi bekle
}

/*

Amaç STM32F407 mikrodenetleyicisini harici osilatör (HSE) ile çalıştırıp, PLL üzerinden 168 MHz sistem saat frekansı elde etmektir.

RCC->CR: 		Clock Control Register
RCC_CR_HSEON:   HSE osilatörü (harici kristal) aktif edilir. Discovery kartta bu 8 MHz kristaldir.
RCC_CR_HSERDY:  HSE osilatörünün hazır olup olmadığını bildirir (hazır = 1)

Bu ayarlarla HSE = 8 MHz iken PLL çıkışı = 168 MHz olur
	PLLM = 8: HSE frekansı 8 MHz olduğundan, 1 MHz'e ölçekleme yapılır.
	PLLN = 336: 1 MHz × 336 = 336 MHz VCO çıkışı
	PLLP = 2: Sisteme 336 / 2 = 168 MHz verilir.
	PLLSRC = HSE: PLL kaynak olarak harici kristali kullanır.
	PLLQ = 7: USB, SDIO gibi çevresel saatlerin oluşturulmasında kullanılır. (48 MHz: 336 / 7 ≈ 48 MHz)


RCC_CR_PLLON: PLL'i etkinleştirir
RCC_CR_PLLRDY: PLL kilitlenip çıkış sinyali üretmeye başladığında 1 olur. Hazır beklenmelidir.

Yüksek saat hızlarında flash belleğe erişim süresi kritik hale gelir
	LATENCY_5WS: 168 MHz’de en az 5 wait state gereklidir (bkz: STM32F4 datasheet).
	ICEN: Instruction Cache enable
	DCEN: Data Cache enable
	PRFTEN: Prefetch buffer enable
	Bu ayarlar, flash erişimini hızlandırır ve stabilite sağlar.

Prescaler ayarları sistem saatini AHB, APB1 ve APB2 bus’larına böler
	HB: Ana veri yolu. Genellikle en yüksek frekansta çalışır. (168 MHz)
	APB1: Düşük hızlı çevresel birimler (TIM2-5, USART2, I2C1 vs.) → max 42 MHz
	APB2: Yüksek hızlı çevresel birimler (TIM1, USART1, SPI1 vs.) → max 84 MHz
	Bu bölme işlemleri, çevresel donanımların güvenli çalışmasını sağlar.

Sistem saat kaynağı seçim biti
	00 = HSI (default)
	01 = HSE
	10 = PLL
	SW: Sistem saat kaynağı seçim biti
	SWS: Seçilen sistem saat kaynağının hangi olduğunu bildirir (feedback)
	PLL seçilir ve sistem çekirdeği, tüm işlemciler 168 MHz ile çalışmaya başlar.

Clock_Init() fonksiyonu çalıştıktan sonra SystemCoreClock değişkeni güncellenmediği için
bazı CMSIS tabanlı delay fonksiyonları yanlış çalışabilir. Gerekirse elle güncelleyin: SystemCoreClock = 168000000;

*/
