# Resultados

## Discusión

El programa base recorre un espacio de claves y detiene la búsqueda cuando encuentra coincidencia. Se probó en dos hosts con tres implementaciones y tres llaves que representan casos fácil, medio y difícil. En todos los escenarios el paralelo reduce el tiempo respecto al secuencial. El comportamiento del speedup depende del equilibrio entre cómputo y comunicación y de cuánto trabajo real recibe cada proceso. En `pc_josue` el speedup crece de forma regular y la eficiencia se mantiene por debajo de 1. En `laptop_josue` `impl1` muestra superlinealidad en la llave fácil porque cada proceso opera con menos datos que caben mejor en caché. `impl2` es la más consistente entre llaves y niveles de procesos. `impl3` mantiene un paralelismo funcional pero con eficiencia cercana a 0.5.

## Conclusiones

El costo de sincronización y envío de señales de parada influye en la eficiencia cuando aumentan los procesos. La distribución del trabajo y el momento en que aparece la llave afectan directamente los tiempos observados. El hardware condiciona el techo de rendimiento y puede habilitar superlinealidad en equipos con cachés más sensibles al tamaño del lote por proceso. Se aprendió que medir con varias llaves reduce sesgos, que usar dos enfoques de análisis ayuda a separar escalabilidad interna y comparación entre estrategias, y que repetir pruebas en más de un host evita conclusiones atadas a una sola máquina.

## Recomendaciones

- Usar `MPI_Bcast` para difundir la señal de encontrado o un `MPI_Reduce` sobre una bandera global en la primera implementación. Evitar un envío punto a punto a todos los procesos.
- Emplear `MPI_Isend` o `MPI_Issend` con `MPI_Waitall` para no frenar al emisor. Insertar puntos de progreso con `MPI_Iprobe` en bucles largos.
- Particionar en bloques pequeños por proceso y reasignar cuando termine un bloque. Preferir esquema maestro trabajador para repartir trabajo restante.
- Probar p iguales al número de núcleos físicos antes de usar hiperprocesos. Verificar que el aumento de p no degrade la eficiencia por sobrecoste.
- Minimizar escritura durante el cómputo. Acumular métricas en memoria y escribir al final o en intervalos amplios.
