/******************************************************************************
/ flat_set.hpp
/
/ Copyright (c) 2024 Christian Fillion
/ https://cfillion.ca
/
/ Permission is hereby granted, free of charge, to any person obtaining a copy
/ of this software and associated documentation files (the "Software"), to deal
/ in the Software without restriction, including without limitation the rights to
/ use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
/ of the Software, and to permit persons to whom the Software is furnished to
/ do so, subject to the following conditions:
/
/ The above copyright notice and this permission notice shall be included in all
/ copies or substantial portions of the Software.
/
/ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/ EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/ OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/ NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/ HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/ WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/ FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/ OTHER DEALINGS IN THE SOFTWARE.
/
******************************************************************************/

#pragma once

#include <algorithm>
#include <vector>

// intended as a (mostly) drop-in replacement of std::set
// with values stored in contiguous memory instead of in a tree
template<typename T>
class FlatSet {
public:
  // const_iterator like std::set
  // values are sorted internally and the sort key shouldn't be changed
  using iterator = typename std::vector<T>::const_iterator;

  iterator begin() const { return m_data.begin(); }
  iterator end()   const { return m_data.end();   }
  size_t   size()  const { return m_data.size();  }
  bool     empty() const { return m_data.empty(); }

  iterator find(const T &v) const
  {
    auto it { lowerBound(v) };
    return it == end() || !(v < *it) ? it : end();
  }

  void reserve(size_t s) { m_data.reserve(s); }
  std::pair<iterator, bool> insert(const T &v) // std::move is C++11
  {
    auto it { lowerBound(v) };
    if(it != end() && !(v < *it))
      return std::make_pair(it, false);
    return std::make_pair(m_data.insert(unconstIterator(it), v), true);
  }

  void clear() { return m_data.clear(); }
  iterator erase(iterator it) { return m_data.erase(unconstIterator(it)); }
  template<typename F> void erase_if(F pred)
  {
    for(auto it { m_data.begin() }; it != m_data.end();) {
      if(pred(*it))
        it = m_data.erase(unconstIterator(it));
      else
        ++it;
    }
  }

private:
  iterator lowerBound(const T &v) const
  {
    return std::lower_bound(begin(), end(), v);
  }

  // for compat with pre-C++11
  typename std::vector<T>::iterator unconstIterator(iterator cit)
  {
    return m_data.begin() + std::distance(begin(), cit);
  }

  std::vector<T> m_data;
};
