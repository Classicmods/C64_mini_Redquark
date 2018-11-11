/*
 *  THEC64 Mini
 *  Copyright (C) 2017 Chris Smith
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/* zpipe.c: example of proper use of zlib's inflate() and deflate()
   Not copyrighted -- provided to the public domain
   Version 1.4  11 December 2005  Mark Adler */

#include <stdio.h>
#include <string.h>
#include "zlib.h"

#define CHUNK 16384

/* ------------------------------------------------------------------------- */
/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int 
tsf_compress(FILE *source, FILE *dest, int level)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    //ret = deflateInit(&strm, level);
    ret = deflateInit2(&strm, level, Z_DEFLATED, 15 + 16, 9, Z_DEFAULT_STRATEGY);

    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */

            if(ret == Z_STREAM_ERROR) {  /* state not clobbered */
printf("Stream Error 1\n");
                (void)deflateEnd(&strm);
                return ret;
            }

            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        if( strm.avail_in != 0) {     /* all input will be used */
        }

        /* done when last data in file processed */
    } while (flush != Z_FINISH);

    if( ret != Z_STREAM_END) {        /* stream will be complete */
printf("Stream error END\n");
    }

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int
tsf_decompress(FILE *source, FILE *dest)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    //ret = inflateInit(&strm);
    ret = inflateInit2( &strm, 32 + MAX_WBITS ); // Support gzip

    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            
            if(ret == Z_STREAM_ERROR) {  /* state not clobbered */
            }
            
            switch (ret) {
            case Z_NEED_DICT: ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

/* report a zlib or i/o error */
void
tsf_zerror(int ret)
{
    fputs("zpipe: ", stderr);
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            fputs("error reading stdin\n", stderr);
        if (ferror(stdout))
            fputs("error writing stdout\n", stderr);
        break;
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    }
}

// ----------------------------------------------------------------------------------
//
int
tsf_compress_file( char *src_filename, char *dst_filename )
{
    FILE *src = NULL;
    FILE *dst = NULL;
    int ret = 0;

    do {
        src = fopen( src_filename, "r" );
        if( src == NULL ) {
            printf("Error opening compressed source file %s\n", src_filename );
            ret = -1;
            break;
        }

        dst = fopen( dst_filename, "w+" );
        if( dst == NULL ) {
            printf("Error opening target file %s\n", dst_filename );
            ret = -1;
            break;
        }

        ret = tsf_compress(src, dst, Z_BEST_COMPRESSION);
        if( ret != Z_OK ) {
            tsf_zerror( ret );
        }
    } while(0);

    if( src != NULL) fclose(src);
    if( dst != NULL) fclose(dst);

    if( ret != Z_OK ) {
        printf( "Error decompressing %s\n", src_filename );
        ret = -1;
    } else ret = 0;


    return ret;
}

// ----------------------------------------------------------------------------------
//
int
tsf_decompress_file( char *src_filename, char *dst_filename )
{
    FILE *src = NULL;
    FILE *dst = NULL;
    int ret = 0;

    do {
        src = fopen( src_filename, "r" );
        if( src == NULL ) {
            printf("Error opening compressed source file %s\n", src_filename );
            ret = -1;
            break;
        }

        dst = fopen( dst_filename, "w+" );
        if( dst == NULL ) {
            printf("Error opening target file %s\n", dst_filename );
            ret = -1;
            break;
        }

        ret = tsf_decompress(src, dst);
        if( ret != Z_OK ) {
            tsf_zerror( ret );
        }
    } while(0);

    if( src != NULL) fclose(src);
    if( dst != NULL) fclose(dst);

    if( ret != Z_OK ) {
        printf( "Error decompressing %s\n", src_filename );
        ret = -1;
    } else ret = 0;


    return ret;
}

// ----------------------------------------------------------------------------------
