# Guía de métricas y estrategias

Este documento tiene como objetivo que todos los integrantes del equipo estén preparados para:

- Medir correctamente el rendimiento del proyecto.
- Responder preguntas sobre la paralelización, estrategias usadas y análisis de resultados.
- Relacionar los resultados con la Ley de Amdahl.

## Métricas y variables importantes

### Tiempo Secuencial ($T_s$)

- Definición: Tiempo total que tarda el programa en ejecutarse en un solo procesador.
- Propósito: Base de comparación para calcular speedup.
- Cómo obtenerlo: Medición del programa sin paralelización.

### Tiempo Paralelo ($T_p$)

- Definición: Tiempo total que tarda el programa en ejecutarse usando múltiples procesadores.
- Propósito: Evaluar el rendimiento de la paralelización.
- Cómo obtenerlo: Medición del programa paralelo con diferentes cantidades de procesadores.

### Número de Procesadores ($p$)

- Definición: Cantidad de unidades de procesamiento utilizadas en la ejecución paralela.
- Importancia: Permite calcular speedup y eficiencia.

### Speedup ($S$)

- Definición: Aceleración obtenida al paralelizar.
- Fórmula:

$S = \frac{T_s}{T_p}$

- Interpretación: Cuánto más rápido es el programa paralelo respecto al secuencial.
- Relación con Amdahl: No puede superar el límite teórico impuesto por la fracción secuencial del programa.

### Eficiencia ($E$)

- Definición: Uso relativo de los procesadores.
- Fórmula:

$E = \frac{S}{p}$

- Interpretación: Qué tan bien se aprovechan los procesadores; valores menores a 1 indican ineficiencia.

### Fracción secuencial ($f$)

- Definición: Parte del programa que no puede paralelizarse.
- Uso: Calcular el límite máximo de speedup según la Ley de Amdahl.
- Fórmula del límite:

$S_{max} = \frac{1}{f}$

## Estrategias de paralelización

- Cada integrante debe estar listo para explicar **la estrategia de paralelización utilizada** (por ejemplo: bucles paralelos, uso de barreras, división de tareas, reducción, etc.).
- Por cada estrategia aplicada, se deben medir:

  - Tiempo paralelo ($T_p$)
  - Speedup ($S$)
  - Eficiencia ($E$)
- Comparar resultados de diferentes estrategias puede mostrar cuál es más eficiente o escalable.

## Datos a tener listos para el reporte y presentación

1. $T_s$: Tiempo de ejecución secuencial.
2. $T_p$: Tiempo de ejecución paralelo con distintas cantidades de procesadores.
3. $p$: Número de procesadores utilizados en cada prueba.
4. $S$: Speedup obtenido.
5. $E$: Eficiencia correspondiente.
6. $f$: Fracción secuencial del programa (estimada o calculada según observaciones).

## Preguntas posibles para la defensa

- ¿Cuál fue la estrategia de paralelización implementada?
- ¿Cómo se distribuyó la carga de trabajo entre los procesadores?
- ¿Qué valores de speedup y eficiencia obtuvieron por cada estrategia?
- ¿Cuál es la relación de sus resultados con la Ley de Amdahl?
- ¿Qué factores afectaron la eficiencia y cómo los podrían mejorar?
- ¿Cuál fue la fracción secuencial estimada y cómo limita el speedup máximo?
