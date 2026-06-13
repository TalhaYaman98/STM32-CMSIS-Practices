#include "Watchdog_CMSIS.h"

void IWDG_Init(uint8_t prescaler, uint16_t reload){
    RCC->CSR |= RCC_CSR_LSION;                                  // LSI osilatörünü başlat
    while(!(RCC->CSR & RCC_CSR_LSIRDY));                       // LSI hazır olana kadar bekle

    IWDG->KR = 0x5555;                                          // Write Access Enable: PR ve RLR yazma koruması kaldır

    IWDG->PR  = prescaler;                                      // Prescaler değeri [PR bit 2:0]
    IWDG->RLR = reload & 0xFFF;                                 // Reload değeri (12-bit, max 0xFFF) [RLR bit 11:0]

    IWDG->KR = 0xAAAA;                                          // Reload: sayaç RLR değerine yüklenir
    IWDG->KR = 0xCCCC;                                          // Start: IWDG'yi başlat
}

/*

Amaç: IWDG'yi yapılandırmak ve başlatmak.
Referans: RM0090 — Bölüm 22 (IWDG).

IWDG Genel Bilgi:
    12-bit aşağı sayaçtır (0xFFF'den 0'a sayar).
    Clock kaynağı LSI (~32 kHz), sistem clock'undan (HSE/PLL) tamamen bağımsızdır.
    Sayaç 0'a ulaşırsa MCU resetlenir (sistem resetine eşdeğer).
    Bir kez başlatıldıktan sonra DURDURULAMAZ — yalnızca reset ile kapanır.
    Debug modunda IWDG'nin durması istenirse DBGMCU_APB1_FZ register'ında
    DBG_IWDG_STOP biti set edilmelidir (debug sırasında istenmeyen resetleri önlemek için).

RCC->CSR |= RCC_CSR_LSION ve while(!LSIRDY):
    Ne yapar: LSI (~32 kHz dahili osilatör) osilatörünü başlatır ve kararlı
    hâle gelmesini bekler.
    Neden gerekli: IWDG donanımı LSI'yi otomatik açar denilse de, pratikte
    IWDG_Init() ilk çağrıldığında LSI henüz "ready" olmayabilir. LSI hazır
    olmadan PR/RLR yazımları ve KR komutları güvenilir şekilde işlenmeyebilir.
    Bu satırlar olmadan bazı durumlarda IWDG yapılandırması tutarsız davranabilir.

IWDG->KR = 0x5555 (Write Access Enable):
    Ne yapar: PR ve RLR register'larına yazma korumasını kaldırır.
    Neden: KR register'ı normalde reload/start komutları için kullanılır;
    0x5555 özel anahtarı PR/RLR'a erişimi açar.
    Not: Bu erişim IWDG->KR'a 0xAAAA veya 0xCCCC yazılana kadar veya
    bir reload işlemine kadar açık kalır.

IWDG->PR (Prescaler Register, bit 2:0):
    Ne yapar: LSI clock'unu böler.
    Prescaler tablosu:
        000 = /4    001 = /8    010 = /16   011 = /32
        100 = /64   101 = /128  110 = /256  111 = /256
    Formül: IWDG_clk = LSI / prescaler_bölücü
        Örnek: prescaler=4 (/64) → IWDG_clk = 32000 / 64 = 500 Hz

IWDG->RLR (Reload Register, bit 11:0, max 0xFFF=4095):
    Ne yapar: Sayacın başlangıç/yeniden yükleme değeri.
    Timeout formülü: T = (RLR + 1) / IWDG_clk
        Örnek: RLR=2499, IWDG_clk=500 Hz → T = 2500/500 = 5 saniye

PVU/RVU bekleme döngüsü hakkında not:
    RM0090'a göre PR/RLR yazıldıktan sonra SR register'ındaki PVU (Prescaler
    Value Update) ve RVU (Reload Value Update) bitlerinin temizlenmesi
    beklenmesi önerilir; bu bitler donanımın yeni değerleri kabul ettiğini
    gösterir.
    Pratikte (özellikle debug ortamında, DBG_IWDG_STOP aktifken) bu bitler
    hiç temizlenmeyebilir ve while(SR & (PVU|RVU)) sonsuz döngüye girer.
    HAL_IWDG_Init() de bu kontrolü yapmaz; PR/RLR yazıldıktan sonra doğrudan
    KR=0xAAAA / KR=0xCCCC gönderir. Bu implementasyon da HAL ile aynı
    yaklaşımı izler — PVU/RVU kontrolü kasıtlı olarak yapılmaz.

IWDG->KR = 0xAAAA (Reload):
    Ne yapar: Sayacı RLR değerine yeniden yükler ("refresh" / "kicking the dog").
    Bu komut aynı zamanda Write Access'i (0x5555) kapatır.

IWDG->KR = 0xCCCC (Start):
    Ne yapar: IWDG sayacını başlatır.
    Bu noktadan sonra düzenli olarak IWDG_Refresh() çağrılmalıdır;
    aksi takdirde sayaç 0'a ulaşır ve MCU resetlenir.

*/

