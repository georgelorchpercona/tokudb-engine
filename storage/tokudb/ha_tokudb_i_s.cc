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

#include "ha_tokudb_i_s.h"


static struct st_mysql_information_schema tokudb_trx_information_schema = {
    MYSQL_INFORMATION_SCHEMA_INTERFACE_VERSION
};

static ST_FIELD_INFO tokudb_trx_field_info[] = {
    {"trx_id", 0, MYSQL_TYPE_LONGLONG, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"trx_mysql_thread_id", 0, MYSQL_TYPE_LONGLONG, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"trx_time", 0, MYSQL_TYPE_LONGLONG, 0, 0, NULL, SKIP_OPEN_TABLE },
    {NULL, 0, MYSQL_TYPE_NULL, 0, 0, NULL, SKIP_OPEN_TABLE}
};

struct tokudb_trx_extra {
    THD *thd;
    TABLE *table;
};

static int tokudb_trx_callback(DB_TXN *txn,
                               iterate_row_locks_callback iterate_locks,
                               void *locks_extra, void *extra) {

    uint64_t txn_id = txn->id64(txn);
    uint64_t client_id = txn->get_client_id(txn);
    uint64_t start_time = txn->get_start_time(txn);
    struct tokudb_trx_extra *e = reinterpret_cast<struct tokudb_trx_extra *>(extra);
    THD *thd = e->thd;
    TABLE *table = e->table;
    table->field[0]->store(txn_id, false);
    table->field[1]->store(client_id, false);
    uint64_t tnow = (uint64_t) time(NULL);
    table->field[2]->store(tnow >= start_time ? tnow - start_time : 0, false);
    int error = schema_table_store_record(thd, table);
    if (!error && thd_killed(thd))
        error = ER_QUERY_INTERRUPTED;
    return error;
}

