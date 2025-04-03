#ifndef SCV
#define SCV

/*
 * headers needed: 
 * <stdint.h> - uint8_t, uptr_t, uint16_t...
 * <stdbool.h> - true/false, bool
 * <string.h> - memcpy, memmove, strlen, strncmp, memset
 *
 */

#ifndef nil
#define nil (void *)0
#endif

typedef int8_t	i8;
typedef uint8_t	u8;
typedef uint8_t	byte;

typedef int16_t	i16;
typedef uint16_t	u16;

typedef int32_t	i32;
typedef uint32_t	u32;

typedef int64_t	i64;
typedef uint64_t	u64;

typedef float	f32;
typedef double	f64;

typedef uintptr_t	uptr;

#define scvBreakpoint __builtin_debugtrap()
#define scvMin(a, b) ((a) < (b) ? (a) : (b))
#define scvMax(a, b) ((a) > (b) ? (a) : (b))
#define scvAssert(expr)                         \
  do {                                          \
    if (!(expr)) {                              \
      scvAssertFail(#expr, __FILE__, __LINE__); \
    }                                           \
  } while (0)
#define SCV_ERRBUF_SIZE 1024

#ifndef SCV_DEFAULT_ALIGNMENT
#define SCV_DEFAULT_ALIGNMENT (2*sizeof(void *))
#endif

#define scvIsPowerOfTwo(x) (((x) & ((x) - 1)) == 0)
#define scvArenaAllocErr(a, size, e) scvArenaAllocAlign(a, size, e, SCV_DEFAULT_ALIGNMENT)
#define scvArenaAlloc(a, size) scvArenaAllocAlign(a, size, nil, SCV_DEFAULT_ALIGNMENT)

typedef struct SCVError SCVError;
typedef struct SCVString SCVString;
typedef struct SCVSlice  SCVSlice;
typedef struct SCVArena SCVArena;

void scvAssertFail(char *expr, char *file, int line);
i64 scvWrite(int fd, void *ptr, u64 size, SCVError *error);
void* scvArenaAllocAlign(SCVArena *arena, u64 size, SCVError *err, u64 align);

struct SCVString {
  u8 *base;
  u64 len;
};

struct SCVSlice {
  void* base;
  u64   len;
  u64   cap;
};

struct SCVError {
  SCVString message;
  uptr      tag;
  byte      errbuf[SCV_ERRBUF_SIZE];
};

struct SCVArena {
  byte *buf;
  u64  size;
  u64  currOffset;
  u64  prevOffset;
};

// strings

void
scvClear(void *ptr, u64 size)
{
  memset(ptr, 0, size);
}

bool
scvIsStringsEquals(SCVString s1, SCVString s2)
{
  if (s1.base == s2.base && s1.len == s2.len) {
    return true;
  }
  if (s1.len != s2.len) {
    return false;
  }

  return strncmp((char *)s1.base, (char *)s2.base, s1.len);
}

SCVString
scvUnsafeString(u8 *buf, u64 len)
{
  SCVString s;

  s.base = buf;
  s.len  = len;

  return s;
}

SCVString
scvUnsafeCString(char* str)
{
  return scvUnsafeString((u8 *)str, strlen(str));
}

SCVString
scvString(SCVSlice s)
{
  SCVString str;

  str.base = s.base;
  str.len  = s.len;

  return str;
}

// slices


// len in SCVSlice not in bytes but in size of element
// same for index
#define scvSliceSetIndex(sl, el, i)                       \
  do {                                                    \
   scvAssert(i < (sl).len);                               \
   memcpy((u8 *)(sl).base + i * sizeof(el), &(el), sizeof(el)); \
  } while (0)

#define scvSliceAppend(sl, el)                                          \
  do {                                                                  \
    scvAssert((sl).len < (sl).cap - 1);                                 \
    memcpy((u8 *)(sl).base + (sl).len * sizeof(el), &(el), sizeof(el)); \
    (sl).len++;                                                         \
  } while (0)

#define scvSliceGet(sl, t, i) &(((t *)(sl).base)[(i)])

SCVSlice
scvUnsafeSlice(void *buf, u64 len)
{
  SCVSlice s;

  s.base = buf;
  s.len = s.cap = len;

  return s;
}

SCVSlice
scvSliceLeft(SCVSlice s, u64 lbytes)
{
  SCVSlice ret;

  ret.base = (u8 *)s.base + lbytes;
  ret.len  = s.len - lbytes;
  ret.cap  = s.cap - lbytes;

  return ret;
}

SCVSlice
scvSliceRight(SCVSlice s, u64 rbytes)
{
  SCVSlice ret;

  scvAssert(rbytes <= s.cap);

  ret.base = s.base;
  ret.len  = rbytes;
  ret.cap  = s.cap;

  return ret;
}

u64
scvSlicePutU64(SCVSlice s, u64 x)
{
  int	ndigits = 0, rx = 0, i = 0;
  char *buf = s.base;

  if (x == 0) {
    buf[0] = '0';
    return 1;
  }

  while (x > 0) {
    rx = (10 * rx) + (x % 10);
    x /= 10;
    ++ndigits;
  }

  while (ndigits > 0) {
    buf[i++] = (rx % 10) + '0';
    rx /= 10;
    --ndigits;
  }

  return i;
}

u64
scvSlicePutI64(SCVSlice s, i64 x)
{
  int	ndigits = 0, rx = 0, i = 0;
  char *buf = s.base;
  int  sign = x < 0;

  if (x == 0) {
    buf[0] = '0';
    return 1;
  }

  if (sign) {
    x = -x;
    buf[i++] = '-';
  }

  while (x > 0) {
    rx = (10 * rx) + (x % 10);
    x /= 10;
    ++ndigits;
  }

  while (ndigits > 0) {
    buf[i++] = (rx % 10) + '0';
    rx /= 10;
    --ndigits;
  }

  return i;
}

u64
scvSlicePutString(SCVSlice sl, SCVString s)
{
  u64 toWrite = scvMin(sl.len, s.len);
  memcpy(sl.base, s.base, toWrite);
  return toWrite;
}

u64
scvSlicePutCString(SCVSlice sl, char *cstr)
{
  return scvSlicePutString(sl, scvUnsafeCString(cstr));
}

SCVSlice
scvSlice(SCVArena *arena, u64 size, u64 len, u64 cap)
{
  SCVSlice s;
  void *buf = scvArenaAlloc(arena, size * cap);
  scvAssert(buf);

  s.base = buf;
  s.len = len;
  s.cap = cap;

  return s;
}

#define scvMakeSlice(arena, type, len, capacity) scvSlice((arena), sizeof(type), (len), (capacity))

// syscalls

typedef struct SCVSyscallResult SCVSyscallResult;
struct SCVSyscallResult {
	uptr r1;
	uptr r2;
	uptr err;
};

SCVSyscallResult scvSyscall(uptr trap, uptr a1, uptr a2, uptr a3);
SCVSyscallResult scvSyscall6(uptr trap, uptr a1, uptr a2, uptr a3, uptr a4, uptr a5, uptr a6);

void 
scvErrorSet(SCVError* err, char* msg, uptr tag) 
{
  SCVString msgstr;
  u64 msglen;
  if (!err || !tag) {
    return;
  }
  msgstr = scvUnsafeCString(msg);
  msglen = scvMin(msgstr.len, sizeof(err->errbuf));
  memcpy(err->errbuf, msgstr.base, msglen); 

  err->message = scvUnsafeString(err->errbuf, msglen);
  err->tag = tag;
}

void
scvExit(u32 status)
{
  scvSyscall(SYS_exit, status, 0, 0);
}

void
scvClose(u32 fd)
{
  scvSyscall(SYS_close, fd, 0, 0);
}

i64
scvWrite(int fd, void *ptr, u64 size, SCVError *err)
{
  SCVSyscallResult r = scvSyscall(SYS_write, fd, (uptr)ptr, (uptr)size);
  scvErrorSet(err, "write failed with code", r.err);
  return r.r1;
}

i64
scvRead(int fd, SCVSlice buf, SCVError *error)
{
  SCVSyscallResult r = scvSyscall(SYS_read, fd, (uptr)buf.base, buf.len);
  scvErrorSet(error, "read failed with code", r.err);
  return r.r1;
}

i32
scvGetTimeofday(struct timeval *tv, SCVError *err)
{
  SCVSyscallResult r = scvSyscall(SYS_gettimeofday, (uptr)tv, 0, 0);
  scvErrorSet(err, "gettimeofday failed with code", r.err);
  return r.r1;
}

// pathname must be null terminated string
i32
scvOpenat(i32 dirfd, SCVString pathname, i32 flags, mode_t mode, SCVError *err)
{
  scvAssert(pathname.base[pathname.len-1] == '0');
  SCVSyscallResult r = scvSyscall6(SYS_openat, (uptr)dirfd, (uptr)pathname.base, (uptr)flags, (uptr)mode, 0, 0);
  scvErrorSet(err, "could not open file with code", r.err);
  return r.r1;
}

#define scvOpen(pathname, flags, error) scvOpenAt(AT_FDCWD, pathname, flags, (mode_t)0, error)

void
scvFStat(i32 fd, struct stat *s, SCVError *err)
{
  SCVSyscallResult r = scvSyscall(SYS_fstat64, (uptr)fd, (uptr)s, 0);
  scvErrorSet(err, "fstat failed with code", r.err);
}

void*
scvMmap(void *addr, u64 len, i32 prot, i32 flags, i32 fd, i64 offset, SCVError *error)
{
  SCVSyscallResult r = scvSyscall6(SYS_mmap, (uptr)addr, (uptr)len, (uptr)prot, (uptr)flags, (uptr)fd, (uptr)offset);
  scvErrorSet(error, "mmap failed with code", r.err);

  return (void *)r.r1;
}

// printing
void
scvPrintNewline(void)
{
  scvWrite(2, "\n", 1, nil);
}

void
scvPrint(char *cstr)
{
  scvWrite(2, cstr, strlen(cstr), nil);
}

void
scvPrintCString(char *cstr)
{
  scvPrint(cstr);
  scvPrintNewline();
}

void
scvPrintString(SCVString s)
{
  scvWrite(2, s.base, s.len, nil);
  scvPrintNewline();
}

void
scvPrintI64(i64 x)
{
  char buffer[20];
  int  n;
  SCVSlice s;

  s = scvUnsafeSlice(buffer, sizeof(buffer));
  n = scvSlicePutI64(s, x);
  scvPrintString(scvString(scvSliceRight(s, n)));
}

void
scvPrintU64(u64 x)
{
  char buffer[20];
  int  n;
  SCVSlice s;

  s = scvUnsafeSlice(buffer, sizeof(buffer));
  n = scvSlicePutU64(s, x);
  scvPrintString(scvString(scvSliceRight(s, n)));
}

void
scvPrintMsgCode(char *msg, i32 code)
{
  scvPrint(msg);
  scvPrint(" ");
  scvPrintI64((i64)code);
}

void
scvPrintError(SCVError *err)
{
  if (err) {
    scvWrite(2, err->message.base, err->message.len, nil);
    scvPrint(" ");
    scvPrintU64(err->tag);
  }
}

void
scvFatalError(char *msg, SCVError *err)
{
  scvPrintCString(msg);
  scvPrintError(err);
  scvExit(1);
}

void
scvAssertFail(char *expr, char *file, int line)
{

  SCVSlice s;
  char buffer[1024] = {0};
  u64 n = 0;

  s = scvUnsafeSlice(buffer, sizeof(buffer));

  n += scvSlicePutCString(scvSliceLeft(s, n), file);
  n += scvSlicePutCString(scvSliceLeft(s, n), ":");
  n += scvSlicePutI64(scvSliceLeft(s, n), (i64)line);
  n += scvSlicePutCString(scvSliceLeft(s, n), ": Assertion `");
  n += scvSlicePutCString(scvSliceLeft(s, n), expr);
  n += scvSlicePutCString(scvSliceLeft(s, n), "' failed.");

  scvPrintString(scvString(scvSliceRight(s, n)));
  scvBreakpoint;
}


struct tm 
scvTimeToTm(i64 t) 
{
  i64 quadricentennials, centennials, quadrennials, annuals;
	i64 yday, mday, wday;
	i64 year, month, leap;
	i64 hour, min, sec;
	struct tm tm;

	int	daysSinceJan1st[][13] = {
		{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 }, /* non-leap year. */
		{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }, /* leap year. */
	};


	/* Re-bias from 1970 to 1601: 1970 - 1601 = 369 = 3*100 + 17*4 + 1 years (incl. 89 leap days) = (3*100*(365+24/100) + 17*4*(365+1/4) + 1*365)*24*3600 seconds. */
	sec = t + 11644473600;

	wday = (sec / 86400 + 1) % 7; /* day of week */

	/* Remove multiples of 400 years (incl. 97 leap days). */
	quadricentennials = sec / 12622780800; /* 400*365.2425*24*3600 .*/
	sec %= 12622780800;

	/* Remove multiples of 100 years (incl. 24 leap days), can't be more than 3 (because multiples of 4*100=400 years (incl. leap days) have been removed). */
	centennials = sec / 3155673600; /* 100*(365+24/100)*24*3600. */
	if (centennials > 3) {
		centennials = 3;
	}
	sec -= centennials * 3155673600;

	/* Remove multiples of 4 years (incl. 1 leap day), can't be more than 24 (because multiples of 25*4=100 years (incl. leap days) have been removed). */
	quadrennials = sec / 126230400; /*  4*(365+1/4)*24*3600. */
	if (quadrennials > 24) {
		quadrennials = 24;
	}
	sec -= quadrennials * 126230400;

	/* Remove multiples of years (incl. 0 leap days), can't be more than 3 (because multiples of 4 years (incl. leap days) have been removed). */
	annuals = sec / 31536000; /* 365*24*3600 */
	if (annuals > 3) {
		annuals = 3;
	}
	sec -= annuals * 31536000;

	/* Calculate the year and find out if it's leap. */
	year = 1601 + quadricentennials * 400 + centennials * 100 + quadrennials * 4 + annuals;
	leap = (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));

	/* Calculate the day of the year and the time. */
	yday = sec / 86400;
	sec %= 86400;
	hour = sec / 3600;
	sec %= 3600;
	min  = sec / 60;
	sec %= 60;

	/* Calculate the month. */
	for (month = 1, mday = 1; month <= 12; ++month) {
		if (yday < daysSinceJan1st[leap][month]) {
			mday += yday - daysSinceJan1st[leap][month-1];
			break;
		}
	}

	tm.tm_sec = sec;          /*  [0,59]. */
	tm.tm_min = min;          /*  [0,59]. */
	tm.tm_hour = hour;        /*  [0,23]. */
	tm.tm_mday = mday;        /*  [1,31]  (day of month). */
	tm.tm_mon = month - 1;    /*  [0,11]  (month). */
	tm.tm_year = year - 1900; /*  70+     (year since 1900). */
	tm.tm_wday = wday;        /*  [0,6]   (day since Sunday AKA day of week). */
	tm.tm_yday = yday;        /*  [0,365] (day since January 1st AKA day of year). */
	tm.tm_isdst = -1;         /*  daylight saving time flag. */

	return tm;
}

