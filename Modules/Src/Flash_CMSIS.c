
#include <Flash_CMSIS.h>

/* Flash_Unlock — Flash yazma korumasını kaldır */
void Flash_Unlock(void){
    if(FLASH->CR & FLASH_CR_LOCK){                              // Flash kilitli mi kontrol et
        FLASH->KEYR = 0x45670123;                               // Kilit açma anahtarı 1
        FLASH->KEYR = 0xCDEF89AB;                               // Kilit açma anahtarı 2
    }
}

/*

Amaç: Flash program/erase işlemleri için yazma korumasını kaldırmak.
Referans: RM0090 — Bölüm 3 (Flash Interface).

FLASH->CR & FLASH_CR_LOCK:
    Ne yapar: CR register'ındaki LOCK bitini kontrol eder.
    LOCK = 1: Flash yazma/silme işlemleri engellenmiş.
    LOCK = 0: Flash yazma/silme işlemleri serbest.
    Neden kontrol: Zaten açıksa tekrar anahtar göndermek LOCK bitini
    yeniden set eder (güvenlik mekanizması) — kontrol şarttır.

FLASH->KEYR = 0x45670123 ve 0xCDEF89AB:
    Ne yapar: İki özel anahtar sırasıyla KEYR register'ına yazılır.
    Bu iki anahtar doğru sırayla gönderildiğinde LOCK biti temizlenir.
    Yanlış sıra veya yanlış değer → donanım LOCK bitini kalıcı set eder
    (bir sonraki reset'e kadar Flash erişilemez olur).
    Anahtarlar RM0090'da sabit değerler olarak tanımlanmıştır.

*/

/* Flash_Lock — Flash yazma korumasını etkinleştir */
void Flash_Lock(void){
    FLASH->CR |= FLASH_CR_LOCK;                                 // LOCK bitini set et
}

/*

Amaç: Flash işlemleri tamamlandıktan sonra yazma korumasını geri etkinleştirmek.

FLASH->CR |= FLASH_CR_LOCK:
    Ne yapar: CR register'ındaki LOCK bitini set eder.
    Neden gerekli: Flash işlemi bittikten sonra kilitleme güvenlik açısından
    iyi bir pratiktir; yanlışlıkla Flash'a yazılması önlenir.
    Not: LOCK biti yalnızca doğru KEYR anahtarları ile açılabilir;
    yazılımla doğrudan temizlenemez (hardware protection).

*/

/*  Flash_EraseSector — Belirtilen sektörü sil */
void Flash_EraseSector(uint8_t sector){
    while(FLASH->SR & FLASH_SR_BSY);                            // Flash meşgulse bekle

    Flash_Unlock();                                             // Yazma korumasını kaldır

    FLASH->CR &= ~FLASH_CR_SNB;                                 // Sektör numarası bitlerini temizle [CR bit 6:3]
    FLASH->CR |= (sector << FLASH_CR_SNB_Pos);                  // Silinecek sektörü seç
    FLASH->CR |= FLASH_CR_PSIZE_1;                              // Program boyutu: 32-bit (VDD = 3.3V) [PSIZE = 10]
    FLASH->CR |= FLASH_CR_SER;                                  // Sector Erase modu seç
    FLASH->CR |= FLASH_CR_STRT;                                 // Silme işlemini başlat

    while(FLASH->SR & FLASH_SR_BSY);                            // Silme tamamlanana kadar bekle

    FLASH->CR &= ~FLASH_CR_SER;                                 // SER bitini temizle
    FLASH->CR &= ~FLASH_CR_SNB;                                 // SNB bitlerini temizle

    Flash_Lock();                                               // Yazma korumasını geri etkinleştir
}

