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

#define USING_LOG_PREFIX  SQL_ENG
#include "sql/engine/expr/ob_expr_time.h"
#include "lib/timezone/ob_time_convert.h"
#include "sql/engine/ob_exec_context.h"
#include "sql/engine/expr/ob_datum_cast.h"
#include "sql/engine/expr/ob_expr_day_of_func.h"
#include "sql/engine/expr/ob_expr_util.h"

using namespace oceanbase::common;
using namespace oceanbase::sql;

namespace oceanbase
{
namespace sql
{

ObExprTime::ObExprTime(ObIAllocator &alloc)
    : ObFuncExprOperator(alloc, T_FUN_SYS_TIME, N_TIME, 1, NOT_ROW_DIMENSION)
{
}

ObExprTime::~ObExprTime()
{
}

int ObExprTime::calc_result_type1(ObExprResType &type,
                                  ObExprResType &type1,
                                  ObExprTypeCtx &type_ctx) const
{
  int ret = OB_SUCCESS;
  //param will be casted to ObTimeType before calculation
  //see ObExprOperator::cast_operand_type for more details if necessary
  type1.set_calc_type(ObTimeType);
  type.set_type(ObTimeType);
  //deduce scale now.
  int16_t scale1 = MIN(type1.get_scale(), MAX_SCALE_FOR_TEMPORAL);
  int16_t scale = (SCALE_UNKNOWN_YET == scale1) ? MAX_SCALE_FOR_TEMPORAL : scale1;
  type.set_scale(scale);
  type_ctx.set_cast_mode(type_ctx.get_cast_mode() | CM_NULL_ON_WARN);
  return ret;
}

int ObExprTime::cg_expr(ObExprCGCtx &op_cg_ctx, const ObRawExpr &raw_expr, ObExpr &rt_expr) const
{
  UNUSED(op_cg_ctx);
  UNUSED(raw_expr);
  int ret = OB_SUCCESS;
  if (rt_expr.arg_cnt_ != 1) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("time expr should have one param", K(ret), K(rt_expr.arg_cnt_));
  } else if (OB_ISNULL(rt_expr.args_) || OB_ISNULL(rt_expr.args_[0])) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("children of time expr is null", K(ret), K(rt_expr.args_));
  } else {
    rt_expr.eval_func_ = ObExprTime::calc_time;
  }
  return ret;
}

int ObExprTime::calc_time(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum)
{
  int ret = OB_SUCCESS;
  ObDatum *param_datum = NULL;
  if (OB_FAIL(expr.args_[0]->eval(ctx, param_datum))) {
    LOG_WARN("eval param value failed");
  } else if (OB_UNLIKELY(param_datum->is_null())) {
    expr_datum.set_null();
  } else {
    expr_datum.set_time(param_datum->get_time());
  }
  return ret;
}

ObExprTimeBase::ObExprTimeBase(ObIAllocator &alloc, int32_t date_type, ObExprOperatorType type,
                              const char *name)
    : ObFuncExprOperator(alloc, type, name, 1, NOT_ROW_DIMENSION)
{
  dt_type_ = date_type;
}

ObExprTimeBase::~ObExprTimeBase()
{
}

static const char* daynames[7] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
static const char* monthnames[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
int ObExprTimeBase::cg_expr(ObExprCGCtx &op_cg_ctx,
                      const ObRawExpr &raw_expr,
                      ObExpr &rt_expr) const
{
  UNUSED(op_cg_ctx);
  UNUSED(raw_expr);
  int ret = OB_SUCCESS;
  if (rt_expr.arg_cnt_ != 1) {
    ret = OB_INVALID_ARGUMENT;
    LOG_WARN("hour/minute/second expr should have one param", K(ret), K(rt_expr.arg_cnt_));
  } else if (OB_ISNULL(rt_expr.args_) || OB_ISNULL(rt_expr.args_[0])) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("children of hour/minute/second expr is null", K(ret), K(rt_expr.args_));
  } else {
    if(get_type() == T_FUN_SYS_DAY_NAME) {
      rt_expr.eval_func_ = ObExprDayName::calc_dayname;
    } else {
      switch (dt_type_) {
        case DT_HOUR :
          rt_expr.eval_func_ = ObExprHour::calc_hour;
          break;
        case DT_MIN :
          rt_expr.eval_func_ = ObExprMinute::calc_minute;
          break;
        case DT_SEC :
          rt_expr.eval_func_ = ObExprSecond::calc_second;
          break;
        case DT_USEC :
          rt_expr.eval_func_ = ObExprMicrosecond::calc_microsecond;
          break;
        case DT_MDAY :
          rt_expr.eval_func_ = ObExprDayOfMonth::calc_dayofmonth;
          break;
        case DT_WDAY :
          rt_expr.eval_func_ = ObExprDayOfWeek::calc_dayofweek;
          break;
        case DT_YDAY :
          rt_expr.eval_func_ = ObExprDayOfYear::calc_dayofyear;
          break;
        case DT_YEAR :
          rt_expr.eval_func_ = ObExprYear::calc_year;
          break;
        case DT_MON :
          rt_expr.eval_func_ = ObExprMonth::calc_month;
          break;
        case DT_MON_NAME :
          rt_expr.eval_func_ = ObExprMonthName::calc_month_name;
          break;
        default:
          LOG_WARN("ObExprTimeBase cg_expr switch default", K(dt_type_));
          break;
      }      
    }
  }
  return ret;
}

