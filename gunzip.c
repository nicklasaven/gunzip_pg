
#include <stdio.h>
#include "postgres.h"
#include "fmgr.h"
#include "zlib.h"

#include <assert.h>
#include "utils/elog.h"


#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif


typedef struct
{
    size_t alloced;
    size_t used;
    char *data;
} RAWDATA;

int uncomp(RAWDATA *rd, unsigned char *source, size_t sourceLen);

/**
 *Memory handling
 * Just a function handling allocation error on malloc
 */
static inline void* st_malloc(size_t len)
{

    void *res = malloc(len);
    if(res==NULL)
    {
        fprintf(stderr, "Misslyckad allokering av minne\n");
        exit(EXIT_FAILURE);
    }
    return res;
}


/**
 *Memory handling
 * Just a function handling allocation error on realloc
 */
static inline void* st_realloc(void *ptr, size_t len)
{

    void *res = realloc(ptr, len);
    if(res==NULL)
    {
        free(ptr);
        fprintf(stderr, "Misslyckad allokering av minne\n");
        exit(EXIT_FAILURE);
    }
    return res;
}


/**
 *Memory handling
 * Just a function handling allocation error on calloc
 */
static inline void* st_calloc(int n, size_t s)
{

    void *res = calloc(n, s);
    if(res==NULL)
    {
        fprintf(stderr, "Misslyckad allokering av minne\n");
        exit(EXIT_FAILURE);
    }
    return res;
}

static inline RAWDATA* init_data(size_t init_size)
{
    RAWDATA *rd = st_malloc(sizeof(RAWDATA));
    rd->data = st_calloc(1, init_size);
    rd->alloced = init_size;
    rd->used = 0;

    return rd;

}


static inline void realloc_data(RAWDATA *rd)
{
    size_t new_size = rd->alloced * 2;
    rd->data = st_realloc(rd->data, new_size);
    rd->alloced = new_size;

    return;
}


static inline void realloc_data_needed(RAWDATA *rd, size_t needed)
{
    size_t new_size = rd->alloced;
    while (new_size < needed)
    {
        new_size*=2;

    };



    if(new_size>rd->alloced)
    {
        rd->data = st_realloc(rd->data, new_size);
        rd->alloced = new_size;
    }

    return;
}



static inline void reset_data(RAWDATA *rd)
{
    rd->used = 0;
    return;
}



static inline void destroy_data(RAWDATA *rd)
{
    free(rd->data);
    rd->used = 0;
    rd->alloced = 0;

    free(rd);
    rd = NULL;
}



int uncomp(RAWDATA *rd, unsigned char *source, size_t sourceLen)
{
    int ret;
    z_stream strm;
    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    //ret = inflateInit(&strm);
    ret = inflateInit2(&strm, 16+MAX_WBITS);
    if (ret != Z_OK)
        return ret;


    strm.avail_out = rd->alloced - rd->used;;
    strm.next_out = (unsigned char *) rd->data;
    /* decompress until deflate stream ends or end of file */
    do
    {
        strm.avail_in = sourceLen;

        if (strm.avail_in == 0)
            break;
        strm.next_in = (unsigned char *) source;

        /* run inflate() on input until output buffer not full */
        while(1)
        {
            strm.avail_out = rd->alloced - rd->used;
	//elog(NOTICE, "strm.avail_out = %d, rd->alloced =%ld, rd->used= %ld ",strm.avail_out,rd->alloced, rd->used);
            strm.next_out =(unsigned char *) rd->data + rd->used;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            rd->used = rd->alloced - strm.avail_out;

            if(strm.avail_out == 0)
                realloc_data(rd);
            else
                break;
        }
        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    if(strm.avail_out == 0)
        realloc_data(rd);

     //elog(NOTICE, "slut: strm.avail_out = %d, rd->alloced =%ld, rd->used= %ld ",strm.avail_out,rd->alloced, rd->used);
    rd->data[rd->used] = '\0';
    rd->used++;

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}


Datum gunzip(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(gunzip);
Datum gunzip(PG_FUNCTION_ARGS) {
    bytea *gzipped_input = PG_GETARG_BYTEA_P(0);
    unsigned char *src = (unsigned char *) VARDATA(gzipped_input);
    text *destination;
    size_t srclen =  VARSIZE(gzipped_input) - VARHDRSZ;

    RAWDATA *rd = init_data(srclen * 2);// Vi börjar med dubbelt så stor output som input så får vi se

//  unsigned char *dst_ptr = (unsigned char *) VARDATA(destination);
    if(uncomp (rd,src, srclen) != Z_OK)
    {
        elog(ERROR,"Failed unzipping\n");
	PG_RETURN_NULL();
    }

    destination = (text *) palloc(VARHDRSZ + rd->used);
    strcpy(VARDATA(destination), rd->data);
    SET_VARSIZE(destination, VARHDRSZ + strlen(rd->data));


    destroy_data(rd);
    PG_RETURN_TEXT_P(destination);
}
