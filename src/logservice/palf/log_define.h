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

#ifndef OCEANBASE_LOGSERVICE_LOG_DEFINE_
#define OCEANBASE_LOGSERVICE_LOG_DEFINE_
#include <cstdint>                                       // UINT64_MAX
#include <string.h>                                      // strncmp...
#include <dirent.h>                                      // dirent
#include "lib/ob_errno.h"                                // errno
#include "lib/utility/ob_print_utils.h"                  // databuff_printf
#include "lib/container/ob_fixed_array.h"                // ObFixedArray
#include "share/ob_force_print_log.h"                    // force_print

namespace oceanbase
{
namespace common
{
class ObIAllocator;
}

namespace share
{
class SCN;
}
namespace palf
{
#define TMP_SUFFIX ".tmp"

#define PALF_EVENT(info_string, palf_id, args...) FLOG_INFO("[PALF_EVENT] "info_string, "palf_id", palf_id, args)

typedef int FileDesc;
typedef uint64_t block_id_t ;
typedef uint64_t offset_t;
constexpr int64_t INVALID_PALF_ID = -1;
class LSN;
class LogWriteBuf;

// ==================== palf env start =============================
const int64_t MIN_DISK_SIZE_PER_PALF_INSTANCE = 512 * 1024 * 1024ul;
// =====================palf env end ===============================

// ==================== block and log start ========================
constexpr offset_t MAX_LOG_HEADER_SIZE = 4 * 1024;
constexpr offset_t MAX_INFO_BLOCK_SIZE = 4 * 1024;
constexpr offset_t MAX_META_ENTRY_SIZE = 4 * 1024;
constexpr offset_t MAX_LOG_BODY_SIZE = 2 * 1024 * 1024 + 16 * 1024;                 // The max size of one log body is (2MB + 16KB).

const int64_t PALF_PHY_BLOCK_SIZE = 1 << 26;                                        // 64MB
const int64_t PALF_BLOCK_SIZE = PALF_PHY_BLOCK_SIZE - MAX_INFO_BLOCK_SIZE;          // log block size is 64M-MAX_INFO_BLOCK_SIZE by default.
const int64_t PALF_META_BLOCK_SIZE = PALF_PHY_BLOCK_SIZE - MAX_INFO_BLOCK_SIZE;     // meta block size is 64M-MAX_INFO_BLOCK_SIZE by default.

constexpr offset_t MAX_LOG_BUFFER_SIZE = MAX_LOG_BODY_SIZE + MAX_LOG_HEADER_SIZE;

constexpr offset_t LOG_DIO_ALIGN_SIZE = 4 * 1024;
constexpr offset_t LOG_DIO_ALIGNED_BUF_SIZE = MAX_LOG_BUFFER_SIZE + LOG_DIO_ALIGN_SIZE;
constexpr block_id_t LOG_MAX_BLOCK_ID = UINT64_MAX/PALF_BLOCK_SIZE - 1;
constexpr block_id_t LOG_INVALID_BLOCK_ID = LOG_MAX_BLOCK_ID + 1;
typedef common::ObFixedArray<share::SCN, ObIAllocator> SCNArray;
typedef common::ObFixedArray<LSN, ObIAllocator> LSNArray;
typedef common::ObFixedArray<LogWriteBuf *, ObIAllocator> LogWriteBufArray;
// ==================== block and log end ===========================

// ====================== Consensus begin ===========================
const int64_t LEADER_DEFAULT_GROUP_BUFFER_SIZE = 1 << 25;                           // leader's group buffer size is 32M
const int64_t MAX_ALLOWED_SKEW_FOR_REF_US = 3600L * 1000 * 1000;          // 1h
// follower's group buffer size is 8MB larger than leader's.
const int64_t FOLLOWER_DEFAULT_GROUP_BUFFER_SIZE = LEADER_DEFAULT_GROUP_BUFFER_SIZE + 8 * 1024 * 1024L;
const int64_t PALF_RECONFIRM_FETCH_MAX_LSN_INTERVAL = 1 * 1000 * 1000;
const int64_t PALF_FETCH_LOG_INTERVAL_US = 2 * 1000 * 1000L;                 // 2s
const int64_t PALF_FETCH_LOG_RENEW_LEADER_INTERVAL_US = 5 * 1000 * 1000;            // 5s
const int64_t PALF_LEADER_RECONFIRM_SYNC_TIMEOUT_US = 10 * 1000 * 1000L;     // 10s
const int64_t PREPARE_LOG_BUFFER_SIZE = 2048;
const int64_t PALF_LEADER_ACTIVE_SYNC_TIMEOUT_US = 10 * 1000 * 1000L;        // 10s
const int32_t PALF_MAX_REPLAY_TIMEOUT = 500 * 1000;
const int32_t PALF_LOG_LOOP_INTERVAL_US = 1 * 1000;                                 // 1ms
const int64_t PALF_SLIDING_WINDOW_SIZE = 1 << 11;                                   // must be 2^n(n>0), default 2^11 = 2048
const int64_t PALF_MAX_LEADER_SUBMIT_LOG_COUNT = PALF_SLIDING_WINDOW_SIZE / 2;      // max number of concurrent submitting group log in leader
const int64_t PALF_RESEND_MSLOG_INTERVAL_US = 500 * 1000L;                   // 500 ms
const int64_t PALF_BROADCAST_LEADER_INFO_INTERVAL_US = 5 * 1000 * 1000L;     // 5s
const int64_t FIRST_VALID_LOG_ID = 1;  // The first valid log_id is 1.
const int64_t PALF_PARENT_CHILD_TIMEOUT_US = 4 * 1000 * 1000L;               // 4000ms, 4s
const int64_t PALF_PARENT_KEEPALIVE_INTERVAL_US = 1 * 1000 * 1000L;          // 1000ms, 1s
const int64_t PALF_CHILD_RESEND_REGISTER_INTERVAL_US = 4 * 1000 * 1000L;     // 4000ms
const int64_t PALF_CHECK_PARENT_CHILD_INTERVAL_US = 1 * 1000 * 1000;                // 1000ms
const int64_t PALF_DUMP_DEBUG_INFO_INTERVAL_US = 10 * 1000 * 1000;                  // 10s
constexpr int64_t INVALID_PROPOSAL_ID = INT64_MAX;

inline int64_t max_proposal_id(const int64_t a, const int64_t b)
{
  if ((INVALID_PROPOSAL_ID == a && INVALID_PROPOSAL_ID == b) ||
      (INVALID_PROPOSAL_ID != a && INVALID_PROPOSAL_ID == b)) {
    return a;
  } else if (INVALID_PROPOSAL_ID == a && INVALID_PROPOSAL_ID != b) {
    return b;
  } else {
    return MAX(a, b);
  }
}
// ====================== Consensus end ==============================

// =========== LSN begin ==============
const uint64_t LOG_INVALID_LSN_VAL = UINT64_MAX;
const uint64_t LOG_MAX_LSN_VAL = LOG_INVALID_LSN_VAL - 1;
const uint64_t PALF_INITIAL_LSN_VAL = 0;
// =========== LSN end ==============

// =========== Disk io start ==================
constexpr int LOG_READ_FLAG = O_RDONLY | O_DIRECT | O_SYNC;
constexpr int LOG_WRITE_FLAG = O_RDWR | O_DIRECT | O_SYNC;
constexpr mode_t FILE_OPEN_MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
// =========== Disk io end ====================

enum ObReplicaState {
  INVALID_STATE = 0,
  INIT = 1,
  ACTIVE = 2,
  RECONFIRM = 3,
  PENDING = 4,
};

inline const char *replica_state_to_string(const ObReplicaState &state)
{
  #define CHECK_OB_REPLICA_STATE(x) case(ObReplicaState::x): return #x
  switch (state)
  {
    CHECK_OB_REPLICA_STATE(INIT);
    CHECK_OB_REPLICA_STATE(ACTIVE);
    CHECK_OB_REPLICA_STATE(RECONFIRM);
    CHECK_OB_REPLICA_STATE(PENDING);
    default:
      return "INVALID_STATE";
  }
  #undef CHECK_OB_REPLICA_STATE
}

enum LogType
{
  LOG_UNKNOWN = 0,
  LOG_SUBMIT = 201,
  LOG_PADDING = 301,
  // max value of log_type
  LOG_TYPE_MAX  = 1000
};

enum LogReplicaType
{
  INVALID_REPLICA = 0,
  NORMAL_REPLICA,           // full replica
  ARBIRTATION_REPLICA,      // arbitration replica
};

inline const char *replica_type_2_str(const LogReplicaType state)
{
  #define CHECK_REPLICA_TYPE_STR(x) case(LogReplicaType::x): return #x
  switch(state)
  {
    CHECK_REPLICA_TYPE_STR(NORMAL_REPLICA);
    CHECK_REPLICA_TYPE_STR(ARBIRTATION_REPLICA);
    default:
      return "InvalidReplicaType";
  }
  #undef CHECK_REPLICA_TYPE_STR
}


inline int log_replica_type_to_string(const LogReplicaType replica_type, char *str_buf_, const int64_t str_len)
{
  int ret = OB_SUCCESS;
  if (LogReplicaType::NORMAL_REPLICA == replica_type) {
    strncpy(str_buf_, "NORMAL_REPLICA", str_len);
  } else if (LogReplicaType::ARBIRTATION_REPLICA == replica_type) {
    strncpy(str_buf_, "ARBIRTATION_REPLICA", str_len);
  } else {
    ret = OB_INVALID_ARGUMENT;
  }
  return ret;
}

inline bool is_valid_log_id(const int64_t log_id)
{
  return (log_id > 0);
}

inline bool is_valid_block_id(block_id_t  block_id)
{
  return block_id >= 0 && block_id < LOG_MAX_BLOCK_ID;
}

inline bool is_tmp_block(const char *block_name)
{
  bool bool_ret = false;
  if (NULL != block_name && NULL != strstr(block_name, TMP_SUFFIX)) {
    bool_ret = true;
  }
  return bool_ret;
}

inline int convert_to_tmp_block(const char *log_dir,
                               const block_id_t  block_id,
                               char *buf,
                               const int64_t buf_len)
{
  int64_t pos = 0;
  return databuff_printf(buf, buf_len, pos, "%s/%lu%s", log_dir,
          block_id, TMP_SUFFIX);
}

inline int convert_to_normal_block(const char *log_dir,
                                   const block_id_t  block_id,
                                   char *buf,
                                   const int64_t buf_len)
{
  int64_t pos = 0;
  return databuff_printf(buf, buf_len, pos, "%s/%lu", log_dir, block_id);
}

class ObBaseDirFunctor
{
public:
  virtual int func(const dirent *entry) = 0;
};

int scan_dir(const char *dir_name, ObBaseDirFunctor &functor);

struct TimeoutChecker
{
  explicit TimeoutChecker(const int64_t timeout_us)
      : begin_time_us_(common::ObTimeUtility::current_time()), timeout_us_(timeout_us) { }
  ~TimeoutChecker() { }
  void reset()
  {
    begin_time_us_ = common::ObTimeUtility::current_time();
  }

