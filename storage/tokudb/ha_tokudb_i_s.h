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

#ifndef _HA_TOKUDB_I_S_H
#define _HA_TOKUDB_I_S_H


extern struct st_mysql_plugin i_s_trx;
extern struct st_mysql_plugin i_s_lock_waits;
extern struct st_mysql_plugin i_s_locks;
extern struct st_mysql_plugin i_s_file_map;
extern struct st_mysql_plugin i_s_fractal_tree_info;
extern struct st_mysql_plugin i_s_fractal_tree_block_map;

#endif // _HA_TOKUDB_I_S_H
