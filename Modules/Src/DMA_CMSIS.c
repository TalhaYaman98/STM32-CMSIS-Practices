
#include "DMA_CMSIS.h"

void DMA_UART_Init(void){
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;                        // DMA1 saatini aktif et [AHB1ENR bit 21]
    (void)RCC->AHB1ENR;                                         // Bus senkronizasyonu

    DMA1_Stream6->CR &= ~DMA_SxCR_EN;                          // Stream'i disable et (konfigürasyon öncesi)
    while(DMA1_Stream6->CR & DMA_SxCR_EN);                     // Disable tamamlanana kadar bekle

    DMA1_Stream6->CR  = 0;                                      // CR register'ını sıfırla
    DMA1_Stream6->FCR = 0;                                      // FIFO kontrol register'ını sıfırla

    DMA1_Stream6->CR |= (4 << DMA_SxCR_CHSEL_Pos);             // Channel 4 seç (USART2_TX) [CR bit 27:25]
    DMA1_Stream6->CR |= DMA_SxCR_DIR_0;                        // Transfer yönü: Memory → Peripheral [DIR = 01]
    DMA1_Stream6->CR |= DMA_SxCR_MINC;                         // Memory adresi otomatik artır [CR bit 10]
    DMA1_Stream6->CR &= ~DMA_SxCR_PINC;                        // Peripheral adresi sabit [CR bit 9]
    DMA1_Stream6->CR &= ~DMA_SxCR_MSIZE;                       // Memory veri boyutu: 8-bit [MSIZE = 00]
    DMA1_Stream6->CR &= ~DMA_SxCR_PSIZE;                       // Peripheral veri boyutu: 8-bit [PSIZE = 00]
    DMA1_Stream6->CR &= ~DMA_SxCR_CIRC;                        // Circular mode kapalı [CR bit 8]
    DMA1_Stream6->CR |= (1 << DMA_SxCR_PL_Pos);                // Öncelik: Medium [PL = 01]

    DMA1_Stream6->PAR = (uint32_t)&USART2->DR;                 // Peripheral adresi: USART2 Data Register

    USART2->CR3 |= USART_CR3_DMAT;                             // USART2 DMA TX enable [CR3 bit 7]

    /* HISR/HIFCR: Stream6 flag'lerini temizle */
    DMA1->HIFCR |= DMA_HIFCR_CTCIF6  |                         // Transfer complete flag temizle
                   DMA_HIFCR_CHTIF6  |                         // Half transfer flag temizle
                   DMA_HIFCR_CTEIF6  |                         // Transfer error flag temizle
                   DMA_HIFCR_CDMEIF6 |                         // Direct mode error flag temizle
                   DMA_HIFCR_CFEIF6;                           // FIFO error flag temizle
}