u64 scvCntFrq(void);
u64 scvCntVct(void);

#ifdef __APPLE__
 #ifdef __aarch64__

u64 
scvCntVct(void)
{
  u64 r;
  __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(r));

  return r;
}

u64 
scvCntFrq(void) 
{
    u64 freq;
    __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(freq));
    return freq;
}
 #endif
#endif

enum SCVTimerType {
  SCV_NS,
  SCV_US,
  SCV_MS,
  SCV_SEC,
};

typedef struct SCVTimer SCVTimer;
struct SCVTimer {
  u64 prev;
  u64 freq;
};

void
scvInitTimer(SCVTimer *timer)
{
  scvAssert(timer);
  timer->freq = scvCntFrq();
}

void
scvTimerTic(SCVTimer *timer)
{
  timer->prev = scvCntVct(); 
}

u64
scvTimerToc(SCVTimer *timer, enum SCVTimerType t)
{
  switch (t) {
    case SCV_NS:
      return (u64)((f64)(scvCntVct() - timer->prev) * 1000000000.0 / (f64)timer->freq);
    case SCV_US:
      return (u64)((f64)(scvCntVct() - timer->prev) * 1000000.0 / (f64)timer->freq);
    case SCV_MS:
      return (u64)((f64)(scvCntVct() - timer->prev) * 1000.0 / (f64)timer->freq);
    default:
      return (u64)((f64)(scvCntVct() - timer->prev) / (f64)timer->freq);
  }
}

