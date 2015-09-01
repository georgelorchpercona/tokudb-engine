/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
/* -*- mode: C; c-basic-offset: 4 -*- */
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

#include <mysql/plugin.h>
#include "ha_tokudb_sysvars.h"

//******************************************************************************
// global variables
//******************************************************************************
#ifdef TOKUDB_VERSION
#define tokudb_stringify_2(x) #x
#define tokudb_stringify(x) tokudb_stringify_2(x)
#define TOKUDB_VERSION_STR tokudb_stringify(TOKUDB_VERSION)
#else
#define TOKUDB_VERSION_STR NULL
#endif

ulonglong   tokudb_cache_size = 0;
uint32_t    tokudb_cachetable_pool_threads = 0;
my_bool     tokudb_checkpoint_on_flush_logs = FALSE;
uint32_t    tokudb_checkpoint_pool_threads = 0;
uint32_t    tokudb_checkpointing_period = 0;
ulong       tokudb_cleaner_iterations = 0;
ulong       tokudb_cleaner_period = 0;
uint32_t    tokudb_client_pool_threads = 0;
my_bool     tokudb_compress_buffers_before_eviction = TRUE;
char*       tokudb_data_dir = NULL;
ulong       tokudb_debug = 0;
my_bool     tokudb_directio = FALSE;
my_bool     tokudb_enable_partial_eviction = TRUE;
int         tokudb_fs_reserve_percent = 0;
uint32_t    tokudb_fsync_log_period = 0;
char*       tokudb_log_dir = NULL;
ulonglong   tokudb_max_lock_memory = 0;
uint32_t    tokudb_read_status_frequency = 0;
char*       tokudb_tmp_dir = NULL;
char*       tokudb_version = (char *) TOKUDB_VERSION_STR;
uint32_t    tokudb_write_status_frequency = 0;

// file system reserve as a percentage of total disk space
#if TOKU_INCLUDE_HANDLERTON_HANDLE_FATAL_SIGNAL
char*       tokudb_gdb_path = NULL;
my_bool     tokudb_gdb_on_fatal = FALSE;
#endif

#if TOKUDB_CHECK_JEMALLOC
uint        tokudb_check_jemalloc = 0;
#endif

static MYSQL_SYSVAR_ULONGLONG(cache_size, tokudb_cache_size,
    PLUGIN_VAR_READONLY, "TokuDB cache table size",
    NULL, NULL, 0,
    0, ~0ULL, 0);

static MYSQL_SYSVAR_UINT(cachetable_pool_threads, tokudb_cachetable_pool_threads,
    PLUGIN_VAR_READONLY, "TokuDB cachetable ops thread pool size", NULL, NULL, 0,
    0, 1024, 0);

static MYSQL_SYSVAR_BOOL(checkpoint_on_flush_logs, tokudb_checkpoint_on_flush_logs,
    0, "TokuDB Checkpoint on Flush Logs ",
    NULL, NULL, FALSE);

static MYSQL_SYSVAR_UINT(checkpoint_pool_threads, tokudb_checkpoint_pool_threads,
    PLUGIN_VAR_READONLY, "TokuDB checkpoint ops thread pool size", NULL, NULL, 0,
    0, 1024, 0);

static void tokudb_checkpointing_period_update(THD* thd,
                                               struct st_mysql_sys_var* sys_var,
                                               void* var, const void* save) {
    uint * checkpointing_period = (uint *) var;
    *checkpointing_period = *(const ulonglong *) save;
    int r = db_env->checkpointing_set_period(db_env, *checkpointing_period);
    assert(r == 0);
}

static MYSQL_SYSVAR_UINT(checkpointing_period, tokudb_checkpointing_period, 
    0, "TokuDB Checkpointing period", 
    NULL, tokudb_checkpointing_period_update, 60, 
    0, ~0U, 0);


static void tokudb_cleaner_iterations_update(THD* thd,
                                             struct st_mysql_sys_var* sys_var,
                                             void* var, const void* save) {
    ulong * cleaner_iterations = (ulong *) var;
    *cleaner_iterations = *(const ulonglong *) save;
    int r = db_env->cleaner_set_iterations(db_env, *cleaner_iterations);
    assert(r == 0);
}

#define DEFAULT_CLEANER_ITERATIONS 5

static MYSQL_SYSVAR_ULONG(cleaner_iterations, tokudb_cleaner_iterations,
    0, "TokuDB cleaner_iterations", 
    NULL, tokudb_cleaner_iterations_update, DEFAULT_CLEANER_ITERATIONS,
    0, ~0UL, 0);

