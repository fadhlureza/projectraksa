# SKILL SOP: PROTOKOL DARURAT PEMUTUSAN BEBAN (emergency_trip)

Gunakan protokol darurat ini SEGERA apabila terdeteksi salah satu kondisi bahaya kritis berikut:
1. **Indikasi Kebakaran/Asap:** `smoke_detected` == `true` (`smoke_voltage` > 1.60V).
2. **Suhu Ekstrem Berbahaya:** `temp` >= 35.0°C disertai kenaikan tren yang cepat.
3. **Arus Kritis Membahayakan:** `current` >= 14.5A (mendekati batas trip MCB 16A).
4. **Permintaan Darurat Pengguna:** Operator meminta prosedur pemutusan arus darurat.

## ATURAN KESELAMATAN SIBER-FISIK (WAJIB DIPATUHI)
- Ingat bahwa Anda berada pada mode **Observabilitas (Read-Only)**.
- **DILARANG** menyatakan bahwa Anda telah memutus relai secara mandiri melalui *chat*.
- Tugas Anda adalah memverifikasi status `relay_state` saat ini (apakah sistem proteksi otomatis *firmware* sudah memutusnya atau belum) dan memandu operator melakukan pemutusan daya secara aman dan instan.

## PROSEDUR RESPONS DARURAT

### 1. Peringatan Prioritas Utama (Header)
Awali respons dengan label tegas:
`[PERINGATAN DARURAT RAKSA - TINDAKAN SEGERA DIPERLUKAN]`

### 2. Verifikasi Status Relai & Proteksi Otomatis
Periksa nilai `relay_state` dan `voltage` pada data JSON:
- **Jika `relay_state` == `false`:** Informasikan bahwa mekanisme *Safety Trip* otomatis pada *firmware* ESP32-S3 (atau pemutusan sebelumnya) telah berhasil memutus aliran daya pada Relai Tongling T90.
- **Jika `relay_state` == `true`:** Nyatakan dengan tegas bahwa aliran listrik AC 220V **MASIH MENYALA** dan harus segera diputus oleh operator.

### 3. Instruksi Pemutusan Daya bagi Operator (Urutan Prioritas)
Berikan instruksi langkah demi langkah berikut kepada operator:
1. **Pemutusan via Dasbor Web (Jarak Jauh):** Segera tekan tombol **Toggle Switch Relay (OFF)** pada panel kendali utama Dasbor Web RAKSA untuk memutus kontak Relai T90 (30A).
2. **Pemutusan Mekanis (Lokal):** Jika berada di dekat ruang server atau jika dasbor tidak merespons, segera turunkan tuas **MCB 1-Fase 16A** pada kompartemen *High-Voltage Box* RAKSA.
3. **Isolasi Ruang Server:** Apabila `smoke_detected` bernilai `true`, jangan membuka pintu rak (`door_open`) secara gegabah tanpa alat pemadam api ringan (APAR) jenis bersih (*clean agent* / CO2) untuk menghindari suplai oksigen berlebih pada titik api (*backdraft*).
4. **Keberlangsungan Pemantauan:** Informasikan bahwa meskipun listrik AC utama diputus, unit komputasi RAKSA (*Low-Voltage Box*) akan tetap hidup menggunakan baterai cadangan Li-Ion 18650 (`batt_pct`) untuk terus melaporkan kondisi suhu dan asap secara *real-time*.

### 4. Pencatatan Insiden
Setelah memberikan peringatan darurat, catat waktu kejadian dan nilai puncak sensor (`temp`, `current`, `smoke_voltage`) ke dalam `MEMORY.md` sebagai rekam jejak audit insiden.