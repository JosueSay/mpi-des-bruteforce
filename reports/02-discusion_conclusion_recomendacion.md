# Resultados

## Discusión
<!-- Analizar funcionamiento del programa base, pruebas realizadas, y comportamiento del speedup. -->

## Conclusiones
<!-- Resumir los retos encontrados y lo aprendido durante la implementación. -->

## Recomendaciones

- **Envío a todos con `MPI_Send`** tiene coste O(N). Alternativas:
  - `MPI_Bcast(found, ...)` desde el que detecta (necesita sincronización).
  - `MPI_Issend` / `MPI_Isend` para envío no bloqueante y luego `MPI_Waitall` para evitar bloquear al emisor.
  - Un patrón maestro/worker donde el maestro notifica la parada por `MPI_Bcast` o un `MPI_Reduce` con `MAX` sobre bandera `found`.
- **Progreso de la recepción**: algunas implementaciones MPI requieren llamadas a funciones MPI para hacer progreso en `MPI_Irecv` (si no hay hilo de progreso); el diseño actual funciona porque las llamadas MPI (e.g., `MPI_Send`) y finalización en `MPI_Wait` facilitan progreso, pero en bucles muy largos conviene insertar puntos de progreso (`MPI_Iprobe` o pequeñas llamadas no bloqueantes) para asegurar prontitud en la recepción.
