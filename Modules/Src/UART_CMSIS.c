#include <UART_CMSIS.h>

extern uint32_t SystemCoreClock;                        // Sistem saat frekansını tanımla (168MHz)

void GPIO_UART_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;                      // GPIOA saatini aktif et

    // PA2 ve PA3: Alternate Function (AF7 = USART2)
    GPIOA->MODER &= ~((3 << (2 * 2)) | (3 << (3 * 2)));       // PA2 ve PA3 mode bitlerini temizle
    GPIOA->MODER |= ((2 << (2 * 2)) | (2 << (3 * 2)));        // PA2 ve PA3'ü Alternate Function moduna al

    GPIOA->AFR[0] &= ~((0xF << (4 * 2)) | (0xF << (4 * 3)));  // PA2 ve PA3 AF bitlerini temizle
    GPIOA->AFR[0] |= ((7 << (4 * 2)) | (7 << (4 * 3)));       // PA2 ve PA3 için AF7 (USART2) seç
}

/*

1. GPIOA Saatinin Aktifleştirilmesi:
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
		RCC (Reset and Clock Control) register'ındaki AHB1ENR (AHB1 peripheral clock enable register) kullanılıyor
		GPIOA'nın saat sinyali aktif ediliyor
		Bu yapılmazsa GPIOA registerlarına erişilemez

2. Pin Modu Ayarları (PA2 ve PA3):
	GPIOA->MODER &= ~((3 << (2 * 2)) | (3 << (3 * 2)));
		MODER register'ında her pin için 2 bit kullanılır
		PA2 için: bit 4-5, PA3 için: bit 6-7
		3 << (2 * 2) = 3 << 4 = 11 binary (bit 4-5'i temizle)
		3 << (3 * 2) = 3 << 6 = 11 binary (bit 6-7'yi temizle)
		Bu satır önce ilgili bitleri sıfırlıyor

	GPIOA->MODER |= ((2 << (2 * 2)) | (2 << (3 * 2)));
		2 << 4 = 10 binary → PA2'yi Alternate Function moduna alıyor
		2 << 6 = 10 binary → PA3'ü Alternate Function moduna alıyor
		Mod seçenekleri: 00=Input, 01=Output, 10=AF, 11=Analog

3. Alternate Function Seçimi:
	GPIOA->AFR[0] &= ~((0xF << (4 * 2)) | (0xF << (4 * 3)));
		AFR[0] register'ı pin 0-7 için alternate function seçimi
		Her pin için 4 bit kullanılır
		PA2 için: bit 8-11, PA3 için: bit 12-15
		0xF = 1111 binary, önce ilgili bitleri temizliyor

	GPIOA->AFR[0] |= ((7 << (4 * 2)) | (7 << (4 * 3)));
		PA2 için: 7 << 8 → AF7 seçiliyor
		PA3 için: 7 << 12 → AF7 seçiliyor
		AF7 = USART1/2/3 fonksiyonu

*/

void USART2_Init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;                     // USART2 saatini aktif et

    // USART2 clock = APB1 = 42 MHz → 9600 baud
    USART2->BRR = 42000000 / 9600;                            // Baud rate hesapla ve ayarla (42MHz / 9600)

    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE;               // Transmit ve Receive aktif et
    USART2->CR1 |= USART_CR1_UE;                              // USART2'yi aktif et
}

/*

1. USART2 Saatinin Aktifleştirilmesi:
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
		USART2, APB1 bus'ında bağlı
		APB1ENR register'ında USART2EN biti aktif ediliyor
		Bu olmadan USART2 registerlarına erişilemez

2. Baud Rate Hesaplama ve Ayarlama:
	USART2->BRR = 42000000 / 9600;
		BRR (Baud Rate Register) baud rate'i belirler
		APB1 clock = 42MHz (Clock_Init'te ayarlandı)
		Formül: BRR = fCK / Baud Rate
		42,000,000 / 9600 = 4375 (ondalık)
		Bu değer BRR register'ına yazılır

3. Transmit ve Receive Aktifleştirme:
	USART2->CR1 |= USART_CR1_TE | USART_CR1_RE;
		CR1 (Control Register 1) USART kontrol ayarları
		TE (Transmitter Enable) biti: Veri gönderme aktif
		RE (Receiver Enable) biti: Veri alma aktif
		Bu bitler olmadan TX/RX çalışmaz

4. USART2'yi Aktifleştirme:
	USART2->CR1 |= USART_CR1_UE;
		UE (USART Enable) biti USART2'yi tamamen aktif eder
		Bu bit son olarak açılmalı
		Tüm konfigürasyon tamamlandıktan sonra yapılır

Varsayılan Ayarlar:
	8 bit veri (default)
	1 stop bit (default)
	Parity yok (default)
	Hardware flow control yok (default)

*/

void USART2_Send_Char(char c) {
    while (!(USART2->SR & USART_SR_TXE));                     // TX buffer boş olana kadar bekle
    USART2->DR = c;                                           // Karakteri data register'a yaz
}

char USART2_Read_Char(void) {
    while (!(USART2->SR & USART_SR_RXNE));                    // RX buffer dolu olana kadar bekle
    return USART2->DR;                                        // Data register'dan karakteri oku
}

void USART2_Send_String(const char *str) {
    while (*str) {                                            // String sonu kontrolü
        USART2_Send_Char(*str++);                             // Her karakteri sırayla gönder
    }
}

