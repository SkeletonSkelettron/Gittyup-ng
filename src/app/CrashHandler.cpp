//
//          Copyright (c) 2026, Gittyup contributors
//
// This software is licensed under the MIT License. The LICENSE.md file
// describes the conditions under which this software may be distributed.
//

#include "CrashHandler.h"
#include <QCoreApplication>
#include <QString>
#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <exception>
#include <typeinfo>

#include <windows.h>
#include <dbghelp.h>
#include <strsafe.h>

#ifdef __GNUC__
#include <cxxabi.h>
#endif

namespace {

// The C++ runtimes call std::terminate() for their exceptions.
const DWORD kGccException = 0x20474343;
const DWORD kMsvcException = 0xE06D7363;

// The exception codes in the dumps of crashes that raise no exception.
const DWORD kFatalAppExit = 0x40000015;
const DWORD kInvalidParameter = 0xC0000417;

const wchar_t *const kName = L"" GITTYUP_NAME;
const wchar_t *const kPrefix = L"" GITTYUP_NAME L"-" GITTYUP_VERSION;

const int kMaxFrames = 64;

// What happened, written down by another thread: the crashed one may have no
// stack left, and a dump shows the stack of a thread right from another one.
struct Crash {
  DWORD thread = 0;
  // Described from the exception when it's empty.
  char reason[4096] = {};
  EXCEPTION_POINTERS *exception = nullptr;
  // The exception of crashes that raise none.
  EXCEPTION_RECORD record = {};
  EXCEPTION_POINTERS pointers = {};
  // The registers that the stack is walked from.
  CONTEXT context = {};
  ULONG_PTR stackLow = 0;
  ULONG_PTR stackHigh = 0;
  HANDLE written = nullptr;
};

Crash sCrash;

// Prepared at startup. Nothing is allocated for them in a crash.
wchar_t sDir[MAX_PATH];
wchar_t sMessage[512];
bool sQuiet = false;

LPTOP_LEVEL_EXCEPTION_FILTER sDefaultFilter = nullptr;
QtMessageHandler sDefaultHandler = nullptr;
std::terminate_handler sDefaultTerminate = nullptr;

// The thread that reports the first crash.
std::atomic<DWORD> sCrashed{0};

// Whether this thread reports a crash. Another thread that crashes meanwhile
// waits for the process to end, and a crash of this thread while or after it
// reports isn't reported again.
bool begin() {
  DWORD self = GetCurrentThreadId();
  DWORD none = 0;
  if (sCrashed.compare_exchange_strong(none, self))
    return true;

  if (none != self)
    Sleep(INFINITE);
  return false;
}

// The address that the module was linked at, from its file. The loader
// writes where it loaded the module into the header in memory.
DWORD64 linkedBase(const wchar_t *path) {
  DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  HANDLE file = CreateFileW(path, GENERIC_READ, share, nullptr, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE)
    return 0;

  BYTE header[4096];
  DWORD read = 0;
  BOOL ok = ReadFile(file, header, sizeof(header), &read, nullptr);
  CloseHandle(file);
  if (!ok || read < sizeof(IMAGE_DOS_HEADER))
    return 0;

  auto dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(header);
  if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 ||
      static_cast<DWORD>(dos->e_lfanew) + sizeof(IMAGE_NT_HEADERS64) > read)
    return 0;

  auto nt =
      reinterpret_cast<const IMAGE_NT_HEADERS64 *>(header + dos->e_lfanew);
  if (nt->Signature != IMAGE_NT_SIGNATURE)
    return 0;

  switch (nt->OptionalHeader.Magic) {
    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
      return nt->OptionalHeader.ImageBase;
    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
      return reinterpret_cast<const IMAGE_NT_HEADERS32 *>(nt)
          ->OptionalHeader.ImageBase;
    default:
      return 0;
  }
}

