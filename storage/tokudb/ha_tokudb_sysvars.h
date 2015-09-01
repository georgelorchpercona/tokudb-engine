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

#ifndef _HA_TOKUDB_SYSVARS_H
#define _HA_TOKUDB_SYSVARS_H

enum srv_row_format_t {
    SRV_ROW_FORMAT_UNCOMPRESSED = 0,
    SRV_ROW_FORMAT_ZLIB = 1,
    SRV_ROW_FORMAT_SNAPPY = 2,
    SRV_ROW_FORMAT_QUICKLZ = 3,
    SRV_ROW_FORMAT_LZMA = 4,
    SRV_ROW_FORMAT_FAST = 5,
    SRV_ROW_FORMAT_SMALL = 6,
    SRV_ROW_FORMAT_DEFAULT = 7
};

enum srv_empty_scan_mode_t {
    TOKUDB_EMPTY_SCAN_DISABLED = 0,
    TOKUDB_EMPTY_SCAN_LR = 1,
    TOKUDB_EMPTY_SCAN_RL = 2,
};

extern ulonglong    tokudb_cache_size;
extern uint32_t     tokudb_cachetable_pool_threads;
extern my_bool      tokudb_checkpoint_on_flush_logs;
extern uint32_t     tokudb_checkpoint_pool_threads;
extern uint32_t     tokudb_checkpointing_period;
extern ulong        tokudb_cleaner_iterations;
extern ulong        tokudb_cleaner_period;
extern uint32_t     tokudb_client_pool_threads;
extern my_bool      tokudb_compress_buffers_before_eviction;
extern char*        tokudb_data_dir;
extern ulong        tokudb_debug;
extern my_bool      tokudb_directio;
extern my_bool      tokudb_enable_partial_eviction;
extern int          tokudb_fs_reserve_percent;
extern uint32_t     tokudb_fsync_log_period;
extern char*        tokudb_log_dir;
extern ulonglong    tokudb_max_lock_memory;
extern uint32_t     tokudb_read_status_frequency;
extern char*        tokudb_tmp_dir;
extern char*        tokudb_version;
extern uint32_t     tokudb_write_status_frequency;

#if TOKU_INCLUDE_HANDLERTON_HANDLE_FATAL_SIGNAL
extern char*        tokudb_gdb_path;
extern my_bool      tokudb_gdb_on_fatal;
#endif

#if TOKUDB_CHECK_JEMALLOC
extern uint         tokudb_check_jemalloc;
#endif


extern struct st_mysql_sys_var* tokudb_system_variables[];

#define DEFAULT_TOKUDB_KILLED_TIME 4000
#define DEFAULT_TOKUDB_LOCK_TIMEOUT 4000 /*milliseconds*/

bool get_tokudb_alter_print_error(THD* thd);
double get_tokudb_analyze_delete_fraction(THD* thd);
uint get_tokudb_analyze_time(THD* thd);
uint get_tokudb_block_size(THD* thd);
bool get_tokudb_bulk_fetch(THD* thd);
bool get_tokudb_commit_sync(THD* thd);
bool get_tokudb_create_index_online(THD* thd);
bool get_tokudb_disable_hot_alter(THD* thd);
bool get_tokudb_disable_prefetching(THD* thd);
bool get_tokudb_disable_slow_alter(THD* thd);
bool get_tokudb_disable_slow_update(THD* thd);
bool get_tokudb_disable_slow_upsert(THD* thd);
bool get_tokudb_empty_scan(THD* thd);
uint get_tokudb_fanout(THD* thd);
bool get_tokudb_hide_default_row_format(THD* thd);
uint64_t get_tokudb_killed_time(THD* thd);
bool get_tokudb_load_save_space(THD* thd);
char* get_tokudb_last_lock_timeout(THD* thd);
void set_tokudb_last_lock_timeout(THD* thd, char* last);
uint64_t get_tokudb_loader_memory_size(THD* thd);
uint64_t get_tokudb_lock_timeout(THD* thd);
uint get_tokudb_lock_timeout_debug(THD* thd);
double get_tokudb_optimize_index_fraction(THD* thd);
const char* get_tokudb_optimize_index_name(THD* thd);
uint64_t get_tokudb_optimize_throttle(THD* thd);
uint get_tokudb_pk_insert_mode(THD* thd);
bool get_tokudb_prelock_empty(THD* thd);
uint get_tokudb_read_block_size(THD* thd);
uint get_tokudb_read_buf_size(THD* thd);
srv_row_format_t get_tokudb_row_format(THD *thd);
bool get_tokudb_rpl_check_readonly(THD* thd);
bool get_tokudb_rpl_lookup_rows(THD* thd);
uint64_t get_tokudb_rpl_lookup_rows_delay(THD* thd);
bool get_tokudb_rpl_unique_checks(THD* thd);
uint64_t get_tokudb_rpl_unique_checks_delay(THD* thd);
bool get_tokudb_support_xa(THD* thd);

#endif // _HATOKU_SYSVARS_H