static void tokudb_cleaner_period_update(THD* thd,
                                         struct st_mysql_sys_var* sys_var,
                                         void* var, const void * save) {
    ulong * cleaner_period = (ulong *) var;
    *cleaner_period = *(const ulonglong *) save;
    int r = db_env->cleaner_set_period(db_env, *cleaner_period);
    assert(r == 0);
}

#define DEFAULT_CLEANER_PERIOD 1

static MYSQL_SYSVAR_ULONG(cleaner_period, tokudb_cleaner_period,
    0, "TokuDB cleaner_period", 
    NULL, tokudb_cleaner_period_update, DEFAULT_CLEANER_PERIOD,
    0, ~0UL, 0);

static MYSQL_SYSVAR_UINT(client_pool_threads, tokudb_client_pool_threads,
    PLUGIN_VAR_READONLY, "TokuDB client ops thread pool size", NULL, NULL, 0,
    0, 1024, 0);

static MYSQL_SYSVAR_BOOL(compress_buffers_before_eviction,
    tokudb_compress_buffers_before_eviction,
    PLUGIN_VAR_READONLY,
    "TokuDB Enable buffer compression before partial eviction",
    NULL, NULL, TRUE);

static MYSQL_SYSVAR_STR(data_dir, tokudb_data_dir,
    PLUGIN_VAR_READONLY, "TokuDB Data Directory",
    NULL, NULL, NULL);

static MYSQL_SYSVAR_ULONG(debug, tokudb_debug,
    0, "TokuDB Debug",
    NULL, NULL, 0,
    0, ~0UL, 0);

static MYSQL_SYSVAR_BOOL(directio, tokudb_directio,
    PLUGIN_VAR_READONLY, "TokuDB Enable Direct I/O ",
    NULL, NULL, FALSE);

static void tokudb_enable_partial_eviction_update(THD* thd,
                                             struct st_mysql_sys_var* sys_var,
                                             void* var, const void* save) {
    my_bool * enable_partial_eviction = (my_bool *) var;
    *enable_partial_eviction = *(const my_bool *) save;
    int r = db_env->evictor_set_enable_partial_eviction(db_env, *enable_partial_eviction);
    assert(r == 0);
}

static MYSQL_SYSVAR_BOOL(enable_partial_eviction, tokudb_enable_partial_eviction,
    0, "TokuDB enable partial node eviction", 
    NULL, tokudb_enable_partial_eviction_update, TRUE);

static MYSQL_SYSVAR_INT(fs_reserve_percent, tokudb_fs_reserve_percent,
    PLUGIN_VAR_READONLY, "TokuDB file system space reserve (percent free required)",
    NULL, NULL, 5,
    0, 100, 0);

static void tokudb_fsync_log_period_update(THD* thd,
                                           struct st_mysql_sys_var* sys_var,
                                           void* var, const void* save) {
    uint32 *period = (uint32 *) var;
    *period = *(const ulonglong *) save;
    db_env->change_fsync_log_period(db_env, *period);
}

static MYSQL_SYSVAR_UINT(fsync_log_period, tokudb_fsync_log_period,
    0, "TokuDB fsync log period",
    NULL, tokudb_fsync_log_period_update, 0,
    0, ~0U, 0);

static MYSQL_SYSVAR_STR(log_dir, tokudb_log_dir,
    PLUGIN_VAR_READONLY, "TokuDB Log Directory",
    NULL, NULL, NULL);

static MYSQL_SYSVAR_ULONGLONG(max_lock_memory, tokudb_max_lock_memory,
    PLUGIN_VAR_READONLY, "TokuDB max memory for locks",
    NULL, NULL, 0,
    0, ~0ULL, 0);

static MYSQL_SYSVAR_UINT(read_status_frequency, tokudb_read_status_frequency,
    0, "TokuDB frequency that show processlist updates status of reads",
    NULL, NULL, 10000,
    0, ~0U, 0);

static MYSQL_SYSVAR_STR(tmp_dir, tokudb_tmp_dir,
    PLUGIN_VAR_READONLY, "Tokudb Tmp Dir",
    NULL, NULL, NULL);

static MYSQL_SYSVAR_STR(version, tokudb_version,
    PLUGIN_VAR_READONLY, "TokuDB Version",
    NULL, NULL, NULL);

