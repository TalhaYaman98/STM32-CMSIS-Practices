#include "SPI_CMSIS.h"

void GPIO_SPI1_Init(void){
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;                                           // GPIOA saatini aktif et [AHB1ENR bit 0]
    (void)RCC->AHB1ENR;                                                             // Bus senkronizasyonu için dummy read

    GPIOA->MODER &= ~((3 << (5*2)) | (3 << (6*2)) | (3 << (7*2)));                // PA5, PA6, PA7 mod bitlerini temizle [Binary: 11 11 11 → 00 00 00]
    GPIOA->MODER |=  ((2 << (5*2)) | (2 << (6*2)) | (2 << (7*2)));                // PA5, PA6, PA7 → AF modu (10) [Binary: 10 10 10]

    GPIOA->MODER &= ~(3 << (4*2));                                                  // PA4 mod bitlerini temizle
    GPIOA->MODER |=  (1 << (4*2));                                                  // PA4 → Output modu (01)

    GPIOA->OTYPER &= ~((1<<4) | (1<<5) | (1<<6) | (1<<7));                        // PA4, PA5, PA6, PA7 → Push-pull (0) [OTYPER bit 4-7]

    GPIOA->OSPEEDR |= ((3 << (5*2)) | (3 << (6*2)) | (3 << (7*2)));               // PA5, PA6, PA7 → Very high speed (11) [SPI saat hattı için gerekli]

    GPIOA->PUPDR &= ~((3 << (4*2)) | (3 << (5*2)) | (3 << (6*2)) | (3 << (7*2))); // PA4, PA5, PA6, PA7 → No pull (00)

    GPIOA->AFR[0] &= ~((0xF << (5*4)) | (0xF << (6*4)) | (0xF << (7*4)));         // PA5, PA6, PA7 AF bitlerini temizle [AFRL bit 20-31]
    GPIOA->AFR[0] |=  ((5   << (5*4)) | (5   << (6*4)) | (5   << (7*4)));         // PA5, PA6, PA7 → AF5 (SPI1) [Binary: 0101 0101 0101]

    GPIOA->BSRR = (1 << 4);                                                         // PA4 set → CS HIGH [BSRR alt 16 bit = set]
}

