# RAKSA Periodic Heartbeat Task

Setiap kali heartbeat terpicu secara berkala:
1. Panggil tool `raksa_status` untuk mengecek telemetri sistem kelistrikan terkini.
2. Periksa apakah terdapat anomali operasional berdasarkan parameter SOUL.md:
   - Tegangan di luar batas normal (< 200V atau > 240V AC).
   - Arus beban mendekati atau melampaui batas waspada (>= 10.0A).
   - Suhu rak/boks meningkat tidak wajar (> 32.0°C atau tren lonjakan cepat).
   - Sensor MQ-2 mendeteksi indikasi gas/asap (`smoke_voltage` > 1.60V).
   - Pintu rak terdeteksi terbuka (`door_open == true`).
   - Kapasitas baterai cadangan menipis (`batt_pct < 20%`).
3. Jika kondisi normal, pertahankan status aman tanpa tindakan tambahan.
4. Jika terdeteksi anomali signifikan, catat ringkasan insiden tersebut ke file `/spiffs/memory/MEMORY.md` menggunakan tool `edit_file` atau `write_file` agar terekam dalam riwayat memori sistem.
