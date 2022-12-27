#ifndef PEONY_ARRAY_VIEW_HXX
#define PEONY_ARRAY_VIEW_HXX

#include <algorithm>
#include <vector>

template<class T>
class PArrayView
{
public:
  using reference = T&;
  using const_reference = const T&;
  using iterator = T*;
  using const_iterator = const T*;

  PArrayView()
    : m_data(nullptr)
    , m_size(0)
  {
  }
  PArrayView(T* p_data, size_t p_size)
    : m_data(p_data)
    , m_size(p_size)
  {
  }
  PArrayView(T* p_begin, T* p_end)
    : m_data(p_begin)
    , m_size(std::distance(p_begin, p_end))
  {
  }
  template<class U>
  PArrayView(const std::vector<U>& p_container)
    : PArrayView((T*)p_container.data(), p_container.size())
  {
  }

  [[nodiscard]] constexpr bool empty() const { return m_size == 0; }
  [[nodiscard]] constexpr size_t size() const { return m_size; }

  [[nodiscard]] constexpr T* data() { return m_data; }
  [[nodiscard]] constexpr const T* data() const { return m_data; }

  [[nodiscard]] constexpr T& front() { return *m_data; }
  [[nodiscard]] constexpr const T& front() const { return *m_data; }
  [[nodiscard]] constexpr T& back() { return *(m_data + m_size - 1); }
  [[nodiscard]] constexpr const T& back() const { return *(m_data + m_size - 1); }

  [[nodiscard]] constexpr T& operator[](size_t p_i) { return m_data[p_i]; }
  [[nodiscard]] constexpr const T& operator[](size_t p_i) const { return m_data[p_i]; }

  [[nodiscard]] constexpr iterator begin() { return m_data; }
  [[nodiscard]] constexpr const_iterator begin() const { return m_data; }
  [[nodiscard]] constexpr const_iterator cbegin() const { return m_data; }
  [[nodiscard]] constexpr iterator end() { return m_data + m_size; }
  [[nodiscard]] constexpr const_iterator end() const { return m_data + m_size; }
  [[nodiscard]] constexpr const_iterator cend() const { return m_data + m_size; }

  [[nodiscard]] constexpr iterator find(const T& p_value) { return std::find(begin(), end(), p_value); }
  [[nodiscard]] constexpr const_iterator find(const T& p_value) const { return std::find(begin(), end(), p_value); }
  [[nodiscard]] constexpr bool contains(const T& p_value) const { return std::find(begin(), end(), p_value) != end(); }

  template<class Predicate>
  [[nodiscard]] constexpr iterator find_if(Predicate p_f)
  {
    return std::find_if(begin(), end(), p_f);
  }
  template<class Predicate>
  [[nodiscard]] constexpr const_iterator find_if(Predicate p_f) const
  {
    return std::find_if(begin(), end(), p_f);
  }

private:
  T* m_data;
  size_t m_size;
};

template<class T>
bool
operator==(const PArrayView<T>& p_lhs, const PArrayView<T>& p_rhs)
{
  if (p_lhs.size() != p_rhs.size())
    return false;
  return std::equal(p_lhs.begin(), p_lhs.end(), p_rhs.begin());
}

#endif // PEONY_ARRAY_VIEW_HXX
