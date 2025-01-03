#ifndef ADS_SET_H
#define ADS_SET_H

#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <bitset>

template <typename Key, size_t N = 7>
class ADS_set {
public:
  class Iterator;
  using value_type = Key;
  using key_type = Key;
  using reference = value_type &;
  using const_reference = const value_type &;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using const_iterator = Iterator;
  using iterator = const_iterator;
  //using key_compare = std::less<key_type>;                         // B+-Tree
  using key_equal = std::equal_to<key_type>;                       // Hashing
  using hasher = std::hash<key_type>;                              // Hashing
private:
  struct Bucket;
  struct HashTable;
  HashTable hashTable;
  size_type numOfElements;
public:
  ADS_set(): hashTable{new Bucket*[4]}, numOfElements{0} {
    hashTable.buckets[0] = new Bucket;
    hashTable.buckets[1] = new Bucket;
  }                         // PH1

  ADS_set(std::initializer_list<key_type> ilist): ADS_set{std::begin(ilist),std::end(ilist)} {}                   // PH1
 
  template<typename InputIt> 
  ADS_set(InputIt first, InputIt last): ADS_set{} {
    insert(first, last);
  }     // PH1

  ADS_set(const ADS_set &other): ADS_set(other.begin(), other.end()) {}

  ~ADS_set() {}

  ADS_set &operator=(const ADS_set &other) {
    if (this == &other) return *this;
    ADS_set temp{other};
    swap(temp);
    return *this;
  }
  
  ADS_set &operator=(std::initializer_list<key_type> ilist) {
    ADS_set tmp{ilist};
    swap(tmp);
    return *this;
  }

  size_type size() const {
    return numOfElements;
  }

  bool empty() const {
    return size() == 0;
  }

  void insert(std::initializer_list<key_type> ilist) {
    insert(std::begin(ilist),std::end(ilist));
  }                  // PH1


  std::pair<iterator,bool> insert(const key_type &key) {
    size_t x = static_cast<size_t>(hashTable.getIndex(key));
    size_t y {0};
    
    bool end {false};
    Bucket* b = hashTable.buckets[x];

    while (!end) {
      for (size_type i = 0; i < b->bucketSize; ++i) {
        if (key_equal{}(b->entries[i], key)) {
          Iterator newIt(hashTable.buckets, (1u << hashTable.roundNumber) + hashTable.nextToSplit, x, y, i);
          return std::make_pair(newIt, false);
        }
      }
      if (b->nextBucket == nullptr) {
        end = true;
      } else {
        y++;
        b = b->nextBucket;
      }
    }
    
    numOfElements++;
    if(b->append(key)) {
      hashTable.split();
      return std::make_pair(find(key), true);
    }

    Iterator newIt(hashTable.buckets, hashTable.tableSize, x, y, b->bucketSize-1);

    return std::make_pair(newIt, true);
  }

  template<typename InputIt> void insert(InputIt first, InputIt last) {
    for (InputIt it {first}; it != last; it++) {
      if (count(*it)) continue;
      hashTable.insert(*it);
      numOfElements++;
    }
  }

  void clear() {
    ADS_set temp;
    this->swap(temp);
  }

  size_type erase(const key_type &key) {
    if (hashTable.erase(key)) {
      numOfElements--;
      return 1;
    };
    
    return 0;
  };

  size_type count(const key_type &key) const {
    unsigned index = hashTable.getIndex(key);
    for (Bucket* b{hashTable.buckets[index]}; b != nullptr; b = b->nextBucket) {
      for (size_type i = 0; i < b->bucketSize; ++i) {
        if (key_equal{}(b->entries[i], key)) return 1;
      }
    }
    return 0;
  }                      

  iterator find(const key_type &key) const {
    size_t x = static_cast<size_t>(hashTable.getIndex(key));
    size_t y {0};

    for (Bucket* b{hashTable.buckets[x]}; b != nullptr; b = b->nextBucket) {
      for (size_type i = 0; i < b->bucketSize; ++i) {
        if (key_equal{}(b->entries[i], key)) {
          Iterator newIt(hashTable.buckets, hashTable.tableSize, x, y, i);
          return newIt;
        }
      }
      y++;
    }

    return this->end();
  }