void IWDG_Refresh(void){
    IWDG->KR = 0xAAAA;                                          // Reload: sayacı RLR değerine sıfırla
}

/*

Amaç: IWDG sayacını yeniden yükleyerek reset'i önlemek ("watchdog kicking").

IWDG->KR = 0xAAAA:
    Ne yapar: Sayacı RLR değerine geri yükler, sayım yeniden başlar.
    Neden periyodik çağrı gerekir: Sayaç sürekli azalır; bu fonksiyon
    çağrılmazsa belirlenen timeout süresinde sayaç 0'a ulaşır ve MCU resetlenir.

Kullanım stratejisi:
    Ana döngüde (while(1)) düzenli aralıklarla çağrılmalıdır.
    Refresh aralığı, timeout süresinden kısa olmalıdır (güvenlik payı ile).
    Örnek: Timeout = 5s ise, her 1-2 saniyede bir refresh yapmak güvenlidir.

Tipik kullanım amacı:
    Yazılımın "donmadığını" donanımsal olarak doğrulamak.
    Eğer ana döngü bir yerde sonsuz döngüye girer veya kilitlenirse,
    IWDG_Refresh() çağrılamaz ve MCU otomatik olarak resetlenir.

Reset sebebini ayırt etme:
    Sistem yeniden başladığında RCC->CSR register'ı okunarak resetin
    IWDG kaynaklı olup olmadığı kontrol edilebilir (IWDG_SR_... değil,
    RCC_CSR_IWDGRSTF biti). Kontrol sonrası RCC->CSR |= RCC_CSR_RMVF;
    ile flag'ler temizlenmelidir, aksi halde bir sonraki resette eski
    flag hâlâ set görünür.

*/

void WWDG_Init(uint8_t t_value, uint8_t w_value, uint8_t prescaler){
    RCC->APB1ENR |= RCC_APB1ENR_WWDGEN;                        // WWDG clock enable [APB1ENR bit 11]
    (void)RCC->APB1ENR;                                         // Bus senkronizasyonu

    WWDG->CFR &= ~WWDG_CFR_WDGTB;                              // Prescaler bitlerini temizle [CFR bit 8:7]
    WWDG->CFR |= (prescaler << WWDG_CFR_WDGTB_Pos);            // Prescaler değeri yaz

    WWDG->CFR &= ~WWDG_CFR_W;                                  // Window bitlerini temizle [CFR bit 6:0]
    WWDG->CFR |= (w_value & 0x7F);                             // Pencere değeri (W[6:0])

    WWDG->CR &= ~WWDG_CR_T;                                    // Sayaç bitlerini temizle [CR bit 6:0]
    WWDG->CR |= (t_value & 0x7F);                              // Başlangıç sayaç değeri (T[6:0])

    WWDG->CR |= WWDG_CR_WDGA;                                  // WWDG enable [CR bit 7]
                                                                 // DİKKAT: Bu bit set edildikten sonra
                                                                 // yazılımla TEKRAR TEMİZLENEMEZ (donanımsal kilit)
}