/*

Amaç: USART2 TX için DMA1 Stream6 Channel4'ü yapılandırmak.
DMA, CPU müdahalesi olmadan bellek → USART2->DR transferi yapar.

STM32F4 DMA Yapısı:
    DMA1 ve DMA2 olmak üzere iki kontrolör vardır.
    Her kontrolörün 8 stream'i (0–7), her stream'in 8 channel'ı (0–7) vardır.
    Hangi stream/channel'ın hangi çevresel birime ait olduğu RM0090 Table 42/43'te belirtilir.
    USART2_TX → DMA1, Stream6, Channel4 (RM0090 Table 42).

RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN:
    Ne yapar: DMA1 çevresinin AHB1 bus üzerindeki saat sinyalini açar.
    Neden: Clock kapalıyken DMA register'larına yazılan değerler etkisiz kalır.

DMA1_Stream6->CR &= ~DMA_SxCR_EN ve while(CR & EN):
    Ne yapar: Stream'i disable eder ve tamamen kapanmasını bekler.
    Neden: EN=1 iken CR register'ına yazılmamalıdır; donanım bunu görmezden gelir.
    while ile bekleme: DMA donanımı o anki transfer byte'ını tamamladıktan sonra kapanır.

DMA_SxCR_CHSEL (bit 27:25) = 4:
    Ne yapar: Bu stream için Channel 4'ü seçer.
    Neden: USART2_TX DMA1 Stream6 Channel4'e atanmıştır (RM0090 Table 42).
    Yanlış channel seçimi DMA'nın hiç tetiklenmemesine yol açar.

DMA_SxCR_DIR = 01 (Memory → Peripheral):
    Ne yapar: Transfer yönünü belirler.
    00 = Peripheral → Memory (ADC okuma, UART RX)
    01 = Memory → Peripheral (UART TX, SPI TX) ← seçilen
    10 = Memory → Memory (bellek kopyalama)

DMA_SxCR_MINC (Memory Increment):
    Ne yapar: Her transfer sonrası memory adresini otomatik artırır.
    Neden: Dizi (buffer) gönderilirken her byte farklı adreste; artış olmadan hep
    aynı byte gönderilir.
    PINC kapalı: USART2->DR adresi sabittir, artmamalıdır.

MSIZE ve PSIZE = 00 (8-bit):
    Ne yapar: Hem bellek hem çevresel birim için 8-bit veri genişliği seçer.
    UART 8-bit veri frame'i kullandığından her transfer 1 byte'tır.

DMA1_Stream6->PAR = &USART2->DR:
    Ne yapar: Peripheral Address Register'a USART2 Data Register adresini yazar.
    DMA her transfer döngüsünde bu adrese veri yazar.
    MAR (Memory Address Register) her DMA_UART_Transmit çağrısında ayarlanır.

USART2->CR3 |= USART_CR3_DMAT:
    Ne yapar: USART2'nin DMA TX talebini aktif eder.
    Neden: Bu bit set edilmezse USART2, DMA'ya transfer talebi göndermez ve
    DMA hiç tetiklenmez.

HIFCR flag temizleme:
    Ne yapar: Stream6'ya ait tüm durum flag'lerini temizler.
    Neden: Önceki bir transferden kalan flag'ler yeni transferi engelleyebilir.
    HISR/LIFSR: Stream 4–7 için HISR, Stream 0–3 için LISR kullanılır.

*/

void DMA_UART_Transmit(uint8_t *pData, uint16_t size){
    DMA1_Stream6->CR  &= ~DMA_SxCR_EN;                         // Stream'i disable et
    while(DMA1_Stream6->CR & DMA_SxCR_EN);                     // Kapanmasını bekle

    DMA1->HIFCR |= DMA_HIFCR_CTCIF6  |                         // Önceki transfer flag'lerini temizle
                   DMA_HIFCR_CHTIF6  |
                   DMA_HIFCR_CTEIF6  |
                   DMA_HIFCR_CDMEIF6 |
                   DMA_HIFCR_CFEIF6;

    DMA1_Stream6->M0AR = (uint32_t)pData;                       // Memory adresi: gönderilecek buffer
    DMA1_Stream6->NDTR = size;                                  // Transfer edilecek byte sayısı

    DMA1_Stream6->CR  |= DMA_SxCR_EN;                          // Stream'i enable et → transfer başlar
}

/*

Amaç: Verilen buffer'ı DMA ile USART2 üzerinden göndermek.

DMA1_Stream6->CR &= ~DMA_SxCR_EN ve while:
    Ne yapar: Önceki transfer tamamlanmışsa veya aktifse stream'i güvenle kapatır.
    Neden: Aktif stream'e MAR/NDTR yazmak tanımsız davranışa yol açabilir.

HIFCR flag temizleme:
    Ne yapar: Her yeni transfer başlamadan önce durum flag'lerini temizler.
    Neden: TCIF (Transfer Complete) flag'i temizlenmezse yeni transfer başlamayabilir
    veya interrupt tabanlı sistemlerde yanlış tetikleme oluşabilir.

DMA1_Stream6->MAR = pData:
    Ne yapar: Memory Address Register'a gönderilecek dizinin başlangıç adresini yazar.
    Her çağrıda farklı buffer gönderilebilir.

DMA1_Stream6->NDTR = size:
    Ne yapar: Number of Data Register'a transfer edilecek byte sayısını yazar.
    DMA her transfer sonrası bu sayacı 1 azaltır; 0'a ulaşınca transfer tamamlanır.
    Circular mode kapalı olduğundan transfer tamamlanınca stream otomatik durur.

DMA1_Stream6->CR |= DMA_SxCR_EN:
    Ne yapar: Stream'i aktif eder; DMA USART2'nin TXE talebini beklemeye başlar.
    USART2 her byte göndermek için hazır olduğunda DMA bir sonraki byte'ı otomatik yazar.
    CPU bu süreçte tamamen serbest kalır.

Transfer tamamlanma kontrolü:
    Polling: while(!(DMA1->HISR & DMA_HISR_TCIF6)); ile beklenebilir.
    Interrupt: DMA_SxCR_TCIE biti set edilerek transfer complete interrupt kullanılabilir.
    Bu implementasyon non-blocking'dir; TCIF kontrolü çağıran koda bırakılmıştır.

*/