  void swap(ADS_set &other) {
    std::swap(hashTable.buckets, other.hashTable.buckets);
    std::swap(hashTable.nextToSplit, other.hashTable.nextToSplit);
    std::swap(hashTable.roundNumber, other.hashTable.roundNumber);
    std::swap(hashTable.tableSize, other.hashTable.tableSize);
    std::swap(hashTable.tableMaxSize, other.hashTable.tableMaxSize);
    std::swap(numOfElements, other.numOfElements);
  }

  const_iterator begin() const {
    return const_iterator{hashTable.buckets, hashTable.tableSize};
  };
  const_iterator end() const {
    return const_iterator{};
  };

  void dump(std::ostream &o = std::cerr) const {
    o << "[size = " << numOfElements << "\n";
    for (size_type i{0}; i < hashTable.tableSize; i++) {
      std::string index{std::bitset<64>( i ).to_string()};
      if (i < hashTable.nextToSplit || i > ((1u << hashTable.roundNumber) - 1)) 
        index = index.substr(index.size() - (hashTable.roundNumber + 1));
      else index = ' ' + index.substr(index.size() - hashTable.roundNumber);
      o << index << " : ";

      Bucket* b{hashTable.buckets[i]};
      while (b != nullptr) {
        for (size_type j{0}; j < b->bucketSize; j++) {
          o << b->entries[j] << ' ';
        } 
        b = b->nextBucket;
        if (b != nullptr) o << "-> ";
      }

      o << "\n";
    }
  }

  friend bool operator==(const ADS_set &lhs, const ADS_set &rhs) {
    if (lhs.numOfElements != rhs.numOfElements) return false;
    for (const auto& key : lhs) {
      if (!rhs.count(key)) {
        return false;
      }
    };

    return true;
  }

  friend bool operator!=(const ADS_set &lhs, const ADS_set &rhs) {
    return !(lhs == rhs);
  }
};

template <typename Key, size_t N>
struct ADS_set<Key, N>::Bucket {
  size_type bucketSize{0};
  key_type entries[N]{};
  Bucket* nextBucket{nullptr};

  bool append(const key_type& key) {
    Bucket* curr = this;

    while (curr->bucketSize == N) {
      if (curr->nextBucket == nullptr) {
        curr->nextBucket = new Bucket;
        curr = curr->nextBucket;
        curr->entries[0] = key;
        curr->bucketSize++;
        return true;
      }

      curr = curr->nextBucket;
    }

    curr->entries[curr->bucketSize++] = key;
    return false;
  }
};


template <typename Key, size_t N>
struct ADS_set<Key, N>::HashTable {
  Bucket** buckets{nullptr};
  size_type roundNumber{1};
  size_type nextToSplit{0};
  size_type tableSize{2};
  size_type tableMaxSize{4};

  void deleteLinkedBuckets(Bucket* currentBucket) {
    if (currentBucket == nullptr) {
        return;
    }
    deleteLinkedBuckets(currentBucket->nextBucket);
    delete currentBucket;
  }

  unsigned getIndex(const key_type& key) const {
    unsigned index = static_cast<unsigned>(hasher{}(key)) & ((1u << roundNumber) - 1);
    if (index < nextToSplit) index = static_cast<unsigned>(hasher{}(key)) & ((1u << (roundNumber+1)) - 1);;
    return index;
  }

  void insert(const key_type& key) {
      unsigned index = getIndex(key);
      bool overflow = buckets[index]->append(key);
      if (overflow) split();
  }

  bool erase(const key_type& key) {
    unsigned index = getIndex(key);

    Bucket* prev {nullptr};
    for (Bucket* b{buckets[index]}; b != nullptr; b = b->nextBucket) {
      for (size_type i = 0; i < b->bucketSize; ++i) {
        if (key_equal{}(b->entries[i], key)) {
          b->entries[i] = b->entries[b->bucketSize-1];
          b->bucketSize--;

          if (b->bucketSize > 0 || !(prev || b->nextBucket)) return true;

          if (!prev) {
            buckets[index] = b->nextBucket;
          } else if (!b->nextBucket) {
            prev->nextBucket = nullptr;
          } else {
            prev->nextBucket = b->nextBucket;
          }

          delete b;
          return true;
        }
      }
      prev = b;
    }

    return false;
  }

