
#include "RTC_CMSIS.h"


static uint8_t Dec2BCD(uint8_t val){
    return ((val / 10) << 4) | (val % 10);
}

static uint8_t BCD2Dec(uint8_t val){
    return ((val >> 4) * 10) + (val & 0x0F);
}
/*
   Yardımcı fonksiyon: Decimal → BCD dönüşümü
   RTC register'ları BCD (Binary Coded Decimal) formatında çalışır.
   Örnek: 25 → 0x25 (üst nibble = 2, alt nibble = 5)
*/

void RTC_Init(void){
    /* 0. PWR clock'unu aç ve backup domain erişimini etkinleştir */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;                         // PWR çevresinin clock'unu aç
    (void)RCC->APB1ENR;                                         // Bus senkronizasyonu
    PWR->CR |= PWR_CR_DBP;                                      // Backup domain yazma korumasını kaldır

    /* 2. Backup domain reset (temiz başlangıç için) */
    RCC->BDCR |= RCC_BDCR_BDRST;                               // Backup domain reset
    RCC->BDCR &= ~RCC_BDCR_BDRST;                              // Reset'i kaldır

    /* 3. HSE prescaler: RTCPRE = 8 → f_RTC = 8 MHz / 8 = 1 MHz */
    RCC->CFGR &= ~RCC_CFGR_RTCPRE;                             // RTCPRE bitlerini temizle [CFGR bit 20:16]
    RCC->CFGR |= (8 << 16);                                    // RTCPRE = 8

    /* 4. RTC clock kaynağı: HSE / RTCPRE */
    RCC->BDCR &= ~RCC_BDCR_RTCSEL;                             // RTC clock seçim bitlerini temizle
    RCC->BDCR |= RCC_BDCR_RTCSEL_0 | RCC_BDCR_RTCSEL_1;       // HSE / RTCPRE seç [RTCSEL = 11]

    /* 5. RTC clock'unu etkinleştir */
    RCC->BDCR |= RCC_BDCR_RTCEN;                               // RTC clock enable

    /* 6. RTC yazma korumasını kaldır */
    RTC->WPR = 0xCA;                                            // Yazma koruması 1. anahtar
    RTC->WPR = 0x53;                                            // Yazma koruması 2. anahtar

    /* 7. Initialization moduna gir */
    RTC->ISR |= RTC_ISR_INIT;                                   // INIT bit'i set et
    while(!(RTC->ISR & RTC_ISR_INITF));                         // INITF set olana kadar bekle

    /* 8. Prescaler ayarları: f_RTC = 1 MHz → 1 Hz */
    /* PREDIV_A = 124 (7-bit asenkron), PREDIV_S = 7999 (15-bit senkron) */
    /* f_ck_spre = 1_000_000 / (125 × 8000) = 1 Hz */
    RTC->PRER = (124  << RTC_PRER_PREDIV_A_Pos) |              // PREDIV_A = 124
                (7999 << RTC_PRER_PREDIV_S_Pos);               // PREDIV_S = 7999

    /* 9. 24 saat formatı seç */
    RTC->CR &= ~RTC_CR_FMT;                                     // FMT = 0 → 24 saat formatı

    /* 10. Timestamp etkinleştir (yükselen kenarda) */
    RTC->CR |= RTC_CR_TSE;                                      // Timestamp enable
    RTC->CR &= ~RTC_CR_TSEDGE;                                  // Yükselen kenar seç (0 = rising edge)

    /* 11. Initialization modundan çık */
    RTC->ISR &= ~RTC_ISR_INIT;                                  // INIT bit'i temizle

    /* 12. Yazma korumasını geri etkinleştir */
    RTC->WPR = 0xFF;
}

