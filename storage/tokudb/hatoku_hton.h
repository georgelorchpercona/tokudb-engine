/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
#ident "$Id$"
/*======
This file is part of TokuDB


Copyright (c) 2006, 2015, Percona and/or its affiliates. All rights reserved.

    TokuDBis is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License, version 2,
    as published by the Free Software Foundation.

    TokuDB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with TokuDB.  If not, see <http://www.gnu.org/licenses/>.

======= */

#ident "Copyright (c) 2006, 2015, Percona and/or its affiliates. All rights reserved."

#ifndef _HATOKU_HTON_H
#define _HATOKU_HTON_H

#include "db.h"

#include "ha_tokudb_sysvars.h"

extern handlerton *tokudb_hton;

extern DB_ENV *db_env;

static inline srv_row_format_t toku_compression_method_to_row_format(toku_compression_method method) {
    switch (method) {
    case TOKU_NO_COMPRESSION:
        return SRV_ROW_FORMAT_UNCOMPRESSED;        
    case TOKU_ZLIB_WITHOUT_CHECKSUM_METHOD:
    case TOKU_ZLIB_METHOD:
        return SRV_ROW_FORMAT_ZLIB;
    case TOKU_SNAPPY_METHOD:
        return SRV_ROW_FORMAT_SNAPPY;
    case TOKU_QUICKLZ_METHOD:
        return SRV_ROW_FORMAT_QUICKLZ;
    case TOKU_LZMA_METHOD:
        return SRV_ROW_FORMAT_LZMA;
    case TOKU_DEFAULT_COMPRESSION_METHOD:
        return SRV_ROW_FORMAT_DEFAULT;
    case TOKU_FAST_COMPRESSION_METHOD:
        return SRV_ROW_FORMAT_FAST;
    case TOKU_SMALL_COMPRESSION_METHOD:
        return SRV_ROW_FORMAT_SMALL;
    default:
        assert(0);
    }
}

static inline toku_compression_method row_format_to_toku_compression_method(srv_row_format_t row_format) {
    switch (row_format) {
    case SRV_ROW_FORMAT_UNCOMPRESSED:
        return TOKU_NO_COMPRESSION;
    case SRV_ROW_FORMAT_QUICKLZ:
    case SRV_ROW_FORMAT_FAST:
        return TOKU_QUICKLZ_METHOD;
    case SRV_ROW_FORMAT_SNAPPY:
        return TOKU_SNAPPY_METHOD;
    case SRV_ROW_FORMAT_ZLIB:
    case SRV_ROW_FORMAT_DEFAULT:
        return TOKU_ZLIB_WITHOUT_CHECKSUM_METHOD;
    case SRV_ROW_FORMAT_LZMA:
    case SRV_ROW_FORMAT_SMALL:
        return TOKU_LZMA_METHOD;
    default:
        assert(0);
    }
}

static inline enum row_type row_format_to_row_type(srv_row_format_t row_format) {
#if TOKU_INCLUDE_ROW_TYPE_COMPRESSION
    switch (row_format) {
    case SRV_ROW_FORMAT_UNCOMPRESSED:
        return ROW_TYPE_TOKU_UNCOMPRESSED;
    case SRV_ROW_FORMAT_ZLIB:
        return ROW_TYPE_TOKU_ZLIB;
    case SRV_ROW_FORMAT_SNAPPY:
        return ROW_TYPE_TOKU_SNAPPY;
    case SRV_ROW_FORMAT_QUICKLZ:
        return ROW_TYPE_TOKU_QUICKLZ;
    case SRV_ROW_FORMAT_LZMA:
        return ROW_TYPE_TOKU_LZMA;
    case SRV_ROW_FORMAT_SMALL:
        return ROW_TYPE_TOKU_SMALL;
    case SRV_ROW_FORMAT_FAST:
        return ROW_TYPE_TOKU_FAST;
    case SRV_ROW_FORMAT_DEFAULT:
        return ROW_TYPE_DEFAULT;
    }
#endif
    return ROW_TYPE_DEFAULT;
}

static inline srv_row_format_t row_type_to_row_format(enum row_type type) {
#if TOKU_INCLUDE_ROW_TYPE_COMPRESSION
    switch (type) {
    case ROW_TYPE_TOKU_UNCOMPRESSED:
        return SRV_ROW_FORMAT_UNCOMPRESSED;
    case ROW_TYPE_TOKU_ZLIB:
        return SRV_ROW_FORMAT_ZLIB;
    case ROW_TYPE_TOKU_SNAPPY:
        return SRV_ROW_FORMAT_SNAPPY;
    case ROW_TYPE_TOKU_QUICKLZ:
        return SRV_ROW_FORMAT_QUICKLZ;
    case ROW_TYPE_TOKU_LZMA:
        return SRV_ROW_FORMAT_LZMA;
    case ROW_TYPE_TOKU_SMALL:
        return SRV_ROW_FORMAT_SMALL;
    case ROW_TYPE_TOKU_FAST:
        return SRV_ROW_FORMAT_FAST;
    case ROW_TYPE_DEFAULT:
        return SRV_ROW_FORMAT_DEFAULT;
    default:
        return SRV_ROW_FORMAT_DEFAULT;
    }
#endif
    return SRV_ROW_FORMAT_DEFAULT;
}

static inline enum row_type toku_compression_method_to_row_type(toku_compression_method method) {
    return row_format_to_row_type(toku_compression_method_to_row_format(method));
}

static inline toku_compression_method row_type_to_toku_compression_method(enum row_type type) {
    return row_format_to_toku_compression_method(row_type_to_row_format(type));
}

void tokudb_checkpoint_lock(THD * thd);
void tokudb_checkpoint_unlock(THD * thd);

static uint64_t tokudb_get_lock_wait_time_callback(uint64_t default_wait_time) {
    THD *thd = current_thd;
    return get_tokudb_lock_timeout(thd);
}

static uint64_t tokudb_get_loader_memory_size_callback(void) {
    THD *thd = current_thd;
    return get_tokudb_loader_memory_size(thd);
}

static uint64_t tokudb_get_killed_time_callback(uint64_t default_killed_time) {
    THD *thd = current_thd;
    return get_tokudb_killed_time(thd);
}

static int tokudb_killed_callback(void) {
    THD *thd = current_thd;
    return thd_killed(thd);
}

static bool tokudb_killed_thd_callback(void *extra, uint64_t deleted_rows) {
    THD *thd = static_cast<THD *>(extra);
    return thd_killed(thd) != 0;
}




extern HASH tokudb_open_tables;
extern pthread_mutex_t tokudb_mutex;
extern uint32_t tokudb_write_status_frequency;
extern uint32_t tokudb_read_status_frequency;

void toku_hton_update_primary_key_bytes_inserted(uint64_t row_size);

#endif //#ifdef _HATOKU_HTON
