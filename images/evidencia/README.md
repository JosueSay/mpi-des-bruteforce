# Evidencia (capturas y timings)

Carpeta para almacenar evidencia visual y métricas experimentales. Contiene dos subcarpetas:

- `capturas/` → capturas de pantalla o logs (.png, .jpg, .txt)
- `timings/`  → archivos de tiempos/mediciones (.csv, .txt)

Reglas mínimas:

- Nombre de captura: `implX_tipo_npX_keyY_timestamp.png`  
  (ej.: `impl1_run_np4_key123456_20251021-1530.png`)
- Nombre de CSV: `implX_timings_npX_keyY.csv` con columnas: `impl,np,key,tiempo_s,timestamp,notas`
- Cada archivo debe tener un README o comentario corto en su nombre con la fecha y quien lo generó (o incluir esa info en el CSV).

Nota: los archivos de evidencia se generan localmente y **no** deben subirse automáticamente sin revisión; usen PR para integrar evidencia seleccionada al repo.
