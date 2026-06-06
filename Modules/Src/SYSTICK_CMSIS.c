#include "SysTick_CMSIS.h"

volatile uint32_t tick_count = 0;

void SysTick_Init(void){
    SysTick->LOAD = 168000 - 1;                                                     // 1 ms için yeniden yükleme değeri (168 MHz / 1000 - 1 = 167999)
    SysTick->VAL  = 0;                                                              // Sayaç değerini sıfırla (herhangi bir yazma sıfırlar)
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk |                                   // Clock kaynağı: işlemci clock'u (168 MHz)
                     SysTick_CTRL_TICKINT_Msk    |                                  // SysTick interrupt aktif
                     SysTick_CTRL_ENABLE_Msk;                                       // SysTick sayacını başlat
}

/*

Amaç: SysTick zamanlayıcısını 1 ms periyotla interrupt üretecek şekilde yapılandırmak.
SysTick, Cortex-M4 çekirdeğine gömülü 24-bit aşağı sayaçtır (down counter).
STM32'ye özgü değildir; tüm Cortex-M serisi işlemcilerde bulunur.
Referans: PM0214 — STM32 Cortex-M4 Programming Manual, Bölüm 4.5.

SysTick Register'ları:
    CTRL  (0xE000E010): Kontrol ve durum register'ı
    LOAD  (0xE000E014): Yeniden yükleme değeri register'ı
    VAL   (0xE000E018): Mevcut sayaç değeri register'ı

SysTick->LOAD = 168000 - 1:
    Ne yapar: Sayaç bu değerden başlar, 0'a ulaşınca yeniden bu değere yüklenir.
    Formül: LOAD = (SYSCLK / istenen_frekans) - 1
        SYSCLK = 168 MHz, istenen_frekans = 1000 Hz (1 ms)
        LOAD = (168_000_000 / 1000) - 1 = 167_999
    Neden -1: Sayaç 167999'dan 0'a sayar; 0 dahil olduğundan toplam 168000 adım = 1 ms.
    24-bit sınır: Maksimum LOAD değeri 0xFFFFFF = 16_777_215. 167_999 bu sınırın içinde.

SysTick->VAL = 0:
    Ne yapar: Mevcut sayaç değerini sıfırlar.
    Önemli: VAL register'ına herhangi bir değer yazıldığında donanım tarafından
    otomatik olarak sıfırlanır; yazılan değerin önemi yoktur. 0 yazmak en açık ifadedir.
    Neden gerekli: Önceki bir konfigürasyondan kalan değer varsa sayaç yanlış
    noktadan başlayabilir; ilk periyot hatalı olur.

SysTick_CTRL_CLKSOURCE_Msk (bit 2):
    Ne yapar: SysTick clock kaynağını seçer.
    0 = AHB/8 (168/8 = 21 MHz)
    1 = AHB (işlemci clock'u, 168 MHz) ← seçilen
    Neden 168 MHz: delay_us fonksiyonunda 1 µs = 168 tick hesabı bu seçime dayanır.
    AHB/8 seçilirse tüm timing hesapları değişir.

SysTick_CTRL_TICKINT_Msk (bit 1):
    Ne yapar: Sayaç 0'a ulaştığında SysTick_Handler interrupt'ını tetikler.
    0 = Interrupt yok (polling ile COUNTFLAG kontrol edilir)
    1 = Interrupt aktif ← seçilen
    Neden gerekli: tick_count değişkeninin her 1 ms'de otomatik artması için
    interrupt handler'ın çağrılması şarttır.

SysTick_CTRL_ENABLE_Msk (bit 0):
    Ne yapar: SysTick sayacını başlatır.
    0 = Sayaç durdurulmuş
    1 = Sayaç çalışıyor ← seçilen
    Neden en son: LOAD ve VAL ayarlandıktan sonra enable edilmesi güvenlidir;
    aksi takdirde tanımsız bir değerden saymaya başlayabilir.

HAL ile birlikte kullanım (Seçenek B):
    HAL kütüphanesi de SysTick'i 1 ms için kullanır ve stm32f4xx_it.c içindeki
    SysTick_Handler'da HAL_IncTick() çağırır. Bu fonksiyon HAL'ın uwTick sayacını artırır.
    Bizim SysTick_Init() fonksiyonumuz LOAD/VAL/CTRL'yi yeniden yapılandırır;
    HAL zaten aynı ayarları yapmış olduğundan çakışma olmaz.
    stm32f4xx_it.c içindeki SysTick_Handler'a tick_count++ eklenerek her iki
    sayaç (HAL uwTick ve bizim tick_count) aynı anda artırılır.

tick_count değişkeni:
    static: Bu derleme birimine (translation unit) özgüdür; dışarıdan doğrudan
    erişilemez, yalnızca GetTick() fonksiyonu aracılığıyla okunur.
    volatile: Derleyiciye bu değişkenin interrupt handler tarafından değiştirilebileceğini
    bildirir; optimizasyon sırasında register'a alınmasını engeller.
    uint32_t: 32-bit işaretsiz tam sayı; yaklaşık 49.7 gün sonra taşar (overflow).
    delay_ms fonksiyonu taşma durumunu aritmetik olarak doğru ele alır.

*/