/*

Amaç: SPI1 için kullanılacak GPIO pinlerini (PA4, PA5, PA6, PA7) doğru modlarda yapılandırmak.

RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    Ne yapar: GPIOA çevresinin AHB1 bus üzerindeki saat sinyalini açar.
    Neden gerekli: Clock kapalıyken GPIOA register'larına yazılan değerler etkisiz kalır.
    Dummy read: (void)RCC->AHB1ENR; satırı bus senkronizasyonu sağlar; clock enable yazmasının
    fiziksel olarak tamamlanmasını garanti eder.

GPIOA->MODER (PA5, PA6, PA7 → AF modu):
    Ne yapar: Her pin için 2-bitlik MODER alanına 10 (Alternate Function) yazar.
    Neden: SPI1 sinyalleri (SCK, MISO, MOSI) donanım tarafından üretilir/okunur;
    pinin dijital I/O mantığından çıkıp AF bloğuna bağlanması gerekir.
    Güvenli yazım: Önce &= ~ ile ilgili bitler temizlenir, sonra |= ile değer yazılır.
    Bu, diğer pinlerin konfigürasyonunu bozmaz.

GPIOA->MODER (PA4 → Output modu):
    Ne yapar: PA4 için MODER alanına 01 (General Purpose Output) yazar.
    Neden: CS pini yazılım tarafından manuel kontrol edilecek; AF değil, sıradan çıkış olarak
    kullanılır. Bu sayede birden fazla slave için farklı CS pinleri kolayca yönetilebilir.

GPIOA->OTYPER (Push-pull):
    Ne yapar: İlgili pinlerin çıkış tipini push-pull (0) yapar.
    Neden: SPI hatları aktif olarak HIGH veya LOW sürülmelidir; I2C'deki gibi open-drain
    gereksizdir. Push-pull daha güçlü sinyal kenarları ve daha yüksek hız sağlar.

GPIOA->OSPEEDR (Very high speed):
    Ne yapar: PA5, PA6, PA7 için hız bitlerini 11 (very high speed) yapar.
    Neden: SPI saat frekansı MHz mertebesinde olduğundan sinyal kenarlarının hızlı
    geçiş yapması gerekir. Düşük hız ayarında sinyal bozulmaları (ringing, slew rate
    kısıtlaması) veri hatalarına yol açabilir.
    Not: PA4 CS pini için hız önemli değildir; fakat temiz kod için aynı pin grubuna
    dahil edilmemiştir.

GPIOA->PUPDR (No pull):
    Ne yapar: Tüm SPI pinleri için dahili pull-up/pull-down devrelerini kapatır (00).
    Neden: SPI hatları aktif sürücülü olduğundan pull dirençleri gereksizdir ve
    yüksek frekanslarda parazite yol açabilir.

GPIOA->AFR[0] (AF5 seçimi):
    Ne yapar: PA5, PA6, PA7 için Alternate Function Register'a 0101 (AF5) yazar.
    Neden: STM32F4 datasheet Alternate Function tablosuna göre SPI1_SCK, SPI1_MISO
    ve SPI1_MOSI fonksiyonları AF5 ile PA5, PA6, PA7 pinlerine atanmıştır.
    AFR[0] (AFRL): Pin 0-7 için kullanılır; her pin 4 bit.
    AFR[1] (AFRH): Pin 8-15 için kullanılır.
    Hesaplama: PA5 → bit offset = 5*4 = 20, PA6 → 24, PA7 → 28.

GPIOA->BSRR (CS idle HIGH):
    Ne yapar: PA4'ü başlangıçta HIGH yapar (CS pasif/idle durumu).
    Neden: CS aktif-LOW çalışır; haberleşme başlamadan önce HIGH olmalıdır.
    Aksi takdirde slave cihaz açılışta yanlış komut alabilir.
    BSRR avantajı: Atomik set/reset işlemi; ODR gibi read-modify-write gerektirmez,
    yarış durumu (race condition) riski yoktur.

*/

void SPI1_Init(void){
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;                // SPI1 clock enable [APB2ENR bit 12]
    (void)RCC->APB2ENR;                                 // Bus senkronizasyonu için dummy read

    SPI1->CR1 &= ~SPI_CR1_SPE;                         // SPI disable (konfigürasyon öncesi kapatılmalı)

    SPI1->CR1 |=  (3 << 3);                            // BR[2:0] = 011 → fPCLK/16 = 84/16 = 5.25 MHz [CR1 bit 5:3]
    SPI1->CR1 &= ~(SPI_CR1_CPOL | SPI_CR1_CPHA);       // Mod 0: CPOL=0, CPHA=0 [CR1 bit 1, bit 0]
    SPI1->CR1 &= ~SPI_CR1_DFF;                         // 8-bit data frame [CR1 bit 11]
    SPI1->CR1 |=  (SPI_CR1_SSM | SPI_CR1_SSI);         // Software NSS: SSM=1, SSI=1 [CR1 bit 9, bit 8]
    SPI1->CR1 |=  SPI_CR1_MSTR;                        // Master modu [CR1 bit 2]
    SPI1->CR1 &= ~SPI_CR1_LSBFIRST;                    // MSB first [CR1 bit 7]

    SPI1->CR1 |=  SPI_CR1_SPE;                         // SPI enable [CR1 bit 6]
}