/*

Amaç: WWDG'yi yapılandırmak ve başlatmak.
Referans: RM0090 — Bölüm 23 (WWDG).

WWDG Genel Bilgi:
    7-bit aşağı sayaçtır (T[6:0]), MSB (bit 6) "1" olduğu sürece çalışır.
    Clock kaynağı APB1 (sistem clock'una bağlı) — IWDG'den farklı olarak
    bağımsız değildir.
    Sayaç T6 biti 0 olursa (yani 0x3F'in altına düşerse) → RESET.
    Refresh işlemi sayaç W[6:0] değerinin ÜZERİNDEYKEN yapılırsa → RESET.
    Yani refresh sadece "W < sayaç ve sayaç MSB=1" aralığında (pencere) yapılmalı.

RCC->APB1ENR |= RCC_APB1ENR_WWDGEN:
    Ne yapar: WWDG çevresinin APB1 clock'unu açar.
    Neden: IWDG'den farklı olarak WWDG normal peripheral clock yönetimine sahiptir.

WWDG->CFR (Configuration Register):
    WDGTB[1:0] (bit 8:7): Prescaler.
        00 = /1   01 = /2   10 = /4   11 = /8
        WWDG_clk = PCLK1 / 4096 / prescaler_bölücü
        (4096 sabit bölücü donanımda dahildir)
    W[6:0] (bit 6:0): Pencere değeri.
        Sayaç bu değerin ÜZERİNDE olduğu sürece refresh YASAKTIR.
        Sayaç W'nin eşit veya altına düştüğünde refresh YAPILABİLİR.

WWDG->CR (Control Register):
    T[6:0] (bit 6:0): Sayaç başlangıç/yeniden yükleme değeri.
        Bit 6 (MSB) = 1 olmalı; bu bit 0 olursa anında reset tetiklenir.
        Geçerli aralık: 0x40 - 0x7F (64-127).
    WDGA (bit 7): Watchdog Activation.
        1 yazıldığında WWDG çalışmaya başlar.
        ÖNEMLİ: Bu bit donanımsal olarak "sticky"dir — yazılımla 0
        yapılamaz. Yalnızca sistem reset'i ile temizlenir.

Bu fonksiyonda WWDG_Init() içinde CR'ye iki ayrı yazma (&= ~T, |= t_value)
yapılması sorun değildir, çünkü WDGA henüz set edilmeden bu yazımlar
gerçekleşir — WWDG henüz aktif olmadığı için T6=0 ara durumu reset
tetiklemez. WDGA bit en son, T zaten doğru değerdeyken set edilir.
(WWDG_Refresh() fonksiyonunda ise WDGA zaten aktif olduğundan durum
farklıdır — aşağıdaki açıklamaya bakınız.)

Timeout Hesabı:
    t_WWDG = (4096 × prescaler_bölücü × (T[5:0] + 1)) / PCLK1
    Örnek: PCLK1 = 42 MHz, prescaler_bölücü = 8, T = 0x7F (T[5:0]=0x3F=63)
        t_WWDG = (4096 × 8 × 64) / 42_000_000 ≈ 50 ms

Pencere Mantığı Örneği:
    T = 0x7F (127), W = 0x5F (95)
    Sayaç 127'den başlar, her WWDG_clk periyodunda 1 azalır.
    127 → 96 arası: refresh YAPILIRSA reset olur (çok erken, W üzerinde).
    95 → 64 arası: refresh YAPILABİLİR (pencere içi).
    63 (0x3F) → MSB=0 olur: refresh YAPILMAZSA reset olur (çok geç).

Neden "Pencere" Watchdog:
    IWDG sadece "çok geç" durumu yakalar (sayaç 0'a ulaşması).
    WWDG ayrıca "çok erken" durumu da yakalar — bu, yazılımın
    BEKLENENDEN HIZLI çalıştığını (örn. bir adımın atlandığını,
    sonsuz döngüye kısa yoldan girildiğini) tespit etmeye yarar.

*/

void WWDG_Refresh(uint8_t t_value){
    WWDG->CR = WWDG_CR_WDGA | (t_value & 0x7F);   // WDGA korunur, T tek seferde yazılır
}

/*

Amaç: WWDG sayacını T değerine yeniden yükleyerek reset'i önlemek.

WWDG->CR = WWDG_CR_WDGA | (t_value & 0x7F):
    Ne yapar: CR register'ına WDGA biti ile birlikte yeni T[6:0] değerini
    TEK BİR yazma işleminde ayarlar.

    NEDEN TEK YAZMA ZORUNLU — KRİTİK NOKTA:
    WWDG_Init() içindeki gibi "önce temizle, sonra yaz" yaklaşımı
    (WWDG->CR &= ~WWDG_CR_T; ardından WWDG->CR |= t_value;) burada
    KULLANILAMAZ. İlk satır CR'ye geçici olarak T[6:0] = 0x00 yazar;
    bu durumda T6 (MSB) anlık olarak 0 olur. WWDG donanımı T6=0 anını
    "sayaç bitti" olarak algılar ve İKİNCİ YAZMA SATIRINA ULAŞILMADAN
    ANINDA RESET tetikler.
    Tek satırlık yazım, CR register'ının bir bus cycle'da güncellenmesini
    sağlar; T6 hiçbir zaman ara durumda 0 görünmez.
    WDGA biti açıkça OR'lanır çünkü WWDG_CR_WDGA donanımsal olarak
    "sticky"dir (zaten 1'dir), fakat register'ın tamamını yazdığımız için
    bu biti de doğru değerde tutmak gerekir — aksi halde yanlışlıkla 0
    yazılırsa (ki donanım bunu yok sayar, ama okunabilirlik/niyet
    açısından doğru değeri yazmak önemlidir).

KRİTİK UYARI — Pencere Kontrolü:
    Bu fonksiyon çağrılmadan önce mevcut sayaç değeri (WWDG->CR & WWDG_CR_T)
    okunmalı ve W[6:0] değerinin ALTINDA (veya eşit) olduğu doğrulanmalıdır.
    Eğer sayaç hâlâ W'nin ÜZERİNDEYKEN bu fonksiyon çağrılırsa,
    donanım bunu "çok erken refresh" olarak algılar ve ANINDA RESET tetikler.

Güvenli kullanım deseni:
    uint8_t current = WWDG->CR & WWDG_CR_T;
    if(current <= w_value){
        WWDG_Refresh(t_value);  // Pencere içinde, güvenli
    }
    // else: henüz refresh yapma, bekle

Pratik öneri:
    WWDG, IWDG'ye göre daha hassas zamanlamalı bir mekanizmadır.
    Genellikle bir timer interrupt'ı (örn. TIM2) içinde, pencere
    aralığına denk gelecek şekilde çağrılır.
    EWI (Early Wakeup Interrupt) kullanılarak sayaç W'ye yaklaştığında
    interrupt alınabilir (WWDG_CFR_EWI biti); bu sayede tam zamanında
    refresh yapılabilir.

*/

