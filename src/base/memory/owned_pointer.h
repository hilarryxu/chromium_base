#ifndef BASE_OWNED_POINTER_H_
#define BASE_OWNED_POINTER_H_

namespace base {

template<typename T>
struct owned_pointer {
  explicit operator bool() const { return value; }

  owned_pointer() = default;
  owned_pointer(T* source) { value = source; }
  owned_pointer(const owned_pointer& source) { value = source.value; source.reset(); }

  ~owned_pointer() { delete value; }

  auto& operator=(T* source) {
    delete value;
    value = source;
    return *this;
  }
  auto& operator=(const owned_pointer& source) {
    delete value;
    value = source.value;
    source.reset();
    return *this;
  }

  auto operator->() -> T* { return value; }
  auto operator->() const -> const T* { return value; }

  auto operator*() -> T& { return *value; }
  auto operator*() const -> const T& { return *value; }

  auto reset() const -> void { value = nullptr; }

  auto data() -> T* { return value; }
  auto data() const -> const T* { return value; }

private:
  mutable T* value = nullptr;
};

}  // namespace base

#endif  // BASE_OWNED_POINTER_H_