// Arena

uptr 
scvAlignForward(uptr ptr, u64 align)
{
  uptr p, a, modulo;

  scvAssert(scvIsPowerOfTwo(align));

  p = ptr;
  a = (uptr)align;

  modulo = p & (a-1);

  if (modulo != 0) {
    p += a - modulo;
  }

  return p;
}

u64
scvSizeRoundUp(u64 size)
{
  return size + (PAGE_SIZE - (size & (PAGE_SIZE - 1)));
}

void*
scvMmapRealloc(void *prev, u64 prevSize, u64 size, SCVError *err)
{
  void *addr;
  i32  flags;

  if (size == 0) {
    return nil;
  }

  if (prev == nil) {
    addr = nil;
    flags = 0;
  } else {
    addr = (u8 *)prev + prevSize;
    flags = MAP_FIXED;
  }

  size = scvSizeRoundUp(size);

  return scvMmap(addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | flags, -1, 0, err);
}

void
scvArenaInit(SCVArena *arena, SCVError *err)
{
  scvAssert(arena);
  arena->buf = scvMmapRealloc(nil, 0, 0, err);
  arena->size = 0;
  arena->currOffset = 0;
  arena->prevOffset = 0;
}

void*
scvArenaAllocAlign(SCVArena *arena, u64 size, SCVError *err, u64 align)
{
  uptr currPtr = (uptr)arena->buf + (uptr)arena->currOffset;
  uptr offset  = scvAlignForward(currPtr, align);
  offset -= (uptr)arena->buf;

  if (offset + size <= arena->size) {
    void *ptr = &arena->buf[offset];
    arena->prevOffset = offset;
    arena->currOffset = offset + size;
    memset(ptr, 0, size);

    return ptr;
  } else {
    arena->buf = scvMmapRealloc(arena->buf, arena->size, scvSizeRoundUp(size), err);
    if (!arena->buf) {
      return nil;
    }
    arena->size += scvSizeRoundUp(size);

    return scvArenaAllocAlign(arena, size, err, align);
  }
}

enum SCVLogLevel {
  SCV_LOG_PANIC = 0,
  SCV_LOG_ERROR = 1,
  SCV_LOG_WARN  = 2,
  SCV_LOG_INFO  = 3,
};

void 
scvLog(
    char *tag,
    enum SCVLogLevel level,
    u32 logitem,
    char* msg,
    u32 line,
    char* filename
);


#define scvWarn(tag, msg) scvLog(tag, SCV_LOG_WARN, 0, msg, __LINE__, __FILE__)

#define scvInfo(tag, msg) scvLog(tag, SCV_LOG_INFO, 0, msg, __LINE__, __FILE__)

#define scvInfoID(tag, msg, id) scvLog(tag, SCV_LOG_INFO, id, msg, __LINE__, __FILE__)

#endif