void DMA_ADC_Init(void){
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;                        // DMA2 saatini aktif et [AHB1ENR bit 22]
    (void)RCC->AHB1ENR;                                         // Bus senkronizasyonu

    DMA2_Stream0->CR &= ~DMA_SxCR_EN;                          // Stream'i disable et
    while(DMA2_Stream0->CR & DMA_SxCR_EN);                     // Kapanmasını bekle

    DMA2_Stream0->CR  = 0;                                      // CR register'ını sıfırla
    DMA2_Stream0->FCR = 0;                                      // FIFO register'ını sıfırla

    DMA2_Stream0->CR &= ~DMA_SxCR_CHSEL;                       // Channel bitlerini temizle
    /* Channel 0 seçimi: CHSEL = 000, zaten 0, açıklık için belirtildi */

    DMA2_Stream0->CR &= ~DMA_SxCR_DIR;                         // Transfer yönü: Peripheral → Memory [DIR = 00]
    DMA2_Stream0->CR |=  DMA_SxCR_MINC;                        // Memory adresi otomatik artır
    DMA2_Stream0->CR &= ~DMA_SxCR_PINC;                        // Peripheral adresi sabit
    DMA2_Stream0->CR |=  DMA_SxCR_MSIZE_0;                     // Memory veri boyutu: 16-bit [MSIZE = 01]
    DMA2_Stream0->CR |=  DMA_SxCR_PSIZE_0;                     // Peripheral veri boyutu: 16-bit [PSIZE = 01]
    DMA2_Stream0->CR |=  DMA_SxCR_CIRC;                        // Circular mode: sürekli okuma [CR bit 8]
    DMA2_Stream0->CR |=  (2 << DMA_SxCR_PL_Pos);               // Öncelik: High [PL = 10]

    DMA2_Stream0->PAR = (uint32_t)&ADC1->DR;                   // Peripheral adresi: ADC1 Data Register

    ADC1->CR2 |= ADC_CR2_DMA;                                  // ADC1 DMA enable [CR2 bit 8]
    ADC1->CR2 |= ADC_CR2_DDS;                                  // DMA Disable Selection: sürekli DMA talebi [CR2 bit 9]
    ADC1->CR2 |= ADC_CR2_CONT;                                  // Sürekli dönüşüm modu [CR2 bit 1]

    /* LISR/LIFCR: Stream0 flag'lerini temizle */
    DMA2->LIFCR |= DMA_LIFCR_CTCIF0  |                         // Transfer complete flag temizle
                   DMA_LIFCR_CHTIF0  |                         // Half transfer flag temizle
                   DMA_LIFCR_CTEIF0  |                         // Transfer error flag temizle
                   DMA_LIFCR_CDMEIF0 |                         // Direct mode error flag temizle
                   DMA_LIFCR_CFEIF0;                           // FIFO error flag temizle
}

