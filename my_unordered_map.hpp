//
//  MyLib.hpp
//  MySpace
//
//  Created by Pavel Shlykov B20-515 on 21.09.2021.
//

#ifndef MyLib_hpp
#define MyLib_hpp

#include <stdexcept>
#include <cmath>
#include <iterator>
#include <initializer_list>
#include <memory>
#include <algorithm>
#include <utility>
#include <type_traits>

template<typename Key, typename T, typename cmp>
struct __bucket{
    std::pair<Key, T> item;
    size_t hash = -1;
    __bucket* next = nullptr;
    
    __bucket() = default;
    
    __bucket(const std::pair<Key, T>& item, size_t hash, __bucket* next= nullptr): item(item), hash(hash), next(next){}
    __bucket(std::pair<Key, T>&& item, size_t hash, __bucket* next= nullptr): item(std::move(item)), hash(hash), next(next){}
    
    __bucket(const __bucket& b){
        item = b.item;
        hash = b.hash;
        next = b.next;
    }
    
    __bucket& operator=(const __bucket& b){
        if (&b == this) return *this;
        __bucket tmp = b;
        std::swap(item, tmp.item);
        std::swap(hash, tmp.hash);
        std::swap(next, tmp.next);
        return *this;
    }
    
    __bucket(__bucket&& b): item(std::move(b.item)), hash(b.hash), next(b.next){
        b.next = nullptr;
    }
    
    __bucket& operator=(__bucket&& b) {
        if (&b == this) return *this;
        __bucket tmp = std::move(b);
        std::swap(item, tmp.item);
        std::swap(hash, tmp.hash);
        std::swap(next, tmp.next);
        return *this;
    }
};



template <typename Key,
            typename T,
            typename Hash = std::hash<Key>,
            typename Cmp = std::equal_to<Key>,
            typename Allocator = std::allocator<std::pair<Key, T> > >

/**!
 @brief MyUnordered map is an associative container that contains key-value pairs with unique keys. Search, insertion, and removal of elements have average constant-time complexity.
  Internally, the elements are not sorted in any particular order, but organized into buckets.
         Which bucket an element is placed into depends entirely on the hash of its key.
         Keys with the same hash code appear in the same bucket.
         This allows fast access to individual elements, since once the hash is computed, it refers to the exact bucket the element is placed into.
 */
class MyUnorderedMap{
    using bucket = __bucket<Key, T, Cmp>;
    using item = std::pair<Key, T>;
    using mumap = MyUnorderedMap;
    using AllocTraits = std::allocator_traits<Allocator>;
    
    
    struct Buckets{
        bucket* next = nullptr;
    };
    
    static_assert((std::is_same<item, typename Allocator::value_type>::value), "Invalid allocator::value_type");
    
public:
    template<bool is_const>
    class Any_iterator{
        std::conditional_t<is_const, const bucket, bucket>* it;
        
    public:
        using value_type = T;
        using iterator_category = std::forward_iterator_tag;
        Any_iterator(std::conditional_t<is_const, const bucket, bucket>* p): it(p) {}
        
        std::conditional_t<is_const, const Any_iterator, Any_iterator>& operator++(){
            it = it->next;
            return *this;
        }
        
        
        std::conditional_t<is_const, const item, item>* operator->(){
            return &it->item;
        }
        
        std::conditional_t<is_const, const item, item>& operator*(){
            return it->item;
        }
        
        bool operator==(Any_iterator iter){
            return it == iter.it;
        }
        
        
        bool operator!=(Any_iterator iter){
            return !(*this == iter);
        }
    };
    
    
    using const_iterator = Any_iterator<true>;
    using iterator = Any_iterator<false>;
    
    iterator begin(){
        return iterator(__start.next);
    }
    
    iterator end(){
        iterator it(__end);
        return it;
    }
    
    const_iterator cbegin() const{
        return const_iterator(__start.next);
    }
    
    const_iterator cend() const{
        const_iterator it(__end);
        return it;
    }

    
private:
    
    Hash hash;
    Cmp cmp;
    
    typename AllocTraits::template rebind_alloc<bucket> bucket_alloc;
    typename AllocTraits::template rebind_alloc<Buckets> array_alloc;
    
    using B_AllocTraits = std::allocator_traits<decltype(bucket_alloc)>;
    using A_AllocTraits = std::allocator_traits<decltype(array_alloc)>;
    