/*

Amaç: Belirtilen Flash sektörünü silmek (tüm bitler 1 = 0xFF yapılır).
Referans: RM0090 — Bölüm 3.5 (Flash Erase).

FLASH->SR & FLASH_SR_BSY:
    Ne yapar: SR register'ındaki BSY (Busy) bitini kontrol eder.
    BSY = 1: Flash işlem devam ediyor (program veya erase).
    BSY = 0: Flash hazır.
    Neden önce kontrol: Önceki bir işlem devam ederken yeni işlem başlatmak
    tanımsız davranışa yol açabilir.

FLASH->CR &= ~FLASH_CR_SNB ve |= (sector << SNB_Pos):
    Ne yapar: CR register'ındaki SNB[3:0] (Sector Number) alanına
    silinecek sektör numarasını (0-11) yazar.
    Neden önce temizle: Önceki sektör numarasının kalıntısını önlemek için.

FLASH->CR |= FLASH_CR_PSIZE_1 (PSIZE = 10):
    Ne yapar: Program/erase işlemi için veri genişliğini 32-bit seçer.
    PSIZE tablosu:
        00 = x8  (1.8V - 2.1V VDD)
        01 = x16 (2.1V - 2.7V VDD)
        10 = x32 (2.7V - 3.6V VDD) ← STM32F4 Discovery (3.3V) için doğru
        11 = x64 (harici Vpp gerektirir)
    Yanlış PSIZE seçimi veri bütünlüğü sorunlarına yol açabilir.

FLASH->CR |= FLASH_CR_SER (Sector Erase):
    Ne yapar: Silme modunu "sektör silme" olarak ayarlar.
    SER = 1, MER = 0: Yalnızca seçili sektör silinir.
    MER = 1: Tüm Flash silinir (Mass Erase) — dikkatli kullanılmalıdır!

FLASH->CR |= FLASH_CR_STRT (Start):
    Ne yapar: Silme işlemini başlatır.
    Bu bit set edildikten sonra donanım BSY bitini set eder ve
    silme tamamlanınca otomatik temizler.

ÖNEMLİ UYARILAR:
    Flash sektörü silinmeden üzerine veri yazılamaz (0→1 yazılamaz,
    yalnızca 1→0 yazılabilir; silme işlemi tüm bitleri 1 yapar).
    Silme işlemi geri alınamaz — yanlış sektör silinirse kod veya veri kaybı yaşanır.
    Sektör 0 genellikle interrupt vector table'ı içerir; silinmesi sistemi çökertir.
    Uygulama kodu genellikle Sektör 0-4'te bulunur; kullanıcı verisi için
    Sektör 6-11 (128 KB) tercih edilmelidir.

*/

/* Flash_ReadWord — 32-bit veri oku */
uint32_t Flash_ReadWord(uint32_t address){
    return *(__IO uint32_t *)address;                           // Belirtilen adresten 32-bit oku
}

/*

Amaç: Flash belleğin belirtilen adresinden 32-bit veri okumak.

*(__IO uint32_t *)address:
    Ne yapar: Verilen adresi uint32_t pointer'a cast ederek dereference eder.
    __IO: volatile qualifier — derleyicinin bu okumayı optimize etmemesini sağlar.
    Neden volatile: Flash okumalarının her zaman gerçek bellek adresinden
    yapılmasını garanti eder; register'a alınmış (cached) değer dönmez.

Flash okuma için unlock gerekmez:
    Flash okuma işlemi her zaman serbesttir; LOCK biti yalnızca
    yazma/silme işlemlerini etkiler. Okuma için CR veya SR
    register'larına müdahale gerekmez.

Adres hizalaması:
    32-bit okuma için adres 4'ün katı olmalıdır (word-aligned).
    Hizasız adres erişimi Hard Fault üretebilir.

*/

/* Flash_ReadBuffer — Birden fazla 32-bit veri oku */
void Flash_ReadBuffer(uint32_t address, uint32_t *data, uint32_t length){
    for(uint32_t i = 0; i < length; i++){
        data[i] = Flash_ReadWord(address + (i * 4));            // Her word 4 byte offset ile okunur
    }
}

/*

Amaç: Flash'tan ardışık 32-bit verileri bir diziye okumak.

address + (i * 4):
    Her uint32_t 4 byte kapladığından, her iterasyonda adres 4 artırılır.
    Örnek: address=0x08060000, i=2 → 0x08060008 okunur.

*/