// The module of the address and the offset into it. In parentheses is the
// address in the module as it was linked, which addr2line looks up.
void describe(DWORD64 address, char *text, size_t size) {
  HMODULE module = nullptr;
  DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
  LPCWSTR pointer = reinterpret_cast<LPCWSTR>(static_cast<ULONG_PTR>(address));
  if (!GetModuleHandleExW(flags, pointer, &module)) {
    StringCchPrintfA(text, size, "0x%llx", address);
    return;
  }

  wchar_t path[MAX_PATH] = {};
  GetModuleFileNameW(module, path, MAX_PATH);
  const wchar_t *name = wcsrchr(path, L'\\');
  char utf8[MAX_PATH * 3] = {};
  WideCharToMultiByte(CP_UTF8, 0, name ? name + 1 : path, -1, utf8,
                      static_cast<int>(sizeof(utf8)), nullptr, nullptr);

  // Frames are mostly in the module of the frame before.
  static HMODULE lastModule = nullptr;
  static DWORD64 lastBase = 0;
  if (module != lastModule) {
    lastModule = module;
    lastBase = linkedBase(path);
  }

  DWORD64 offset = address - reinterpret_cast<ULONG_PTR>(module);
  if (lastBase) {
    StringCchPrintfA(text, size, "%s+0x%llx (0x%llx)", utf8, offset,
                     lastBase + offset);
  } else {
    StringCchPrintfA(text, size, "%s+0x%llx", utf8, offset);
  }
}

const char *exceptionName(DWORD code) {
  switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
      return "access violation";
    case EXCEPTION_STACK_OVERFLOW:
      return "stack overflow";
    case EXCEPTION_ILLEGAL_INSTRUCTION:
      return "illegal instruction";
    case EXCEPTION_PRIV_INSTRUCTION:
      return "privileged instruction";
    case EXCEPTION_IN_PAGE_ERROR:
      return "page error";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
      return "integer division by zero";
    case EXCEPTION_BREAKPOINT:
      return "breakpoint";
    case 0xC0000374:
      return "heap corruption";
    case 0xC0000409:
      return "stack buffer overrun";
    case kGccException:
    case kMsvcException:
      return "C++ exception";
    default:
      return "exception";
  }
}

void describeException(const EXCEPTION_RECORD *record, char *text,
                       size_t size) {
  char where[MAX_PATH * 3 + 64];
  describe(reinterpret_cast<ULONG_PTR>(record->ExceptionAddress), where,
           sizeof(where));

  DWORD code = record->ExceptionCode;
  const char *name = exceptionName(code);
  if (code == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2) {
    ULONG_PTR kind = record->ExceptionInformation[0];
    const char *access = (kind == 0)   ? "reading"
                         : (kind == 8) ? "executing"
                                       : "writing";
    DWORD64 address = record->ExceptionInformation[1];
    StringCchPrintfA(text, size, "Exception 0x%08lx, %s %s 0x%llx, at %s.",
                     code, name, access, address, where);
    return;
  }

  StringCchPrintfA(text, size, "Exception 0x%08lx, %s, at %s.", code, name,
                   where);
}

// The return addresses on the stack of the crashed thread. Its stack may be
// broken: the walk stops where it leaves the stack or doesn't go up.
int walk(DWORD64 *frames) {
  int count = 0;
#if defined(_M_X64) || defined(__x86_64__)
  CONTEXT context = sCrash.context;
  while (count < kMaxFrames && context.Rip) {
    frames[count++] = context.Rip;
    DWORD64 top = context.Rsp;
    if (top < sCrash.stackLow || top + sizeof(DWORD64) > sCrash.stackHigh)
      break;

    DWORD64 base = 0;
    PRUNTIME_FUNCTION function =
        RtlLookupFunctionEntry(context.Rip, &base, nullptr);
    if (function) {
      PVOID data = nullptr;
      DWORD64 frame = 0;
      RtlVirtualUnwind(UNW_FLAG_NHANDLER, base, context.Rip, function, &context,
                       &data, &frame, nullptr);
    } else {
      // Leaf functions keep nothing on the stack but the return address.
      context.Rip = *reinterpret_cast<const DWORD64 *>(top);
      context.Rsp = top + sizeof(DWORD64);
    }

    if (context.Rsp <= top)
      break;
  }
#else
  Q_UNUSED(frames);
#endif
  return count;
}

void write(HANDLE file, const char *text) {
  DWORD written = 0;
  WriteFile(file, text, static_cast<DWORD>(strlen(text)), &written, nullptr);
}

