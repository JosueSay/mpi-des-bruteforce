# Análisis e Interpretación del Rendimiento Paralelo

## 1. Resumen general (`perf_summary`)

El resumen contiene las métricas calculadas para cada implementación (`implementation`), conjunto de datos o experimento (`key`) y número de procesos ($p$):

| Columna              | Descripción                                                                                       |
| -------------------- | ------------------------------------------------------------------------------------------------- |
| **$T_s$**            | Tiempo secuencial promedio (1 proceso).                                                           |
| **$T_p$**            | Tiempo paralelo promedio con $p$ procesos.                                                        |
| **Speedup ($S$)**    | Cuánto se acelera la ejecución respecto al tiempo secuencial. $S = \frac{T_s}{T_p}$               |
| **Efficiency ($E$)** | Qué tan bien se aprovechan los recursos paralelos. $E = \frac{S}{p}$                              |
| **FracSec ($f$)**    | Fracción secuencial estimada según la Ley de Amdahl. Mide la porción no paralelizable del código. |

Estas métricas permiten evaluar el comportamiento de escalabilidad y el aprovechamiento del paralelismo.

## 2. Gráfica: Tiempo de ejecución vs número de procesos ($T_p$ vs $p$)

**Qué muestra:**

* Eje X: número de procesos ($p$).
* Eje Y: tiempo paralelo promedio ($T_p$).
* Una línea por implementación y por `key`.

**Interpretación:**

* **Tendencia descendente esperada:** al aumentar $p$, el tiempo $T_p$ debería disminuir.
* **Curvas planas o crecientes:** indican saturación o sobrecosto de comunicación.
* **Diferencias entre implementaciones:** muestran cuál versión escala mejor o tiene menor overhead.

**Conclusión general:**
Si las curvas bajan de forma estable y sin fluctuaciones, la paralelización está siendo efectiva.

## 3. Gráfica: Speedup vs número de procesos

**Qué muestra:**

* Eje X: número de procesos ($p$).
* Eje Y: *speedup* ($S = \frac{T_s}{T_p}$).
* Línea punteada: ideal $S = p$ (escalado lineal perfecto).

**Interpretación:**

* Cuanto más cerca esté la curva de la línea ideal, mejor escalabilidad.
* Curvas que se aplanan indican el límite impuesto por la fracción secuencial o el overhead de comunicación.
* Cruces o caídas reflejan comportamiento irregular o desequilibrio de carga.

**Conclusión general:**
Un *speedup* que crece pero se aleja de la línea ideal indica buena paralelización con límites naturales.
Si se aproxima mucho a la ideal, el programa paraleliza de forma excelente.

## 4. Gráfica: Eficiencia vs número de procesos

**Qué muestra:**

* Eje X: número de procesos ($p$).
* Eje Y: eficiencia ($E = \frac{S}{p}$).
* Líneas de referencia:

  * $E = 1$ → eficiencia perfecta.
  * $E = 0.5$ → punto de eficiencia moderada.

**Interpretación:**

* Eficiencia constante alta ($> 0.8$): escalado casi ideal.
* Eficiencia decreciente: el aumento de procesos no compensa el costo adicional.
* Eficiencia muy baja ($< 0.5$): paralelismo excesivo o mala distribución del trabajo.

**Conclusión general:**
La eficiencia mide la calidad del paralelismo: valores altos implican un uso óptimo de recursos; valores bajos muestran pérdidas por overhead.

## 5. Gráfica: Comparación secuencial vs paralelo ($T_s$ vs $T_p$ promedio)

**Qué muestra:**

* Barras agrupadas por implementación.

  * Azul: tiempo secuencial promedio ($T_s$).
  * Naranja: tiempo paralelo promedio ($T_p$).

**Interpretación:**

* Barras naranjas mucho más bajas que las azules indican paralelización efectiva.
* Barras cercanas sugieren poca o nula ganancia con el paralelismo.
* Diferencias entre implementaciones comparan la eficiencia global de diseño.

**Conclusión general:**
Permite observar de forma directa qué implementación logra mayor reducción del tiempo promedio.

## 6. Gráfica: Distribución de tiempo paralelo por *key* (Boxplot)

**Qué muestra:**

* Eje X: distintos valores de `key` (datasets o casos).
* Eje Y: distribución de tiempos paralelos ($T_p$).
* Colores: implementaciones.

**Interpretación:**

* Cajas compactas y bajas: tiempos estables y rápidos.
* Cajas altas o con valores atípicos: variabilidad o desequilibrio entre ejecuciones.
* Comparación entre colores: muestra cuál implementación tiene menor dispersión o mejor consistencia.

**Conclusión general:**
Revela la estabilidad y robustez del rendimiento frente a distintos casos de prueba.

## 7. Interpretación global de las métricas

| Situación observada                | Interpretación                                     |
| ---------------------------------- | -------------------------------------------------- |
| $T_p$ disminuye al aumentar $p$    | Escalabilidad positiva.                            |
| $S \approx p$                      | Escalado casi lineal (ideal).                      |
| $E \approx 1$                      | Excelente uso de recursos.                         |
| $E$ decrece con $p$                | Overhead de comunicación o sincronización.         |
| $f$ aumenta                        | Alta porción secuencial (límite de Amdahl).        |
| Diferencias entre implementaciones | Distinto grado de optimización o balance de carga. |