  int operator()()
  {
    int ret = OB_SUCCESS;
    if ((common::ObTimeUtility::current_time() - begin_time_us_ >= timeout_us_)) {
      ret = OB_TIMEOUT;
    }
    return ret;
  }

  int64_t begin_time_us_;
  int64_t timeout_us_;
};

inline bool palf_reach_time_interval(const int64_t interval, int64_t &warn_time)
{
  bool bool_ret = false;
  if ((common::ObTimeUtility::current_time() - warn_time >= interval) ||
      common::OB_INVALID_TIMESTAMP == warn_time) {
    warn_time = common::ObTimeUtility::current_time();
    bool_ret = true;
  }
  return bool_ret;
}

inline bool is_valid_palf_id(const int64_t id)
{
  return 0 <= id;
}

inline bool is_valid_file_desc(const FileDesc &fd)
{
  return 0 <= fd;
}


int block_id_to_string(const block_id_t block_id,
                       char *str,
                       const int64_t str_len);
int block_id_to_tmp_string(const block_id_t block_id,
                           char *str,
                           const int64_t str_len);

int convert_sys_errno();

class GetBlockIdRangeFunctor : public ObBaseDirFunctor
{
public:
  GetBlockIdRangeFunctor(const char *dir)
    : dir_(dir),
      min_block_id_(LOG_INVALID_BLOCK_ID),
      max_block_id_(LOG_INVALID_BLOCK_ID)
  {
  }
  virtual ~GetBlockIdRangeFunctor() = default;

  int func(const dirent *entry) override final;
  block_id_t get_min_block_id() const { return min_block_id_; }
  block_id_t get_max_block_id() const { return max_block_id_; }
private:
  const char *dir_;
  block_id_t min_block_id_;
  block_id_t max_block_id_;

  DISALLOW_COPY_AND_ASSIGN(GetBlockIdRangeFunctor);
};
int reuse_block_at(const int fd, const char *block_path);
} // end namespace palf
} // end namespace oceanbase

#endif