void delay_ms(uint32_t ms){
    uint32_t start = tick_count;                                                    // Başlangıç tick değerini al
    while((tick_count - start) < ms);                                               // İstenen süre geçene kadar bekle
}

/*

Amaç: Milisaniye cinsinden bloklayıcı (blocking) gecikme sağlamak.

uint32_t start = tick_count:
    Ne yapar: Fonksiyon çağrıldığı andaki tick_count değerini kaydeder.
    Neden: Gecikmenin başlangıç referans noktasıdır.

while((tick_count - start) < ms):
    Ne yapar: tick_count ile start arasındaki fark istenen ms değerine
    ulaşana kadar CPU'yu meşgul tutar (busy-wait / polling).
    Overflow güvenliği: tick_count taştığında (uint32_t sınırını aştığında)
    aritmetik taşma kuralı gereği fark hesabı hâlâ doğru sonuç verir.
    Örnek: start = 0xFFFFFFFE, ms = 5 ise:
        tick_count = 0x00000003 olduğunda
        0x00000003 - 0xFFFFFFFE = 0x00000005 = 5 ✅
    Bloklama uyarısı: Bu fonksiyon CPU'yu meşgul tutar; RTOS veya interrupt
    tabanlı sistemlerde non-blocking alternatif tercih edilmelidir.

*/

void delay_us(uint32_t us){
    uint32_t ticks   = us * 168;                                                    // 1 µs = 168 tick (168 MHz clock)
    uint32_t start   = SysTick->VAL;                                                // Mevcut sayaç değerini al
    uint32_t elapsed = 0;                                                           // Geçen tick sayacı

    while(elapsed < ticks){
        uint32_t current = SysTick->VAL;                                            // Anlık sayaç değeri
        if(current < start){
            elapsed += start - current;                                             // Normal durum: sayaç aşağı indi
        } else {
            elapsed += (168000 - 1) - current + start;                             // Overflow: sayaç sıfırlanıp LOAD'dan başladı
        }
        start = current;                                                            // Başlangıç noktasını güncelle
    }
}

/*

Amaç: Mikrosaniye cinsinden yüksek çözünürlüklü bloklayıcı gecikme sağlamak.
tick_count 1 ms çözünürlüğünde olduğundan µs gecikme için doğrudan SysTick->VAL okunur.

uint32_t ticks = us * 168:
    Ne yapar: İstenen µs süresini SysTick tick sayısına çevirir.
    Formül: 1 µs = SYSCLK / 1_000_000 = 168_000_000 / 1_000_000 = 168 tick
    Dikkat: us değeri büyükse (örn. >25000) ticks uint32_t sınırını aşabilir.
    Büyük gecikmeler için delay_ms kullanılması önerilir.

SysTick->VAL:
    Ne yapar: SysTick'in anlık sayaç değerini verir (24-bit, 0 ile LOAD arasında).
    Aşağı sayaç: Her clock tick'te 1 azalır; 0'a ulaşınca LOAD değerinden başlar.
    Doğrudan okuma: tick_count gibi bir interrupt'a gerek yoktur; donanım değeri
    her an okunabilir, bu sayede µs çözünürlüğü sağlanır.

Normal durum (current < start):
    Sayaç aşağı doğru ilerlemiş; geçen tick = start - current.
    Örnek: start = 1000, current = 600 → elapsed += 400

Overflow durumu (current >= start):
    Sayaç 0'a ulaşmış ve LOAD değerinden (167999) yeniden başlamış.
    Geçen tick = (LOAD - current) + start
    Örnek: start = 100, current = 167000 (sayaç yenilendi)
        elapsed += (167999 - 167000) + 100 = 1099

start = current güncellemesi:
    Her döngü iterasyonunda başlangıç noktası güncellenir; böylece birden fazla
    overflow durumu da doğru biriktirilir.

Hassasiyet notu:
    168 MHz'de 1 tick ≈ 5.95 ns. Fonksiyon çağrısı ve döngü overhead'i birkaç
    tick ekler; çok kısa gecikmeler (1-2 µs) hafif sapma gösterebilir.

*/

uint32_t GetTick(void){
    return tick_count;                                                              // Başlangıçtan bu yana geçen ms değerini döndür
}

/*

Amaç: Sistem başlangıcından itibaren geçen süreyi milisaniye cinsinden döndürmek.
HAL kütüphanesindeki HAL_GetTick() fonksiyonunun CMSIS eşdeğeridir.

return tick_count:
    Ne yapar: Her 1 ms'de SysTick_Handler tarafından artırılan tick_count
    değerini döndürür.
    Kullanım alanları:
        Zaman damgası (timestamp): Olayların ne zaman gerçekleştiğini kaydetmek.
        Non-blocking bekleme: İki GetTick() çağrısı arasındaki fark ile
        bloklama olmadan süre ölçümü yapılabilir.
        Timeout mekanizması: İletişim protokollerinde yanıt bekleme süresi kontrolü.

Non-blocking kullanım örneği:
    uint32_t start = GetTick();
    while(GetTick() - start < 1000){
        // 1 saniye boyunca başka işler yapılabilir
    }

volatile okuma:
    tick_count volatile tanımlandığından derleyici her okumada gerçek bellek
    adresine erişir; optimize edilmiş (cached) değer döndürülmez.

*/