/*

Amaç: ADC1 için DMA2 Stream0 Channel0'ı yapılandırmak.
ADC sürekli dönüşüm modunda çalışır; her dönüşüm sonucu otomatik olarak belleğe kopyalanır.

ADC1 → DMA2, Stream0, Channel0 (RM0090 Table 43).

DMA2_Stream0->CR &= ~DMA_SxCR_DIR (Peripheral → Memory):
    Ne yapar: Transfer yönünü Peripheral → Memory (00) yapar.
    Neden: ADC1->DR'dan okunan değer bellek dizisine aktarılacak.

MSIZE_0 ve PSIZE_0 (16-bit):
    Ne yapar: Hem bellek hem çevresel birim için 16-bit veri genişliği seçer.
    Neden: ADC 12-bit çözünürlüktedir; sonuç uint16_t (16-bit) değişkene sığar.
    8-bit seçilirse üst bitler kaybolur, ADC değeri hatalı okunur.

DMA_SxCR_CIRC (Circular Mode):
    Ne yapar: Transfer tamamlandığında NDTR otomatik yeniden yüklenir ve
    transfer tekrar başlar. Buffer sürekli güncellenir.
    Neden: ADC sürekli dönüşüm modunda çalışıyorsa her dönüşüm sonucu
    DMA tarafından otomatik alınmalıdır; tek seferlik transfer yetmez.
    Dikkat: Circular modda buffer boyutu dikkatlice seçilmeli; uygulama
    okunmamış veriyi DMA üzerine yazabilir (overwrite).

ADC1->CR2 |= ADC_CR2_DMA:
    Ne yapar: ADC1'in her dönüşüm sonrası DMA'ya transfer talebi göndermesini sağlar.
    Neden: Bu bit set edilmezse ADC dönüşüm yapar ama DMA tetiklenmez.

ADC1->CR2 |= ADC_CR2_DDS (DMA Disable Selection):
    Ne yapar: DDS=1 yapıldığında ADC sürekli DMA talebi gönderir.
    DDS=0 olursa son transfer sonrası DMA devre dışı kalır (tek seferlik).
    Neden: Circular mode ile birlikte kullanıldığında DDS=1 zorunludur;
    aksi takdirde ilk buffer dolduktan sonra DMA durur.

ADC1->CR2 |= ADC_CR2_CONT:
    Ne yapar: ADC'yi sürekli dönüşüm moduna alır; her dönüşüm tamamlanınca
    otomatik yeni dönüşüm başlar.
    Neden: DMA ile verimli kullanım için ADC'nin sürekli çalışması gerekir.

DMA2->LIFCR flag temizleme:
    Stream 0–3 için LIFCR, Stream 4–7 için HIFCR kullanılır.
    Stream0 LIFCR'nin alt 6 bitinde yer alır.

*/

void DMA_ADC_Start(uint16_t *pData, uint16_t size){
    DMA2_Stream0->CR  &= ~DMA_SxCR_EN;                         // Stream'i disable et
    while(DMA2_Stream0->CR & DMA_SxCR_EN);                     // Kapanmasını bekle

    DMA2->LIFCR |= DMA_LIFCR_CTCIF0  |                         // Flag'leri temizle
                   DMA_LIFCR_CHTIF0  |
                   DMA_LIFCR_CTEIF0  |
                   DMA_LIFCR_CDMEIF0 |
                   DMA_LIFCR_CFEIF0;

    DMA2_Stream0->M0AR = (uint32_t)pData;                       // Memory adresi: ADC sonuçlarının yazılacağı dizi
    DMA2_Stream0->NDTR = size;                                  // Kaç dönüşüm sonucu alınacak

    DMA2_Stream0->CR  |= DMA_SxCR_EN;                          // Stream'i enable et

    ADC1->CR2 |= ADC_CR2_SWSTART;                              // ADC dönüşümünü başlat
}

/*

Amaç: ADC DMA transferini başlatmak; ADC sonuçları otomatik olarak pData dizisine yazılır.

DMA2_Stream0->MAR = pData:
    Ne yapar: Memory Address Register'a sonuçların yazılacağı dizinin başlangıç adresini yazar.
    uint16_t dizi: Her ADC sonucu 16-bit (12-bit değer + 4 boş bit).

DMA2_Stream0->NDTR = size:
    Ne yapar: Kaç adet 16-bit transfer yapılacağını belirler.
    Circular modda bu değer her tur tamamlanınca otomatik yeniden yüklenir.
    Örnek: size=10 → 10 ADC örneği alınır, sonra buffer başından tekrar yazılır.

ADC1->CR2 |= ADC_CR2_SWSTART:
    Ne yapar: Yazılım komutu ile ADC dönüşümünü başlatır.
    Sürekli mod (CONT=1) aktif olduğundan ilk dönüşüm başladıktan sonra
    ADC otomatik olarak devam eder; SWSTART tekrar set edilmesine gerek yoktur.

Kullanım örneği:
    uint16_t adc_buf[10] = {0};
    DMA_ADC_Init();
    DMA_ADC_Start(adc_buf, 10);
    // adc_buf[] artık her zaman güncel ADC değerlerini içerir
    // CPU müdahalesi gerekmez

Okuma güvenliği:
    Circular modda DMA sürekli yazarken CPU aynı anda okuyorsa veri tutarsızlığı
    (data race) oluşabilir. Kritik uygulamalarda:
    a) Double buffer (ping-pong) modu kullanılabilir (DMA_SxCR_DBM).
    b) HTIF (half transfer) ve TCIF (transfer complete) interrupt'larıyla
       buffer bölümleri sırayla işlenebilir.

*/