/*

Amaç: RTC'yi HSE/128 clock kaynağı ile 1 Hz ck_spre frekansında yapılandırmak.
Referans: RM0090 — Bölüm 26 (RTC).

RTC Genel Bilgi:
    STM32F4 RTC'si BCD (Binary Coded Decimal) formatında saat/tarih tutar.
    Backup domain'de çalışır; VDD kesilse bile VBAT pini ile çalışmaya devam edebilir.
    Yazma işlemleri için önce PWR->CR |= DBP (Disable Backup Protection) yapılmalı,
    ardından RTC->WPR anahtarları gönderilmelidir.

RCC->BDCR & RCC_BDCR_RTCEN (Başlangıç Kontrolü):
    Ne yapar: RTC'nin zaten çalışıp çalışmadığını kontrol eder.
    Neden: Her reset sonrası RTC_Init() çağrıldığında backup domain resetlenirse
    mevcut saat bilgisi kaybolur. RTCEN bit'i set ise RTC daha önce yapılandırılmış
    ve çalışıyor demektir; tekrar init etmeye gerek yoktur.
    Dikkat: Bu kontrol yalnızca VDD kesintisiz kaldığında işe yarar.
    VDD tamamen kesilirse backup domain resetlenir ve RTCEN = 0 olur;
    bu durumda RTC yeniden yapılandırılmalıdır (VBAT pinine pil bağlanırsa
    VDD kesilse bile backup domain korunur).

RCC->APB1ENR |= RCC_APB1ENR_PWREN:
    Ne yapar: PWR (Power Control) çevresinin clock'unu açar.
    Neden: PWR->CR register'ına erişmek için PWR clock'unun açık olması gerekir.

PWR->CR |= PWR_CR_DBP (Disable Backup Protection):
    Ne yapar: Backup domain register'larına yazma korumasını kaldırır.
    Neden: RTC, RCC->BDCR ve backup register'ları (BKPxR) normalde yazma
    korumalıdır. Bu bit set edilmeden bu register'lara yazılamaz.

RCC->BDCR |= RCC_BDCR_BDRST:
    Ne yapar: Backup domain'i (RTC dahil) resetler.
    Neden: Önceki bir konfigürasyondan kalma BDCR içeriğini temizler; temiz
    başlangıç için önerilir. Reset sonrası BDRST temizlenmelidir.

RCC->BDCR RTCSEL = 11 (HSE/128):
    Ne yapar: RTC clock kaynağını HSE/128 olarak seçer.
    RTCSEL değerleri:
        00 = Clock yok
        01 = LSE (32.768 kHz dış kristal)
        10 = LSI (~32 kHz iç osilatör)
        11 = HSE / RTCPRE
    RTCPRE: RCC->CFGR içindeki RTCPRE[4:0] alanı HSE'yi böler.
        RTCPRE = 10000 (binary) = 16 → bekle, STM32F4'te RTCPRE değeri
        doğrudan bölücü değeridir: 0b10000 = 16'dır.
        Ancak 128'e bölmek için: RTCPRE = 128 → binary 0b10000000.
        Not: STM32F4'te RTCPRE değerleri 2–31 arasında anlamlıdır;
        tam 128'e bölmek için RTCPRE = 0b11111 (31) maksimum değerdir.

    ÖNEMLI DÜZELTME: STM32F4 RCC->CFGR RTCPRE alanı [20:16] bitleridir,
    5-bit genişliğinde (max bölücü = 31). HSE = 8 MHz için:
        RTCPRE = 8 → 8 MHz / 8 = 1 MHz → PREDIV_A ve PREDIV_S ile 1 Hz
        RTCPRE = 31 → 8 MHz / 31 ≈ 258 kHz
    Önerilen: RTCPRE = 8 → f_RTC = 1 MHz;
    ardından PREDIV_A = 124, PREDIV_S = 7999 → 1 Hz.
    (Not: PREDIV_S 15-bit, max 32767; 7999 bu sınır içindedir.)

RTC->WPR = 0xCA ve 0x53:
    Ne yapar: RTC register yazma korumasını iki adımlı özel anahtar ile kaldırır.
    0xCA ardından 0x53 yazılmazsa RTC register'larına yazılamaz.
    Konfigürasyon sonunda 0xFF yazılarak koruma yeniden etkinleştirilir.

RTC->ISR |= RTC_ISR_INIT (Initialization Mode):
    Ne yapar: RTC sayacını durdurur ve register yazımına izin verir.
    INITF bayrağı set olana kadar beklemek gerekir (~2 RTC clock cycle).
    Neden: INIT modu olmadan TR, DR, PRER gibi register'lara yazılamaz.

RTC->PRER (Prescaler Register):
    PREDIV_A[6:0] (bit 22:16): Asenkron prescaler. Max 127.
    PREDIV_S[14:0] (bit 14:0): Senkron prescaler. Max 32767.
    Formül: f_ck_spre = f_RTC / ((PREDIV_A+1) × (PREDIV_S+1))
    Hedef: 1 Hz ck_spre (saniyede bir saat güncelleme).

RTC->CR |= RTC_CR_TSE (Timestamp Enable):
    Ne yapar: Timestamp özelliğini etkinleştirir.
    Seçilen kenarda (rising/falling) TSSR/TSDR register'larına anlık saat/tarih
    kaydedilir; bu özellik harici tetikleme veya tamper olaylarını kaydetmek için
    kullanılır. TSEDGE = 0 → yükselen kenar seçilir.

RTC->CR &= ~RTC_CR_FMT:
    Ne yapar: 24 saat formatını seçer (FMT = 0).
    FMT = 1 yapılırsa 12 saat formatına geçilir ve AM/PM biti kullanılır.

*/