/* KULLANIM VE TEST SENARYOLARI

   main.c içinde WDG_TEST_SELECT makrosu (1-4) ile farklı senaryolar
   test edilebilir. Tüm testlerde RCC->CSR okunarak reset sebebi
   (reset_reason) doğrulanır ve RCC->CSR |= RCC_CSR_RMVF; ile flag'ler
   temizlenir.

   Ortak hazırlık:
       DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP | DBGMCU_APB1_FZ_DBG_WWDG_STOP;
       Bu satır, debug sırasında CPU durduğunda IWDG/WWDG sayaçlarının
       da donmasını sağlar. WWDG testlerinde (timeout ~50ms gibi çok kısa
       olduğundan) bu satır YORUMA ALINMAMALI; aksi halde herhangi bir
       breakpoint'te anında reset oluşur ve debug imkansız hale gelir.

   TEST A (WDG_TEST_SELECT = 1) — IWDG, kasıtlı kilitleme:
       IWDG_Init(4, 2499);  // IWDG_clk=500Hz, RLR=2499 -> ~5 sn timeout
       while(1) içinde 3 kez IWDG_Refresh() + HAL_Delay(1000), sonra
       sonsuz boş döngüye girilir (refresh durur).
       Beklenen: ~5 sn sonra reset, reset_reason içinde IWDGRSTF (bit24).

   TEST B1 (WDG_TEST_SELECT = 2) — WWDG, pencere üstü ihlali:
       WWDG_Init(0x7F, 0x5F, 3);
       Hemen ardından WWDG_Refresh(0x7F) çağrılır. Sayaç hâlâ 0x7F
       (W=0x5F'nin üzerinde) olduğundan bu pencere ihlalidir.
       Beklenen: ANINDA reset, reset_reason içinde WWDGRSTF (bit30).

   TEST B2 (WDG_TEST_SELECT = 3) — WWDG, timeout ihlali:
       WWDG_Init(0x7F, 0x5F, 3);
       Hiç refresh çağrılmaz.
       Beklenen: ~50 ms sonra reset, reset_reason içinde WWDGRSTF (bit30).

   TEST B3 (WDG_TEST_SELECT = 4) — WWDG, doğru kullanım:
       WWDG_Init(0x7F, 0x5F, 3); + PD12 GPIO init.
       while(1) içinde:
           wwdg_current = WWDG->CR & WWDG_CR_T;
           if(wwdg_current <= 0x5F){
               WWDG_Refresh(0x7F);
               GPIOD->ODR ^= (1 << 12);   // Her başarılı refresh'te LED toggle
           }
       Beklenen: Reset OLMAMALI. PD12 LED, WWDG clock periyoduna bağlı
       bir hızda yanıp söner (sürekli açık/kapalı görünmez).

       NOT — neden delay KULLANILMAZ:
       WWDG timeout'u (~50ms) çok kısadır. Döngü içine HAL_Delay(100) gibi
       bir gecikme eklenirse, sayaç pencereye düşmeden timeout'a uğrar ve
       sistem sürekli resetlenir (TEST B2 davranışına döner). Bu yüzden
       wwdg_current sürekli, gecikmesiz kontrol edilmelidir.

*/