static MYSQL_SYSVAR_UINT(write_status_frequency, tokudb_write_status_frequency,
    0, "TokuDB frequency that show processlist updates status of writes",
    NULL, NULL, 1000,
    0, ~0U, 0);

#if TOKU_INCLUDE_HANDLERTON_HANDLE_FATAL_SIGNAL
static MYSQL_SYSVAR_STR(gdb_path, tokudb_gdb_path,
    PLUGIN_VAR_READONLY|PLUGIN_VAR_RQCMDARG, "TokuDB path to gdb for extra debug info on fatal signal",
    NULL, NULL, "/usr/bin/gdb");

static MYSQL_SYSVAR_BOOL(gdb_on_fatal, tokudb_gdb_on_fatal,
    0, "TokuDB enable gdb debug info on fatal signal",
    NULL, NULL, true);
#endif

#if TOKUDB_CHECK_JEMALLOC
static MYSQL_SYSVAR_UINT(check_jemalloc, tokudb_check_jemalloc, 0, "Check if jemalloc is linked",
                         NULL, NULL, 1, 0, 1, 0);
#endif


//******************************************************************************
// session variables
//******************************************************************************
static MYSQL_THDVAR_BOOL(alter_print_error,
    0, "Print errors for alter table operations",
    NULL, NULL, false);

static MYSQL_THDVAR_DOUBLE(analyze_delete_fraction,
    0, "fraction of rows allowed to be deleted",
    NULL, NULL, 1.0, 0, 1.0, 1);

static MYSQL_THDVAR_UINT(analyze_time,
    0, "analyze time (seconds)",
    NULL, NULL, 5, 0, ~0U, 1);

static MYSQL_THDVAR_UINT(block_size,
    0, "fractal tree block size",
    NULL, NULL, 4<<20, 4096, ~0U, 1);

static MYSQL_THDVAR_BOOL(bulk_fetch,
    PLUGIN_VAR_THDLOCAL, "enable bulk fetch",
    NULL, NULL, true);

static void tokudb_checkpoint_lock_update(THD* thd,
                                          struct st_mysql_sys_var* var,
                                          void* var_ptr, const void* save) {
    my_bool* val = (my_bool *) var_ptr;
    *val= *(my_bool *) save ? true : false;
    if (*val) {
        tokudb_checkpoint_lock(thd);
    } else {
        tokudb_checkpoint_unlock(thd);
    }
}
  
static MYSQL_THDVAR_BOOL(checkpoint_lock,
    0, "Tokudb Checkpoint Lock",
    NULL, tokudb_checkpoint_lock_update, false);

static MYSQL_THDVAR_BOOL(commit_sync, 
    PLUGIN_VAR_THDLOCAL, "sync on txn commit",
    NULL, NULL, true);

static MYSQL_THDVAR_BOOL(create_index_online,
    0, "if on, create index done online",
    NULL, NULL, true);

static MYSQL_THDVAR_BOOL(disable_hot_alter,
    0, "if on, hot alter table is disabled",
    NULL, NULL, false);

static MYSQL_THDVAR_BOOL(disable_prefetching,
    0, "if on, prefetching disabled",
    NULL, NULL, false);

static MYSQL_THDVAR_BOOL(disable_slow_alter,
    0, "if on, alter tables that require copy are disabled",
    NULL, NULL, false);

static const char *tokudb_empty_scan_names[] = {
    "disabled",
    "lr",
    "rl",
    NullS
};

static TYPELIB tokudb_empty_scan_typelib = {
    array_elements(tokudb_empty_scan_names) - 1,
    "tokudb_empty_scan_typelib",
    tokudb_empty_scan_names,
    NULL
};

static MYSQL_THDVAR_ENUM(empty_scan,
    PLUGIN_VAR_OPCMDARG,
    "TokuDB algorithm to check if the table is empty when opened. ",
    NULL, NULL, TOKUDB_EMPTY_SCAN_RL, &tokudb_empty_scan_typelib);

static MYSQL_THDVAR_UINT(fanout,
    0, "fractal tree fanout",
    NULL, NULL, 16, 2, 16*1024, 1);

static MYSQL_THDVAR_BOOL(hide_default_row_format,
    0, "hide the default row format",
    NULL, NULL, true);

static MYSQL_THDVAR_ULONGLONG(killed_time,
    0, "TokuDB killed time",
    NULL, NULL, DEFAULT_TOKUDB_KILLED_TIME, 0, ~0ULL, 1);

static MYSQL_THDVAR_STR(last_lock_timeout,
    PLUGIN_VAR_MEMALLOC, "last TokuDB lock timeout",
    NULL, NULL, NULL);

