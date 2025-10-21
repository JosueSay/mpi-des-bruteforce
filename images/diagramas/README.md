# Diagramas

Aquí van los diagramas de flujo y esquemas del proyecto. Usaremos **Mermaid** para definir los diagramas en texto dentro de un `.md` y además se adjuntará la imagen renderizada (PNG/SVG) en la misma carpeta.

## Convenciones

- **Formato de entrega por diagrama** (todos juntos en la carpeta `images/diagramas/`):
  - `implX_diagrama_descripcion.png` (imagen renderizada).
  - `diagramas.md` un archivo que describa los diagramas hechos.

- **Contenido del `.md`** (obligatorio):
  1. Título y una línea de propósito.
  2. Relación con el código (qué archivo/función representa).
  3. Bloque de código Mermaid (ej.: ```mermaid ...```).
  4. Mini explicación/leyenda de nodos relevantes.

- **Nombres**:
  - Seguir el patrón: `impl{n}_{tipo}_{breve}.md/png` por ejemplo:  
    `impl1_naive_flow.md` y `impl1_naive_flow.png`.
