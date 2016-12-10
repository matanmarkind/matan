/*
 * This is an attempt at a high performance data structure to replace std::map.
 * Inspired by - CppCon 2014: Chandler Carruth "Efficiency with Algorithms, Performance with Data Structures"
 *
 * The idea is to take advantage of sequential memory by using a vector as the
 * basic data structure. The reason for this being Big is that I am going to
 * try to tailor this to values with large sizeof. I am making a bet that we
 * will mostly
 * want to interact with the keys for a map (sorting & searching) and
 * so it will be more efficient if I can keep the vector with keys small
 * and put the large values elsewhere in memory so that more keys can be
 * put into cache.
 */
//TODO: SmallMap.hh - everything in 1 vector
//TODO: HugeMap.hh - values are so big/so disinteresting that we store in list so waste no time with reallocating vals
#include <vector>
#include <iostream>
#include <tuple>
#include <algorithm>
#include <iterator>
#include <cassert>
#include <cstring>
#include <functional>
#include <utility>
#include <cmath>

#include <boost/foreach.hpp>
#include <boost/range/combine.hpp>

#include "timsort.hh"

#define likely(x)    __builtin_expect (!!(x), 1)
#define unlikely(x)  __builtin_expect (!!(x), 0)

namespace matan {
  template <typename K,
            typename V,
            bool enforceSortedOnRemove=true>
  class BigMap {
    //TODO: Use concepts to guarantee K implements operator< and Iter is ForwardIterable I think sorting requires RamdomAccessIterator
    //TODO: need to be able to remove elements
    /*
     * I selected timsort for this because it is optimized for partially
     * sorted data, which is perfect when I am adding to a map that
     * is already sorted
     *
     * Can't use a reference to val since it isn't safe for sorting.
     * std::reference_wrapper doesn't actually work properly, doesn't properly
     * mimic the behavior of a reference. So using a pointer. This matters
     * if you want to iterate through.
     *
     * Not using linked list for vals since that would mean iterating
     * through vals would have no sequential memory benefits and then
     * we might as well just be using a hashmap.
     *
     */

  private:
    typedef std::pair<K, V*> KV;
    typedef const std::pair<const K, const V*> ConstKV;
    std::vector<KV> m_keys;
    std::vector<V> m_vals;
    bool m_sorted = true;
    bool m_deepSorted = true;

    template <typename IterK, typename IterV>
    void zipAppend(const IterK& keys, const IterV& vals);

    void rawAppend(const K& key, const V& val);

    void rawAppend(const std::pair<const K, const V>& kv) {
      rawAppend(kv.first, kv.second);
    };

    bool replace(const K& k, const V& v);
    bool replace(const std::pair<const K, const V>& kv) {
      return replace(kv.first, kv.second);
    };

    static bool lowerKeyComp(const ConstKV& a, const K& b) { return a.first < b; };
    static bool keyComp(ConstKV& a, ConstKV& b) { return a.first < b.first; };
  public:
    BigMap() = default; //how to do default constructor/destructor??

    BigMap(const int n);

    template <typename Iter>
    BigMap(const Iter& pairs);

    template <typename IterK, typename IterV>
    BigMap(const IterK& keys, const IterV& vals);

    ~BigMap() = default;

    //TODO:perhaps I should construct a vector of references and have a  rawBegin that leaves the pointers in place for the user to handle
    typename std::vector<KV>::iterator begin() { return m_keys.begin(); }
    typename std::vector<KV>::const_iterator  begin() const  { return m_keys.begin(); }
    typename std::vector<KV>::iterator  end()  { return m_keys.end(); }
    typename std::vector<KV>::const_iterator  end() const  { return m_keys.end(); }
    typename std::vector<KV>::reverse_iterator  rbegin()  { return m_keys.rbegin(); }
    typename std::vector<KV>::const_reverse_iterator  rbegin() const  { return m_keys.rbegin(); }
    typename std::vector<KV>::reverse_iterator rend()  { return m_keys.rend(); }
    typename std::vector<KV>::const_reverse_iterator rend() const  { return m_keys.rend(); }