/*

Amaç: SPI1 çevresini master modunda, Mod 0, 8-bit, yazılım CS kontrolü ile yapılandırmak.

RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    Ne yapar: SPI1 çevresinin APB2 bus üzerindeki saat sinyalini açar (bit 12).
    Neden: SPI1, APB2 üzerinde konumludur (I2C ve USART2 gibi yavaş çevresel birimler
    APB1 üzerindedir; SPI1, TIM1, USART1 gibi hızlı birimler APB2 üzerindedir).
    APB2 clock = 84 MHz (168 MHz / 2).

SPI1->CR1 &= ~SPI_CR1_SPE;
    Ne yapar: SPI'yi devre dışı bırakır.
    Neden: SPE=1 iken CR1'deki konfigürasyon bitlerine yazılmamalıdır; referans
    manualine göre bazı bitler yalnızca SPI kapalıyken değiştirilebilir.

BR[2:0] = 011 → fPCLK/16:
    Ne yapar: SPI saat frekansını APB2'nin 1/16'sına böler → 84/16 = 5.25 MHz.
    BR tablosu (CR1 bit 5:3):
        000 = fPCLK/2    → 42 MHz
        001 = fPCLK/4    → 21 MHz
        010 = fPCLK/8    → 10.5 MHz
        011 = fPCLK/16   → 5.25 MHz  ← seçilen
        100 = fPCLK/32   → 2.625 MHz
        101 = fPCLK/64   → 1.3 MHz
        110 = fPCLK/128  → 656 kHz
        111 = fPCLK/256  → 328 kHz
    Öneri: Slave cihazın maksimum SPI frekansına göre BR ayarlanmalıdır.
    Başlangıç için düşük bir değer seçmek güvenlidir; çalıştıktan sonra artırılabilir.

CPOL ve CPHA (Mod 0):
    Ne yapar: Her ikisi de 0 yapılır → Mod 0.
    CPOL=0: Saat hattı boşta LOW.
    CPHA=0: Veri yükselen kenarda örneklenir (1. kenar).
    Neden: En yaygın SPI modu Mod 0'dır. W25Q flash, ILI9341 TFT, MPU6500 gibi
    pek çok slave cihaz Mod 0 veya Mod 3 ile çalışır. Slave'in datasheet'i kontrol edilmeli.

DFF = 0 (8-bit):
    Ne yapar: Data Frame Format bitini 0 yapar → 8-bit veri çerçevesi.
    DFF=1 yapılırsa 16-bit frame seçilir. Çoğu uygulama 8-bit kullanır.

SSM=1, SSI=1 (Software Slave Management):
    SSM=1: NSS pini donanım tarafından değil yazılım tarafından yönetilir.
    SSI=1: SSM=1 modunda NSS'in iç değerini belirler. SSI=0 bırakılırsa SPI,
    NSS'i LOW görür ve MODF (Mode Fault) hatası üreterek MSTR bitini temizler.
    SSI=1 → "NSS HIGH, ben master'ım" anlamına gelir.
    Neden yazılım NSS: Birden fazla slave kullanımında her slave için ayrı GPIO pini
    CS olarak kullanılabilir; donanım NSS tek slave için kısıtlayıcıdır.

MSTR=1 (Master modu):
    Ne yapar: SPI1'i master olarak yapılandırır; saat sinyalini bu cihaz üretir.
    Slave modunda saat dışarıdan gelir.

LSBFIRST=0 (MSB first):
    Ne yapar: Verinin en anlamlı biti (MSB) önce gönderilir.
    Neden: SPI protokolünün ve çoğu slave cihazın varsayılanı MSB first'tür.
    Slave'in datasheet'inde LSB first gerekiyorsa bu bit 1 yapılmalıdır.

SPE=1 (SPI Enable):
    Ne yapar: Tüm konfigürasyon tamamlandıktan sonra SPI çevresini aktif eder.
    Neden en son: SPE set edildikten sonra SPI donanımı çalışmaya başlar;
    konfigürasyon bitleri artık değiştirilemez.

*/

