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
#include <initializer_list>

#include <boost/foreach.hpp>
#include <boost/range/combine.hpp>

#include "timsort.hh"
#include "general.hh"

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
  public:
    typedef std::pair<K, V*> KV;
    typedef const std::pair<const K, const V*> ConstKV;
    typedef typename std::vector<KV>::iterator iterator;
    typedef typename std::vector<KV>::const_iterator const_iterator;
    typedef typename std::vector<KV>::reverse_iterator reverse_iterator;
    typedef typename std::vector<KV>::const_reverse_iterator const_reverse_iterator;

  private:
    std::vector<KV> m_keys;
    std::vector<V> m_vals;
    bool m_sorted = true;
    bool m_deepSorted = true;

    iterator rawFind(const K& key);

    template <typename IterK, typename IterV>
    void zipAppend(const IterK& keys, const IterV& vals);

    template <bool noReplace=false>
    void rawAppend(const K& key, const V& val); //The promise not to change the original is maintained, but I need a non const copy internally

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

    /*
     * I could create my own iterator that rewraps the <K, V*> into <K, V&>,
     * but that will slow things down since I need to hold the actual key
     * iterator and then every time I increment rewrap into the new <K, V&>.
     *
     * I decided that if I wanted a high performance data structure that
     * would be silly. Open to arguments...
     * I also may just not be good at making iterators...
     */
    /*
    struct iterator {
          iterator(KVIt&& rawIt, KVIt&& end) :
            it(rawIt->first, *(rawIt->second)), keyIt(rawIt), end(end) {}

          std::pair<K, V&> operator*() { return it; };
          const std::pair<K, V&>* operator->() const { return &it; };

          iterator& operator++() { //prefix
            ++keyIt;
            if (keyIt == end)
              return *this;
            it.first = keyIt->first;
            it.second = *(keyIt->second);
            return *this;
          }

          iterator operator++(int) { //postfix
            iterator temp = *this;
            ++(*this);
            return temp;
          }

          bool operator==(const iterator& other) const { return (keyIt == other.keyIt); };
          bool operator!=(const iterator& other) const { return (keyIt != other.keyIt); };

          std::pair<K, V&> it;
          KVIt keyIt;
          const KVIt end;
        };
        */

    iterator begin() { return m_keys.begin(); }
    const_iterator begin() const { return m_keys.begin(); }
    iterator end() { return m_keys.end(); }
    const_iterator end() const { return m_keys.end(); }
    reverse_iterator rbegin() { return m_keys.rbegin(); }
    const_reverse_iterator rbegin() const  { return m_keys.rbegin(); }
    reverse_iterator rend() { return m_keys.rend(); }
    const_reverse_iterator rend() const { return m_keys.rend(); }

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
    iterator find(const K& key) {return rawFind(key); };
    const const_iterator find(const K& key) const { return rawFind(key); };
    V& operator[](const K& key);
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

    void batchInsert(const std::initializer_list<std::pair<K, V>>&& pairs);

    template <typename IterK, typename IterV>
    void batchInsert(const IterK& keys, const IterV& vals);

    void append(const K& key, const V& val);

    void append(const std::pair<K, V>& kv) { append(kv.first, kv.second); };

    template <typename Iter>
    void batchAppend(const Iter& pairs);

    void batchAppend(const std::initializer_list<std::pair<K, V>>&& pairs);

    template <typename IterK, typename IterV>
    void batchAppend(const IterK& keys, const IterV& vals);

    template <bool enforceSorted=enforceSortedOnRemove>
    bool remove(const K& key);
    //TODO: If the user can pass in val, that will make things faster since won't have to search. overload

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
  bool BigMap<K, V, enforceSortedOnRemove>::hasVal(const V& val) const {
    return std::find(m_vals.begin(), m_vals.end(), val) != m_vals.end();
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  V& BigMap<K, V, enforceSortedOnRemove>::operator[](const K& key) {
    iterator it = rawFind(key);
    if (it == end()) {
      if (m_sorted) {
        insert(key, V());
        return *(rawFind(key)->second);
      } else {
        rawAppend<true>(key, V());
        return m_vals.back();
      }
    }
    return *(it->second);
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  void BigMap<K, V, enforceSortedOnRemove>::reserve(const int n) {
    m_keys.reserve(n);
    m_vals.reserve(n);
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  void BigMap<K, V, enforceSortedOnRemove>::insert(const K& key, const V& val) {
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
  void BigMap<K, V, enforceSortedOnRemove>::batchInsert(const std::initializer_list<std::pair<K, V>>&& pairs) {
    for (const std::pair<K, V>& it : pairs) {
      rawAppend(it);
    }
    sort();
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  bool BigMap<K, V, enforceSortedOnRemove>::replace(const K& key, const V& val) {
    BigMap::iterator it = rawFind(key);
    if (it == m_keys.end())
      return false;

    *(it->second) = const_cast<V&>(val); //The promise not to change the original is maintained, but I need a non const copy internally
    return true;
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
  template <bool noReplace>
  void BigMap<K, V, enforceSortedOnRemove>::rawAppend(const K& key, const V& val) {
    if (!noReplace && replace(key, val))
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
  void BigMap<K, V, enforceSortedOnRemove>::batchAppend(const std::initializer_list<std::pair<K, V>>&& pairs) {
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

    iterator it = rawFind(key);
    std::cout << it->first << ' ' << it->second << ' ' << *(it->second) << std::endl;

    if (it == m_keys.end())
        return false;

    if (unlikely(m_keys.back().first == key)) {
      std::cout << "1" << std::endl;
      KV& backKey = m_keys.back();
      const V& backVal = m_vals.back();
      if (likely(backKey.second != &backVal)) {
        for (auto& kv : m_keys) {
          if (unlikely(kv.second == &backVal)) {
            *backKey.second = backVal;
            kv.second = backKey.second;
            break;
          }
        }
      }
      m_vals.pop_back();
      m_keys.pop_back();
    } else if (enforceSorted && m_sorted && m_deepSorted) {
      std::cout << "2" << std::endl;
      const iterator end = m_keys.end();
      for(; it != (end-1); it++) {
        it->first = (it+1)->first;
        *(it->second) = *((it+1)->second);
      }
      m_keys.pop_back();
      m_vals.pop_back();
    } else if (enforceSorted && m_sorted) {
      std::cout << "3" << std::endl;
      const V* pBackVal = &m_vals.back();
      int backValKInd = -1;
      const int valKInd = std::distance(m_keys.begin(), it);
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
        KV& kv = *it;
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

      KV& backVal = *std::find_if(m_keys.begin(), m_keys.end(), checkVal);
      *(it->second) = *backVal.second;
      it->first = backVal.first;
      backVal = m_keys.back();
      m_vals.pop_back();
      m_keys.pop_back();
    }
    return true;
  }

  template <typename K, typename V, bool enforceSortedOnRemove>
  typename BigMap<K, V, enforceSortedOnRemove>::iterator BigMap<K, V, enforceSortedOnRemove>::rawFind(const K& key) {
    const iterator beg = m_keys.begin();
    const iterator end = m_keys.end();
    if (m_sorted) {
      const auto& it = std::lower_bound(beg, end, key, lowerKeyComp);
      if (it != end && it->first == key)
        return it;
      else
        return end;
    }

    for (auto it = beg; it != end; it++) {
      if (unlikely(it->first==key))
        return it;
    }

    return end; //Hope the constness isn't a problem
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

#undef unlikely
#undef likely
