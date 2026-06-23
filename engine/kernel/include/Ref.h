#pragma once

namespace arch::core::kernel {

template <typename T> class Ref {
public:
  Ref() : _ptr(nullptr) {}
  Ref(T *ptr) : _ptr(ptr) {}

  // Allow conversion: Ref<T> to Ref<const T>
  template <typename U> Ref(const Ref<U> &other) : _ptr(other.get()) {}

  T *operator->() const { return _ptr; }
  T &operator*() const { return *_ptr; }

  T *get() const { return _ptr; }

  explicit operator bool() const { return _ptr != nullptr; }

  bool operator==(const Ref &other) const { return _ptr == other._ptr; }
  bool operator!=(const Ref &other) const { return _ptr != other._ptr; }
  bool operator==(std::nullptr_t) const { return _ptr == nullptr; }
  bool operator!=(std::nullptr_t) const { return _ptr != nullptr; }

private:
  T *_ptr;
};

} // namespace arch::core::kernel