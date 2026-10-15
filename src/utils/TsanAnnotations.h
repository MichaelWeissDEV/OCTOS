#ifndef OCTOS_TSAN_ANNOTATIONS_H
#define OCTOS_TSAN_ANNOTATIONS_H

// ThreadSanitizer cannot see the happens-before edge created by Qt's own (uninstrumented) work queue:
// QThreadPool::start() hands a runnable to a worker thread through library-internal atomics. Without help
// it reports a race for every member of such a runnable that the worker reads. These macros state that
// edge explicitly and precisely for exactly one address (the runnable); they compile to nothing in normal
// builds. This replaces a broad `race:` suppression that would also have hidden real races in the code
// the runnable calls.
#if defined(__has_feature)
#  if __has_feature(thread_sanitizer)
#    define OCTOS_TSAN_ENABLED 1
#  endif
#endif
#if defined(__SANITIZE_THREAD__)
#  define OCTOS_TSAN_ENABLED 1
#endif

#ifdef OCTOS_TSAN_ENABLED
extern "C" {
void __tsan_acquire(void* addr);
void __tsan_release(void* addr);
}
// Call on the producing thread after the object is fully constructed, right before handing it over.
#  define OCTOS_TSAN_RELEASE(addr) __tsan_release(static_cast<void*>(addr))
// Call as the first statement on the consuming thread.
#  define OCTOS_TSAN_ACQUIRE(addr) __tsan_acquire(static_cast<void*>(addr))
#else
#  define OCTOS_TSAN_RELEASE(addr) ((void)0)
#  define OCTOS_TSAN_ACQUIRE(addr) ((void)0)
#endif

#endif  // OCTOS_TSAN_ANNOTATIONS_H
