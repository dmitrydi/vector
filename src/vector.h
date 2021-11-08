
#include <cstddef>
#include <utility>
#include <memory>
#include <iostream>

template <typename T>
struct RawMemory {
	T *buf = nullptr;
	size_t cp = 0;
	static T* Allocate(size_t n) {
		return static_cast<T*>(operator new(n * sizeof(T)));
	}
	static void Deallocate(T* buf) {
		operator delete (buf);
	}

	RawMemory() = default;

	RawMemory(size_t n) {
		buf = Allocate(n);
		cp = n;
	}

	RawMemory(const RawMemory&) = delete;

	RawMemory(RawMemory&& other) {
		Swap(other);
	}

	~RawMemory() {
		Deallocate(buf);
	}

	RawMemory& operator=(const RawMemory&) = delete;

	RawMemory& operator=(RawMemory&& other) {
		Swap(other);
		return *this;
	}

	T* operator+(size_t n) {
		return buf + n;
	}

	const T* operator+(size_t n) const {
		return buf + n;
	}

	T& operator[](size_t n) {
		return buf[n];
	}

	const T& operator[](size_t n) const {
		return buf[n];
	}
	void Swap(RawMemory& other) {
		std::swap(buf, other.buf);
		std::swap(cp, other.cp);
	}
};

template <typename T>
class Vector {
private:
	RawMemory<T> data;
	size_t sz = 0;
	static void Construct(void* buf) {
		auto x = new (buf) T;
	}
	static void Construct(void *buf, const T& elem) {
		new (buf) T(elem);
	}
	static void Destroy(T* buf) {
		buf->~T();
	}

	  void Swap(Vector& other) {
		  data.Swap(other.data);
		  std::swap(sz, other.sz);
	  }
public:
  Vector() = default;
  Vector(size_t n): data(n) {
	  std::uninitialized_value_construct_n(data.buf, n);
	  sz = n;
  }
  Vector(const Vector& other): data(other.sz) {
	  std::uninitialized_copy_n(other.data.buf, other.sz, data.buf);
	  sz = other.sz;
  }
  Vector(Vector&& other): data(other.sz) {
	  Swap(other);
  }

  ~Vector() {
	  std::destroy_n(data.buf, sz);
  }

  Vector& operator = (const Vector& other) {
	  if (other.sz > data.cp) {
		  Vector tmp(other);
		  Swap(tmp);
	  } else {
		  for (size_t i = 0; i < sz && i < other.sz; ++i) {
			  data[i] = other[i];
		  }
		  if (sz < other.sz) {
			  std::uninitialized_copy_n(other.data.buf + sz , other.sz - sz, data.buf + sz);
		  } else if (sz > other.sz) {
			  std::destroy_n(data.buf + other.sz, sz - other.sz);
		  }
		  sz = other.sz;
	  }
	  return *this;
  }

  Vector& operator = (Vector&& other) noexcept {
	  Swap(other);
	  return *this;
  }

  void Reserve(size_t n) {
	  if (n > data.cp) {
		  RawMemory<T> data2(n);
		  std::uninitialized_move_n(data.buf, sz, data2.buf);
		  std::destroy_n(data.buf, sz);
		  data.Swap(data2);
	  }
  }

  void Resize(size_t n) {
	  Reserve(n);
	  if (sz < n) {
		  std::uninitialized_value_construct_n(data.buf + sz, n - sz);
	  } else {
		  std::destroy_n(data + n, sz - n);
	  }
	  sz = n;
  }

  void PushBack(const T& elem) {
	  if (sz == data.cp) {
		  Reserve(sz == 0 ? 1 : sz*2);
	  }
	  new (data + sz) T(elem);
	  ++sz;
  }

  void PushBack(T&& elem) {
	  if (sz == data.cp) {
		  Reserve(sz == 0 ? 1 : sz*2);
	  }
	  new (data + sz) T(std::move(elem));
	  ++sz;
  }

  template <typename ... Args>
  T& EmplaceBack(Args&&... args) {
	  if (sz == data.cp) {
		  Reserve(sz == 0 ? 1 : sz*2);
	  }
	  auto ptr = new (data + sz) T(std::forward<Args>(args)...);
	  ++sz;
	  return *ptr;
  }

  void PopBack() {
	  std::destroy_at(data + sz - 1);
	  --sz;
  }

  size_t Size() const noexcept {
	  return sz;
  }

  size_t Capacity() const noexcept {
	  return data.cp;
  }

  const T& operator[](size_t i) const {
	  return data[i];
  }
  T& operator[](size_t i) {
	  return data[i];
  }

  // методы с использованием итераторов
  using iterator = T*;
  using const_iterator = const T*;

  iterator begin() noexcept {
	  return data.buf;
  }

  iterator end() noexcept {
	  return data + sz;
  }

  const_iterator begin() const noexcept {
	  return data.buf;
  }

  const_iterator end() const noexcept {
	  return data + sz;
  }

  const_iterator cbegin() const noexcept {
	  return data.buf;
  }

  const_iterator cend() const noexcept {
	  return data + sz;
  }

  // Вставляет элемент перед pos
  // Возвращает итератор на вставленный элемент
  iterator Insert(const_iterator pos, const T& elem) {
	  size_t dpos = pos - begin();
	  if (sz == data.cp) {
		  Reserve(sz + 1);
	  }
	  auto it = data + sz - 1;
	  for (size_t i = sz; i != dpos; --i, --it) {
		  std::uninitialized_move_n(it, 1, it + 1);
		  std::destroy_at(it);
	  }
	  auto ptr = new(data + dpos) T(elem);
	  ++sz;
	  return ptr;
  }

  iterator Insert(const_iterator pos, T&& elem) {
	  size_t dpos = pos - begin();
	  if (sz == data.cp) {
		  Reserve(sz + 1);
	  }
	  auto it = end();
	  for (size_t i = sz; i != dpos; --i, --it) {
		  std::uninitialized_move_n(it-1, 1, it);
		  std::destroy_at(it-1);
	  }

	  auto ptr = new(data + dpos) T(std::move(elem));
	  ++sz;
	  return ptr;
  }

  // Конструирует элемент по заданным аргументам конструктора перед pos
  // Возвращает итератор на вставленный элемент
  template <typename ... Args>
  iterator Emplace(const_iterator it, Args&&... args) {
	  size_t dpos = it - begin();
	  if (sz == data.cp) {
		  Reserve(sz + 1);
	  }
	  auto _it = data + sz - 1;
	  for (size_t i = sz; i != dpos; --i, --_it) {
		  std::uninitialized_move_n(_it, 1, _it + 1);
		  std::destroy_at(_it);
	  }
	  auto ptr = new(data + dpos) T(std::forward<Args>(args)...);
	  ++sz;
	  return ptr;
  }

  // Удаляет элемент на позиции pos
  // Возвращает итератор на элемент, следующий за удалённым
  iterator Erase(const_iterator it) {
	  size_t dpos = it - begin();
	  std::move(data+dpos+1, end(), data+dpos);
	  PopBack();
	  return data + dpos;
  }
};
