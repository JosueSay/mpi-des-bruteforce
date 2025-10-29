/*
  impl2_seq.c — Implementación 2 (SECUENCIAL) = altA
  - Lee texto plano por stdin.
  - Cifra con key_true para generar el 'cipher' y luego busca la clave por fuerza bruta 0..key_true.
  - Mide tiempo y escribe SIEMPRE en: data/impl2/sec.csv
  - Encabezado fijo:
    implementation,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname,phrase,text
  - 'implementation' = altA, p=1, repetition=1
  - DES real si se compila con -DUSE_OPENSSL -lcrypto; si no, XOR fallback.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../include/common.h"
#include "../include/impl2.h"

#ifdef USE_OPENSSL
  #include <openssl/des.h>
#endif

/* ===== Encabezado estándar CSV ===== */
static const char *CSV_HEADER =
"implementation,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname,phrase,text";

/* ===== Utilidades ===== */
static void ensure_dir(const char* path){
  struct stat st;
  if(stat(path,&st)!=0){ mkdir(path,0755); }
}
static unsigned char* readAllStdin(size_t *out_len){
  size_t cap=4096,len=0; unsigned char *buf=(unsigned char*)malloc(cap);
  if(!buf) return NULL;
  for(;;){
    size_t n=fread(buf+len,1,cap-len,stdin); len+=n;
    if(n==0) break;
    if(len==cap){ cap*=2; unsigned char *t=(unsigned char*)realloc(buf,cap);
      if(!t){ free(buf); return NULL; } buf=t; }
  }
  *out_len=len; return buf;
}
static void* xmalloc(size_t n){ void *p = malloc(n); if(!p){ perror("oom"); exit(1);} return p; }
static unsigned char* padBlock8(const unsigned char *in, size_t len, size_t *out_len){
  size_t pad = 8 - (len % 8); if(pad==0) pad=8;
  *out_len = len + pad;
  unsigned char *out = (unsigned char*)xmalloc(*out_len);
  memcpy(out, in, len);
  memset(out+len, (int)pad, pad);
  return out;
}
static unsigned char* unpadBlock8(const unsigned char *in, size_t len, size_t *out_len){
  if(len==0){ *out_len=0; return NULL; }
  unsigned char pad = in[len-1];
  size_t cut = (pad>=1 && pad<=8 && pad<=len) ? (len-pad) : len;
  unsigned char *out = (unsigned char*)xmalloc(cut?cut:1);
  if(cut){ memcpy(out, in, cut); }
  *out_len = cut;
  return out;
}

/* ===== Cifrado/descifrado ===== */
#ifdef USE_OPENSSL
static void makeKeyFrom56(uint64_t key56, DES_cblock *k){
  memset(k, 0, sizeof(DES_cblock));
  for(int i=0;i<8;i=i+1){
    unsigned char seven = (unsigned char)((key56 >> (56 - 7*(i+1))) & 0x7Fu);
    (*k)[i] = (unsigned char)(seven << 1); /* paridad en LSB */
  }
  DES_set_odd_parity(k);
}
#endif

int encryptDesEcb(uint64_t key56,
                  const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len){
#ifdef USE_OPENSSL
  size_t p_len; unsigned char *p = padBlock8(in, len, &p_len);
  *out = (unsigned char*)xmalloc(p_len); *out_len = p_len;
  DES_cblock k; makeKeyFrom56(key56, &k);
  DES_key_schedule ks; DES_set_key_unchecked(&k, &ks);
  for(size_t i=0;i<p_len;i+=8){
    DES_cblock ib, ob; memcpy(ib, p+i, 8);
    DES_ecb_encrypt(&ib, &ob, &ks, DES_ENCRYPT);
    memcpy(*out+i, ob, 8);
  }
  free(p); return 0;
#else
  size_t p_len; unsigned char *p = padBlock8(in, len, &p_len);
  *out = (unsigned char*)xmalloc(p_len); *out_len = p_len;
  unsigned char b = (unsigned char)(key56 & 0xFFu);
  for(size_t i=0;i<p_len;i=i+1){ (*out)[i] = (unsigned char)(p[i]^b); }
  free(p); return 0;
#endif
}