#if MYSQL_VERSION_ID >= 50600
static int tokudb_trx_fill_table(THD *thd, TABLE_LIST *tables, Item *cond) {
#else
static int tokudb_trx_fill_table(THD *thd, TABLE_LIST *tables, COND *cond) {
#endif
    TOKUDB_DBUG_ENTER("");
    int error;

    rw_rdlock(&tokudb_hton_initialized_lock);

    if (!tokudb_hton_initialized) {
        error = ER_PLUGIN_IS_NOT_LOADED;
        my_error(error, MYF(0), tokudb_hton_name);
    } else {
        struct tokudb_trx_extra e = { thd, tables->table };
        error = db_env->iterate_live_transactions(db_env, tokudb_trx_callback, &e);
        if (error)
            my_error(ER_GET_ERRNO, MYF(0), error, tokudb_hton_name);
    }

    rw_unlock(&tokudb_hton_initialized_lock);
    TOKUDB_DBUG_RETURN(error);
}

static int tokudb_trx_init(void *p) {
    ST_SCHEMA_TABLE *schema = (ST_SCHEMA_TABLE *) p;
    schema->fields_info = tokudb_trx_field_info;
    schema->fill_table = tokudb_trx_fill_table;
    return 0;
}

static int tokudb_trx_done(void *p) {
    return 0;
}



struct st_mysql_plugin i_s_trx = {
    MYSQL_INFORMATION_SCHEMA_PLUGIN,
    &tokudb_trx_information_schema,
    "TokuDB_trx",
    "Percona",
    "Percona TokuDB Storage Engine with Fractal Tree(tm) Technology",
    PLUGIN_LICENSE_GPL,
    tokudb_trx_init,     /* plugin init */
    tokudb_trx_done,     /* plugin deinit */
    TOKUDB_PLUGIN_VERSION,
    NULL,                      /* status variables */
    NULL,                      /* system variables */
#ifdef MARIA_PLUGIN_INTERFACE_VERSION
    tokudb_version,
    MariaDB_PLUGIN_MATURITY_STABLE /* maturity */
#else
    NULL,                      /* config options */
    0,                         /* flags */
#endif
};



static struct st_mysql_information_schema tokudb_lock_waits_information_schema = {
    MYSQL_INFORMATION_SCHEMA_INTERFACE_VERSION
};

static ST_FIELD_INFO tokudb_lock_waits_field_info[] = {
    {"requesting_trx_id", 0, MYSQL_TYPE_LONGLONG, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"blocking_trx_id", 0, MYSQL_TYPE_LONGLONG, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"lock_waits_dname", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"lock_waits_key_left", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"lock_waits_key_right", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"lock_waits_start_time", 0, MYSQL_TYPE_LONGLONG, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"lock_waits_table_schema", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"lock_waits_table_name", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"lock_waits_table_dictionary_name", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {NULL, 0, MYSQL_TYPE_NULL, 0, 0, NULL, SKIP_OPEN_TABLE}
};

struct tokudb_lock_waits_extra {
    THD *thd;
    TABLE *table;
};

static int tokudb_lock_waits_callback(DB *db, uint64_t requesting_txnid,
                                      const DBT *left_key,
                                      const DBT *right_key,
                                      uint64_t blocking_txnid,
                                      uint64_t start_time, void *extra) {

    struct tokudb_lock_waits_extra *e = reinterpret_cast<struct tokudb_lock_waits_extra *>(extra);
    THD *thd = e->thd;
    TABLE *table = e->table;
    table->field[0]->store(requesting_txnid, false);
    table->field[1]->store(blocking_txnid, false);
    const char *dname = tokudb_get_index_name(db);
    size_t dname_length = strlen(dname);
    table->field[2]->store(dname, dname_length, system_charset_info);
    String left_str;
    tokudb_pretty_left_key(db, left_key, &left_str);
    table->field[3]->store(left_str.ptr(), left_str.length(), system_charset_info);
    String right_str;
    tokudb_pretty_right_key(db, right_key, &right_str);
    table->field[4]->store(right_str.ptr(), right_str.length(), system_charset_info);
    table->field[5]->store(start_time, false);

    String database_name, table_name, dictionary_name;
    tokudb_split_dname(dname, database_name, table_name, dictionary_name);
    table->field[6]->store(database_name.c_ptr(), database_name.length(), system_charset_info);
    table->field[7]->store(table_name.c_ptr(), table_name.length(), system_charset_info);
    table->field[8]->store(dictionary_name.c_ptr(), dictionary_name.length(), system_charset_info);

    int error = schema_table_store_record(thd, table);

    if (!error && thd_killed(thd))
        error = ER_QUERY_INTERRUPTED;

    return error;
}

#if MYSQL_VERSION_ID >= 50600
static int tokudb_lock_waits_fill_table(THD *thd, TABLE_LIST *tables, Item *cond) {
#else
static int tokudb_lock_waits_fill_table(THD *thd, TABLE_LIST *tables, COND *cond) {
#endif
    TOKUDB_DBUG_ENTER("");
    int error;

    rw_rdlock(&tokudb_hton_initialized_lock);

    if (!tokudb_hton_initialized) {
        error = ER_PLUGIN_IS_NOT_LOADED;
        my_error(error, MYF(0), tokudb_hton_name);
    } else {
        struct tokudb_lock_waits_extra e = { thd, tables->table };
        error = db_env->iterate_pending_lock_requests(db_env, tokudb_lock_waits_callback, &e);
        if (error)
            my_error(ER_GET_ERRNO, MYF(0), error, tokudb_hton_name);
    }

    rw_unlock(&tokudb_hton_initialized_lock);
    TOKUDB_DBUG_RETURN(error);
}

static int tokudb_lock_waits_init(void *p) {
    ST_SCHEMA_TABLE *schema = (ST_SCHEMA_TABLE *) p;
    schema->fields_info = tokudb_lock_waits_field_info;
    schema->fill_table = tokudb_lock_waits_fill_table;
    return 0;
}

static int tokudb_lock_waits_done(void *p) {
    return 0;
}

struct st_mysql_plugin i_s_lock_waits = {
    MYSQL_INFORMATION_SCHEMA_PLUGIN,
    &tokudb_lock_waits_information_schema,
    "TokuDB_lock_waits",
    "Percona",
    "Percona TokuDB Storage Engine with Fractal Tree(tm) Technology",
    PLUGIN_LICENSE_GPL,
    tokudb_lock_waits_init,     /* plugin init */
    tokudb_lock_waits_done,     /* plugin deinit */
    TOKUDB_PLUGIN_VERSION,
    NULL,                      /* status variables */
    NULL,                      /* system variables */
#ifdef MARIA_PLUGIN_INTERFACE_VERSION
    tokudb_version,
    MariaDB_PLUGIN_MATURITY_STABLE /* maturity */
#else
    NULL,                      /* config options */
    0,                         /* flags */
#endif
};



static struct st_mysql_information_schema tokudb_locks_information_schema = {
    MYSQL_INFORMATION_SCHEMA_INTERFACE_VERSION
};

static ST_FIELD_INFO tokudb_locks_field_info[] = {
    {"locks_trx_id", 0, MYSQL_TYPE_LONGLONG, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"locks_mysql_thread_id", 0, MYSQL_TYPE_LONGLONG, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"locks_dname", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"locks_key_left", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"locks_key_right", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"locks_table_schema", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"locks_table_name", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"locks_table_dictionary_name", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {NULL, 0, MYSQL_TYPE_NULL, 0, 0, NULL, SKIP_OPEN_TABLE}
};

struct tokudb_locks_extra {
    THD *thd;
    TABLE *table;
};

static int tokudb_locks_callback(DB_TXN *txn,
                                 iterate_row_locks_callback iterate_locks,
                                 void *locks_extra, void *extra) {
    uint64_t txn_id = txn->id64(txn);
    uint64_t client_id = txn->get_client_id(txn);
    struct tokudb_locks_extra *e = reinterpret_cast<struct tokudb_locks_extra *>(extra);
    THD *thd = e->thd;
    TABLE *table = e->table;
    int error = 0;
    DB *db;
    DBT left_key, right_key;
    while (error == 0 && iterate_locks(&db, &left_key, &right_key, locks_extra) == 0) {
        table->field[0]->store(txn_id, false);
        table->field[1]->store(client_id, false);

        const char *dname = tokudb_get_index_name(db);
        size_t dname_length = strlen(dname);
        table->field[2]->store(dname, dname_length, system_charset_info);

        String left_str;
        tokudb_pretty_left_key(db, &left_key, &left_str);
        table->field[3]->store(left_str.ptr(), left_str.length(), system_charset_info);

        String right_str;
        tokudb_pretty_right_key(db, &right_key, &right_str);
        table->field[4]->store(right_str.ptr(), right_str.length(), system_charset_info);

        String database_name, table_name, dictionary_name;
        tokudb_split_dname(dname, database_name, table_name, dictionary_name);
        table->field[5]->store(database_name.c_ptr(), database_name.length(), system_charset_info);
        table->field[6]->store(table_name.c_ptr(), table_name.length(), system_charset_info);
        table->field[7]->store(dictionary_name.c_ptr(), dictionary_name.length(), system_charset_info);

        error = schema_table_store_record(thd, table);

        if (!error && thd_killed(thd))
            error = ER_QUERY_INTERRUPTED;
    }
    return error;
}

#if MYSQL_VERSION_ID >= 50600
static int tokudb_locks_fill_table(THD *thd, TABLE_LIST *tables, Item *cond) {
#else
static int tokudb_locks_fill_table(THD *thd, TABLE_LIST *tables, COND *cond) {
#endif
    TOKUDB_DBUG_ENTER("");
    int error;

    rw_rdlock(&tokudb_hton_initialized_lock);

    if (!tokudb_hton_initialized) {
        error = ER_PLUGIN_IS_NOT_LOADED;
        my_error(error, MYF(0), tokudb_hton_name);
    } else {
        struct tokudb_locks_extra e = { thd, tables->table };
        error = db_env->iterate_live_transactions(db_env, tokudb_locks_callback, &e);
        if (error)
            my_error(ER_GET_ERRNO, MYF(0), error, tokudb_hton_name);
    }

    rw_unlock(&tokudb_hton_initialized_lock);
    TOKUDB_DBUG_RETURN(error);
}

static int tokudb_locks_init(void *p) {
    ST_SCHEMA_TABLE *schema = (ST_SCHEMA_TABLE *) p;
    schema->fields_info = tokudb_locks_field_info;
    schema->fill_table = tokudb_locks_fill_table;
    return 0;
}

static int tokudb_locks_done(void *p) {
    return 0;
}
struct st_mysql_plugin i_s_locks = {
    MYSQL_INFORMATION_SCHEMA_PLUGIN,
    &tokudb_locks_information_schema,
    "TokuDB_locks",
    "Percona",
    "Percona TokuDB Storage Engine with Fractal Tree(tm) Technology",
    PLUGIN_LICENSE_GPL,
    tokudb_locks_init,     /* plugin init */
    tokudb_locks_done,     /* plugin deinit */
    TOKUDB_PLUGIN_VERSION,
    NULL,                      /* status variables */
    NULL,                      /* system variables */
#ifdef MARIA_PLUGIN_INTERFACE_VERSION
    tokudb_version,
    MariaDB_PLUGIN_MATURITY_STABLE /* maturity */
#else
    NULL,                      /* config options */
    0,                         /* flags */
#endif
};



struct st_mysql_information_schema tokudb_file_map_information_schema = {
    MYSQL_INFORMATION_SCHEMA_INTERFACE_VERSION
};

static ST_FIELD_INFO tokudb_file_map_field_info[] = {
    {"dictionary_name", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"internal_file_name", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"table_schema", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"table_name", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"table_dictionary_name", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {NULL, 0, MYSQL_TYPE_NULL, 0, 0, NULL, SKIP_OPEN_TABLE}
};

static int tokudb_file_map(TABLE *table, THD *thd) {
    int error;
    DB_TXN* txn = NULL;
    DBC* tmp_cursor = NULL;
    DBT curr_key;
    DBT curr_val;
    memset(&curr_key, 0, sizeof curr_key);
    memset(&curr_val, 0, sizeof curr_val);
    error = txn_begin(db_env, 0, &txn, DB_READ_UNCOMMITTED, thd);
    if (error) {
        goto cleanup;
    }
    error = db_env->get_cursor_for_directory(db_env, txn, &tmp_cursor);
    if (error) {
        goto cleanup;
    }
    while (error == 0) {
        error = tmp_cursor->c_get(tmp_cursor, &curr_key, &curr_val, DB_NEXT);
        if (!error) {
            // We store the NULL terminator in the directory so it's included in the size.
            // See #5789
            // Recalculate and check just to be safe.
            const char *dname = (const char *) curr_key.data;
            size_t dname_len = strlen(dname);
            assert(dname_len == curr_key.size - 1);
            table->field[0]->store(dname, dname_len, system_charset_info);

            const char *iname = (const char *) curr_val.data;
            size_t iname_len = strlen(iname);
            assert(iname_len == curr_val.size - 1);
            table->field[1]->store(iname, iname_len, system_charset_info);

            // split the dname
            String database_name, table_name, dictionary_name;
            tokudb_split_dname(dname, database_name, table_name, dictionary_name);
            table->field[2]->store(database_name.c_ptr(), database_name.length(), system_charset_info);
            table->field[3]->store(table_name.c_ptr(), table_name.length(), system_charset_info);
            table->field[4]->store(dictionary_name.c_ptr(), dictionary_name.length(), system_charset_info);

            error = schema_table_store_record(thd, table);
        }
        if (!error && thd_killed(thd))
            error = ER_QUERY_INTERRUPTED;
    }
    if (error == DB_NOTFOUND) {
        error = 0;
    }
cleanup:
    if (tmp_cursor) {
        int r = tmp_cursor->c_close(tmp_cursor);
        assert(r == 0);
    }
    if (txn) {
        commit_txn(txn, 0);
    }
    return error;
}

#if MYSQL_VERSION_ID >= 50600
static int tokudb_file_map_fill_table(THD *thd, TABLE_LIST *tables, Item *cond) {
#else
static int tokudb_file_map_fill_table(THD *thd, TABLE_LIST *tables, COND *cond) {
#endif
    TOKUDB_DBUG_ENTER("");
    int error;
    TABLE *table = tables->table;

    rw_rdlock(&tokudb_hton_initialized_lock);

    if (!tokudb_hton_initialized) {
        error = ER_PLUGIN_IS_NOT_LOADED;
        my_error(error, MYF(0), tokudb_hton_name);
    } else {
        error = tokudb_file_map(table, thd);
        if (error)
            my_error(ER_GET_ERRNO, MYF(0), error, tokudb_hton_name);
    }

    rw_unlock(&tokudb_hton_initialized_lock);
    TOKUDB_DBUG_RETURN(error);
}

static int tokudb_file_map_init(void *p) {
    ST_SCHEMA_TABLE *schema = (ST_SCHEMA_TABLE *) p;
    schema->fields_info = tokudb_file_map_field_info;
    schema->fill_table = tokudb_file_map_fill_table;
    return 0;
}

static int tokudb_file_map_done(void *p) {
    return 0;
}

struct st_mysql_plugin i_s_file_map = {
    MYSQL_INFORMATION_SCHEMA_PLUGIN,
    &tokudb_file_map_information_schema,
    "TokuDB_file_map",
    "Percona",
    "Percona TokuDB Storage Engine with Fractal Tree(tm) Technology",
    PLUGIN_LICENSE_GPL,
    tokudb_file_map_init,     /* plugin init */
    tokudb_file_map_done,     /* plugin deinit */
    TOKUDB_PLUGIN_VERSION,
    NULL,                      /* status variables */
    NULL,                      /* system variables */
#ifdef MARIA_PLUGIN_INTERFACE_VERSION
    tokudb_version,
    MariaDB_PLUGIN_MATURITY_STABLE /* maturity */
#else
    NULL,                      /* config options */
    0,                         /* flags */
#endif
};



static struct st_mysql_information_schema tokudb_fractal_tree_info_information_schema = {
    MYSQL_INFORMATION_SCHEMA_INTERFACE_VERSION
};

static ST_FIELD_INFO tokudb_fractal_tree_info_field_info[] = {
    {"dictionary_name", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"internal_file_name", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"bt_num_blocks_allocated", 0, MYSQL_TYPE_LONGLONG, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"bt_num_blocks_in_use", 0, MYSQL_TYPE_LONGLONG, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"bt_size_allocated", 0, MYSQL_TYPE_LONGLONG, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"bt_size_in_use", 0, MYSQL_TYPE_LONGLONG, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"table_schema", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"table_name", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"table_dictionary_name", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {NULL, 0, MYSQL_TYPE_NULL, 0, 0, NULL, SKIP_OPEN_TABLE}
};

static int tokudb_report_fractal_tree_info_for_db(const DBT *dname,
                                                  const DBT *iname,
                                                  TABLE *table, THD *thd) {
    int error;
    uint64_t bt_num_blocks_allocated;
    uint64_t bt_num_blocks_in_use;
    uint64_t bt_size_allocated;
    uint64_t bt_size_in_use;

    DB *db = NULL;
    error = db_create(&db, db_env, 0);
    if (error) {
        goto exit;
    }
    error = db->open(db, NULL, (char *)dname->data, NULL, DB_BTREE, 0, 0666);
    if (error) {
        goto exit;
    }
    error = db->get_fractal_tree_info64(db,
                                        &bt_num_blocks_allocated, &bt_num_blocks_in_use,
                                        &bt_size_allocated, &bt_size_in_use);
    if (error) {
        goto exit;
    }

    // We store the NULL terminator in the directory so it's included in the size.
    // See #5789
    // Recalculate and check just to be safe.
    {
        size_t dname_len = strlen((const char *)dname->data);
        assert(dname_len == dname->size - 1);
        table->field[0]->store((char *)dname->data, dname_len, system_charset_info);
        size_t iname_len = strlen((const char *)iname->data);
        assert(iname_len == iname->size - 1);
        table->field[1]->store((char *)iname->data, iname_len, system_charset_info);
    }
    table->field[2]->store(bt_num_blocks_allocated, false);
    table->field[3]->store(bt_num_blocks_in_use, false);
    table->field[4]->store(bt_size_allocated, false);
    table->field[5]->store(bt_size_in_use, false);

    // split the dname
    {
        String database_name, table_name, dictionary_name;
        tokudb_split_dname((const char *)dname->data, database_name, table_name, dictionary_name);
        table->field[6]->store(database_name.c_ptr(), database_name.length(), system_charset_info);
        table->field[7]->store(table_name.c_ptr(), table_name.length(), system_charset_info);
        table->field[8]->store(dictionary_name.c_ptr(), dictionary_name.length(), system_charset_info);
    }
    error = schema_table_store_record(thd, table);

exit:
    if (db) {
        int close_error = db->close(db, 0);
        if (error == 0)
            error = close_error;
    }
    return error;
}

static int tokudb_fractal_tree_info(TABLE *table, THD *thd) {
    int error;
    DB_TXN* txn = NULL;
    DBC* tmp_cursor = NULL;
    DBT curr_key;
    DBT curr_val;
    memset(&curr_key, 0, sizeof curr_key);
    memset(&curr_val, 0, sizeof curr_val);
    error = txn_begin(db_env, 0, &txn, DB_READ_UNCOMMITTED, thd);
    if (error) {
        goto cleanup;
    }
    error = db_env->get_cursor_for_directory(db_env, txn, &tmp_cursor);
    if (error) {
        goto cleanup;
    }
    while (error == 0) {
        error = tmp_cursor->c_get(tmp_cursor, &curr_key, &curr_val, DB_NEXT);
        if (!error) {
            error = tokudb_report_fractal_tree_info_for_db(&curr_key, &curr_val, table, thd);
            if (error)
                error = 0; // ignore read uncommitted errors
        }
        if (!error && thd_killed(thd))
            error = ER_QUERY_INTERRUPTED;
    }
    if (error == DB_NOTFOUND) {
        error = 0;
    }
cleanup:
    if (tmp_cursor) {
        int r = tmp_cursor->c_close(tmp_cursor);
        assert(r == 0);
    }
    if (txn) {
        commit_txn(txn, 0);
    }
    return error;
}

#if MYSQL_VERSION_ID >= 50600
static int tokudb_fractal_tree_info_fill_table(THD *thd, TABLE_LIST *tables, Item *cond) {
#else
static int tokudb_fractal_tree_info_fill_table(THD *thd, TABLE_LIST *tables, COND *cond) {
#endif
    TOKUDB_DBUG_ENTER("");
    int error;
    TABLE *table = tables->table;

    // 3938: Get a read lock on the status flag, since we must
    // read it before safely proceeding
    rw_rdlock(&tokudb_hton_initialized_lock);

    if (!tokudb_hton_initialized) {
        error = ER_PLUGIN_IS_NOT_LOADED;
        my_error(error, MYF(0), tokudb_hton_name);
    } else {
        error = tokudb_fractal_tree_info(table, thd);
        if (error)
            my_error(ER_GET_ERRNO, MYF(0), error, tokudb_hton_name);
    }

    //3938: unlock the status flag lock
    rw_unlock(&tokudb_hton_initialized_lock);
    TOKUDB_DBUG_RETURN(error);
}

static int tokudb_fractal_tree_info_init(void *p) {
    ST_SCHEMA_TABLE *schema = (ST_SCHEMA_TABLE *) p;
    schema->fields_info = tokudb_fractal_tree_info_field_info;
    schema->fill_table = tokudb_fractal_tree_info_fill_table;
    return 0;
}

static int tokudb_fractal_tree_info_done(void *p) {
    return 0;
}

struct st_mysql_plugin i_s_fractal_tree_info = {
    MYSQL_INFORMATION_SCHEMA_PLUGIN,
    &tokudb_fractal_tree_info_information_schema,
    "TokuDB_fractal_tree_info",
    "Percona",
    "Percona TokuDB Storage Engine with Fractal Tree(tm) Technology",
    PLUGIN_LICENSE_GPL,
    tokudb_fractal_tree_info_init,     /* plugin init */
    tokudb_fractal_tree_info_done,     /* plugin deinit */
    TOKUDB_PLUGIN_VERSION,
    NULL,                      /* status variables */
    NULL,                      /* system variables */
#ifdef MARIA_PLUGIN_INTERFACE_VERSION
    tokudb_version,
    MariaDB_PLUGIN_MATURITY_STABLE /* maturity */
#else
    NULL,                      /* config options */
    0,                         /* flags */
#endif
};



static struct st_mysql_information_schema tokudb_fractal_tree_block_map_information_schema = {
    MYSQL_INFORMATION_SCHEMA_INTERFACE_VERSION
};

static ST_FIELD_INFO tokudb_fractal_tree_block_map_field_info[] = {
    {"dictionary_name", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"internal_file_name", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"checkpoint_count", 0, MYSQL_TYPE_LONGLONG, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"blocknum", 0, MYSQL_TYPE_LONGLONG, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"offset", 0, MYSQL_TYPE_LONGLONG, 0, MY_I_S_MAYBE_NULL, NULL, SKIP_OPEN_TABLE },
    {"size", 0, MYSQL_TYPE_LONGLONG, 0, MY_I_S_MAYBE_NULL, NULL, SKIP_OPEN_TABLE },
    {"table_schema", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"table_name", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {"table_dictionary_name", 256, MYSQL_TYPE_STRING, 0, 0, NULL, SKIP_OPEN_TABLE },
    {NULL, 0, MYSQL_TYPE_NULL, 0, 0, NULL, SKIP_OPEN_TABLE}
};

struct tokudb_report_fractal_tree_block_map_iterator_extra {
    int64_t num_rows;
    int64_t i;
    uint64_t *checkpoint_counts;
    int64_t *blocknums;
    int64_t *diskoffs;
    int64_t *sizes;
};

// This iterator is called while holding the blocktable lock.  We should be as quick as possible.
// We don't want to do one call to get the number of rows, release the blocktable lock, and then do another call to get all the rows because the number of rows may change if we don't hold the lock.
// As a compromise, we'll do some mallocs inside the lock on the first call, but everything else should be fast.
static int tokudb_report_fractal_tree_block_map_iterator(uint64_t checkpoint_count,
                                                         int64_t num_rows,
                                                         int64_t blocknum,
                                                         int64_t diskoff,
                                                         int64_t size,
                                                         void *iter_extra) {
    struct tokudb_report_fractal_tree_block_map_iterator_extra *e =
        static_cast<struct tokudb_report_fractal_tree_block_map_iterator_extra *>(iter_extra);

    assert(num_rows > 0);
    if (e->num_rows == 0) {
        e->checkpoint_counts = (uint64_t *) tokudb_my_malloc(num_rows * (sizeof *e->checkpoint_counts),
                                                             MYF(MY_WME|MY_ZEROFILL|MY_FAE));
        e->blocknums = (int64_t *) tokudb_my_malloc(num_rows * (sizeof *e->blocknums),
                                                    MYF(MY_WME|MY_ZEROFILL|MY_FAE));
        e->diskoffs = (int64_t *) tokudb_my_malloc(num_rows * (sizeof *e->diskoffs),
                                                   MYF(MY_WME|MY_ZEROFILL|MY_FAE));
        e->sizes = (int64_t *) tokudb_my_malloc(num_rows * (sizeof *e->sizes),
                                                MYF(MY_WME|MY_ZEROFILL|MY_FAE));
        e->num_rows = num_rows;
    }

    e->checkpoint_counts[e->i] = checkpoint_count;
    e->blocknums[e->i] = blocknum;
    e->diskoffs[e->i] = diskoff;
    e->sizes[e->i] = size;
    ++(e->i);

    return 0;
}

static int tokudb_report_fractal_tree_block_map_for_db(const DBT *dname,
                                                       const DBT *iname,
                                                       TABLE *table, THD *thd) {
    int error;
    DB *db;
    // avoid struct initializers so that we can compile with older gcc versions
    struct tokudb_report_fractal_tree_block_map_iterator_extra e = {};

    error = db_create(&db, db_env, 0);
    if (error) {
        goto exit;
    }
    error = db->open(db, NULL, (char *)dname->data, NULL, DB_BTREE, 0, 0666);
    if (error) {
        goto exit;
    }
    error = db->iterate_fractal_tree_block_map(db, tokudb_report_fractal_tree_block_map_iterator, &e);
    {
        int close_error = db->close(db, 0);
        if (!error) {
            error = close_error;
        }
    }
    if (error) {
        goto exit;
    }

    // If not, we should have gotten an error and skipped this section of code
    assert(e.i == e.num_rows);
    for (int64_t i = 0; error == 0 && i < e.num_rows; ++i) {
        // We store the NULL terminator in the directory so it's included in the size.
        // See #5789
        // Recalculate and check just to be safe.
        size_t dname_len = strlen((const char *)dname->data);
        assert(dname_len == dname->size - 1);
        table->field[0]->store((char *)dname->data, dname_len, system_charset_info);

        size_t iname_len = strlen((const char *)iname->data);
        assert(iname_len == iname->size - 1);
        table->field[1]->store((char *)iname->data, iname_len, system_charset_info);

        table->field[2]->store(e.checkpoint_counts[i], false);
        table->field[3]->store(e.blocknums[i], false);
        static const int64_t freelist_null = -1;
        static const int64_t diskoff_unused = -2;
        if (e.diskoffs[i] == diskoff_unused || e.diskoffs[i] == freelist_null) {
            table->field[4]->set_null();
        } else {
            table->field[4]->set_notnull();
            table->field[4]->store(e.diskoffs[i], false);
        }
        static const int64_t size_is_free = -1;
        if (e.sizes[i] == size_is_free) {
            table->field[5]->set_null();
        } else {
            table->field[5]->set_notnull();
            table->field[5]->store(e.sizes[i], false);
        }

        // split the dname
        String database_name, table_name, dictionary_name;
        tokudb_split_dname((const char *)dname->data, database_name, table_name,dictionary_name);
        table->field[6]->store(database_name.c_ptr(), database_name.length(), system_charset_info);
        table->field[7]->store(table_name.c_ptr(), table_name.length(), system_charset_info);
        table->field[8]->store(dictionary_name.c_ptr(), dictionary_name.length(), system_charset_info);

        error = schema_table_store_record(thd, table);
    }

exit:
    if (e.checkpoint_counts != NULL) {
        tokudb_my_free(e.checkpoint_counts);
        e.checkpoint_counts = NULL;
    }
    if (e.blocknums != NULL) {
        tokudb_my_free(e.blocknums);
        e.blocknums = NULL;
    }
    if (e.diskoffs != NULL) {
        tokudb_my_free(e.diskoffs);
        e.diskoffs = NULL;
    }
    if (e.sizes != NULL) {
        tokudb_my_free(e.sizes);
        e.sizes = NULL;
    }
    return error;
}

static int tokudb_fractal_tree_block_map(TABLE *table, THD *thd) {
    int error;
    DB_TXN* txn = NULL;
    DBC* tmp_cursor = NULL;
    DBT curr_key;
    DBT curr_val;
    memset(&curr_key, 0, sizeof curr_key);
    memset(&curr_val, 0, sizeof curr_val);
    error = txn_begin(db_env, 0, &txn, DB_READ_UNCOMMITTED, thd);
    if (error) {
        goto cleanup;
    }
    error = db_env->get_cursor_for_directory(db_env, txn, &tmp_cursor);
    if (error) {
        goto cleanup;
    }
    while (error == 0) {
        error = tmp_cursor->c_get(tmp_cursor, &curr_key, &curr_val, DB_NEXT);
        if (!error) {
            error = tokudb_report_fractal_tree_block_map_for_db(&curr_key, &curr_val, table, thd);
        }
        if (!error && thd_killed(thd))
            error = ER_QUERY_INTERRUPTED;
    }
    if (error == DB_NOTFOUND) {
        error = 0;
    }
cleanup:
    if (tmp_cursor) {
        int r = tmp_cursor->c_close(tmp_cursor);
        assert(r == 0);
    }
    if (txn) {
        commit_txn(txn, 0);
    }
    return error;
}

#if MYSQL_VERSION_ID >= 50600
static int tokudb_fractal_tree_block_map_fill_table(THD *thd, TABLE_LIST *tables, Item *cond) {
#else
static int tokudb_fractal_tree_block_map_fill_table(THD *thd, TABLE_LIST *tables, COND *cond) {
#endif
    TOKUDB_DBUG_ENTER("");
    int error;
    TABLE *table = tables->table;

    // 3938: Get a read lock on the status flag, since we must
    // read it before safely proceeding
    rw_rdlock(&tokudb_hton_initialized_lock);

    if (!tokudb_hton_initialized) {
        error = ER_PLUGIN_IS_NOT_LOADED;
        my_error(error, MYF(0), tokudb_hton_name);
    } else {
        error = tokudb_fractal_tree_block_map(table, thd);
        if (error)
            my_error(ER_GET_ERRNO, MYF(0), error, tokudb_hton_name);
    }

    //3938: unlock the status flag lock
    rw_unlock(&tokudb_hton_initialized_lock);
    TOKUDB_DBUG_RETURN(error);
}

static int tokudb_fractal_tree_block_map_init(void *p) {
    ST_SCHEMA_TABLE *schema = (ST_SCHEMA_TABLE *) p;
    schema->fields_info = tokudb_fractal_tree_block_map_field_info;
    schema->fill_table = tokudb_fractal_tree_block_map_fill_table;
    return 0;
}

static int tokudb_fractal_tree_block_map_done(void *p) {
    return 0;
}

struct st_mysql_plugin i_s_fractal_tree_block_map = {
    MYSQL_INFORMATION_SCHEMA_PLUGIN,
    &tokudb_fractal_tree_block_map_information_schema,
    "TokuDB_fractal_tree_block_map",
    "Percona",
    "Percona TokuDB Storage Engine with Fractal Tree(tm) Technology",
    PLUGIN_LICENSE_GPL,
    tokudb_fractal_tree_block_map_init,     /* plugin init */
    tokudb_fractal_tree_block_map_done,     /* plugin deinit */
    TOKUDB_PLUGIN_VERSION,
    NULL,                      /* status variables */
    NULL,                      /* system variables */
#ifdef MARIA_PLUGIN_INTERFACE_VERSION
    tokudb_version,
    MariaDB_PLUGIN_MATURITY_STABLE /* maturity */
#else
    NULL,                      /* config options */
    0,                         /* flags */
#endif
};