  void split() {
    nextToSplit++;
    if (tableSize == tableMaxSize) {
      tableMaxSize *= 2;
      Bucket** newBuckets = new Bucket*[tableMaxSize];

      for (size_type i{0}; i < tableSize; i++) {
        newBuckets[i] = buckets[i];
      }

      delete[] buckets;
      buckets = newBuckets;
    } 

    buckets[tableSize++] = new Bucket;

    Bucket* newBucket = new Bucket;
    for (Bucket* b{buckets[nextToSplit-1]}; b != nullptr; b = b->nextBucket) {
      for (size_type i = 0; i < b->bucketSize; ++i) {
        unsigned index = getIndex(b->entries[i]);

        if (index == nextToSplit-1) {
          newBucket->append(b->entries[i]);
        } else {
          buckets[index]->append(b->entries[i]);
        }
      }
    }

    deleteLinkedBuckets(buckets[nextToSplit-1]);
    buckets[nextToSplit-1] = newBucket;


    if(nextToSplit == static_cast<size_type>(1 << roundNumber)) { 
      roundNumber++; 
      nextToSplit = 0; 
    }
  }

  ~HashTable() {
    for (size_type i{0}; i < tableSize; i++) deleteLinkedBuckets(buckets[i]);
    delete[] buckets;
  }
};

template <typename Key, size_t N>
class ADS_set<Key,N>::Iterator {
public:
  using value_type = Key;
  using difference_type = std::ptrdiff_t;
  using reference = const value_type &;
  using pointer = const value_type *;
  using iterator_category = std::forward_iterator_tag;
private:
  Bucket** ht;
  Bucket* currBucket;
  size_type x, y, z, tableSZ;
  pointer ptr;

  void advance() {
    if (z >= currBucket->bucketSize) {
      z = 0;
      if (currBucket->nextBucket == nullptr) {
        y = 0;
        do {
          ++x;
          if (x == tableSZ) {
            ptr = nullptr;
            return;
          } 
        } while(ht[x]->bucketSize == 0);

        currBucket = ht[x];
      } else {
        currBucket = currBucket->nextBucket;
        ++y;
      }
      
      ptr = currBucket->entries;
    }
  }

public:
  explicit Iterator(Bucket** ht, size_t tableSZ): ht{ht}, x{0}, y{0}, z{0}, tableSZ{tableSZ} {
    this->currBucket = ht[0];
    if (this->currBucket->bucketSize == 0) {
      advance();
    } else {
       ptr = currBucket->entries;
    }
  }

  Iterator(Bucket** ht, size_t tableSZ, size_t x, size_t y, size_t z): ht{ht}, x{x}, y{y}, z{z}, tableSZ{tableSZ} {
    this->currBucket = ht[x];
    for (size_t i {0}; i < y; i++) {
      this->currBucket = currBucket->nextBucket;
    }
    ptr = currBucket->entries+z;
  }

  Iterator(): ht{nullptr}, currBucket{nullptr}, x{0}, y{0}, z{0}, tableSZ{0}, ptr{nullptr} {}

  reference operator*() const {
    return *ptr;
  };

  pointer operator->() const {
    return ptr;
  };
  Iterator &operator++() {
    ++z;
    ++ptr;
    skip();
    return *this;
  };
  Iterator operator++(int) {
    Iterator temp {*this};
    ++*this;
    return temp;
  };
  friend bool operator==(const Iterator &lhs, const Iterator &rhs) {
    return lhs.ptr == rhs.ptr;
  };
  friend bool operator!=(const Iterator &lhs, const Iterator &rhs) {
    return !(lhs == rhs);
  };
};


template <typename Key, size_t N>
void swap(ADS_set<Key,N> &lhs, ADS_set<Key,N> &rhs) { lhs.swap(rhs); }

#endif 