// Writes the report and the dump of the crash, then tells where they are.
DWORD WINAPI writeReport(void *) {
  SYSTEMTIME time;
  GetLocalTime(&time);
  CreateDirectoryW(sDir, nullptr);

  wchar_t path[MAX_PATH];
  StringCchPrintfW(path, MAX_PATH,
                   L"%ls\\%ls-%04u%02u%02u-%02u%02u%02u-%lu-%lu", sDir, kPrefix,
                   time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
                   time.wSecond, GetCurrentProcessId(), sCrash.thread);

  wchar_t reportPath[MAX_PATH];
  wchar_t dumpPath[MAX_PATH];
  StringCchPrintfW(reportPath, MAX_PATH, L"%ls.txt", path);
  StringCchPrintfW(dumpPath, MAX_PATH, L"%ls.dmp", path);

  if (!sCrash.reason[0])
    describeException(sCrash.exception->ExceptionRecord, sCrash.reason,
                      sizeof(sCrash.reason));

  // What happened goes first. The dump and the stack may fail.
  HANDLE report =
      CreateFileW(reportPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  char text[1024];
  StringCchPrintfA(text, sizeof(text),
                   "%s crashed on %04u-%02u-%02u at %02u:%02u:%02u.\r\n\r\n",
                   GITTYUP_NAME " " GITTYUP_VERSION BUILD_DESCRIPTION,
                   time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute,
                   time.wSecond);
  write(report, text);
  write(report, sCrash.reason);
  write(report, "\r\n\r\n");

  HANDLE dump = CreateFileW(dumpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
  MINIDUMP_EXCEPTION_INFORMATION exception = {sCrash.thread, sCrash.exception,
                                              FALSE};
  bool dumped = false;
  if (dump != INVALID_HANDLE_VALUE) {
    dumped =
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dump,
                          MiniDumpWithDataSegs, &exception, nullptr, nullptr);
    CloseHandle(dump);
  }

  DWORD64 frames[kMaxFrames];
  int count = walk(frames);
  StringCchPrintfA(text, sizeof(text), "Stack of thread %lu:\r\n",
                   sCrash.thread);
  write(report, text);
  for (int i = 0; i < count; ++i) {
    char frame[MAX_PATH * 3 + 64];
    describe(frames[i], frame, sizeof(frame));
    StringCchPrintfA(text, sizeof(text), "#%-3d%s\r\n", i, frame);
    write(report, text);
  }

  write(report, "\r\naddr2line -f -C -e <module> <address in parentheses> "
                "shows the function and the line.\r\n");
  write(report, dumped ? "The dump of the process is next to this report.\r\n"
                       : "The dump of the process couldn't be written.\r\n");
  if (report != INVALID_HANDLE_VALUE)
    CloseHandle(report);

  SetEvent(sCrash.written);

  if (!sQuiet) {
    static wchar_t reason[sizeof(sCrash.reason)];
    MultiByteToWideChar(CP_UTF8, 0, sCrash.reason, -1, reason,
                        static_cast<int>(sizeof(sCrash.reason)));

    static wchar_t message[sizeof(sCrash.reason) + 1024];
    StringCchPrintfW(message, sizeof(message) / sizeof(message[0]),
                     L"%ls\n%ls\n\n%ls", sMessage, reportPath, reason);
    MessageBoxW(nullptr, message, kName,
                MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
  }

  return 0;
}

// Writes the crash down on another thread and waits for it.
void report() {
  sCrash.thread = GetCurrentThreadId();
  auto tib = reinterpret_cast<const NT_TIB *>(NtCurrentTeb());
  sCrash.stackLow = reinterpret_cast<ULONG_PTR>(tib->StackLimit);
  sCrash.stackHigh = reinterpret_cast<ULONG_PTR>(tib->StackBase);
  sCrash.written = CreateEventW(nullptr, TRUE, FALSE, nullptr);

  HANDLE thread = CreateThread(nullptr, 0, &writeReport, nullptr, 0, nullptr);
  if (!thread) {
    writeReport(nullptr);
    return;
  }

  // Writing may wait for a lock that the crashed thread holds. Wait for the
  // message to be closed only when it's written.
  if (WaitForSingleObject(sCrash.written, 60 * 1000) == WAIT_OBJECT_0)
    WaitForSingleObject(thread, INFINITE);
  CloseHandle(thread);
}

// Reports a crash that raised no exception, from where it's called.
void reportHere(DWORD code) {
  RtlCaptureContext(&sCrash.context);
  sCrash.record.ExceptionCode = code;
  sCrash.record.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
#if defined(_M_X64) || defined(__x86_64__)
  sCrash.record.ExceptionAddress =
      reinterpret_cast<PVOID>(static_cast<ULONG_PTR>(sCrash.context.Rip));
#endif
  sCrash.pointers.ExceptionRecord = &sCrash.record;
  sCrash.pointers.ContextRecord = &sCrash.context;
  sCrash.exception = &sCrash.pointers;
  report();
}

LONG WINAPI onException(EXCEPTION_POINTERS *info) {
  // The C++ runtime calls std::terminate() for its exceptions, which reports
  // them with their message.
  DWORD code = info->ExceptionRecord->ExceptionCode;
  bool cpp = (code == kGccException || code == kMsvcException);
  if ((!cpp || !sDefaultFilter) && begin()) {
    sCrash.exception = info;
    sCrash.context = *info->ContextRecord;
    report();
  }

  return sDefaultFilter ? sDefaultFilter(info) : EXCEPTION_CONTINUE_SEARCH;
}

void onMessage(QtMsgType type, const QMessageLogContext &context,
               const QString &message) {
  if (type == QtFatalMsg && begin()) {
    QByteArray text = message.toUtf8();
    StringCchPrintfA(sCrash.reason, sizeof(sCrash.reason), "Fatal error: %s",
                     text.constData());
    if (context.file) {
      char where[1024];
      StringCchPrintfA(where, sizeof(where), "\r\n(%s:%d, %s)", context.file,
                       context.line, context.function ? context.function : "");
      StringCchCatA(sCrash.reason, sizeof(sCrash.reason), where);
    }

    reportHere(kFatalAppExit);
    if (sDefaultHandler)
      sDefaultHandler(type, context, message);

    // Qt ends the process with a fail-fast exception, which Windows shows in
    // a dialog of its own that waits to be closed.
    TerminateProcess(GetCurrentProcess(), kFatalAppExit);
  }

  if (sDefaultHandler)
    sDefaultHandler(type, context, message);
}

[[noreturn]] void onTerminate() {
  if (begin()) {
    StringCchCopyA(sCrash.reason, sizeof(sCrash.reason),
                   "std::terminate() was called.");
    if (std::exception_ptr exception = std::current_exception()) {
      const char *type = "of an unknown type";
#ifdef __GNUC__
      char *name = nullptr;
      if (std::type_info *info = abi::__cxa_current_exception_type()) {
        int status = 0;
        name = abi::__cxa_demangle(info->name(), nullptr, nullptr, &status);
        type = name ? name : info->name();
      }
#endif

      try {
        std::rethrow_exception(exception);
      } catch (const std::exception &e) {
#ifndef __GNUC__
        type = typeid(e).name();
#endif
        StringCchPrintfA(sCrash.reason, sizeof(sCrash.reason),
                         "Uncaught C++ exception %s: %s", type, e.what());
      } catch (...) {
        StringCchPrintfA(sCrash.reason, sizeof(sCrash.reason),
                         "Uncaught C++ exception %s.", type);
      }

#ifdef __GNUC__
      free(name);
#endif
    }

    reportHere(kFatalAppExit);
  }

  if (sDefaultTerminate)
    sDefaultTerminate();
  std::abort();
}

void onAbort(int) {
  if (begin()) {
    StringCchCopyA(sCrash.reason, sizeof(sCrash.reason), "abort() was called.");
    reportHere(kFatalAppExit);
  }
}

// The C runtime ends the process after an invalid parameter. Only its debug
// version says where.
void onInvalidParameter(const wchar_t *expression, const wchar_t *function,
                        const wchar_t *file, unsigned int line, uintptr_t) {
  if (begin()) {
    StringCchCopyA(sCrash.reason, sizeof(sCrash.reason),
                   "A function of the C runtime was called with an invalid "
                   "parameter.");
    if (function) {
      char where[2048];
      StringCchPrintfA(where, sizeof(where), "\r\n%ls: %ls (%ls:%u)", function,
                       expression ? expression : L"", file ? file : L"", line);
      StringCchCatA(sCrash.reason, sizeof(sCrash.reason), where);
    }

    reportHere(kInvalidParameter);
  }

  TerminateProcess(GetCurrentProcess(), kInvalidParameter);
}

} // namespace

void CrashHandler::install(bool quiet) {
  static bool installed = false;
  if (installed)
    return;
  installed = true;

  sQuiet = quiet;
  GetTempPathW(MAX_PATH, sDir);
  StringCchCatW(sDir, MAX_PATH, kName);

  QString message = QCoreApplication::translate(
      "CrashHandler", "%1 closed unexpectedly. What happened was saved to:");
  message = message.arg(GITTYUP_NAME).left(511);
  sMessage[message.toWCharArray(sMessage)] = L'\0';

  // The main thread keeps this much of its stack to report its overflow.
  ULONG guarantee = 64 * 1024;
  SetThreadStackGuarantee(&guarantee);

  sDefaultFilter = SetUnhandledExceptionFilter(&onException);
  sDefaultHandler = qInstallMessageHandler(&onMessage);
  sDefaultTerminate = std::set_terminate(&onTerminate);
  signal(SIGABRT, &onAbort);
  _set_invalid_parameter_handler(&onInvalidParameter);
}