static MYSQL_THDVAR_BOOL(load_save_space,
    0, "compress intermediate bulk loader files to save space",
    NULL, NULL, true);

static MYSQL_THDVAR_ULONGLONG(loader_memory_size,
    0, "TokuDB loader memory size",
    NULL, NULL, 100*1000*1000, 0, ~0ULL, 1);

static MYSQL_THDVAR_ULONGLONG(lock_timeout,
    0, "TokuDB lock timeout",
    NULL, NULL, DEFAULT_TOKUDB_LOCK_TIMEOUT, 0, ~0ULL, 1);

static MYSQL_THDVAR_UINT(lock_timeout_debug,
    0, "TokuDB lock timeout debug",
    NULL, NULL, 1, 0, ~0U, 1);

static MYSQL_THDVAR_DOUBLE(optimize_index_fraction,
    0, "optimize index fraction (default 1.0 all)",
    NULL, NULL, 1.0, 0, 1.0, 1);

static MYSQL_THDVAR_STR(optimize_index_name,
    PLUGIN_VAR_THDLOCAL + PLUGIN_VAR_MEMALLOC,
    "optimize index name (default all indexes)",
    NULL, NULL, NULL);

static MYSQL_THDVAR_ULONGLONG(optimize_throttle,
    0, "optimize throttle (default no throttle)",
    NULL, NULL, 0, 0, ~0ULL, 1);

static MYSQL_THDVAR_UINT(pk_insert_mode,
    0, "set the primary key insert mode",
    NULL, NULL, 1, 0, 2, 1);

static MYSQL_THDVAR_BOOL(prelock_empty,
    0, "Tokudb Prelock Empty Table",
    NULL, NULL, true);

static MYSQL_THDVAR_UINT(read_block_size,
    0, "fractal tree read block size",
    NULL, NULL, 64*1024, 4096, ~0U, 1);

static MYSQL_THDVAR_UINT(read_buf_size,
    0, "fractal tree read block size", //TODO: Is this a typo?
    NULL, NULL, 128*1024, 0, 1*1024*1024, 1);

static const char *tokudb_row_format_names[] = {
    "tokudb_uncompressed",
    "tokudb_zlib",
    "tokudb_snappy",
    "tokudb_quicklz",
    "tokudb_lzma",
    "tokudb_fast",
    "tokudb_small",
    "tokudb_default",
    NullS
};

static TYPELIB tokudb_row_format_typelib = {
    array_elements(tokudb_row_format_names) - 1,
    "tokudb_row_format_typelib",
    tokudb_row_format_names,
    NULL
};

static MYSQL_THDVAR_ENUM(row_format,
    PLUGIN_VAR_OPCMDARG,
    "Specifies the compression method for a table during this session. "
    "Possible values are TOKUDB_UNCOMPRESSED, TOKUDB_ZLIB, TOKUDB_SNAPPY, "
    "TOKUDB_QUICKLZ, TOKUDB_LZMA, TOKUDB_FAST, TOKUDB_SMALL and TOKUDB_DEFAULT",
    NULL, NULL, SRV_ROW_FORMAT_ZLIB, &tokudb_row_format_typelib);


static MYSQL_THDVAR_BOOL(rpl_check_readonly,
    PLUGIN_VAR_THDLOCAL, "check if the slave is read only",
    NULL, NULL, true);

static MYSQL_THDVAR_BOOL(rpl_lookup_rows,
    PLUGIN_VAR_THDLOCAL, "lookup a row on rpl slave",
    NULL, NULL, true);

static MYSQL_THDVAR_ULONGLONG(rpl_lookup_rows_delay,
    PLUGIN_VAR_THDLOCAL, "time in milliseconds to add to lookups on replication slave",
    NULL, NULL, 0, 0, ~0ULL, 1);

static MYSQL_THDVAR_BOOL(rpl_unique_checks,
    PLUGIN_VAR_THDLOCAL, "enable unique checks on replication slave",
    NULL, NULL, true);

static MYSQL_THDVAR_ULONGLONG(rpl_unique_checks_delay,
    PLUGIN_VAR_THDLOCAL, "time in milliseconds to add to unique checks test on replication slave",
    NULL, NULL, 0, 0, ~0ULL, 1);

#if TOKU_INCLUDE_UPSERT
static MYSQL_THDVAR_BOOL(disable_slow_update, 
    PLUGIN_VAR_THDLOCAL, "disable slow update",
    NULL, NULL, false);