uint8_t SPI1_TransmitReceive(uint8_t data){
    while(!(SPI1->SR & SPI_SR_TXE));                   // TX buffer boş olana kadar bekle [SR bit 1]
    SPI1->DR = data;                                    // Veriyi TX buffer'a yaz

    while(!(SPI1->SR & SPI_SR_RXNE));                  // RX buffer dolu olana kadar bekle [SR bit 0]
    return (uint8_t)SPI1->DR;                          // Okunan veriyi döndür (RXNE flag'i DR okumasıyla temizlenir)
}

/*

Amaç: SPI1 üzerinden tek byte tam çift yönlü (full-duplex) gönderme ve alma işlemi yapmak.
Bu fonksiyon SPI1_Transmit() ve SPI1_Receive() fonksiyonlarının çekirdeğidir.

while(!(SPI1->SR & SPI_SR_TXE)):
    Ne yapar: SR register'ındaki TXE (Transmit Buffer Empty) bitini polling ile bekler.
    TXE=1: TX buffer boş, yeni veri yazılabilir.
    TXE=0: Önceki veri henüz gönderilmedi, beklenir.
    Neden: DR'a yazılacak veri için yer açılmadan yazmak önceki veriyi bozabilir.

SPI1->DR = data:
    Ne yapar: Gönderilecek byte'ı Data Register'a yazar; donanım otomatik olarak
    seri gönderimi başlatır.
    SPI full-duplex: Her TX işlemi eş zamanlı bir RX işlemi üretir. MOSI'den
    gönderirken MISO'dan aynı anda veri alınır.

while(!(SPI1->SR & SPI_SR_RXNE)):
    Ne yapar: SR register'ındaki RXNE (Receive Buffer Not Empty) bitini bekler.
    RXNE=1: RX buffer'da okunacak veri var.
    RXNE=0: Veri henüz gelmedi, beklenir.
    Neden: RXNE beklenmeden DR okunursa önceki frame'in verisi okunabilir veya
    geçersiz veri döndürülür.

return (uint8_t)SPI1->DR:
    Ne yapar: RX buffer'dan alınan byte'ı döndürür.
    DR okuması RXNE flag'ini otomatik olarak temizler.
    uint8_t cast: DR register 16-bit genişliğindedir; 8-bit frame modunda
    yalnızca alt 8 bit geçerlidir, cast ile üst bitler atılır.

OVR (Overrun) uyarısı:
    Eğer RX buffer okunmadan yeni veri gelirse SR'da OVR flag'i set olur.
    Bu fonksiyon her TX sonrası DR'ı okuduğu için normal kullanımda OVR oluşmaz.
    Hata yönetimi gerektiren uygulamalarda SR'daki OVR biti kontrol edilmelidir.

Interrupt / DMA alternatifi:
    Polling (busy-wait) yaklaşımı basit uygulamalar için uygundur.
    Yüksek performans veya düşük güç gerektiren sistemlerde SPI interrupt'ı
    veya DMA ile non-blocking transfer tercih edilmelidir.

*/

void SPI1_Transmit(uint8_t *pData, uint32_t size){
    for(uint32_t i = 0; i < size; i++){
        SPI1_TransmitReceive(pData[i]);                 // Her byte gönderilir, dönüş değeri (dummy RX) kullanılmaz
    }
}

/*

Amaç: Birden fazla byte'ı SPI1 üzerinden göndermek; gelen veriyle ilgilenilmez.

SPI1_TransmitReceive(pData[i]):
    Ne yapar: Her byte için full-duplex transfer yapar.
    Dönüş değeri: SPI full-duplex olduğundan slave'den eş zamanlı veri gelir;
    ancak bu fonksiyonda yalnızca gönderme amaçlandığından dönüş değeri görmezden gelinir.
    Neden yine de RXNE beklenir: RX buffer temizlenmezse OVR (Overrun) hatası oluşur
    ve sonraki transferler bozulabilir. TransmitReceive içinde DR okunarak bu önlenir.

Kullanım örneği:
    uint8_t cmd[] = {0x9F};
    SPI1_CS_Low();
    SPI1_Transmit(cmd, 1);       // Komut gönder
    SPI1_CS_High();

*/

