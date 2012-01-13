//-----------------------------------------------------------------------------
/** @file libboardgame_util/IntervalChecker.cpp */
//-----------------------------------------------------------------------------

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "IntervalChecker.h"

#include <limits>
#include <boost/format.hpp>
#include "Assert.h"
#include "Log.h"

namespace libboardgame_util {

using namespace std;
using boost::format;
using libboardgame_util::log;

//-----------------------------------------------------------------------------

#ifndef LIBBOARDGAME_UTIL_INTERVAL_CHECKER_DEBUG
#define LIBBOARDGAME_UTIL_INTERVAL_CHECKER_DEBUG 0
#endif

//-----------------------------------------------------------------------------

IntervalChecker::IntervalChecker(TimeSource& time_source, double time_interval,
                                 function<bool()> f)
    : m_time_source(time_source),
      m_is_first_check(true),
      m_is_deterministic(false),
      m_result(false),
      m_count(1),
      m_count_interval(1),
      m_time_interval(time_interval),
      m_function(f)
{
#if LIBBOARDGAME_UTIL_INTERVAL_CHECKER_DEBUG
    log(format("IntervalChecker::IntervalChecker: time_interval=%1%")
            % time_interval);
#endif
}

bool IntervalChecker::check_expensive()
{
    if (m_result)
        return true;
    if (m_is_deterministic)
    {
        m_result = m_function();
        m_count = m_count_interval;
        return m_result;
    }
    double time = m_time_source();
    if (! m_is_first_check)
    {

        double diff = time - m_last_time;
        double adjust_factor;
        if (diff == 0)
            adjust_factor = 10;
        else
        {
            adjust_factor = m_time_interval / diff;
            if (adjust_factor > 10)
                adjust_factor = 10;
            else if (adjust_factor < 0.1)
                adjust_factor = 0.1;
        }
        double new_count_interval = adjust_factor * double(m_count_interval);
        if (new_count_interval > double(numeric_limits<unsigned int>::max()))
            m_count_interval = numeric_limits<unsigned int>::max();
        else if (new_count_interval < 1)
            m_count_interval = 1;
        else
            m_count_interval = (unsigned int)(new_count_interval);
        m_result = m_function();
#if LIBBOARDGAME_UTIL_INTERVAL_CHECKER_DEBUG
        log(format("IntervalChecker::check_expensive: "
                   "diff=%1% adjust_factor=%2% count_interval=%3%")
            % diff % adjust_factor % m_count_interval);
#endif
    }
    else
    {
#if LIBBOARDGAME_UTIL_INTERVAL_CHECKER_DEBUG
        log("IntervalChecker::check_expensive: is_first_check");
#endif
        m_is_first_check = false;
    }
    m_last_time = time;
    m_count = m_count_interval;
    return m_result;
}

void IntervalChecker::set_deterministic(unsigned int interval)
{
    LIBBOARDGAME_ASSERT(interval >= 1);
    m_is_deterministic = true;
    m_count = interval;
    m_count_interval = interval;
}

//-----------------------------------------------------------------------------

} // namespace libboardgame_util