int decryptDesEcb(uint64_t key56,
                  const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len){
#ifdef USE_OPENSSL
  if(len%8!=0) return -1;
  unsigned char *tmp = (unsigned char*)xmalloc(len);
  DES_cblock k; makeKeyFrom56(key56,&k);
  DES_key_schedule ks; DES_set_key_unchecked(&k,&ks);
  for(size_t i=0;i<len;i+=8){
    DES_cblock ib, ob; memcpy(ib, in+i, 8);
    DES_ecb_encrypt(&ib, &ob, &ks, DES_DECRYPT);
    memcpy(tmp+i, ob, 8);
  }
  *out = unpadBlock8(tmp, len, out_len); free(tmp); return 0;
#else
  unsigned char *tmp = (unsigned char*)xmalloc(len);
  unsigned char b = (unsigned char)(key56 & 0xFFu);
  for(size_t i=0;i<len;i=i+1){ tmp[i] = (unsigned char)(in[i]^b); }
  *out = tmp; *out_len = len; return 0;
#endif
}

int containsPhrase(const unsigned char *buf, size_t len, const char *phrase){
  (void)len; if(!phrase || !*phrase) return 0;
  return strstr((const char*)buf, phrase) != NULL;
}

/* ===== CLI ===== */
static void usage(const char* p){
  fprintf(stderr,"uso: echo \"<texto>\" | %s <phrase> <key_true> <hostname>\n", p);
  fprintf(stderr,"(Salida CSV: data/impl2/sec.csv)\n");
}

int main(int argc, char **argv){
  if(argc<4){ usage(argv[0]); return 2; }
  const char *phrase   = argv[1];
  uint64_t    key_true = strtoull(argv[2], NULL, 10);
  const char *hostname = argv[3];

  ensure_dir("data"); ensure_dir("data/impl2");

  size_t plain_len=0; unsigned char *plain = readAllStdin(&plain_len);
  if(!plain || plain_len==0){ fprintf(stderr,"stdin vacio\n"); return 3; }

  unsigned char *cipher=NULL; size_t cipher_len=0;
  if(encryptDesEcb(key_true, plain, plain_len, &cipher, &cipher_len)!=0){
    fprintf(stderr,"encrypt error\n"); free(plain); return 4;
  }

  struct timespec t0 = nowMono();
  uint64_t iters=0, found_key=(uint64_t)(~0ULL); int found=0;

  for(uint64_t k=0;k<=key_true;k=k+1){
    unsigned char *dec=NULL; size_t dec_len=0;
    if(decryptDesEcb(k, cipher, cipher_len, &dec, &dec_len)==0){
      iters = iters + 1;
      if(containsPhrase(dec, dec_len, phrase)){ found=1; found_key=k; free(dec); break; }
      free(dec);
    }
  }
  time_span_t dt = diffMono(t0, nowMono());

  if(found==1){
    printf("%llu encontrado; tiempo=%.6f s; iters=%llu\n",
      (unsigned long long)found_key, dt.secs, (unsigned long long)iters);
  }else{
    printf("no encontrado; tiempo=%.6f s; iters=%llu\n",
      dt.secs, (unsigned long long)iters);
  }

  FILE *fp = fopen("data/impl2/sec.csv","a+");
  if(fp){
    ensureHeader(fp, CSV_HEADER);
    char ts[64]; isoUtcNow(ts,sizeof ts);
    char *pc = (char*)xmalloc(plain_len+1); memcpy(pc,plain,plain_len); pc[plain_len]='\0';
    fprintf(fp,
      "altA,%llu,%d,%d,%.9f,%llu,%d,%d,%s,%s,\"%s\",\"%s\"\n",
      (unsigned long long)key_true, /* key */
      1,                            /* p */
      1,                            /* repetition */
      dt.secs,                      /* time_seconds */
      (unsigned long long)iters,    /* iterations_done */
      found,                        /* found */
      0,                            /* finder_rank */
      ts, hostname, phrase, pc
    );
    fclose(fp); free(pc);
  }

  free(plain); free(cipher);
  return 0;
}
