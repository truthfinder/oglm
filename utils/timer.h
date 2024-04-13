#ifndef timer_h__
#define timer_h__

template <typename T> class check_ts { // temporary class for performance debugging purposes
public:
  static int64_t freq() { return T::freq(); }
  check_ts(const bool started = true) : t(0ll), r(0ll), f(1.0/T::freq()) { if (started) { start(); } }
  ~check_ts() { stop(); }
  int64_t start() { return t = T::tics(); }
  int64_t stop() { return r = T::tics() - t; }
  double stop_sec() { return f * stop(); }
  int64_t reset() { __int64 c = T::tics(); r = c - t; t = c; return r; }
  double reset_sec() { return f * reset(); }
  int64_t passed() const { return r; }
  double passed_sec() const { return f * passed(); }
  int64_t current() const { return T::tics() - t; }
  double current_sec() const { return f * current(); }

private:
  int64_t t, r;
  double f;
};

class qpc_timer { // temporary class for performance debugging purposes
  static int64_t _freq() { LARGE_INTEGER r; QueryPerformanceFrequency(&r); return r.QuadPart; }
public:
  static int64_t tics() { LARGE_INTEGER r; QueryPerformanceCounter(&r); return r.QuadPart; }
  static int64_t freq() { static __int64 r = _freq(); return r; }
};

typedef check_ts<qpc_timer> ts;

#endif // timer_h__