    typename std::vector<V>::iterator deepBegin() {return m_vals.begin(); }
    typename std::vector<V>::const_iterator deepBegin() const  { return m_vals.begin(); }
    typename std::vector<V>::iterator deepEnd()  { return m_vals.end(); }
    typename std::vector<V>::const_iterator deepEnd() const  { return m_vals.end(); }
    typename std::vector<V>::reverse_iterator deepRbegin()  { return m_vals.rbegin(); }
    typename std::vector<V>::const_reverse_iterator deepRbegin() const  { return m_vals.rbegin(); }
    typename std::vector<V>::reverse_iterator deepRend()  { return m_vals.rend(); }
    typename std::vector<V>::const_reverse_iterator deepRend() const  { return m_vals.rend(); }

    size_t size() const { return m_keys.size(); }
    bool isSorted() const { return m_sorted; }
    bool isDeepSorted() const { return m_deepSorted; }
    V& operator[](const K& key);
    const V& operator[] (const K& key) const;
    void reserve(const int n);

    void insert(const K& key, const V& val);

    void insert(const std::pair<const K, const V>& kv) {
      insert(kv.first, kv.second);
    }

    /*
     * appending is good if don't want to spend time sorting till later.
     *
     * The batch versions are slightly faster since calling them repeatedly
     * would (a) have the overhead of calling to the function repeatedly
     * and (b) would try to reset the bools every time.
     */
    template <typename Iter>
    void batchInsert(const Iter& pairs);

    template <typename IterK, typename IterV>
    void batchInsert(const IterK& keys, const IterV& vals);

    void append(const K& key, const V& val);

    void append(const std::pair<K, V>& kv) { append(kv.first, kv.second); };

    template <typename Iter>
    void batchAppend(const Iter& pairs);

    template <typename IterK, typename IterV>
    void batchAppend(const IterK& keys, const IterV& vals);

    template <bool enforceSorted=enforceSortedOnRemove>
    bool remove(const K& key);
    //TODO: If the user can pass in val, that will make things faster since won't have to search. overload

    bool hasKey(const K& key) const;
    bool hasVal(const V& val) const;
    void sort(); //sort m_keys
    void deepSort(); //reorder m_vals according to m_keys.

  };