static int ob_expr_convert_to_time(const ObDatum &datum,
                                  ObObjType type,
                                  bool with_date,
                                  bool is_dayofmonth,
                                  ObEvalCtx &ctx,
                                  ObTime &ot)
{
  int ret = OB_SUCCESS;
  const ObSQLSessionInfo *session = ctx.exec_ctx_.get_my_session();
  if (OB_ISNULL(session)) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("session is null", K(ret));
  } else if (with_date) {
    ObTime ot2;
    ObDateSqlMode date_sql_mode;
    date_sql_mode.init(session->get_sql_mode());
    if (OB_FAIL(ob_datum_to_ob_time_with_date(datum, type, get_timezone_info(session),
        ot2, get_cur_time(ctx.exec_ctx_.get_physical_plan_ctx()), is_dayofmonth, date_sql_mode))) {
      LOG_WARN("cast to ob time failed", K(ret));
    } else {
      ot = ot2;
    }
  } else {
    ObTime ot2(DT_TYPE_TIME);
    if (OB_FAIL(ob_datum_to_ob_time_without_date(datum, type, get_timezone_info(session), ot2))) {
      LOG_WARN("cast to ob time failed", K(ret));
    } else {
      ot = ot2;
    }
  }
  return ret;
}

int ObExprTimeBase::calc(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum,
                        int32_t type, bool with_date, bool is_dayofmonth /* false */)
{
  int ret = OB_SUCCESS;
  ObDatum *param_datum = NULL;
  const ObSQLSessionInfo *session = NULL;
  if (OB_ISNULL(session = ctx.exec_ctx_.get_my_session())) {
    ret = OB_ERR_UNEXPECTED;
    LOG_WARN("session is null", K(ret));
  } else if (OB_FAIL(expr.args_[0]->eval(ctx, param_datum))) {
    LOG_WARN("eval param value failed");
  } else if (OB_UNLIKELY(param_datum->is_null())) {
    expr_datum.set_null();
  } else {
    ObTime ot;
    if (OB_FAIL(ob_expr_convert_to_time(*param_datum, expr.args_[0]->datum_meta_.type_, with_date,
                                        is_dayofmonth, ctx, ot))) {
      LOG_WARN("cast to ob time failed", K(ret), K(lbt()), K(session->get_stmt_type()));
      LOG_USER_WARN(OB_ERR_CAST_VARCHAR_TO_TIME);
      uint64_t cast_mode = 0;
      ObSQLUtils::get_default_cast_mode(session->get_stmt_type(), session, cast_mode);
      if (CM_IS_WARN_ON_FAIL(cast_mode) || CM_IS_WARN_ON_FAIL(expr.args_[0]->extra_)) {
        ret = OB_SUCCESS;
        expr_datum.set_null();
      }
    } else if (expr.type_ == T_FUN_SYS_DAY_NAME) {
      if(!ot.parts_[DT_YEAR] && !ot.parts_[DT_MON] && !ot.parts_[DT_MDAY]) {
        expr_datum.set_null();
      } else {
        int idx = ot.parts_[type] - 1;
        if(0 <= idx  && idx < 7) {
          const ObString &name = daynames[idx];
          if (OB_FAIL(ObExprUtil::set_expr_ascii_result(expr, ctx, expr_datum, name))) {
            LOG_WARN("failed to exec set_expr_ascii_result", K(expr), K(ctx), K(expr_datum), K(name));
          }          
        } else {
          ret = OB_ERR_UNEXPECTED;
          LOG_WARN("the parameter idx should be within a reasonable range", K(idx));
        }
      }
    } else if (expr.type_ == T_FUN_SYS_MONTH_NAME) {
      if(!ot.parts_[DT_MON]) {
        expr_datum.set_null();
      } else {
        int idx = ot.parts_[type] - 1;
        if(0 <= idx  && idx < 12) {
          const ObString &month_name = monthnames[idx];
          if (OB_FAIL(ObExprUtil::set_expr_ascii_result(expr, ctx, expr_datum, month_name))) {
            LOG_WARN("failed to exec set_expr_ascii_result", K(expr), K(ctx), K(expr_datum), K(month_name));
          }          
        } else {
          ret = OB_ERR_UNEXPECTED;
          LOG_WARN("the parameter idx should be within a reasonable range", K(idx));
        }
      }
    } else if (with_date && !is_dayofmonth && ot.parts_[DT_DATE] + DAYS_FROM_ZERO_TO_BASE < 0) {
      expr_datum.set_null();
    } else {
      expr_datum.set_int32(ot.parts_[type]);
    }
  }
  return ret;
}