void RTC_SetTime(uint8_t hour, uint8_t min, uint8_t sec){
    RTC->WPR = 0xCA;                                            // Yazma korumasını kaldır
    RTC->WPR = 0x53;

    RTC->ISR |= RTC_ISR_INIT;                                   // Init modu
    while(!(RTC->ISR & RTC_ISR_INITF));

    RTC->TR = (Dec2BCD(hour) << RTC_TR_HU_Pos)  |              // Saat (BCD)
              (Dec2BCD(min)  << RTC_TR_MNU_Pos)  |             // Dakika (BCD)
              (Dec2BCD(sec)  << RTC_TR_SU_Pos);                 // Saniye (BCD)

    RTC->ISR &= ~RTC_ISR_INIT;                                  // Init modundan çık
    RTC->WPR = 0xFF;                                            // Yazma korumasını etkinleştir
}

/*

Amaç: RTC saat register'ına (TR) saat, dakika ve saniye yazmak.

RTC->TR (Time Register):
    HT[1:0]  (bit 21:20): Saatin onlar basamağı (BCD)
    HU[3:0]  (bit 19:16): Saatin birler basamağı (BCD)
    MNT[2:0] (bit 14:12): Dakikanın onlar basamağı (BCD)
    MNU[3:0] (bit 11:8):  Dakikanın birler basamağı (BCD)
    ST[2:0]  (bit 6:4):   Saniyenin onlar basamağı (BCD)
    SU[3:0]  (bit 3:0):   Saniyenin birler basamağı (BCD)

BCD formatı: Dec2BCD(25) = 0x25 → üst nibble=2, alt nibble=5.
TR register'ına doğrudan decimal değer YAZILAMAz; önce BCD'ye
dönüştürülmelidir.

Neden Init modu: TR register'ına yalnızca initialization modunda yazılabilir.
Normal çalışmada RTC donanımı TR'yi güncellediğinden yazma engellenir.

*/

void RTC_SetDate(uint8_t year, uint8_t month, uint8_t day, uint8_t weekday){
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    RTC->ISR |= RTC_ISR_INIT;
    while(!(RTC->ISR & RTC_ISR_INITF));

    RTC->DR = (Dec2BCD(year)    << RTC_DR_YU_Pos)  |           // Yıl (BCD, son 2 hane: 0-99)
              (Dec2BCD(month)   << RTC_DR_MU_Pos)   |          // Ay (BCD)
              (Dec2BCD(day)     << RTC_DR_DU_Pos)   |          // Gün (BCD)
              (weekday          << RTC_DR_WDU_Pos);             // Haftanın günü (1=Pazartesi, 7=Pazar)

    RTC->ISR &= ~RTC_ISR_INIT;
    RTC->WPR = 0xFF;
}

/*

Amaç: RTC tarih register'ına (DR) yıl, ay, gün ve haftanın günü yazmak.

RTC->DR (Date Register):
    YT[3:0]  (bit 23:20): Yılın onlar basamağı (BCD)
    YU[3:0]  (bit 19:16): Yılın birler basamağı (BCD) — yalnızca son 2 hane (0-99)
    WDU[2:0] (bit 15:13): Haftanın günü (1=Pazartesi ... 7=Pazar)
    MT       (bit 12):    Ayın onlar basamağı (0 veya 1)
    MU[3:0]  (bit 11:8):  Ayın birler basamağı (BCD)
    DT[1:0]  (bit 5:4):   Günün onlar basamağı (BCD)
    DU[3:0]  (bit 3:0):   Günün birler basamağı (BCD)

Yıl notu: RTC yalnızca son 2 haneyi saklar (örn. 2025 → 25).
Haftanın günü otomatik hesaplanmaz; kullanıcı tarafından doğru girilmelidir.

*/

