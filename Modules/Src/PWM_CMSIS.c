
#include <PWM_CMSIS.h>

extern uint32_t SystemCoreClock;

void GPIO_PD12_PWM_Init(void){
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;               // GPIOD clock enable
    (void)RCC->AHB1ENR;                                // Okuma ile senkronizasyon

    GPIOD->MODER &= ~(0x3 << (12 * 2));                // PD12 mod bitlerini temizle
    GPIOD->MODER |=  (0x2 << (12 * 2));                // PD12 = Alternatif fonksiyon (10)

    GPIOD->OTYPER &= ~(1 << 12);                       // Push-pull çıkış
    GPIOD->OSPEEDR |=  (0x3 << (12 * 2));              // Çok yüksek hız
    GPIOD->PUPDR &= ~(0x3 << (12 * 2));                // Pull-up / Pull-down yok

    GPIOD->AFR[1] &= ~(0xF << ((12 - 8) * 4));          // AFR[1] içinde PD12 AF bits temizle
    GPIOD->AFR[1] |=  (0x2 << ((12 - 8) * 4));          // PD12 AF2 (TIM4_CH1)
}

/*

STM32’de GPIO Alternate Function (AFR) register pinin alternatif fonksiyonunu seçmek için kullanılır. 
Bir GPIO pini, sadece giriş/çıkış değil; aynı zamanda USART, SPI, I2C, TIM, ADC trigger gibi çevresel birimlere de yönlendirilebilir. 
İşte AFR bunun seçimini yapar.

Temel Mantık
    Her GPIO pininin AF numarası (AF0–AF15) vardır.
    Hangi çevresel birim hangi AF numarasına denk geliyor, bu mikrodenetleyicinin datasheet veya reference manual’ındaki tabloda belirtilir.
    AFR register’ı iki parçadan oluşur:
        Pin 0–7 için → AFRL kullanılır.
        Pin 8–15 için → AFRH kullanılır.
        GPIOx_AFRL → pin 0–7 için (her pin 4 bit)
        GPIOx_AFRH → pin 8–15 için (her pin 4 bit)

GPIOx->AFR[] aslında bir dizi (array) gibi tanımlanmış iki register’dır:
    AFR[0] = GPIOx_AFRL → Pin 0–7 için (Low)  
    AFR[1] = GPIOx_AFRH → Pin 8–15 için (High)

Yani:
    Pin numarası 0–7 ise → AFR[0] (AFRL) kullanılır 
    Pin numarası 8–15 ise → AFR[1] (AFRH) kullanılır
    Her pin için 4 bit ayrılmıştır.

Örnek:
    PA2 (USART2_TX, AF7) → pin 2 → AFR[0]’ın [11:8] bitleri
    PA9 (USART1_TX, AF7) → pin 9 → AFR[1]’in [7:4] bitleri

    GPIOx->AFR[0] |= (AF_numarası << (pin * 4));
    GPIOx->AFR[1] |= (AF_numarası << ((pin - 8) * 4));
    
*/

void TIM4_PWM_Init(uint32_t freq_hz, uint32_t duty){
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;                // TIM4 clock enable
    (void)RCC->APB1ENR;

    uint32_t timer_clk = SystemCoreClock / 2;          // APB1 prescaler = 4 → Timer clk = 84 MHz
    uint32_t psc = 0;                                  // Prescaler = 0 → 84 MHz doğrudan
    uint32_t arr = (timer_clk / freq_hz) - 1;          // ARR = (84 MHz / freq) - 1
    uint32_t ccr = (uint32_t)((arr + 1) * duty);       // CCR = ARR * duty

    TIM4->PSC = psc;                                   // Prescaler ayarla
    TIM4->ARR = arr;                                   // Auto-reload değeri
    TIM4->CCR1 = ccr;                                  // Capture/Compare register (duty)

    TIM4->CCMR1 &= ~(TIM_CCMR1_OC1M);                  // CH1 mod bitlerini temizle
    TIM4->CCMR1 |= (6 << TIM_CCMR1_OC1M_Pos);          // OC1M = 110: PWM mode 1
    TIM4->CCMR1 |= TIM_CCMR1_OC1PE;                    // Preload enable (ARR güncelleme güvenli)

    TIM4->CCER |= TIM_CCER_CC1E;                       // CH1 output enable
    TIM4->CR1 |= TIM_CR1_ARPE;                         // ARR preload enable
    TIM4->EGR |= TIM_EGR_UG;                           // Update event (PSC/ARR/CCR hemen yükle)
    TIM4->CR1 |= TIM_CR1_CEN;                          // Timer enable
}

void PWM_SetDutyCycle(uint32_t duty) {
    uint32_t arr = TIM4->ARR;                                     // ARR değerini al
    TIM4->CCR1 = (duty * (arr + 1)) / 100;                        // Yeni duty uygula
}