static MYSQL_THDVAR_BOOL(disable_slow_upsert, 
    PLUGIN_VAR_THDLOCAL, "disable slow upsert",
    NULL, NULL, false);
#endif

#if TOKU_INCLUDE_XA
static MYSQL_THDVAR_BOOL(support_xa,
    PLUGIN_VAR_OPCMDARG, "Enable TokuDB support for the XA two-phase commit",
    NULL, NULL, true);
#endif



//******************************************************************************
// all system variables
//******************************************************************************
struct st_mysql_sys_var *tokudb_system_variables[] = {
    // global vars
    MYSQL_SYSVAR(cache_size),
    MYSQL_SYSVAR(checkpoint_on_flush_logs),
    MYSQL_SYSVAR(cachetable_pool_threads),
    MYSQL_SYSVAR(checkpoint_pool_threads),
    MYSQL_SYSVAR(checkpointing_period),
    MYSQL_SYSVAR(cleaner_iterations),
    MYSQL_SYSVAR(cleaner_period),
    MYSQL_SYSVAR(client_pool_threads),
    MYSQL_SYSVAR(compress_buffers_before_eviction),
    MYSQL_SYSVAR(data_dir),
    MYSQL_SYSVAR(debug),
    MYSQL_SYSVAR(directio),
    MYSQL_SYSVAR(enable_partial_eviction),
    MYSQL_SYSVAR(fs_reserve_percent),
    MYSQL_SYSVAR(fsync_log_period),
    MYSQL_SYSVAR(log_dir),
    MYSQL_SYSVAR(max_lock_memory),
    MYSQL_SYSVAR(read_status_frequency),
    MYSQL_SYSVAR(tmp_dir),
    MYSQL_SYSVAR(version),
    MYSQL_SYSVAR(write_status_frequency),

#if TOKU_INCLUDE_HANDLERTON_HANDLE_FATAL_SIGNAL
    MYSQL_SYSVAR(gdb_path),
    MYSQL_SYSVAR(gdb_on_fatal),
#endif

#if TOKUDB_CHECK_JEMALLOC
    MYSQL_SYSVAR(check_jemalloc),
#endif

    // session vars
    MYSQL_SYSVAR(alter_print_error),
    MYSQL_SYSVAR(analyze_delete_fraction),
    MYSQL_SYSVAR(analyze_time),
    MYSQL_SYSVAR(block_size),
    MYSQL_SYSVAR(bulk_fetch),
    MYSQL_SYSVAR(checkpoint_lock),
    MYSQL_SYSVAR(commit_sync),
    MYSQL_SYSVAR(create_index_online),
    MYSQL_SYSVAR(disable_hot_alter),
    MYSQL_SYSVAR(disable_prefetching),
    MYSQL_SYSVAR(disable_slow_alter),
    MYSQL_SYSVAR(empty_scan),
    MYSQL_SYSVAR(fanout),
    MYSQL_SYSVAR(hide_default_row_format),
    MYSQL_SYSVAR(killed_time),
    MYSQL_SYSVAR(last_lock_timeout),
    MYSQL_SYSVAR(load_save_space),
    MYSQL_SYSVAR(loader_memory_size),
    MYSQL_SYSVAR(lock_timeout),
    MYSQL_SYSVAR(lock_timeout_debug),
    MYSQL_SYSVAR(optimize_index_fraction),
    MYSQL_SYSVAR(optimize_index_name),
    MYSQL_SYSVAR(optimize_throttle),
    MYSQL_SYSVAR(pk_insert_mode),
    MYSQL_SYSVAR(prelock_empty),
    MYSQL_SYSVAR(read_block_size),
    MYSQL_SYSVAR(read_buf_size),
    MYSQL_SYSVAR(row_format),
    MYSQL_SYSVAR(rpl_check_readonly),
    MYSQL_SYSVAR(rpl_lookup_rows),
    MYSQL_SYSVAR(rpl_lookup_rows_delay),
    MYSQL_SYSVAR(rpl_unique_checks),
    MYSQL_SYSVAR(rpl_unique_checks_delay),

#if TOKU_INCLUDE_UPSERT
    MYSQL_SYSVAR(disable_slow_update),
    MYSQL_SYSVAR(disable_slow_upsert),
#endif

#if TOKU_INCLUDE_XA
    MYSQL_SYSVAR(support_xa),
#endif
    NULL
};

bool get_tokudb_alter_print_error(THD* thd) {
    return (THDVAR(thd, alter_print_error) != 0);
}

