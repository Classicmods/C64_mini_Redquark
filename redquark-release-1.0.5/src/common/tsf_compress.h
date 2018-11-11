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
#ifndef TSF_COMPRESSION_H

#include <stdio.h>
#include <string.h>
#include "zlib.h"

int tsf_compress(FILE *source, FILE *dest, int level);
int tsf_decompress(FILE *source, FILE *dest);
void tsf_zerror(int ret);


int tsf_compress_file  ( char *src_filename, char *dst_filename );
int tsf_decompress_file( char *src_filename, char *dst_filename );

#endif