    size_t __size = 0;
    size_t __count = 0;
    float __max_load_factor = 1;
    
    Buckets* array = nullptr;
    
    bucket __start;
    bucket* __end = B_AllocTraits::allocate(bucket_alloc, 1);
    
    
    static size_t __constrain_hash(size_t hash, size_t size) noexcept{
        return !(size & (size - 1)) ? hash & (size - 1) :
            (hash < size ? hash : hash % size);
    }
    
    static bool __is_hash_power2(size_t size) noexcept{
        return size > 2 && !(size & (size - 1));
    }
    
    
    bucket* __bucket_insert(const item& pair, size_t h){
        if (array[h].next == nullptr){
            array[h].next = B_AllocTraits::allocate(bucket_alloc, 1);
            B_AllocTraits::construct(bucket_alloc, array[h].next, pair, h, __start.next);
            
            __start.next = array[h].next;
            return array[h].next;
        }
        auto* g = array[h].next;
        if (cmp(g->item.first, pair.first)) return nullptr;
        
        while(g->next != __end && g->next->hash == h){
            if (cmp(g->item.first, pair.first)) return nullptr;
            g = g->next;
        }
        
        if (cmp(g->item.first, pair.first)) return nullptr;
        
        auto* next = g->next;
        g->next = B_AllocTraits::allocate(bucket_alloc, 1);
        B_AllocTraits::construct(bucket_alloc, g->next, pair, h, next);
        return g->next;
    }
    
    
    bucket* __bucket_insert(item&& pair, size_t h){
        if (array[h].next == nullptr){
            array[h].next = B_AllocTraits::allocate(bucket_alloc, 1);
            B_AllocTraits::construct(bucket_alloc, array[h].next, std::move(pair), h, __start.next);
            
            __start.next = array[h].next;
            return array[h].next;
        }
        auto* g = array[h].next;
        if (cmp(g->item.first, pair.first)) return nullptr;
        
        while(g->next != __end && g->next->hash == h){
            if (cmp(g->item.first, pair.first)) return nullptr;
            g = g->next;
        }
        
        if (cmp(g->item.first, pair.first)) return nullptr;
        
        auto* next = g->next;
        g->next = B_AllocTraits::allocate(bucket_alloc, 1);
        B_AllocTraits::construct(bucket_alloc, g->next, std::move(pair), h, next);
        return g->next;
    }
    
    
    void __rehash(size_t new_size){
        Buckets* newarr = A_AllocTraits::allocate(array_alloc, new_size);
        for (size_t i = 0; i < new_size; ++i)
            A_AllocTraits::construct(array_alloc, newarr + i);
        
        A_AllocTraits::deallocate(array_alloc, array, __size);
        array = newarr;
        
        bucket* i = __start.next;
        __start.next = __end;
        __size = new_size;
        while(i != __end){
            size_t h = __constrain_hash(hash(i->item.first), __size);
            bucket* tmp = i->next;
            if (array[h].next == nullptr){
                i->next = __start.next;
                array[h].next = i;
                __start.next = i;
            }
            else{
                i->next = array[h].next->next;
                array[h].next->next = i;
            }
            i->hash = h;
            i = tmp;
        }
    }

    
    bucket* __find(const Key& key) noexcept{
        size_t h = hash(key);
        h = __constrain_hash(h, __size);
        
        if (array[h].next == nullptr) return __end;
        
        for(bucket* g = array[h].next; g != __end && h == g->hash; g = g->next){
            if (cmp(g->item.first, key)) return g;
        }
        return __end;
    }
    
    
    const bucket* __find(const Key& key) const noexcept{
        size_t h = hash(key);
        h = __constrain_hash(h, __size);
        
        if (array[h].next == nullptr) return __end;
        
        for(bucket* g = array[h].next; g != __end && h == g->hash; g = g->next){
            if (cmp(g->item.first, key)) return g;
        }
        return __end;
    }
    
    
    bucket* __find(Key&& key) noexcept{
        size_t h = hash(key);
        h = __constrain_hash(h, __size);
        
        if (array[h].next == nullptr) return __end;
        
        for(bucket* g = array[h].next; g != __end && h == g->hash; g = g->next){
            if (cmp(g->item.first, key)) return g;
        }
        return __end;
    }
    
public:
    
