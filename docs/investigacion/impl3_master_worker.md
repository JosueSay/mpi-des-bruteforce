# Investigación – Implementación 3: Modelo Master–Worker con Balance Dinámico de Carga

La **Implementación 3** del proyecto utiliza un enfoque **Master–Worker** para distribuir dinámicamente el espacio de búsqueda durante un ataque de fuerza bruta basado en MPI. Este enfoque surge como respuesta directa a las limitaciones detectadas en el **acercamiento naive** (impl1), donde la división del rango de llaves en bloques contiguos genera **alta varianza en los tiempos de ejecución** dependiendo de la posición en la que se encuentre la llave correcta.

## Problema con el enfoque naive

- Si la llave se encuentra **muy temprano** en el rango de uno de los procesos → **speedup artificialmente enorme**
- Si la llave aparece **muy tarde** en el primer proceso → speedup ≈ 1 (sin mejora paralela)
- Los procesos pueden **quedar ociosos** mientras otros trabajan
- La métrica de performance queda **sesgada por azar**, no por diseño del algoritmo

Este comportamiento es **inconsistente**, difícil de analizar y poco confiable para medir paralelismo real.

### Solución: Distribución dinámica de trabajo

El enfoque Master–Worker introduce:

- **Asignación de chunks de llaves bajo demanda**
- **Trabajo balanceado entre todos los procesos activos**
- **Detención temprana global** cuando algún worker encuentra la llave
- **Speedups más consistentes y reproducibles**

#### Flujo resumido

| Rol                     | Tareas principales                                                                   |
| ----------------------- | ------------------------------------------------------------------------------------ |
| **Master (rank 0)**     | Administra el puntero global de trabajo (`next_chunk`) y envía trabajo a los workers |
| **Workers (ranks > 0)** | Solicitan chunks, prueban llaves y notifican éxito                                   |

Todos los procesos permanecen **ocupados** hasta que **alguien encuentra la llave** ✅

### Conceptos clave del modelo

| Concepto                       | Beneficio                                    |
| ------------------------------ | -------------------------------------------- |
| **Chunking**                   | Minimiza overhead de comunicación            |
| **Work-Stealing implícito**    | Workers que acaban antes reciben más trabajo |
| **Detección temprana**         | Evita buscar llaves después de éxito         |
| **Distribución más homogénea** | Speedup más estable y cercano al ideal       |

La eficiencia depende del tamaño del **chunk**:

- Muy pequeño → exceso de mensajes MPI
- Muy grande → pérdida temporal de balance
- Se ajusta empíricamente según pruebas

### Impacto en speedup y performance

- La **variancia del tiempo** se reduce de manera significativa
- Los resultados se vuelven **más representativos del paralelismo** real
- El algoritmo ya no depende del **azar** (posición de la llave)

La Implementación 3 tiende a obtener:

S ≈ T_seq / T_par → N − ε

donde `N` es el número de procesos y `ε` es el costo de comunicación.

### Ventajas y desventajas

| Ventajas                   |  Desventajas                        |
| -------------------------- | ----------------------------------- |
| Excelente balance de carga | Overhead de mensajes MPI            |
| Speedup más estable        | Maestro puede ser cuello de botella |
| Paro temprano global       | Tamaño del chunk debe calibrarse    |
| Apto para clusters reales  | Código más complejo                 |

### Justificación teórica

Este modelo es ampliamente utilizado en:

- Aplicaciones HPC de **exploración de espacio de búsqueda**
- **Brute-force cryptanalysis** como este proyecto
- Render distribuido, simulaciones Monte-Carlo, etc.

La técnica es reconocida como una mejora **práctica y teórica** ante el reparto contiguo de trabajo.

### Conclusión de investigación

La implementación Master–Worker **maximiza el grado de paralelismo efectivo**, mitigando la aleatoriedad del naive y permitiendo una **evaluación consistente y objetiva del speedup**.
Este acercamiento representa un diseño **más escalable, más justo entre procesos**, y orientado al rendimiento en entornos distribuidos como MPI.

## Referencias

1. Gropp, D., Thakur, R., & Gropp, W. (2014). *Using MPI: Portable Programming with the Message Passing Interface*. MIT Press. ISBN: 978-0262527392.  

2. MPICH Team. (s. f.). *Master/Worker Pattern in MPI*. MPICH Documentation. Recuperado de [https://www.mpich.org/documentation/](https://www.mpich.org/documentation/)  

3. Rabenseifner, R. (2020). *Hybrid MPI + OpenMP Programming* (Tutorial). HLRS, Stuttgart. Recuperado de [https://fs.hlrs.de/projects/par/mpi/](https://fs.hlrs.de/projects/par/mpi/)