void RTC_GetTime(uint8_t *hour, uint8_t *min, uint8_t *sec){
    uint32_t tr = RTC->TR;                                      // TR register'ını oku (okuma shadow register'ı günceller)

    *hour = BCD2Dec((tr & (RTC_TR_HT_Msk  | RTC_TR_HU_Msk))  >> RTC_TR_HU_Pos);
    *min  = BCD2Dec((tr & (RTC_TR_MNT_Msk | RTC_TR_MNU_Msk)) >> RTC_TR_MNU_Pos);
    *sec  = BCD2Dec((tr & (RTC_TR_ST_Msk  | RTC_TR_SU_Msk))  >> RTC_TR_SU_Pos);
}

/*

Amaç: RTC'den anlık saat, dakika ve saniye bilgisini okumak.

Shadow Register Mekanizması:
    RTC, TR ve DR'yi doğrudan okumak yerine her saniye bu değerleri
    "shadow register"larına kopyalar. Okuma bu shadow register'lardan yapılır.
    TR okunduktan sonra DR okunmalıdır; aksi halde geçiş anında (23:59:59 → 00:00:00)
    TR ve DR tutarsız görünebilir.

BCD2Dec dönüşümü:
    TR register'ı BCD formatında veri tutar. Örn: 0x25 → 25 (decimal).
    BCD2Dec fonksiyonu üst nibble'ı 10 ile çarpar, alt nibble'ı ekler.

*/

void RTC_GetDate(uint8_t *year, uint8_t *month, uint8_t *day, uint8_t *weekday){
    uint32_t dr = RTC->DR;                                      // DR register'ını oku

    *year    = BCD2Dec((dr & (RTC_DR_YT_Msk | RTC_DR_YU_Msk)) >> RTC_DR_YU_Pos);
    *month   = BCD2Dec((dr & (RTC_DR_MT_Msk | RTC_DR_MU_Msk)) >> RTC_DR_MU_Pos);
    *day     = BCD2Dec((dr & (RTC_DR_DT_Msk | RTC_DR_DU_Msk)) >> RTC_DR_DU_Pos);
    *weekday = (dr & RTC_DR_WDU_Msk) >> RTC_DR_WDU_Pos;
}

/*

Amaç: RTC'den anlık tarih bilgisini okumak.

Okuma sırası uyarısı:
    STM32F4 RTC'de TR okunduğunda shadow register kilitlenir ve DR okunana kadar
    güncellenmez. Bu nedenle GetTime() ardından GetDate() çağrılması önerilir;
    böylece saat ve tarih bilgisi tutarlı bir anlık görüntüyü temsil eder.

*/

void RTC_SetAlarm(uint8_t hour, uint8_t min, uint8_t sec){
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    RTC->CR  &= ~RTC_CR_ALRAE;                                  // Alarm A'yı devre dışı bırak (yazma için)
    while(!(RTC->ISR & RTC_ISR_ALRAWF));                        // Alarm A yazma hazır olana kadar bekle

    RTC->ALRMAR = (Dec2BCD(hour) << RTC_ALRMAR_HU_Pos)  |      // Alarm saati (BCD)
                  (Dec2BCD(min)  << RTC_ALRMAR_MNU_Pos)  |     // Alarm dakikası (BCD)
                  (Dec2BCD(sec)  << RTC_ALRMAR_SU_Pos)   |     // Alarm saniyesi (BCD)
                  RTC_ALRMAR_MSK4;                              // Gün/tarih maskesi: günü yoksay

    RTC->CR  |= RTC_CR_ALRAE;                                   // Alarm A'yı etkinleştir
    RTC->CR  |= RTC_CR_ALRAIE;                                  // Alarm A interrupt'ını etkinleştir

    /* EXTI Line 17 (RTC Alarm) yapılandırması */
    EXTI->IMR  |= EXTI_IMR_MR17;                                // EXTI line 17 interrupt mask enable
    EXTI->RTSR |= EXTI_RTSR_TR17;                               // Yükselen kenar tetikleme

    /* NVIC yapılandırması */
    NVIC_SetPriority(RTC_Alarm_IRQn, 1);
    NVIC_EnableIRQ(RTC_Alarm_IRQn);

    RTC->WPR = 0xFF;
}

