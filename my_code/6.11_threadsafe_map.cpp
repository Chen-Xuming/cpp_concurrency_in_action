//
// Created by chen on 2022/9/1.
//
// 采用精细粒度锁操作的map数据结构

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <vector>

template <class Key, class Value, class Hash = std::hash<Key>>
class ConcurrentMap{
public:
    // 桶数默认为19
    ConcurrentMap(std::size_t n = 19, const Hash &h = Hash{}): buckets_(n), hasher_(h){
        for(auto &x: buckets_){
            x.reset(new Bucket);
        }
    }
    ConcurrentMap(const ConcurrentMap&) = delete;
    ConcurrentMap& operator=(const ConcurrentMap&) = delete;

    Value get(const Key &k, const Value &default_value = Value{}){
        return get_bucket(k).get(k, default_value);
    }

    void set(const Key &k, const Value &v){
        get_bucket(k).set(k, v);
    }

    void erase(const Key &k){
        get_bucket(k).erase(k);
    }

    // ConcurrentMap 到 std::map 的映射，方便使用
    std::map<Key, Value> to_map() const{
        std::vector<std::unique_lock<std::shared_mutex>> locks;
        for(auto &x: buckets_){
            locks.emplace_back(std::unique_lock<std::shared_mutex>(x->m));
        }
        std::map<Key, Value> res;
        for(auto &x: buckets_){
            for(auto &y: x->data){
                res.emplace(y);
            }
        }
        return res;
    }


private:
    // 基于list实现的桶。每个桶带有一个锁
    struct Bucket{
        std::list<std::pair<Key, Value>> data;
        mutable std::shared_mutex m;

        Value get(const Key &k, const Value &default_value) const{
            std::shared_lock<std::shared_mutex> l(m);   // 读操作，使用共享锁
            auto it = std::find_if(data.begin(), data.end(), [&](auto &x){
                return x.first == k;
            });
            return it == data.end() ? default_value : it->second;
        }

        void set(const Key &k, const Value &v){
            std::unique_lock<std::shared_mutex> l(m);   // 写操作，使用独占锁
            auto it = std::find_if(data.begin(), data.end(), [&](auto &x){
                return x.first == k;
            });
            if(it == data.end()){
                data.template emplace_back(k, v);
            }else{
                it->second = v;
            }
        }

        void erase(const Key &k){
            std::unique_lock<std::shared_mutex> l(m);   // 写，用独占锁
            auto it = std::find_if(data.begin(), data.end(), [&](auto &x){
                return x.first == k;
            });
            if(it != data.end()){
                data.erase(it);
            }
        }
    };

    std::vector<std::unique_ptr<Bucket>> buckets_;
    Hash hasher_;

    Bucket& get_bucket(const Key &k) const{
        return *buckets_[hasher_(k) % buckets_.size()];
    }
};