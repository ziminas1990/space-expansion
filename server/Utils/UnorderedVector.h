#pragma once

#include <vector>
#include <assert.h>

namespace utils {

// Elements of unordered vector has undefined order during iteration
// through the vector. But in turn removal of the element by it's
// index has O(1) complicity
template<typename T>
class UnorderedVector
{
public:

  const T& operator[](size_t index) const { return m_data[index]; }
  T&       operator[](size_t index)       { return m_data[index]; }
  size_t   size() const { return m_data.size(); }

  bool push(const T& item, bool checkIfExists = true);
  // Add the specified 'item' to the end of array. Do nothing if the
  // same element is already added to the array and the optionally
  // specified 'checkIfExists' is 'true'.
  // Return 'true' if array size has been increased by 1 eventually.
  // Complicity: O(1) if 'checkIfExists' is 'false', otherwise O(N)

  void remove(size_t index);
  // Swap element with the specified 'index' with the last
  // element of the array and remove the last element. This operation
  // has O(1) complicity.

  size_t find(const T& item) const;
  // Return index of the specified 'item'. If 'item' is not found
  // return container's size.

  bool has(const T& item) const;
  // Return true if the specified 'item' is in vector.

  size_t removeAll(const T& value);
  // Remove all elements with he specified 'value'. This operation
  // has O(n) complicity since you need to iterate through the
  // whole array. Return total number of removed elements.

  bool removeFirst(const T& value);
  // Remove first found element with the specified 'value'. Return
  // 'true' if element has been removed, otherwise return false.

  const std::vector<T>& data() const { return m_data; }
        std::vector<T>& data()       { return m_data; }

private:
  std::vector<T> m_data;

};

template<typename T>
bool UnorderedVector<T>::push(const T& item, bool checkIfExists)
{
  if (checkIfExists && find(item) != m_data.size()) {
    // Already exist
    return false;
  }
  m_data.push_back(item);
  return true;
}

template<typename T>
void UnorderedVector<T>::remove(size_t index)
{
  if (index < m_data.size() - 1) {
    m_data[index] = std::move(m_data.back());
  }
  m_data.pop_back();
}

template<typename T>
size_t UnorderedVector<T>::find(const T& item) const
{
  for (size_t i = 0; i < m_data.size(); ++i) {
    if (m_data[i] == item) {
      return i;
    }
  }
  return m_data.size();
}

template<typename T>
bool UnorderedVector<T>::has(const T& item) const
{
  return find(item) < m_data.size();
}

template<typename T>
size_t UnorderedVector<T>::removeAll(const T& value)
{
  size_t nTotal = 0;
  bool lRemoved = false;
  for (size_t i = 0; i < m_data.size(); i += lRemoved ? 0 : 1) {
    lRemoved = m_data[i] == value;
    if (lRemoved) {
      if (i < m_data.size() - 1) {
        m_data[i] = std::move(m_data.back());
      }
      m_data.pop_back();
      ++nTotal;
    }
  }
  return nTotal;
}

template<typename T>
bool UnorderedVector<T>::removeFirst(const T& value)
{
  for (size_t i = 0; i < m_data.size(); ++i) {
    if (m_data[i] == value) {
      if (i < m_data.size() - 1) {
        m_data[i] = std::move(m_data.back());
      }
      m_data.pop_back();
      return true;
    }
  }
  return false;
}

} // namespace utils