/* Flash_WriteWord — 32-bit veri yaz */
void Flash_WriteWord(uint32_t address, uint32_t data){
    while(FLASH->SR & FLASH_SR_BSY);                            // Flash meşgulse bekle

    Flash_Unlock();                                             // Yazma korumasını kaldır

    FLASH->CR &= ~FLASH_CR_PSIZE;                               // PSIZE bitlerini temizle
    FLASH->CR |= FLASH_CR_PSIZE_1;                              // PSIZE = 10 → 32-bit yazma
    FLASH->CR |= FLASH_CR_PG;                                   // Program modu etkinleştir

    *(__IO uint32_t *)address = data;                           // Veriyi adrese yaz

    while(FLASH->SR & FLASH_SR_BSY);                            // Yazma tamamlanana kadar bekle

    FLASH->CR &= ~FLASH_CR_PG;                                  // Program modunu kapat

    Flash_Lock();                                               // Yazma korumasını geri etkinleştir
}

/*

Amaç: Flash belleğin belirtilen adresine 32-bit veri yazmak.

FLASH->CR |= FLASH_CR_PG (Program):
    Ne yapar: Flash program modunu etkinleştirir.
    PG = 1 iken belirtilen adrese yazma yapıldığında donanım
    otomatik olarak Flash programlama işlemini başlatır.
    PG = 0 iken adrese yazma yapılırsa donanım PGSERR (Programming
    Sequence Error) hatası üretir.

*(__IO uint32_t *)address = data:
    Ne yapar: Veriyi belirtilen Flash adresine yazar.
    Bu yazma işlemi donanım tarafından Flash programlama döngüsüne
    dönüştürülür; normal RAM yazmasından farklıdır.

FLASH->CR &= ~FLASH_CR_PG:
    Ne yapar: Program modunu kapatır.
    Neden: PG açık kalırsa sonraki herhangi bir bellek yazması
    istemeden Flash'a program yapabilir — güvenlik riski.

ÖNEMLİ UYARILAR:
    Yazılacak adres önceden silinmiş olmalıdır (tüm bitler 1).
    0 olan bir bite 1 yazılamaz; yalnızca 1→0 dönüşümü mümkündür.
    Aynı adrese iki kez yazma donanım hatası (PGSERR) üretebilir.
    Adres 4'ün katı olmalıdır (word-aligned).

ECC (Error Correction Code):
    STM32F4 Flash'ı ECC destekler; her 32-bit word için 8-bit ECC
    bilgisi donanım tarafından otomatik hesaplanır ve saklanır.
    Aynı adrese iki kez yazma ECC hatasına yol açabilir.

*/

/* Flash_WriteBuffer — Birden fazla 32-bit veri yaz */
void Flash_WriteBuffer(uint32_t address, uint32_t *data, uint32_t length){
    for(uint32_t i = 0; i < length; i++){
        Flash_WriteWord(address + (i * 4), data[i]);            // Her word 4 byte offset ile yazılır
    }
}

/*

Amaç: Flash'a ardışık 32-bit verileri bir diziden yazmak.

Her word için Flash_WriteWord çağrısı:
    Unlock, yazma, BSY bekleme ve Lock işlemleri her word için ayrı yapılır.
    Bu yaklaşım güvenlidir ancak yavaştır — her iterasyonda lock/unlock overhead'i vardır.
    Performans kritik uygulamalarda Unlock bir kez yapılıp döngü sonunda Lock
    çağrılabilir; ancak bu durumda hata yönetimi daha dikkatli yapılmalıdır.

Kullanım örneği:
    uint32_t myData[4] = {0xDEADBEEF, 0x12345678, 0xAABBCCDD, 0x11223344};
    Flash_EraseSector(6);                    // Önce sektörü sil
    Flash_WriteBuffer(FLASH_SECTOR6_ADDR, myData, 4);   // 4 word yaz
    // Doğrulama:
    uint32_t readBack[4];
    Flash_ReadBuffer(FLASH_SECTOR6_ADDR, readBack, 4);  // Geri oku

*/