  template <typename K, typename V, bool enforceSortedOnRemove>
  BigMap<K, V, enforceSortedOnRemove>::BigMap(const int n) {
    m_keys.reserve(n);
    m_vals.reserve(n);
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  template<typename Iter>
  BigMap<K, V, enforceSortedOnRemove>::BigMap(const Iter& pairs)  {
    m_keys.reserve(pairs.size());
    m_vals.reserve(pairs.size());
    batchInsert(pairs);
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  template <typename IterK, typename IterV>
  BigMap<K, V, enforceSortedOnRemove>::BigMap(const IterK& keys,
                       const IterV& vals) {
    m_keys.reserve(keys.size());
    m_vals.reserve(vals.size());
    batchInsert(keys, vals);
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  bool BigMap<K, V, enforceSortedOnRemove>::hasKey(const K& key) const {
    if (m_sorted)
      return std::binary_search(m_keys.begin(),
                                m_keys.end(),
                                ConstKV(key, nullptr),
                                keyComp);

    for (const KV& kv : m_keys) {
      if (unlikely(kv.first == key))
        return true;
    }
    return false;
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  bool BigMap<K, V, enforceSortedOnRemove>::hasVal(const V& val) const {
    return std::find(m_vals.begin(), m_vals.end(), val) != m_vals.end();
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  V& BigMap<K, V, enforceSortedOnRemove>::operator[](const K& key) {
    assert (hasKey(key));
    if (m_sorted)
      return *(*std::lower_bound(m_keys.begin(),
                                 m_keys.end(),
                                 key,
                                 lowerKeyComp)).second;
    for (KV& kv : m_keys) {
      if (unlikely(kv.first==key))
        return *(kv.second);
    }
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  const V& BigMap<K, V, enforceSortedOnRemove>::operator[](const K& key) const {
    assert (hasKey(key));
    if (m_sorted)
      return *(*std::lower_bound(m_keys.begin(),
                                 m_keys.end(),
                                 key,
                                 lowerKeyComp).second);
    for (const KV& kv : m_keys) {
      if (unlikely(kv.first==key))
        return *kv.second;
    }
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  void BigMap<K, V, enforceSortedOnRemove>::reserve(const int n) {
    m_keys.reserve(n);
    m_vals.reserve(n);
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  void BigMap<K, V, enforceSortedOnRemove>::insert(const K& key,
                            const V& val) {
    if (replace(key, val)) {
      if (!m_sorted)
        sort();
    } else if (!m_sorted) {
      m_vals.push_back(val);
      m_keys.push_back( KV(key, &m_vals.back()));
      sort();
    } else {
      m_vals.push_back(val);
      m_keys.resize(m_keys.size()+1);
      const auto breakpoint = std::lower_bound(m_keys.begin(),
                                               m_keys.end(),
                                               key,
                                               lowerKeyComp);
      int i = m_keys.size();
      for (auto it = m_keys.end(); it != breakpoint; it--, i--) {
        m_keys[i] = m_keys[i-1];
      }
      m_keys[i] = KV(key, &m_vals.back());
    }
    m_deepSorted = false;
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  template <typename Iter>
  void BigMap<K, V, enforceSortedOnRemove>::batchInsert(const Iter& pairs) {
    for (const std::pair<K, V>& it : pairs) {
      rawAppend(it);
    }
    sort();
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  bool BigMap<K, V, enforceSortedOnRemove>::replace(const K& key, const V& val) {
    if (hasKey(key)) {
      if (m_sorted) {
        V& ele = *((*std::lower_bound(m_keys.begin(),
                                                        m_keys.end(),
                                                        key,
                                                        lowerKeyComp)).second);
        ele = val;
      } else {
        V& ele = *((*std::find_if(m_keys.begin(),
                                                    m_keys.end(),
                                                    [key](ConstKV kv) {
                                                      return key == kv.first;
                                                    })).second);
        ele = val;
      }
      return true;
    }
    return false;
  };

  template <typename K, typename V, bool enforceSortedOnRemove>
  template <typename IterK, typename IterV>
  void BigMap<K, V, enforceSortedOnRemove>::batchInsert(const IterK& keys, const IterV& vals) {
    zipAppend(keys, vals);
    if (!m_sorted)
      sort();
    m_deepSorted = false;
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  void BigMap<K, V, enforceSortedOnRemove>::rawAppend(const K& key, const V& val) {
    if (replace(key, val))
        return;

    const bool reassign = (m_vals.size() == m_vals.capacity());
    if (reassign) {
      deepSort();
    }
    m_vals.push_back(val);
    if (reassign) {
      V* phead = &m_vals[0];
      const int size = m_keys.size();
      for (int i = 0; i < size; i++) {
        m_keys[i].second = phead+i;
      }
    }
    m_keys.push_back( KV(key, &m_vals.back()) );
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  void BigMap<K, V, enforceSortedOnRemove>::append(const K& key, const V& val) {
    rawAppend(key, val);
    m_sorted = false;
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  template <typename Iter>
  void BigMap<K, V, enforceSortedOnRemove>::batchAppend(const Iter& pairs) {
    for (const std::pair<K, V>& it : pairs) {
      rawAppend(it);
    }
    m_sorted = false;
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  template <typename IterK, typename IterV>
  void BigMap<K, V, enforceSortedOnRemove>::batchAppend(const IterK& keys, const IterV& vals) {
    zipAppend(keys, vals);
    m_sorted = false;
    m_deepSorted = false;
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  template <bool enforceSorted>
  bool BigMap<K, V, enforceSortedOnRemove>::remove(const K& key) {
    //I think always will maintain m_deepSorted if true so don't need the template parameter

    if (!hasKey(key))
        return false;

    if (unlikely(m_keys.back().first == key)) {
      std::cout << "1" << std::endl;
      KV& backKey = m_keys.back();
      V& backVal = m_vals.back();
      if (likely(backKey.second != &backVal)) {
        for (auto& kv : m_keys) {
          if (unlikely(kv.second == &backVal)) {
            *backKey.second = backVal;
            kv.second = backKey.second;
          }
        }
      }
      m_vals.pop_back();
      m_keys.pop_back();
    } else if (enforceSorted && m_sorted && m_deepSorted) {
      std::cout << "2" << std::endl;
      int valInd = std::abs(&((*this)[key]) - &(*m_vals.begin()));
      const int size = m_keys.size();
      for (; valInd < (size-1); valInd++) {
        m_keys[valInd].first = m_keys[valInd+1].first;
        m_vals[valInd] = m_vals[valInd+1];
      }
      m_keys.pop_back();
      m_vals.pop_back();
    } else if (enforceSorted && m_sorted) {
      std::cout << "3" << std::endl;
      const V* pBackVal = &m_vals.back();
      int backValKInd = -1;
      const int valKInd = std::distance(m_keys.begin(),
                                        std::lower_bound(m_keys.begin(),
                                                         m_keys.end(),
                                                         key,
                                                         lowerKeyComp));
      const int size = m_keys.size();
      for (int i=0; i < valKInd; i++) {
        if (unlikely(m_keys[i].second == pBackVal)) {
          backValKInd = i;
          m_keys[backValKInd].second = m_keys[valKInd].second;
          *(m_keys[backValKInd].second) = *pBackVal;
        }
      }
      for (int i=valKInd; i<(size-1); i++) {
        if (unlikely((backValKInd == -1) && (m_keys[i].second == pBackVal))) {
          backValKInd = i;
          m_keys[backValKInd].second = m_keys[valKInd].second;
          *(m_keys[backValKInd].second) = *pBackVal;
        }
        m_keys[i] = m_keys[i+1];
      }
      m_vals.pop_back();
      m_keys.pop_back();
    } else if (m_deepSorted) {
      std::cout << "4" << std::endl;
      if (m_sorted) {
        KV& kv = *std::lower_bound(m_keys.begin(),
                                   m_keys.end(),
                                   key,
                                   lowerKeyComp);
        *kv.second = m_vals.back();
        kv.first = m_keys.back().first;
        m_keys.pop_back();
        m_vals.pop_back();
      } else {
        for (KV& kv : m_keys) {
          if (unlikely(kv.first == key)) {
            *kv.second = m_vals.back();
            kv.first = m_keys.back().first;
            m_keys.pop_back();
            m_vals.pop_back();
            break;
          }
        }
      }
    } else {
      std::cout << "5" << std::endl;
      V* val = &m_vals.back();
      auto checkVal =[this, val](ConstKV kv) {
        return kv.second == val;
      };
      if (m_sorted) {
        KV& kv = *std::lower_bound(m_keys.begin(),
                                   m_keys.end(),
                                   key,
                                   lowerKeyComp);
        KV& backVal = *std::find_if(m_keys.begin(),
                                    m_keys.end(),
                                    checkVal);
        *kv.second = *backVal.second;
        kv.first = backVal.first;
        backVal = m_keys.back();
        m_vals.pop_back();
        m_keys.pop_back();
      } else {
        for (KV& kv : m_keys) {
          if (unlikely(kv.first == key)) {
            KV& backVal = *std::find_if(m_keys.begin(),
                                        m_keys.end(),
                                        checkVal);
            *kv.second = *backVal.second;
            kv.first = backVal.first;
            backVal = m_keys.back();
            m_vals.pop_back();
            m_keys.pop_back();
            break;
          }
        }
      }
    }
    return true;
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  void BigMap<K, V, enforceSortedOnRemove>::sort() {
    matan::timsort(m_keys.begin(), m_keys.end(), keyComp);
    m_sorted = true;
    m_deepSorted = false;
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  void BigMap<K, V, enforceSortedOnRemove>::deepSort() {
    std::vector<V> sortedVals;
    sortedVals.reserve(m_vals.size());
    for (const auto& it : m_keys) {
      sortedVals.push_back(*it.second);
    }
    m_vals = std::move(sortedVals); //should this be a move?
    m_deepSorted = true;
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  template <typename IterK, typename IterV>
  void BigMap<K, V, enforceSortedOnRemove>::zipAppend(const IterK& keys, const IterV& vals) {
    BOOST_FOREACH(const auto kv, boost::combine(keys, vals)) {
      if (!(replace(boost::get<0>(kv), boost::get<1>(kv))))
        rawAppend(boost::get<0>(kv), boost::get<1>(kv));
    }
  }
} //namespace matan
