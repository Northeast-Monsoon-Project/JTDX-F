#ifndef JTDX_FM_TX_WATCHDOG_HPP
#define JTDX_FM_TX_WATCHDOG_HPP
#include <chrono>
#include <algorithm>

// A TX session uses monotonic elapsed time, independent of UTC corrections,
// user input, decoded messages, logging and the number of completed QSOs.
class TxWatchdog
{
public:
  using Clock = std::chrono::steady_clock;
  using Time = Clock::time_point;
  void start(Time now = Clock::now()) { started_ = now; active_ = true; }
  void stop() { active_ = false; }
  bool active() const { return active_; }
  long long elapsedMs(Time now = Clock::now()) const {
    return active_ ? std::max(0LL, static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(now - started_).count())) : 0;
  }
  bool expired(int minutes, Time now = Clock::now()) const {
    return active_ && minutes > 0 && elapsedMs(now) >= minutes * 60000LL;
  }
  int remainingSeconds(int minutes, Time now = Clock::now()) const {
    return minutes == 0 ? -1 : static_cast<int>(std::max(0LL, (minutes * 60000LL - elapsedMs(now) + 999) / 1000));
  }
private:
  bool active_ = false;
  Time started_ {};
};
#endif
