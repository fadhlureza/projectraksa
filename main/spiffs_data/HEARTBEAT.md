# RAKSA Periodic Heartbeat Task

Setiap kali heartbeat terpicu secara berkala:
1. Panggil tool `raksa_status` untuk mengecek telemetri sistem kelistrikan terkini.
2. Periksa apakah terdapat anomali operasional:
   - Tegangan drop (< 190V) atau lonjakan tegangan (> 245V).
   - Arus beban berlebih mendekati kapasitas MCB.
   - Suhu boks/panel meningkat tidak wajar (> 50°C).
   - Sensor MQ-2 mendeteksi indikasi gas/asap.
3. Jika kondisi normal, pertahankan status aman tanpa tindakan tambahan.
4. Jika terdeteksi anomali signifikan, catat ringkasan insiden tersebut ke file `/spiffs/memory/MEMORY.md` menggunakan tool `edit_file` atau `write_file` agar terekam dalam riwayat memori sistem.
