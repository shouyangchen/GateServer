//
// Created by chenshouyang on 25-6-24.
//

#ifndef SINGLETON_H
#define SINGLETON_H
#include <iostream>
#include <memory>
#include <mutex>
inline std::mutex mtx; // Mutex for thread safety
template<typename T>
class singleton {
protected://通过将构造函数设置为protected这样就可以保证子类可以构造父类
    singleton(const singleton &)=delete;
    singleton &operator=(const singleton &)=delete;
    singleton() = default; // Private constructor
    static std::shared_ptr<T> instance;
public:
    static std::shared_ptr<T> getInstance() {
        std::unique_lock<std::mutex>lock(mtx);
        static std::once_flag flag;
        std::call_once(flag,[&] {
            if(instance==nullptr) {
                singleton<T>::instance=std::shared_ptr<T>(new T);
            }
        });
        lock.unlock();
        return singleton<T>::instance;
    }

    void print_address() {
        std::cout << "Singleton instance address: " << std::addressof(instance) << std::endl;
    }

    virtual ~singleton() {
        std::unique_lock<std::mutex>lock(mtx);
        if (instance) {
            instance.reset(); // Reset the shared pointer to release the instance
            std::cout << "Singleton instance destroyed." << std::endl;
        }
        lock.unlock();
    }
};
template<typename T>
std::shared_ptr<T> singleton<T>::instance = nullptr; // 初始化静态成员变量

#endif //SINGLETON_H
