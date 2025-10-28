#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <unistd.h>

#include "../include/common.h"
#include "../include/impl3.h"

#define TAG_REQ  1
#define TAG_CHUNK 2
#define TAG_FOUND 3
#define TAG_STOP  4

static const char *CSV_HEADER =
"implementation,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname,phrase,text";

static unsigned char* readAllStdin(size_t *out_len){
    size_t cap = 4096, len = 0;
    unsigned char *buf = malloc(cap);
    if(!buf) return NULL;
    for(;;){
        size_t n = fread(buf+len,1,cap-len,stdin);
        len += n;
        if(n==0) break;
        if(len==cap){
            cap*=2;
            unsigned char *t = realloc(buf,cap);
            if(!t){ free(buf); return NULL; }
            buf = t;
        }
    }
    *out_len = len;
    return buf;
}

static void printUsage(const char *p){
    fprintf(stderr,"uso: echo \"<texto>\" | mpirun -np <P> %s <frase> <key> <csv_dir> <hostname>\n", p);
}

int main(int argc, char **argv){
    MPI_Init(&argc,&argv);
    int rank,size;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);

    if(argc<5){
        if(rank==0) printUsage(argv[0]);
        MPI_Finalize();
        return 2;
    }

    const char *phrase   = argv[1];
    uint64_t key_true    = strtoull(argv[2],NULL,10);
    const char *csv_dir  = argv[3];
    const char *hostname = argv[4];

    size_t plain_len=0;
    unsigned char *plain = NULL;
    if(rank==0){
        plain = readAllStdin(&plain_len);
        if(!plain || plain_len==0){
            fprintf(stderr,"stdin vacio\n");
            MPI_Abort(MPI_COMM_WORLD,3);
        }
    }

    MPI_Bcast(&plain_len,1,MPI_UNSIGNED_LONG,0,MPI_COMM_WORLD);
    if(rank!=0) plain = malloc(plain_len);
    MPI_Bcast(plain,plain_len,MPI_UNSIGNED_CHAR,0,MPI_COMM_WORLD);

    unsigned char *cipher=NULL;
    size_t cipher_len=0;
    if(rank==0){
        encryptDesEcb(key_true,plain,plain_len,&cipher,&cipher_len);
    }
    MPI_Bcast(&cipher_len,1,MPI_UNSIGNED_LONG,0,MPI_COMM_WORLD);
    if(rank!=0)
        cipher = malloc(cipher_len);
    MPI_Bcast(cipher,cipher_len,MPI_UNSIGNED_CHAR,0,MPI_COMM_WORLD);

    char csvpath[256];
    snprintf(csvpath,sizeof(csvpath),"%s/impl3_par_rank%d.csv",csv_dir,rank);
    FILE *fp = fopen(csvpath,"a+");
    ensureHeader(fp,CSV_HEADER);

    uint64_t next=0;
    uint64_t found_key = UINT64_MAX;
    uint64_t iters = 0;
    int found=0;

    struct timespec t0=nowMono();

    if(rank==0){
        int active=size-1;
        uint64_t chunk = key_true/size;
        if(chunk<1) chunk=1;
        MPI_Status st;
        while(active>0 && !found){
            MPI_Probe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&st);
            int src=st.MPI_SOURCE;
            int tag=st.MPI_TAG;

            if(tag==TAG_REQ){
                MPI_Recv(NULL,0,MPI_BYTE,src,TAG_REQ,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

                if(found || next>key_true){
                    uint64_t stopmsg[2]={UINT64_MAX,UINT64_MAX};
                    MPI_Send(stopmsg,2,MPI_UNSIGNED_LONG_LONG,src,TAG_CHUNK,MPI_COMM_WORLD);
                    active--;
                } else {
                    uint64_t seg[2];
                    seg[0]=next;
                    uint64_t e=next+chunk-1;
                    if(e>key_true) e=key_true;
                    seg[1]=e;
                    next=e+1;
                    MPI_Send(seg,2,MPI_UNSIGNED_LONG_LONG,src,TAG_CHUNK,MPI_COMM_WORLD);
                }
            }
            else if(tag==TAG_FOUND){
                uint64_t payload[2];
                MPI_Recv(payload,2,MPI_UNSIGNED_LONG_LONG,src,TAG_FOUND,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                found=1;
                found_key=payload[0];
                iters   =payload[1];

                uint64_t stopmsg[2]={UINT64_MAX,UINT64_MAX};
                for(int r=1;r<size;r++)
                    MPI_Send(stopmsg,2,MPI_UNSIGNED_LONG_LONG,r,TAG_CHUNK,MPI_COMM_WORLD);
            }
        }
    } else {
        int stop=0;
        while(!stop){
            MPI_Send(NULL,0,MPI_BYTE,0,TAG_REQ,MPI_COMM_WORLD);
            uint64_t seg[2];
            MPI_Recv(seg,2,MPI_UNSIGNED_LONG_LONG,0,TAG_CHUNK,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            if(seg[0]==UINT64_MAX){
                stop=1;
                break;
            }
            for(uint64_t k=seg[0];k<=seg[1];++k){
                unsigned char *dec=NULL; size_t dec_len=0;
                decryptDesEcb(k,cipher,cipher_len,&dec,&dec_len); iters++;
                if(containsPhrase(dec,dec_len,phrase)){
                    found=1;
                    found_key=k;
                    uint64_t payload[2]={k,iters};
                    MPI_Send(payload,2,MPI_UNSIGNED_LONG_LONG,0,TAG_FOUND,MPI_COMM_WORLD);
                    free(dec);
                    stop=1;
                    break;
                }
                free(dec);
            }
        }
    }

    struct timespec t1 = nowMono();
    time_span_t dt = diffMono(t0,t1);
    char ts[64]; isoUtcNow(ts,sizeof ts);

    char pbuf[plain_len+1]; memcpy(pbuf,plain,plain_len); pbuf[plain_len]=0;

    fprintf(fp,
      "impl3,%llu,%d,%d,%.9f,%llu,%d,%d,%s,%s,\"%s\",\"%s\"\n",
      (unsigned long long)found_key,
      size,1,
      dt.secs,
      (unsigned long long)iters,
      found,rank,ts,hostname,phrase,pbuf);

    fclose(fp);
    free(plain);
    free(cipher);
    MPI_Finalize();
    return 0;
}
