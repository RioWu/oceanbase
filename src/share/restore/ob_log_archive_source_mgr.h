/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#ifndef OCEANBASE_SHARE_OB_LOG_ARCHIVE_SOURCE_MGR_H_
#define OCEANBASE_SHARE_OB_LOG_ARCHIVE_SOURCE_MGR_H_
#include "lib/ob_define.h"
#include "lib/utility/ob_macro_utils.h"
#include "lib/utility/ob_print_utils.h"
#include "share/backup/ob_backup_struct.h"
#include "ob_restore_table_operator.h"
#include "ob_log_archive_source.h"

namespace oceanbase
{
namespace common
{
class ObString;
class ObAddr;
}

namespace share
{
typedef common::ObSEArray<ObBackupPathString, 1> DirArray;
// For standby and restore tenant, set log source with the log archive destination explicitly.
class ObLogArchiveSourceMgr final
{
public:
  ObLogArchiveSourceMgr() : is_inited_(false), tenant_id_(OB_INVALID_TENANT_ID), table_operator_() {}
public:
  int init(const uint64_t tenant_id, ObISQLClient *proxy);
public:
  // add source with net service
  int add_service_source(const SCN &recovery_until_scn, const ObAddr &addr);
  // add source with archive dest
  // 1. nfs example
  // file:///data/1/
  // 2. oss example
  // oss://backup_dir/?host=xxx.com&access_id=111&access_key=222
  // 3. cos example
  int add_location_source(const SCN &recovery_until_scn, const ObString &archive_dest);
  // add source with raw pieces
  int add_rawpath_source(const SCN &recovery_until_scn, const DirArray &array);

  // modify log archive source recovery until ts
  int update_recovery_until_ts(const SCN &recovery_until_scn);

  // delete all log archive source
  int delete_source();

  // get log archive source
  int get_source(ObLogArchiveSourceItem &item);

  static int get_backup_dest(const ObLogArchiveSourceItem &item, ObBackupDest& dest);
private:
  const int64_t OB_DEFAULT_LOG_ARCHIVE_SOURCE_ID = 1;
private:
  bool is_inited_;
  uint64_t tenant_id_;       // user tenant id
  ObTenantRestoreTableOperator table_operator_;
private:
  DISALLOW_COPY_AND_ASSIGN(ObLogArchiveSourceMgr);
};
} // namespace share
} // namespace oceanbase
#endif /* OCEANBASE_SHARE_OB_LOG_ARCHIVE_SOURCE_MGR_H_ */
