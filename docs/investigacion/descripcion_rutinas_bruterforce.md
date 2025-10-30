# Descripción de rutinas

## 1. `decrypt(key, *ciph, len)` / `encrypt(key, *ciph, len)`

- En `bruteforce.c` (versión base):

  - Construye una clave `k` con la paridad adecuada a partir de `key` (corrimientos y máscaras).
  - Llama a `des_setparity((char*)&k)` para ajustar paridad de bytes.
  - Llama a `ecb_crypt((char*)&k, (char*)ciph, 16, DES_DECRYPT)` (o `DES_ENCRYPT`) — operación sobre bloque(s) en sitio.
- En `impl1`: se usa la capa `core_crypto` que:

  - `encryptDesEcb(key56, in, len, &out, &out_len)` — hace pad a múltiplos de 8, genera DES key desde 56 bits, cifra bloque por bloque con OpenSSL y devuelve buffer nuevo.
  - `decryptDesEcb(key56, in, len, &out, &out_len)` — descifra bloque por bloque y quita padding; devuelve copia dinámica del texto plano.
- Comentario práctico: `bruteforce.c` opera **in-place** sobre el buffer (ecb_crypt), mientras que `core_crypto` devuelve buffers nuevos y maneja padding.

## 2. `tryKey(key, *ciph, len)`

- Implementación (bruteforce.c):

  1. Reserva `temp[len+1]`.
  2. `memcpy(temp, ciph, len); temp[len]=0;` — hace copia local y asegura terminador NUL.
  3. `decrypt(key, temp, len);` — modifica `temp` con texto descifrado.
  4. `return strstr(temp, search) != NULL;` — devuelve si la frase aparece.
- Propósito: evitar modificar el `cipher` compartido; permitir usar funciones de búsqueda de C que requieren cadena NUL-terminated.

## 3. `memcpy`

- En este contexto `memcpy(temp, ciph, len)` copia exactamente `len` bytes del buffer cifrado hacia la copia local `temp`.
- Motivación:

  - Seguridad: no alterar el buffer original.
  - Compatibilidad: crear una c-string (`temp[len]=0`) para usar `strstr`.
- Coste: O(len) por clave intentada — en prácticas reales esto penaliza rendimiento; por eso en soluciones optimizadas se evita copiar por intento y se descifra en buffer temporal pre-asignado o se usan descifradores que operan en buffer controlado.

## 4. `strstr`

- Uso: detectar la presencia de la **frase buscada** en el texto descifrado (`temp`).
- Comportamiento:

  - Escanea `temp` buscando la primera ocurrencia de la subcadena `search`.
  - Retorna puntero a la ocurrencia o `NULL`.
- Dependencia: `temp` debe estar NUL-terminated (por eso el `memcpy`+`temp[len]=0`).
- Coste: en promedio O(n·m) en peor caso (n = longitud del texto, m = longitud del patrón), pero en la práctica muy rápido para patrones cortos.
