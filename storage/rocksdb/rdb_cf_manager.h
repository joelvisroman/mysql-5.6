/*
   Copyright (c) 2014, SkySQL Ab

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#pragma once

/* C++ system header files */
#include <map>
#include <string>
#include <vector>

/* MySQL header files */
#include "./sql_class.h"

/* RocksDB header files */
#include "rocksdb/db.h"

/* MyRocks header files */
#include "./rdb_cf_options.h"

namespace myrocks {

void get_per_index_cf_name(const char *db_table_name, const char *index_name,
                           std::string *res);

/*
  We need a Column Family (CF) manager. Its functions:
  - create column families (synchronized, don't create the same twice)
  - keep count in each column family.
     = the count is kept on-disk.
     = there are no empty CFs. initially count=1.
     = then, when doing DDL, we increase or decrease it.
       (atomicity is maintained by being in the same WriteBatch with DDLs)
     = if DROP discovers that now count=0, it removes the CF.

  Current state is:
  - CFs are created in a synchronized way. We can't remove them, yet.
*/

class Rdb_cf_manager
{
  std::map<std::string, rocksdb::ColumnFamilyHandle*> m_cf_name_map;
  std::map<uint32_t, rocksdb::ColumnFamilyHandle*> m_cf_id_map;

  mutable mysql_mutex_t m_mutex;

  static
  void get_per_index_cf_name(const char *db_table_name, const char *index_name,
                             std::string *res);

  Rdb_cf_options* m_cf_options;

public:
  static bool is_cf_name_reverse(const char *name);

  /*
    This is called right after the DB::Open() call. The parameters describe column
    families that are present in the database. The first CF is the default CF.
  */
  void init(Rdb_cf_options* cf_options,
            std::vector<rocksdb::ColumnFamilyHandle*> *handles);
  void cleanup();

  /*
    Used by CREATE TABLE.
    - cf_name=nullptr means use default column family
    - cf_name=_auto_ means use 'dbname.tablename.indexname'
  */
  rocksdb::ColumnFamilyHandle* get_or_create_cf(rocksdb::DB *rdb,
                                                const char *cf_name,
                                                const char *db_table_name,
                                                const char *index_name,
                                                bool *is_automatic);

  /* Used by table open */
  rocksdb::ColumnFamilyHandle* get_cf(const char *cf_name,
                                      const char *db_table_name,
                                      const char *index_name,
                                      bool *is_automatic) const;

  /* Look up cf by id; used by datadic */
  rocksdb::ColumnFamilyHandle* get_cf(const uint32_t id) const;

  /* Used to iterate over column families for show status */
  std::vector<std::string> get_cf_names(void) const;

  /* Used to iterate over column families */
  std::vector<rocksdb::ColumnFamilyHandle*> get_all_cf(void) const;

  // void drop_cf(); -- not implemented so far.

  void get_cf_options(
    const std::string &cf_name,
    rocksdb::ColumnFamilyOptions *opts) __attribute__((__nonnull__)) {
      m_cf_options->get_cf_options(cf_name, opts);
  }
};

}  // namespace myrocks
