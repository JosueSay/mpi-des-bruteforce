# Catálogo de Funciones y Librerías — **Impl3 (Master–Worker)**

## tryKey

Función que intenta descifrar un texto con una llave específica y valida si contiene la frase objetivo.

### Entradas

- **key**
  - Tipo: `uint64_t`
  - Clave a probar en el descifrado
- **cipher**
  - Tipo: `unsigned char*`
  - Texto cifrado
- **cipher_len**

  - Tipo: `int`
  - Longitud del texto cifrado
- **phrase**
  - Tipo: `char*`
  - Frase clave a buscar en el texto descifrado

### Salidas

- **resultado**
  - Tipo: `int`
  - 1 si la frase fue encontrada (descifrado correcto), 0 en caso contrario

## request_chunk (Worker)

Envía un mensaje al maestro indicando que necesita más trabajo (más llaves para probar).

### Entradas

- **rank**
  - Tipo: `int`
  - Identificador del proceso worker que pide trabajo

### Salidas

- No retorna valores
- Efecto: Envío de mensaje MPI con etiqueta `TAG_REQ`

## send_chunk (Master)

Asigna un rango de llaves (chunk) a un worker que lo solicita.

### Entradas

- **dest**
  - Tipo: `int`
  - Rank del worker que recibirá el chunk
- **start**
  - Tipo: `uint64_t`
  - Inicio del rango de llaves
- **end**
  - Tipo: `uint64_t`
  - Final del rango de llaves

### Salidas

- No retorna valores
- Efecto: Envío de mensaje MPI con etiqueta `TAG_CHUNK`

## process_chunk (Worker)

Prueba cada llave del chunk asignado hasta encontrar la correcta o terminar el rango.

### Entradas

- **start**, **end**
  - Tipo: `uint64_t`
  - Rango de llaves a probar
- **cipher**, **cipher_len**, **phrase**
  - Igual que en tryKey

### Salidas

- **found_key**
  - Tipo: `uint64_t`
  - Llave encontrada, o `UINT64_MAX` si no hubo éxito

## notify_found (Worker → Master)

Avisa al maestro que una llave válida ha sido encontrada.

### Entradas

- **key**
  - Tipo: `uint64_t`
  - Llave encontrada

### Salidas

- No retorna valores
- Efecto: Envío de mensaje MPI con etiqueta `TAG_FOUND`

## broadcast_stop (Master)

Envía la señal de finalización a todos los procesos cuando se encuentra la llave.

### Entradas

- **found_flag**
  - Tipo: `int`
  - Siempre 1 en este contexto

### Salidas

- No retorna valores
- Efecto: Enviar **STOP** a todos los ranks con `MPI_Bcast`

## generate_chunk_range (Master)

Calcula los rangos `(start, end)` de trabajo para cada asignación dinámica.

### Entradas

- **next_chunk**
  - Tipo: `uint64_t`
  - Primer número no probado
- **chunk_size**
  - Tipo: `uint64_t`
  - Tamaño del bloque asignado

### Salidas

- **start**
  - Tipo: `uint64_t`
- **end**

  - Tipo: `uint64_t`

> `next_chunk = end + 1` después de la asignación.

## Librerías principales utilizadas

| Librería                            | Uso principal                                                                        |
| ----------------------------------- | ------------------------------------------------------------------------------------ |
| `<mpi.h>`                           | Envío/recepción de mensajes: `MPI_Send`, `MPI_Recv`, `MPI_Bcast`, `MPI_Iprobe`, etc. |
| `<openssl/des.h>` *(o equivalente)* | Cifrado/descifrado DES en tryKey                                                     |
| `<stdint.h>`                        | Llaves de 56 bits con `uint64_t`                                                     |
| `<string.h>`                        | Validación de frase con `strstr` + copias de buffer                                  |
| `<stdio.h>` + `<stdlib.h>`          | I/O, memoria, mensajes de debug                                                      |
| `<time.h>`                          | Medición de tiempos si no se usa `MPI_Wtime`                                         |