    /**
     @brief manages maximum average number of elements per bucket
        Sets the maximum load factor to ml.
     @param float f
     */
    void max_load_factor(float f) noexcept{
        __max_load_factor = fabs(f);
    }
    
    
    /**
     @brief manages maximum average number of elements per bucket
        Returns current maximum load factor
     */
    float max_load_factor() const noexcept{
        return __max_load_factor;
    }
    
    /**
     @brief eturns the number of buckets
     */
    size_t size() const noexcept{
        return __size;
    }
    
    
    /**
     @brief copy constructor. Constructs the container with the copy of the contents of other, copies the load factor, the predicate, and the hash function as well. If alloc is not provided, allocator is obtained by calling
     @param const MyUnorderedMap map
     @returns MyUnorderedMap&
     @exception std::bad_alloc();
     */
    size_t count() const noexcept{
        return __count;
    }
    
    
    
    /**
     @brief checks whether the container is empty
     @returns bool
     */
    bool empty() const noexcept{
        return (__start.next == __end ? true : false);
    }
    
    
    /**
     @brief returns average number of elements per bucket
     @returns float
     @exception std::bad_alloc();
     */
    float load_factor() const noexcept{
        return (__count == 0 ? 0 : __size / __count);
    }
    
    
    /**
     @brief default constructor.constructs the unordered_map
     @returns MyUnorderedMap
     */
    MyUnorderedMap(){
        B_AllocTraits::construct(bucket_alloc, __end);
        __start.next = __end;
    }
    
     
    /**
     @brief copy constructor. Constructs the container with the copy of the contents of other, copies the load factor, the predicate, and the hash function as well. If alloc is not provided, allocator is obtained by calling
     @param const MyUnorderedMap map
     @returns MyUnorderedMap&
     @exception std::bad_alloc();
     */
    MyUnorderedMap(const mumap& map): MyUnorderedMap(){
        this->__size = map.__size;
        this->__count = map.__count;
        this->__max_load_factor = map.__max_load_factor;
        if (map.__size > 0){
            array = A_AllocTraits::allocate(array_alloc, map.__size);
            for (size_t i = 0; i < map.__size; ++i)
                A_AllocTraits::construct(array_alloc, array + i);
        }
        
        __start.next = __end;
        try{
            for(auto* g = map.__start.next; g != map.__end; g = g->next){
            // i break the old order, but now idw fix it
            // i can little bit faster but it would be copy-past
                __bucket_insert(g->item, g->hash);
            }
        }catch(...){
            auto* i = __start.next;
            while(i != __end){
                auto* next = i->next;
                B_AllocTraits::destroy(bucket_alloc, i);
                B_AllocTraits::deallocate(bucket_alloc, i, 1);
                i = next;
            }
            B_AllocTraits::destroy(bucket_alloc, __end);
            B_AllocTraits::deallocate(bucket_alloc, __end, 1);
            
            A_AllocTraits::deallocate(array_alloc, array, map.__size);
            throw;
        }
    }
    
    
    /**
     @brief Copy assignment operator. Replaces the contents with a copy of the contents of other.
     @param const MyUnorderedMap& map
     @returns MyUnorderedMap
     @exception std::bad_alloc();
     */
    mumap& operator=(const mumap& map){
        if (&map == this) return *this;
        // allocators copy???
        mumap tmp = map;
        std::swap(tmp.array, array);
        std::swap(tmp.__size, __size);
        std::swap(tmp.__count, __count);
        std::swap(tmp.__start, __start);
        std::swap(tmp.__end, __end);
        std::swap(tmp.__max_load_factor, __max_load_factor);
        return *this;
    }
    
    
    /**
     @brief move constructor. Constructs the container with the contents of other using move semantics. If alloc is not provided, allocator is obtained by move-construction from the allocator belonging to other.
     @param MyUnorderedMap&& map
     @returns MyUnorderedMap
     @exception std::bad_alloc();
     */
    MyUnorderedMap(mumap&& map): __size(map.__size), __count(map.__count), array(map.array),
    __start(std::move(map.__start)), __end(map.__end), __max_load_factor(map.__max_load_factor){
        // allocators move???
        map.array = nullptr;
        map.__size = 0;
        map.__count = 0;
        map.__max_load_factor = 1;
        map.__end = B_AllocTraits::allocate(bucket_alloc, 1);
        B_AllocTraits::construct(bucket_alloc, map.__end);
        map.__start.next = map.__end;
    }
    
    
    /**
     @brief Move assignment operator. Replaces the contents with those of other using move semantics (i.e. the data in other is moved from other into this container). other is in a valid but unspecified state afterwards.
     @param MyUnorderedMap&& map
     @returns MyUnorderedMap&
     @exception std::bad_alloc();
     */
    mumap& operator=(mumap&& map){
        if (&map == this) return *this;
        // allocators move???
        mumap tmp = std::move(map);
        std::swap(tmp.array, array);
        std::swap(tmp.__size, __size);
        std::swap(tmp.__count, __count);
        std::swap(tmp.__start, __start);
        std::swap(tmp.__end, __end);
        std::swap(tmp.__max_load_factor, __max_load_factor);
        map.__start.next = map.__end;
        return *this;
    }
    
    
    /**
     @brief Sets the number of buckets to count and rehashes the container, i.e. puts the elements into appropriate buckets considering that total number of buckets has changed.
        If the new number of buckets makes load factor more than maximum load factor (count < size() / max_load_factor()), then the new number of buckets is at least size() / max_load_factor().
     @param size_t new_size
     @exception std::bad_alloc();
     */
    void rehash(size_t new_size){
        if (new_size * __max_load_factor < __count)
            throw std::out_of_range("unoredered_map::rehash: index is less then the minimum possible");
        __rehash(new_size);
    }
    
    
    /**
     @brief Inserts element(s) into the container, if the container doesn't already contain an element with an equivalent key.
     @param const item& pair
     @returns MyUnorderedMap&
     @exception std::bad_alloc();
     */
    std::pair<iterator, bool> insert(const item& pair){
        if (__size * __max_load_factor < __count + 1)
            __rehash(std::max<size_t>(2 * __count + !__is_hash_power2(__count),
            size_t(ceil(float(__count + 1) / __max_load_factor))));
        
        size_t h =__constrain_hash(hash(pair.first), __size);
        auto* res = __bucket_insert(pair, h);
        if (res){
            ++__count;
            return std::make_pair(iterator(res), true);
        }
        return std::make_pair(iterator(__end), false);
    }
    
    
    /**
     @brief Inserts element(s) into the container, if the container doesn't already contain an element with an equivalent key.
     @param item&& pair
     @returns std::pair<iterator, bool>
     @exception std::bad_alloc();
     */
    std::pair<iterator, bool> insert(item&& pair){
        if (__size * __max_load_factor < __count + 1)
            __rehash(std::max<size_t>(2 * __count + !__is_hash_power2(__count),
            size_t(ceil(float(__count + 1) / __max_load_factor))));
        
        size_t h = __constrain_hash(hash(pair.first), __size);
        auto* res = __bucket_insert(std::move(pair), h);
        if (res){
            ++__count;
            return std::make_pair(iterator(res), true);
        }
        return std::make_pair(iterator(__end), false);
    }
    
    
    /**
     @brief Inserts element(s) into the container, if the container doesn't already contain an element with an equivalent key.
     @param std::initializer_list<item> list
     @exception std::bad_alloc();
     */
    void insert(std::initializer_list<item> list){
        for (auto& i : list)
            insert(std::move(i));
    }
    
    
    /**
     @brief Inserts a new element into the container constructed in-place with the given args if there is no element with the key in the container.
     @param Args&&... args
     @returns std::pair<iterator, bool>
     @exception std::bad_alloc();
     */
    template<typename ...Args>
    std::pair<iterator, bool> emplace(Args&&... args){
        return insert(std::make_pair(std::forward<Args>(args)...));
    }
    
    
    /**
     @brief Inserts element(s) into the container, if the container doesn't already contain an element with an equivalent key.
     @param std::initializer_list<item> list
     @exception std::bad_alloc();
     */
    void operator=(std::initializer_list<item> list){
        clear();
        for (auto& i : list)
            insert(std::move(i));
    }
    

