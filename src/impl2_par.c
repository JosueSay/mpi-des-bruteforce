/*
  impl2_par.c — Implementación 2 (PARALELA MPI) = altA
  - Reparto block-cyclic sin maestro.
  - Stop no bloqueante (broadcast por TAG_FOUND entre pares).
  - Rank 0 agrega TODAS las filas (una por rank) en: data/impl2/par.csv
  - Encabezado fijo:
    implementation,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname,phrase,text
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <mpi.h>

#include "../include/common.h"
#include "../include/impl2.h"

#ifdef USE_OPENSSL
  #include <openssl/des.h>
#endif

#define TAG_FOUND  77
#define TAG_ROW    99

static const char *CSV_HEADER =
"implementation,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname,phrase,text";

/* ==== utilidades ==== */
static void ensure_dir(const char* path){
  struct stat st; if(stat(path,&st)!=0){ mkdir(path,0755); }
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

/* ==== cifrado/descifrado ==== */
#ifdef USE_OPENSSL
static void makeKeyFrom56(uint64_t key56, DES_cblock *k){
  memset(k, 0, sizeof(DES_cblock));
  for(int i=0;i<8;i=i+1){
    unsigned char seven = (unsigned char)((key56 >> (56 - 7*(i+1))) & 0x7Fu);
    (*k)[i] = (unsigned char)(seven << 1);
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

/* ==== fila compacta para enviar a rank 0 (sin phrase/text por MPI) ==== */
typedef struct {
  unsigned long long key;
  int p;
  int repetition;
  double time_seconds;
  unsigned long long iterations_done;
  int found;
  int finder_rank;
  char ts[64];
  char hostname[64];
} csv_row_t;

static void usage(const char *p){
  fprintf(stderr,"uso: echo \"<texto>\" | mpirun -np <P> %s <phrase> <key_true> <chunk> <hostname>\n", p);
  fprintf(stderr,"(Salida CSV: data/impl2/par.csv — rank 0 agrega todas las filas)\n");
}

int main(int argc, char **argv){
  MPI_Init(&argc,&argv);
  int rank,size; MPI_Comm_rank(MPI_COMM_WORLD,&rank); MPI_Comm_size(MPI_COMM_WORLD,&size);

  if(argc<6){ if(rank==0) usage(argv[0]); MPI_Finalize(); return 2; }
  const char *phrase   = argv[1];
  uint64_t    key_true = strtoull(argv[2], NULL, 10);
  uint64_t    chunk    = strtoull(argv[3], NULL, 10); if(chunk==0) chunk=1;
  const char *hostname = argv[4];

  if(rank==0){ ensure_dir("data"); ensure_dir("data/impl2"); }

  /* Rank 0 lee texto y difunde */
  size_t plain_len=0; unsigned char *plain=NULL;
  if(rank==0){
    plain = readAllStdin(&plain_len);
    if(!plain || plain_len==0){ fprintf(stderr,"stdin vacio\n"); MPI_Abort(MPI_COMM_WORLD,3); }
  }
  MPI_Bcast(&plain_len,1,MPI_UNSIGNED_LONG,0,MPI_COMM_WORLD);
  if(rank!=0) plain = (unsigned char*)malloc(plain_len);
  MPI_Bcast(plain,plain_len,MPI_UNSIGNED_CHAR,0,MPI_COMM_WORLD);

  /* Cifrar en rank 0 y difundir cipher */
  unsigned char *cipher=NULL; size_t cipher_len=0;
  if(rank==0){ encryptDesEcb(key_true, plain, plain_len, &cipher, &cipher_len); }
  MPI_Bcast(&cipher_len,1,MPI_UNSIGNED_LONG,0,MPI_COMM_WORLD);
  if(rank!=0) cipher = (unsigned char*)malloc(cipher_len);
  MPI_Bcast(cipher, cipher_len, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

  uint64_t iterations=0, found_key=(uint64_t)(~0ULL); int found=0; int winner_rank=-1;
  struct timespec t0 = nowMono();

  /* Reparto block-cyclic sin maestro */
  for(uint64_t turn=0;;turn=turn+1){
    /* ¿Aviso de hallazgo? */
    int flag=0; MPI_Status st;
    MPI_Iprobe(MPI_ANY_SOURCE, TAG_FOUND, MPI_COMM_WORLD, &flag, &st);
    if(flag){
      uint64_t kwin; MPI_Recv(&kwin,1,MPI_UNSIGNED_LONG_LONG,st.MPI_SOURCE,TAG_FOUND,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      found=1; found_key=kwin; winner_rank=st.MPI_SOURCE; break;
    }

    __uint128_t base128 = (__uint128_t)rank*chunk + (__uint128_t)turn*((__uint128_t)chunk*(__uint128_t)size);
    if(base128 > key_true) break;
    uint64_t base=(uint64_t)base128;
    uint64_t endb = base + (chunk-1); if(endb > key_true) endb = key_true;

    for(uint64_t k=base; k<=endb; k=k+1){
      int f2=0; MPI_Status st2;
      MPI_Iprobe(MPI_ANY_SOURCE, TAG_FOUND, MPI_COMM_WORLD, &f2, &st2);
      if(f2){
        uint64_t kwin; MPI_Recv(&kwin,1,MPI_UNSIGNED_LONG_LONG,st2.MPI_SOURCE,TAG_FOUND,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        found=1; found_key=kwin; winner_rank=st2.MPI_SOURCE; goto FINISH;
      }
      unsigned char *dec=NULL; size_t dec_len=0;
      decryptDesEcb(k, cipher, cipher_len, &dec, &dec_len); iterations = iterations + 1;
      if(containsPhrase(dec, dec_len, phrase)){
        found=1; found_key=k; winner_rank=rank; free(dec);
        for(int r=0;r<size;r=r+1){
          if(r==rank) continue;
          MPI_Request rq; MPI_Isend(&found_key,1,MPI_UNSIGNED_LONG_LONG,r,TAG_FOUND,MPI_COMM_WORLD,&rq);
        }
        goto FINISH;
      }
      free(dec);
    }
  }

FINISH:;
  time_span_t dt = diffMono(t0, nowMono());

  /* Empaquetar fila y enviar a rank 0 */
  csv_row_t row;
  row.key = (found_key==(uint64_t)(~0ULL)?0ULL:(unsigned long long)found_key);
  row.p = size;
  row.repetition = 1;
  row.time_seconds = dt.secs;
  row.iterations_done = (unsigned long long)iterations;
  row.found = found;
  row.finder_rank = (winner_rank<0? -1 : winner_rank);
  isoUtcNow(row.ts,sizeof row.ts);
  snprintf(row.hostname,sizeof row.hostname,"%s",hostname);

  if(rank!=0){
    MPI_Send(&row, sizeof(row), MPI_BYTE, 0, TAG_ROW, MPI_COMM_WORLD);
  }else{
    /* rank 0 agrega su fila + recibe las de los demás y escribe a par.csv */
    FILE *fp = fopen("data/impl2/par.csv","a+");
    ensureHeader(fp, CSV_HEADER);

    char *pc = (char*)xmalloc(plain_len+1); memcpy(pc,plain,plain_len); pc[plain_len]='\0';

    /* Mi fila (rank 0) */
    fprintf(fp,
      "altA,%llu,%d,%d,%.9f,%llu,%d,%d,%s,%s,\"%s\",\"%s\"\n",
      row.key,row.p,row.repetition,row.time_seconds,row.iterations_done,row.found,row.finder_rank,row.ts,row.hostname,phrase,pc);

    /* Recibir y escribir filas de los otros ranks */
    for(int i=1;i<size;i=i+1){
      csv_row_t rcv;
      MPI_Recv(&rcv, sizeof(rcv), MPI_BYTE, MPI_ANY_SOURCE, TAG_ROW, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      fprintf(fp,
        "altA,%llu,%d,%d,%.9f,%llu,%d,%d,%s,%s,\"%s\",\"%s\"\n",
        rcv.key,rcv.p,rcv.repetition,rcv.time_seconds,rcv.iterations_done,rcv.found,rcv.finder_rank,rcv.ts,rcv.hostname,phrase,pc);
    }
    fclose(fp); free(pc);
  }

  free(plain); free(cipher);
  MPI_Finalize();
  return 0;
}