/*

Amaç: RTC Alarm A'yı belirtilen saat:dakika:saniye'de tetiklenecek şekilde ayarlamak.

RTC->CR &= ~RTC_CR_ALRAE:
    Ne yapar: Alarm A'yı devre dışı bırakır.
    Neden: ALRMAR register'ına yalnızca alarm devre dışıyken yazılabilir.
    ALRAWF (Alarm A Write Flag) set olana kadar beklemek gerekir.

RTC->ALRMAR (Alarm A Register):
    HT/HU   (bit 21:16): Alarm saati (BCD)
    MNT/MNU (bit 14:8):  Alarm dakikası (BCD)
    ST/SU   (bit 6:0):   Alarm saniyesi (BCD)
    MSK4 (bit 31): Gün/tarih maskesi — 1 yapılırsa gün karşılaştırması yapılmaz,
                   yalnızca saat:dakika:saniye eşleşmesi aranır.
    MSK3 (bit 23): Saat maskesi — 1 yapılırsa saat karşılaştırması yapılmaz.
    MSK2 (bit 15): Dakika maskesi.
    MSK1 (bit 7):  Saniye maskesi.

EXTI Line 17 (RTC Alarm):
    RTC alarm interrupt'ı EXTI line 17 üzerinden NVIC'e iletilir.
    IMR ve RTSR ayarlanmazsa alarm interrupt'ı NVIC'e ulaşmaz.

*/

void RTC_GetTimestamp(uint8_t *hour, uint8_t *min, uint8_t *sec){
    if(RTC->ISR & RTC_ISR_TSF){                                 // Timestamp flag set mi?
        uint32_t tsr = RTC->TSTR;                               // Timestamp saat register'ını oku

        *hour = BCD2Dec((tsr & (RTC_TSTR_HT_Msk  | RTC_TSTR_HU_Msk))  >> RTC_TSTR_HU_Pos);
        *min  = BCD2Dec((tsr & (RTC_TSTR_MNT_Msk | RTC_TSTR_MNU_Msk)) >> RTC_TSTR_MNU_Pos);
        *sec  = BCD2Dec((tsr & (RTC_TSTR_ST_Msk  | RTC_TSTR_SU_Msk))  >> RTC_TSTR_SU_Pos);

        RTC->ISR &= ~RTC_ISR_TSF;                               // TSF flag'ini temizle
    }
}

/*

Amaç: RTC Timestamp register'ından tetikleme anındaki saat bilgisini okumak.

Timestamp Mekanizması:
    RTC_CR içindeki TSE biti set edildiğinde, RTC_TAFCR'deki TAMP1E veya
    harici bir pin tetiklemesinde anlık saat (TSTR) ve tarih (TSDR) kaydedilir.
    Bu özellik genellikle sistem olaylarını kaydetmek (loglama) için kullanılır.

RTC->ISR & RTC_ISR_TSF:
    Ne yapar: Timestamp Flag'in set olup olmadığını kontrol eder.
    TSF set olmadan TSTR/TSDR okunursa geçersiz veri alınabilir.

RTC->ISR &= ~RTC_ISR_TSF:
    Ne yapar: TSF flag'ini temizler.
    Neden: Temizlenmezse bir sonraki timestamp olayında üzerine yazılamaz
    (TSOVF — Timestamp Overflow Flag tetiklenebilir).

*/

void RTC_Alarm_IRQHandler(void){
    if(RTC->ISR & RTC_ISR_ALRAF){                               // Alarm A flag set mi?
        RTC->ISR &= ~RTC_ISR_ALRAF;                             // Alarm A flag'ini temizle
        EXTI->PR  |= EXTI_PR_PR17;                              // EXTI line 17 pending bit'ini temizle

        /* Kullanıcı işlemleri buraya eklenebilir */
        /* Örn: LED toggle, UART mesajı, görev başlatma */
    }
}

/*

Amaç: RTC Alarm A interrupt'ını işlemek.

RTC->ISR & RTC_ISR_ALRAF:
    Ne yapar: Alarm A Flag'in set olup olmadığını kontrol eder.
    Neden: EXTI Line 17, Alarm A ve Alarm B'yi paylaşır; hangi alarmın
    tetiklendiği kontrol edilmelidir (çoklu alarm durumunda).

RTC->ISR &= ~RTC_ISR_ALRAF:
    Ne yapar: Alarm A pending flag'ini temizler.
    Neden: Temizlenmezse interrupt sürekli tetiklenmeye devam eder.

EXTI->PR |= EXTI_PR_PR17:
    Ne yapar: EXTI line 17'nin pending bit'ini temizler.
    Neden: RTC alarm interrupt'ı EXTI üzerinden gelir; hem RTC ISR flag'i
    hem de EXTI pending bit'i temizlenmelidir, aksi halde NVIC tekrar tetiklenir.
    Temizleme sırası: önce RTC->ISR, sonra EXTI->PR.

*/
