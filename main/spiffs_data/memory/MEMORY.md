# MEMORI OPERASIONAL JANGKA PANJANG RAKSA

## PROFIL & KONFIGURASI SISTEM
- **ID Perangkat:** RAKSA-NODE-01 (Smart PDU & Server Rack Sentinel)
- **Arsitektur Fisik:** Split-Box (Isolasi High-Voltage AC 220V & Low-Voltage DC 3.3V/5V)
- **Kapasitas Proteksi:** Relai Tongling T90 (30A) dengan proteksi MCB 16A (Batas Peringatan Arus: 10.0A)
- **Batas Termal:** 18.0°C - 27.0°C (ASHRAE TC 9.9)

## BASELINE TELEMETRI NORMAL
- **Tegangan AC Normal:** ~220.0 V (50.0 Hz)
- **Tegangan Udara Bersih MQ-2 (`smoke_voltage`):** ~0.30 V - 0.60 V (Ambang bahaya: > 1.60 V)
- **Tegangan Baterai Cadangan Penuh:** 4.10 V - 4.20 V (95% - 100%)

## RIWAYAT INSIDEN & ANOMALI PENTING
- [INIT] Sistem diinisialisasi. Belum ada catatan insiden kritis pada memori flash.

## CATATAN PEMELIHARAAN & PREFERENSI OPERATOR
- Operator mengutamakan laporan ringkas dengan penekanan pada korelasi antara beban daya (`power` / `current`) dan suhu (`temp`).
- Selalu ingatkan status pintu (`door_open`) apabila terdeteksi terbuka lebih dari satu siklus pemeriksaan.