    /**
     @brief Returns a reference to the value that is mapped to a key equivalent to key, performing an insertion if such key does not already exist.
     @param const Key& key
     @returns T&
     @exception std::bad_alloc();
     */
    T& operator[](const Key& key){
        auto it = find(key);
        if (it == end()){
            auto [a, b] = emplace(key, T());
            return a->second;
        }
        return it->second;
    }
    
    
    /**
     @brief Returns a reference to the value that is mapped to a key equivalent to key, performing an insertion if such key does not already exist.
     @param Key&& key
     @returns T&
     @exception std::bad_alloc();
     */
    T& operator[](Key&& key){
        auto it = find(key);
        if (it == end()){
            auto [a, b] = emplace(std::move(key), T());
            return a->second;
        }
        return it->second;
    }

    
    /**
     @brief Finds an element with key equivalent to key.
     @param const Key& key
     @returns iterator
     */
    iterator find(const Key& key){
        if (array == nullptr) return end();
        return iterator(__find(key));
    }
    
    
    /**
     @brief Finds an element with key equivalent to key.
     @param const Key& key
     @returns const_iterator
     */
    const_iterator find(const Key& key) const{
        if (array == nullptr) return cend();
        return const_iterator(__find(key));
    }
    
    
    /**
     @brief Finds an element with key equivalent to key.
     @param Key&& key
     @returns iterator
     */
    iterator find(Key&& key){
        if (array == nullptr) return end();
        return iterator(__find(std::move(key)));
    }
    
    
    /**
     @brief References and iterators to the erased elements are invalidated.
     
     Invalidates  iterators referring to contained elements.
     @param const Key& key
     @returns bool
     */
    bool erase(const Key& key){
        if (array == nullptr) return false;
        size_t h = hash(key);
        h = __constrain_hash(h, __size);
        
        if (array[h].next == nullptr) return false;
        
        for (bucket* g = array[h].next; g != __end && g->hash == h; g = g->next){
            if (cmp(g->item.first, key)){
                
                if (array[h].next == g){
                    if (g->next == __end)
                        array[h].next = nullptr;
                    else if (g->next->hash == h)
                        array[h].next = g->next;
                    else array[h].next = nullptr;
                }
                
                if (g->next != __end){
                    if (array[g->next->hash].next == g->next) array[g->next->hash].next = g;
                }else __end = g;
                
                auto* next = g->next;
                *g = std::move(*g->next);
                B_AllocTraits::destroy(bucket_alloc, next);
                B_AllocTraits::deallocate(bucket_alloc, next, 1);
                --__count;
                return true;
            }
        }
        return false;
    }
    
    
    /**
     @brief References and iterators to the erased elements are invalidated.
     
     Invalidates  iterators referring to contained elements.
     @param Key&& key
     @returns bool
     */
    bool erase(Key&& key){
        if (array == nullptr) return false;
        size_t h = hash(key);
        h = __constrain_hash(h, __size);
        
        if (array[h].next == nullptr) return false;
        
        for (bucket* g = array[h].next; g != __end && g->hash == h; g = g->next){
            if (cmp(g->item.first, key)){
                
                if (array[h].next == g){
                    if (g->next == __end)
                        array[h].next = nullptr;
                    else if (g->next->hash == h)
                        array[h].next = g->next;
                    else array[h].next = nullptr;
                }
                
                if (g->next != __end){
                    if (array[g->next->hash].next == g->next) array[g->next->hash].next = g;
                }else __end = g;
                
                auto* next = g->next;
                *g = std::move(*g->next);
                B_AllocTraits::destroy(bucket_alloc, next);
                B_AllocTraits::deallocate(bucket_alloc, next, 1);
                --__count;
                return true;
            }
        }
        return false;
    }
    
    
    /**
     @brief Erases all elements from the container. After this call, size() returns zero.
     
     Invalidates any references, pointers, or iterators referring to contained elements. May also invalidate past-the-end iterators.
     */
    void clear() noexcept{
        bucket* g = __start.next;
        while (g != __end){
            bucket* next = g->next;
            B_AllocTraits::destroy(bucket_alloc, g);
            B_AllocTraits::deallocate(bucket_alloc, g, 1);
            g = next;
        }
        if (array != nullptr){
            A_AllocTraits::deallocate(array_alloc, array, __size);
            array = nullptr;
        }
        __size = 0;
        __count = 0;
        __start.next = __end;
    }
    
    
    /**
     @brief Move assignment operator. Replaces the contents with those of other using move semantics (i.e. the data in other is moved from other into this container). other is in a valid but unspecified state afterwards.
     */
    ~MyUnorderedMap(){
        clear();
        B_AllocTraits::destroy(bucket_alloc, __end);
        B_AllocTraits::deallocate(bucket_alloc, __end, 1);
    }
    
};

#endif /* MyLib_hpp */
