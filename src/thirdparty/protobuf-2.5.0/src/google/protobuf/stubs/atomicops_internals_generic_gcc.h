// Protocol Buffers - Google's data interchange format
// Atomic operations on top of the GCC/Clang __atomic builtins, for architectures protobuf 2.5.0 has no hand-written
// implementation for (AArch64: Android ARM64). Later protobuf releases ship the same approach
// (atomicops_internals_generic_gcc.h). Do not include this file directly; include atomicops.h.

#ifndef GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_GENERIC_GCC_H_
#define GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_GENERIC_GCC_H_

namespace google {
namespace protobuf {
namespace internal {

inline void MemoryBarrier() {
  __atomic_thread_fence(__ATOMIC_SEQ_CST);
}

#define GOOGLE_PROTOBUF_GENERIC_ATOMICOPS(T)                                          \
inline T NoBarrier_CompareAndSwap(volatile T* ptr, T old_value, T new_value) {        \
  __atomic_compare_exchange_n(ptr, &old_value, new_value, false,                      \
                              __ATOMIC_RELAXED, __ATOMIC_RELAXED);                    \
  return old_value;                                                                   \
}                                                                                     \
inline T NoBarrier_AtomicExchange(volatile T* ptr, T new_value) {                     \
  return __atomic_exchange_n(ptr, new_value, __ATOMIC_RELAXED);                       \
}                                                                                     \
inline T NoBarrier_AtomicIncrement(volatile T* ptr, T increment) {                    \
  return __atomic_add_fetch(ptr, increment, __ATOMIC_RELAXED);                        \
}                                                                                     \
inline T Barrier_AtomicIncrement(volatile T* ptr, T increment) {                      \
  return __atomic_add_fetch(ptr, increment, __ATOMIC_SEQ_CST);                        \
}                                                                                     \
inline T Acquire_CompareAndSwap(volatile T* ptr, T old_value, T new_value) {          \
  __atomic_compare_exchange_n(ptr, &old_value, new_value, false,                      \
                              __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE);                    \
  return old_value;                                                                   \
}                                                                                     \
inline T Release_CompareAndSwap(volatile T* ptr, T old_value, T new_value) {          \
  __atomic_compare_exchange_n(ptr, &old_value, new_value, false,                      \
                              __ATOMIC_RELEASE, __ATOMIC_RELAXED);                    \
  return old_value;                                                                   \
}                                                                                     \
inline void NoBarrier_Store(volatile T* ptr, T value) {                               \
  __atomic_store_n(ptr, value, __ATOMIC_RELAXED);                                     \
}                                                                                     \
inline void Acquire_Store(volatile T* ptr, T value) {                                 \
  __atomic_store_n(ptr, value, __ATOMIC_RELAXED);                                     \
  __atomic_thread_fence(__ATOMIC_SEQ_CST);                                            \
}                                                                                     \
inline void Release_Store(volatile T* ptr, T value) {                                 \
  __atomic_store_n(ptr, value, __ATOMIC_RELEASE);                                     \
}                                                                                     \
inline T NoBarrier_Load(volatile const T* ptr) {                                      \
  return __atomic_load_n(ptr, __ATOMIC_RELAXED);                                      \
}                                                                                     \
inline T Acquire_Load(volatile const T* ptr) {                                        \
  return __atomic_load_n(ptr, __ATOMIC_ACQUIRE);                                      \
}                                                                                     \
inline T Release_Load(volatile const T* ptr) {                                        \
  __atomic_thread_fence(__ATOMIC_SEQ_CST);                                            \
  return __atomic_load_n(ptr, __ATOMIC_RELAXED);                                      \
}

GOOGLE_PROTOBUF_GENERIC_ATOMICOPS(Atomic32)
#ifdef GOOGLE_PROTOBUF_ARCH_64_BIT
GOOGLE_PROTOBUF_GENERIC_ATOMICOPS(Atomic64)
#endif

#undef GOOGLE_PROTOBUF_GENERIC_ATOMICOPS

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_ATOMICOPS_INTERNALS_GENERIC_GCC_H_
