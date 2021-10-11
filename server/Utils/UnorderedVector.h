#pragma once

#include <vector>

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
  // Return 'true' if array size has increased by 1 eventually.

  void remove(size_t index);
  // Swap element with the specified 'index' with the last
  // element of the array and remove the last element. This operation
  // should have O(1) complicity.

  size_t removeAll(const T& value);
  // Remove all elements with he specified 'value'. This operation
  // has O(n) complicity since you need to iterate through the
  // whole array. Return total number of removed elements.

  const std::vector<T>& data() const { return m_data; }
        std::vector<T>& data()       { return m_data; }

private:
  std::vector<T> m_data;

};

template<typename T>
bool UnorderedVector<T>::push(const T& item, bool checkIfExists)
{
  if (checkIfExists) {
    for (const T& element: m_data) {
      if (item == element) {
        // Already exist
        return false;
      }
    }
  }
  m_data.push_back(item);
  return true;
}

template<typename T>
void UnorderedVector<T>::remove(size_t index)
{
  m_data[index] = std::move(m_data.back());
  m_data.pop_back();
}

template<typename T>
size_t UnorderedVector<T>::removeAll(const T& value)
{
  size_t nTotal = 0;
  bool lRemoved = false;
  for (size_t i = 0; i < m_data.size(); i += lRemoved ? 0 : 1) {
    lRemoved = m_data[i] == value;
    if (lRemoved) {
      m_data[i] = std::move(m_data.back());
      m_data.pop_back();
      ++nTotal;
    }
  }
  return nTotal;
}

} // namespace utils
