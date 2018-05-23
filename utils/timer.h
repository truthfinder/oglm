#ifndef timer_h__
#define timer_h__

template <typename T> class check_ts { // temporary class for performance debugging purposes
public:
  static __int64 freq() { return T::freq(); }
  check_ts(const bool started = true) : t(0ll), r(0ll), f(1.0/T::freq()) { if (started) { start(); } }
  ~check_ts() { stop(); }
  __int64 start() { return t = T::tics(); }
  __int64 stop() { return r = T::tics() - t; }
  double stop_sec() { return f * stop(); }
  __int64 reset() { __int64 c = T::tics(); r = c - t; t = c; return r; }
  double reset_sec() { return f * reset(); }
  __int64 passed() const { return r; }
  double passed_sec() const { return f * passed(); }
  __int64 current() const { return T::tics() - t; }
  double current_sec() const { return f * current(); }

private:
  __int64 t, r;
  double f;
};

class qpc_timer { // temporary class for performance debugging purposes
  static __int64 _freq() { LARGE_INTEGER r; QueryPerformanceFrequency(&r); return r.QuadPart; }
public:
  static __int64 tics() { LARGE_INTEGER r; QueryPerformanceCounter(&r); return r.QuadPart; }
  static __int64 freq() { static __int64 r = _freq(); return r; }
};

typedef check_ts<qpc_timer> ts;

#endif // timer_h__
