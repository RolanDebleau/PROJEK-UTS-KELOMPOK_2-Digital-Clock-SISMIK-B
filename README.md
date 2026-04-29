# Digital Clock

## Anggota Kelompok:

```
  1. Fachriel Yoga Wicaksono      (H1H024042)
  2. Dimas Rafif Zaidam           (H1H024043)
  3. Chaedar Ali Amrulloh	      (H1H024044)
  4. Bintang Nugraha Putra        (H1H024045)
  5. M.Fawaz Akbar                (H1H024046)
  6. Gerard Roland Kusuma Sarwoko (H1H024047)
```

---

## Deskripsi Proyek

Sistem ini merupakan **jam digital berbasis Arduino UNO** yang memiliki fitur utama:

- Menampilkan waktu real-time menggunakan modul RTC DS3231
- Tampilan jam, menit, detik, tanggal, bulan, tahun dan persentase baterai pada layar OLED SSD1306
- Pergantian mode tampilan menggunakan tombol push button
- Buzzer sebagai alarm atau notifikasi
- Alarm dapat diatur manual dengan menekan button dan
- Sinkronisasi waktu otomatis saat RTC kehilangan daya

---

## Komponen yang Digunakan

- Arduino UNO
- RTC DS3231
- OLED SSD1306 128x64 (I2C)
- Speaker Piezo
- Push Button
- Rotary Encoder
- Baterai LiPo 9V
- Kabel jumper

---

## Library yang Digunakan

```cpp
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TimeLib.h>
#include <Wire.h>
#include <EEPROM.h>
```

---

## Konfigurasi Pin

| Komponen            | Pin Arduino |
| ------------------- | ----------- |
| OLED SDA            | A4          |
| OLED SCL            | A5          |
| RTC SDA             | A4          |
| RTC SCL             | A5          |
| Buzzer              | D8          |
| Tombol Mode         | D2          |
| Rotary Encoder(CLK) | D3          |
| Rotary Encoder(DT)  | D4          |
| Rotary Encoder(SW)  | D5          |
| ADC Baterai         | A0          |
| Push Button         | D2          |

> [!NOTE]
> OLED dan RTC berbagi jalur I2C yang sama (SDA & SCL), dibedakan melalui alamat I2C masing-masing.

---

## Alur Logika Sistem

### 1. Inisialisasi (Setup)

Saat sistem dinyalakan:

- Serial komunikasi dimulai pada baud rate 9600
- RTC DS3231 diinisialisasi melalui I2C
- Jika RTC kehilangan daya, waktu disetel ulang sesuai waktu kompilasi
- Mengatur pin buzzer sebagai output
- Mengatur pin button sebagai input pullup
- Mengatur interrupt
- Membaca data alarm pada memori EEPROM arduino
- OLED SSD1306 diinisialisasi pada alamat `0x3C`
- Layar diredupkan dan warna teks diset ke putih

---

### 2. Pembacaan Waktu (RTC)

```cpp
RTC_DS3231 rtc;

if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}
```

**Penjelasan:**

- RTC DS3231 membaca waktu secara real-time
- Jika modul kehilangan daya (baterai habis), waktu disetel ulang otomatis sesuai waktu saat program dikompilasi
- Waktu yang terbaca disimpan ke objek `DateTime`

---

### 3. Tampilan OLED

```cpp
void displayScreen(const DateTime tm, int md) {
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.print(tm.toString("hh:mm"));
    display.display();
}
```

**Penjelasan:**

- Kursor diposisikan di pojok kiri atas layar
- Ukuran teks diset ke 2 (besar)
- Waktu ditampilkan dalam format `hh:mm`
- `display.display()` mengirim buffer ke layar fisik

---

### 4. Mode Tampilan

```cpp
uint8_t mode = 0;
```

Variabel `mode` digunakan untuk berpindah antar mode tampilan:

| Mode | Tampilan        |
| ---- | --------------- |
| 0    | Jam & Menit     |
| 1    | Detik / Tanggal |

Perpindahan mode dilakukan melalui push button yang terhubung ke pin digital Arduino.

---

### 5. Loop Utama

```cpp
void loop() {
    DateTime now = rtc.now();
    displayScreen(now, 0);
}
```

Setiap siklus loop:

1. Membaca waktu terkini dari RTC
2. Menampilkan waktu ke layar OLED
3. Mengecek input tombol untuk pergantian mode

---

## Cara Penggunaan

1. Upload kode ke Arduino UNO menggunakan Arduino IDE
2. Pastikan semua library sudah terinstall:
   - `RTClib` by Adafruit
   - `Adafruit GFX Library`
   - `Adafruit SSD1306`
   - `TimeLib` by Michael Margolis
   - `Wire`
   - `EEPROM`
3. Hubungkan rangkaian sesuai konfigurasi pin di atas
4. Nyalakan arduino(sambungkan dari baterai jika perlu), Oled akan menyala dan menampilkan jam, tanggal, bulan, tahun, persentase baterai
5. Tombol digunakan untuk mengubah mode(Display Jam, Set Alarm) sekaligus mematikan alarm
6. Rotary encoder digunakan untuk mengatur waktu alarm saat Set Alarm
7. Jika waktu sekarang telah melewati waktu yang sudah di set alarm, Buzzer akan berbunyi sampai di tekan untuk mematikan

---

## Dokumentasi

[Youtube](test)