double get_tokudb_analyze_delete_fraction(THD* thd) {
    return THDVAR(thd, analyze_delete_fraction);
}

uint get_tokudb_analyze_time(THD* thd) {
    return THDVAR(thd, analyze_time);
}

bool get_tokudb_bulk_fetch(THD* thd) {
    return (THDVAR(thd, bulk_fetch) != 0);
}

uint get_tokudb_block_size(THD* thd) {
    return THDVAR(thd, block_size);
}

bool get_tokudb_commit_sync(THD* thd) {
    return (THDVAR(thd, commit_sync) != 0);
}

bool get_tokudb_create_index_online(THD* thd) {
    return (THDVAR(thd, create_index_online) != 0);
}

bool get_tokudb_disable_hot_alter(THD* thd) {
    return (THDVAR(thd, disable_hot_alter) != 0);
}

bool get_tokudb_disable_prefetching(THD* thd) {
    return (THDVAR(thd, disable_prefetching) != 0);
}

bool get_tokudb_disable_slow_alter(THD* thd) {
    return (THDVAR(thd, disable_slow_alter) != 0);
}

bool get_tokudb_disable_slow_update(THD* thd) {
    return (THDVAR(thd, disable_slow_update) != 0);
}

bool get_tokudb_disable_slow_upsert(THD* thd) {
    return (THDVAR(thd, disable_slow_upsert) != 0);
}

bool get_tokudb_empty_scan(THD* thd) {
    return (THDVAR(thd, empty_scan) != 0);
}

uint get_tokudb_fanout(THD* thd) {
    return THDVAR(thd, fanout);
}

bool get_tokudb_hide_default_row_format(THD* thd) {
    return (THDVAR(thd, hide_default_row_format) != 0);
}

uint64_t get_tokudb_killed_time(THD* thd) {
    return THDVAR(thd, killed_time);
}

char* get_tokudb_last_lock_timeout(THD* thd) {
    return THDVAR(thd, last_lock_timeout);
}

void set_tokudb_last_lock_timeout(THD* thd, char* last) {
    THDVAR(thd, last_lock_timeout) = last;
}

bool get_tokudb_load_save_space(THD* thd) {
    return (THDVAR(thd, load_save_space) != 0);
}

uint64_t get_tokudb_loader_memory_size(THD* thd) {
    return THDVAR(thd, loader_memory_size);
}

uint64_t get_tokudb_lock_timeout(THD* thd) {
    return THDVAR(thd, lock_timeout);
}

uint get_tokudb_lock_timeout_debug(THD* thd) {
    return THDVAR(thd, lock_timeout_debug);
}

double get_tokudb_optimize_index_fraction(THD* thd) {
    return THDVAR(thd, optimize_index_fraction);
}

const char* get_tokudb_optimize_index_name(THD* thd) {
    return THDVAR(thd, optimize_index_name);
}

uint64_t get_tokudb_optimize_throttle(THD* thd) {
    return THDVAR(thd, optimize_throttle);
}

uint get_tokudb_pk_insert_mode(THD* thd) {
    return THDVAR(thd, pk_insert_mode);
}

bool get_tokudb_prelock_empty(THD* thd) {
    return (THDVAR(thd, prelock_empty) != 0);
}

uint get_tokudb_read_block_size(THD* thd) {
    return THDVAR(thd, read_block_size);
}

uint get_tokudb_read_buf_size(THD* thd) {
    return THDVAR(thd, read_buf_size);
}

srv_row_format_t get_tokudb_row_format(THD *thd) {
    return (srv_row_format_t) THDVAR(thd, row_format);
}

bool get_tokudb_rpl_check_readonly(THD* thd) {
    return (THDVAR(thd, rpl_check_readonly) != 0);
}

bool get_tokudb_rpl_lookup_rows(THD* thd) {
    return (THDVAR(thd, rpl_lookup_rows) != 0);
}

uint64_t get_tokudb_rpl_lookup_rows_delay(THD* thd) {
    return THDVAR(thd, rpl_lookup_rows_delay);
}

bool get_tokudb_rpl_unique_checks(THD* thd) {
    return (THDVAR(thd, rpl_unique_checks) != 0);
}

uint64_t get_tokudb_rpl_unique_checks_delay(THD* thd) {
    return THDVAR(thd, rpl_unique_checks_delay);
}

bool get_tokudb_support_xa(THD* thd) {
    return (THDVAR(thd, support_xa) != 0);
}