void SPI1_Receive(uint8_t *pData, uint32_t size){
    for(uint32_t i = 0; i < size; i++){
        pData[i] = SPI1_TransmitReceive(0xFF);          // Dummy byte gönder, gelen veriyi diziye kaydet
    }
}

/*

Amaç: Slave'den birden fazla byte okumak.

SPI1_TransmitReceive(0xFF):
    Ne yapar: MOSI hattına 0xFF (dummy byte) gönderir; bu slave'i clock'lamak için
    gereklidir. SPI senkron bir protokoldür — master clock üretmeden slave veri gönderemez.
    Clock üretmek için MOSI'ye herhangi bir değer yazılmalıdır; 0xFF veya 0x00 yaygın
    tercihlerdir (slave'e göre değişebilir, genellikle fark etmez).
    Dönüş değeri: MISO'dan gelen gerçek veri pData[i]'ye kaydedilir.

Kullanım örneği:
    uint8_t rx[3] = {0};
    SPI1_CS_Low();
    SPI1_Transmit(cmd, 1);       // Önce komut gönder
    SPI1_Receive(rx, 3);         // Ardından 3 byte oku
    SPI1_CS_High();

*/

void SPI1_CS_Low(void){
    GPIOA->BSRR = (1 << (4 + 16));                     // PA4 reset → CS LOW [BSRR üst 16 bit = reset]
}

/*

Amaç: Chip Select hattını aktif (LOW) yaparak slave cihazı seçmek.

GPIOA->BSRR = (1 << (4 + 16)):
    Ne yapar: BSRR register'ının üst 16 bitine (bit reset alanı) PA4'e karşılık gelen
    biti (bit 20) yazar → PA4 LOW olur.
    BSRR yapısı:
        Bit 0-15  (BS): Set bitleri  → ilgili pin HIGH yapılır
        Bit 16-31 (BR): Reset bitleri → ilgili pin LOW yapılır
    Neden BSRR: Atomik işlem; ODR register'ının aksine read-modify-write gerektirmez.
    ISR içinde veya çok görevli ortamda yarış durumu (race condition) riski yoktur.
    CS protokolü: SPI haberleşmesi başlamadan önce CS LOW yapılmalıdır. Slave cihaz
    CS=LOW iken gelen SCK kenarlarını ve veriyi dikkate alır.

*/

void SPI1_CS_High(void){
    GPIOA->BSRR = (1 << 4);                            // PA4 set → CS HIGH [BSRR alt 16 bit = set]
}

/*

Amaç: Chip Select hattını pasif (HIGH) yaparak slave cihazı devre dışı bırakmak.

GPIOA->BSRR = (1 << 4):
    Ne yapar: BSRR register'ının alt 16 bitine (bit set alanı) PA4'e karşılık gelen
    biti (bit 4) yazar → PA4 HIGH olur.
    CS protokolü: Transfer tamamlandıktan sonra CS HIGH yapılmalıdır. Slave cihaz
    CS=HIGH iken SCK ve MOSI hatlarını görmezden gelir.
    Çok slave kullanımı: Her slave için farklı GPIO pini CS olarak atanır;
    aynı BSRR mantığıyla ilgili pine yazılır. Aynı anda yalnızca bir CS LOW
    olmalıdır; aksi takdirde birden fazla slave aynı MISO hattına çıkış yapar
    ve hat çakışması (bus contention) oluşur.

Tipik SPI transfer dizisi:
    SPI1_CS_Low();               // Slave seç
    SPI1_Transmit(cmd, cmdSize); // Komut/adres gönder
    SPI1_Receive(buf, bufSize);  // Veri oku
    SPI1_CS_High();              // Slave bırak

*/
