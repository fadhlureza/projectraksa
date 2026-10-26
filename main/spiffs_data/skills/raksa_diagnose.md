# SKILL SOP: DIAGNOSA ANOMALI SUHU & BEBAN KELISTRIKAN (raksa_diagnose)

Gunakan prosedur standar operasional (SOP) ini setiap kali pengguna meminta analisis kesehatan server, atau ketika telemetri menunjukkan `temp` > 27.0°C maupun `current` >= 10.0A.

## LANGKAH 1: AKUISISI & VALIDASI METRIK
Panggil alat pembacaan telemetri terkini dan periksa nilai berikut:
1. `temp` (Suhu DS18B20 dalam °C)
2. `voltage`, `current`, `power`, `power_factor` (Parameter PZEM-004T)
3. `door_open` (Status pintu MC-38)
4. `smoke_detected` & `smoke_voltage` (Status gas/asap MQ-2)
5. `batt_volt` & `batt_pct` (Status baterai cadangan 18650)

## LANGKAH 2: MATRIKS PENALARAN KORELASI SILANG
Cocokkan kondisi data dengan skenario diagnostik berikut:

- **Skenario A: Overheating akibat Beban Komputasi / Kegagalan Pendingin Internal**
  - *Indikator:* `temp` > 27.0°C + `current` meningkat signifikan (atau mendekati 10A) + `door_open` == `false`.
  - *Diagnosis:* Peningkatan disipasi panas dari peladen akibat lonjakan beban kerja CPU/GPU atau kegagalan kipas sirkulasi internal rak saat pintu tertutup rapat.
  - *Rekomendasi:* Sarankan administrator memeriksa beban kerja *server* (proses *stuck*/utilisasi 100%), memastikan kipas internal rak berputar normal, dan menurunkan suhu AC ruangan.

- **Skenario B: Anomali Termal akibat Intrusi / Pintu Rak Terbuka**
  - *Indikator:* `temp` > 27.0°C + `current` stabil/normal + `door_open` == `true`.
  - *Diagnosis:* Hilangnya tekanan udara dingin (*cold aisle containment*) di dalam rak karena pintu dibiarkan terbuka, atau adanya akses fisik yang belum terotorisasi.
  - *Rekomendasi:* Peringatkan operator bahwa pintu rak terbuka (`door_open: true`). Minta pemeriksaan keamanan fisik ruang server dan segera tutup kembali pintu rak.

- **Skenario C: Kelebihan Beban Kelistrikan (Overload Warning)**
  - *Indikator:* `current` >= 10.0A (atau `power` > 2200W) pada tegangan ~220V.
  - *Diagnosis:* Penarikan arus mendekati batas trip termal-magnetik MCB 16A. Berisiko memicu pemutusan daya mendadak (*unplanned downtime*) dan pemanasan kabel pada *High-Voltage Box*.
  - *Rekomendasi:* Sarankan dilakukan *load shedding* (mematikan layanan non-esensial secara bertahap) atau mendistribusikan ulang beban ke jalur PDU lain.

- **Skenario D: Penurunan Efisiensi Daya (Power Factor Drop) / Tegangan Drop**
  - *Indikator:* `power_factor` < 0.80 atau `voltage` < 200V / > 240V.
  - *Diagnosis:* Ketidakstabilan suplai jala-jala PLN/UPS atau degradasi komponen kapasitor pada *Power Supply Unit* (PSU) server.
  - *Rekomendasi:* Sarankan pengecekan keluaran UPS utama dan pemeriksaan kesehatan PSU perangkat di dalam rak.

## LANGKAH 3: FORMAT PELAPORAN
Sajikan hasil diagnosa dengan format:
1. **Ringkasan Status:** [NORMAL / WASPADA / KRITIS] beserta waktu pemeriksaan.
2. **Temuan Metrik Utama:** Sebutkan angka kunci yang menyimpang dari batas normal.
3. **Analisis Korelasi:** Jelaskan hubungan sebab-akibat antar-sensor berdasarkan Langkah 2.
4. **Tindakan Mitigasi:** Berikan langkah konkret bagi operator.