ObExprHour::ObExprHour(ObIAllocator &alloc)
    : ObExprTimeBase(alloc, DT_HOUR, T_FUN_SYS_HOUR, N_HOUR) {};

ObExprHour::~ObExprHour() {}

int ObExprHour::calc_hour(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum)
{
  return ObExprTimeBase::calc(expr, ctx, expr_datum, DT_HOUR, false);
}

ObExprMinute::ObExprMinute(ObIAllocator &alloc)
    : ObExprTimeBase(alloc, DT_MIN, T_FUN_SYS_MINUTE, N_MINUTE) {};

ObExprMinute::~ObExprMinute() {}

int ObExprMinute::calc_minute(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum)
{
  return ObExprTimeBase::calc(expr, ctx, expr_datum, DT_MIN, false);
}

ObExprSecond::ObExprSecond(ObIAllocator &alloc)
    : ObExprTimeBase(alloc, DT_SEC, T_FUN_SYS_SECOND, N_SECOND) {};

ObExprSecond::~ObExprSecond() {}

int ObExprSecond::calc_second(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum)
{
  return ObExprTimeBase::calc(expr, ctx, expr_datum, DT_SEC, false);
}

ObExprMicrosecond::ObExprMicrosecond(ObIAllocator &alloc)
    : ObExprTimeBase(alloc, DT_USEC, T_FUN_SYS_MICROSECOND, N_MICROSECOND) {};

ObExprMicrosecond::~ObExprMicrosecond() {}

int ObExprMicrosecond::calc_microsecond(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum)
{
  return ObExprTimeBase::calc(expr, ctx, expr_datum, DT_USEC, false);
}

ObExprYear::ObExprYear(ObIAllocator &alloc)
    : ObExprTimeBase(alloc, DT_YEAR, T_FUN_SYS_YEAR, N_YEAR) {};

ObExprYear::~ObExprYear() {}

int ObExprYear::calc_year(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum)
{
  return ObExprTimeBase::calc(expr, ctx, expr_datum, DT_YEAR, true);
}

ObExprMonth::ObExprMonth(ObIAllocator &alloc)
    : ObExprTimeBase(alloc, DT_MON, T_FUN_SYS_MONTH, N_MONTH) {};

ObExprMonth::~ObExprMonth() {}

int ObExprMonth::calc_month(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum)
{
  // 对于MONTH('2020-00-00')场景，为了完全兼容mysql，也需要容忍月和日为0
  return ObExprTimeBase::calc(expr, ctx, expr_datum, DT_MON, true, true);
}

ObExprMonthName::ObExprMonthName(ObIAllocator &alloc)
    : ObExprTimeBase(alloc, DT_MON_NAME, T_FUN_SYS_MONTH_NAME, N_MONTH_NAME) {};

ObExprMonthName::~ObExprMonthName() {}

int ObExprMonthName::calc_month_name(const ObExpr &expr, ObEvalCtx &ctx, ObDatum &expr_datum)
{
  int ret = OB_SUCCESS;
  // NOTE: the last param should be true otherwise '2020-09-00' will not work
  if (OB_FAIL(calc(expr, ctx, expr_datum, DT_MON, true, true))) {
    LOG_WARN("eval month in monthname failed", K(ret), K(expr));
  } 

  return ret;
}

int ObExprMonthName::calc_result_type1(ObExprResType &type,
                                       ObExprResType &type1,
                                       common::ObExprTypeCtx &type_ctx) const
{
  ObCollationType cs_type = type_ctx.get_coll_type();
  type.set_varchar();
  type.set_collation_type(cs_type);
  type.set_collation_level(CS_LEVEL_IMPLICIT);
  type.set_full_length(MONTHNAME_MAX_LENGTH, type1.get_length_semantics());
  common::ObObjTypeClass tc1 = ob_obj_type_class(type1.get_type());
  if (common::ObEnumSetTC == tc1) {
    type1.set_calc_type(common::ObVarcharType);
    type1.set_collation_type(cs_type);
    type1.set_collation_level(CS_LEVEL_IMPLICIT);
  } else if ((common::ObFloatTC == tc1) || (common::ObDoubleTC == tc1)) {
    type1.set_calc_type(common::ObIntType);
  }

  return OB_SUCCESS;
}

} //namespace sql
} //namespace oceanbase
