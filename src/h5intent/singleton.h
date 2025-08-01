//
// Created by haridev on 10/25/22.
//

#ifndef H5INTENT_SINGLETON_H
#define H5INTENT_SINGLETON_H
#include <iostream>
#include <memory>
#include <utility>
/**
 * Make a class singleton when used with the class. format for class name T
 * Singleton<T>::GetInstance()
 * @tparam T
 */
namespace h5intent {
template <typename T>
class Singleton {
 public:
  /**
   * Members of Singleton Class
   */
  /**
   * Uses unique pointer to build a static global instance of variable.
   * @tparam T
   * @return instance of T
   */
  template <typename... Args>
  static std::shared_ptr<T> get_instance(Args... args) {
    if (instance == nullptr)
      instance = std::shared_ptr<T>(new T(std::forward<Args>(args)...));
    return instance;
  }

  /**
   * Operators
   */
  Singleton& operator=(const Singleton) = delete; /* deleting = operatos*/
                                                  /**
                                                   * Constructor
                                                   */
 public:
  Singleton(const Singleton&) = delete; /* deleting copy constructor. */

 protected:
  static std::shared_ptr<T> instance;
  Singleton() {} /* hidden default constructor. */
};

template <typename T>
std::shared_ptr<T> Singleton<T>::instance = nullptr;
}  // namespace h5intent
#endif  // H5INTENT_SINGLETON_H
