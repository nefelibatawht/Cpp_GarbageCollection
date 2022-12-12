## 【文档】利用C++编写C++的内存垃圾清理类

> @Author : Nefelibata. W
>
> @Date :  2022.10.16
>
> @Last Modified : 2022.11.03

### 理论支撑--JVM的追踪垃圾回收算法

#### 回收策略：

**标记-清除-整理** 

#### 标记

暂停运行的线程，仅保留垃圾回收功能的线程并完成对存活对象之外的的扫描和标记

#### 清除

对被标记的对象直接进行清除操作

#### 整理

标记和清除之后会产生很多不连续的内存碎片，需要对零散的对象汇集到连续的内存空间上，并恢复运行线程

### 理论支撑--单例模式

意图是保证一个类仅有一个实例，并提供一个访问它的全局访问点，该实例被所有程序模块共享。

定义一个单例类需要满足：

1. **私有化它的构造函数**，以防止外界创建单例类的对象；
2. 使用类的**私有静态指针**变量指向类的唯一实例；
3. 使用一个**公有的静态方法**获取该实例。

### 项目文件

1. Object.h      &&   Object.cpp    对象基类
2. MemoryManager.h    &&  MemoryManager.cpp   内存管理
3. MemoryBadAlloc.h && MemoryBadAlloc.cpp   异常处理
4. type_pointer   规范指针对象输出的格式
5. main.cpp   定义对象，运行入口

### 代码结构

#### Object.h

- Object是对象基类，这里该头文件主要是定义了基类的```new```和```delete```函数 
- ```setObjectName```函数用来给创建出的对象进行命便于最后的观察

```C++
#include <iostream>
#ifndef GARBAGE_COLLECTION_OBJECT_H
#define GARBAGE_COLLECTION_OBJECT_H
#define DEBUG
#include "MemoryManager.h"
#include <string>

class Object {
public:
    void* operator new(size_t size);
    void operator delete(void *pointer) ;

    void setObjectName(std::string str) {
        objectName_ = str;
    }
    std::string objectName() const;
    virtual ~Object() = default;
private:
    std::string objectName_;
};

#endif // GARBAGE_COLLECTION_OBJECT_H

```

#### MemoryManager.h

- 定义对象的结构体，包含对象指针，对象大小，对象的下标和是否被标记的信息
- 定义MemoryManager类，定义静态方法实现内存管理

```c++
//内存结构体
#define MEMORY_SIZE 256
#define MAX_OBJECT 256
typedef struct {
    void * list[MAX_OBJECT]; // 对象指针
    size_t listPreSize[MAX_OBJECT]; // 对象大小,size_t类型是变量能够达到的最大的大小，无符号型整数
    bool marked[MAX_OBJECT];
    size_t listIndex; // 下标
}MemoryStruct;
```

```C++
class MemoryManager {
public:
    void* operator new(size_t size);
    void operator delete(void *pointer) ;
//重载new和delete
    static void init();
    static MemoryManager* getInstance();

    static void memoryTrim();// 线程停止
    static void toMark(void *pointer); //即管理列表中的内存地址为空（手动delete）
    static void markClear();
    static void markCompress();

    static void swap(size_t front, size_t tail);

//    static void getAllObjPointer();
    static void deleteInstance();

private:
    static MemoryManager *memoryManager;

    static size_t memorySize_;
    static void * Memory_;

    static size_t lastPointer_;

    static MemoryStruct memoryStruct_;

    static std::thread *memoryTrimThread;


};
#endif // GARBAGE_COLLECTION_MEMORY_MANAGER_H

```

#### MemoryBadAlloc.h

- 抛出异常类，在开辟的内存空间不够时抛出异常

```C++
class MemoryBadAlloc : public std::bad_alloc{
public:
    const char* what() const noexcept override;
};

```

#### Object.cpp

- 实现基类的```new```和```delete```函数，由于我们使用单例模式，在创建和释放的时候相当于创建了单例的```MemoryManager```类对象，所以这里需要调用```MemoryManager::getInstance()```

```C++
#include <thread>
#include "Object.h"
using namespace std;
void * Object::operator new(size_t size) {
#ifdef DEBUG
    cout << "Use Object operator\n";
#endif
    return MemoryManager::getInstance()->operator new(size);
}

void Object::operator delete(void *pointer) {
#ifdef DEBUG
    cout << "Object operator delete\n";
#endif

    MemoryManager::getInstance()->operator delete(pointer);
}

std::string Object::objectName() const{
    return objectName_;
}
```

#### MemoryManager.cpp

- 全部项目的核心部分

- ```void* operator new```和```void* operator delete```分别是用来开辟空间（malloc）和释放空间（free）

  ```C++
  void* operator new(size_t size){
  #ifdef DEBUG
      std::cout << "global new\n";
  #endif
      return malloc(size);
  }
  
  
  void operator delete(void *pointer)  {
  #ifdef DEBUG
      std::cout << "global delete\n";
  #endif
      free(pointer);
  }
  ```

- ```void MemoryManager::init()```，用于初始化MemoryManager中的内存大小和分配内存空间

  ```C++
  void MemoryManager::init() {
      memorySize_ = MEMORY_SIZE;
      Memory_ = ::operator new(memorySize_);//malloc function
      for(std::size_t i = 0; i < MAX_OBJECT; ++i) {
          memoryStruct_.list[i] = nullptr;
          memoryStruct_.listPreSize[i] = 0;
          memoryStruct_.marked[i] = false;
      }
      memoryStruct_.listIndexs = 0;
      memoryTrimThread = new std::thread(memoryTrim);
  #ifdef PRI_DEBUG
      std::cout << "thread start\n";
  #endif
  }
  ```

- ```void* MemoryManager::operator new(size_t size)```类内重载operator new , Object &obj

  ```C++
  void* MemoryManager::operator new(size_t size) 
      while(true) {
               //内存不足时抛出异常
              if (size > memorySize_ - lastPointer_) {
                  throw MemoryBadAlloc();
              }
              void *objPointer = (byte_pointer) Memory_ + lastPointer_;
  #ifdef POINTER_DEBUG
              cout << "Use MemoryManager operator , new Pointer: ";
              show_pointer(objPointer);
  #endif
              lastPointer_ += size;
  
              memoryStruct_.list[memoryStruct_.listIndexs] = (byte_pointer) objPointer;
  
  
              memoryStruct_.listPreSize[memoryStruct_.listIndexs] = size;
              memoryStruct_.listIndexs += 1;
  
              return objPointer;
      }
  ```

- ```void MemoryManager::operator delete(void *pointer)```类内重载operator delete

  ```C++
  void MemoryManager::operator delete(void *pointer){
  #ifdef POINTER_DEBUG
      cout << "Use MemoryManager operator, delete Pointer: ";
      show_pointer(pointer);
  
  #endif
      toMark(pointer);
  #ifdef PRI_DEBUG
      printf("ToMarked\n");//Test
      /*
       * 转去delete去KILL线程
       * free()
       */
  #endif
  }
  ```

- ```MemoryManager* MemoryManager::getInstance()```是MemoryManager单例模式

  ```C++
  MemoryManager* MemoryManager::getInstance() {
      if(memoryManager == nullptr) {
  
          // MemoryManager(MEMORY_SIZE);
          memoryManager = static_cast<MemoryManager *>(::operator new(sizeof(MemoryManager)));
          init();
      }
      return memoryManager;//根据Object.cpp的要求，进入类内部重写的operator new中
  }
  ```

- ```void MemoryManager::memoryTrim()```,KILL当前线程

  ```C++
  void MemoryManager::memoryTrim() {
      while(true) {
  #ifdef PRI_DEBUG
  cout << "thread pause by memoryTrim()\n";
  #endif
              markClear();
              markCompress();
          std::this_thread::sleep_for(std::chrono::microseconds(300));
          //获取TID，切断当前线程
          unsigned int thread_num=pthread_self();
          int retval=pthread_kill(thread_num,0);
          //返回0代表线程被KILL
          if(!retval) break;
      }
  printf("Finish Trim!\n");
  }
  ```

- ```void MemoryManager::toMark```，清理内存时的标记函数

  ```C++
  void MemoryManager::toMark(void *pointer) {
      for(std::size_t i = 0; i < MAX_OBJECT; ++i) {
          if(pointer == memoryStruct_.list[i]) {
              memoryStruct_.marked[i] = true;
          }
      }
  }
  ```

- ```void MemoryManager::markClear()```,清理内存时的垃圾清理函数

  ```C++
  void MemoryManager::markClear() {
      for(std::size_t i = 0; i < MAX_OBJECT; ++i) {
          if(memoryStruct_.marked[i]) {
               /*
                * ::operator delete(memoryStruct_.list[i]);
                * 这里无需调用全局delete, 因为这块内存是Memory_所拥有的, free时释放了Memory_的内存
                */
               memoryStruct_.list[i] = nullptr;
               memoryStruct_.marked[i] = false;
  
               /*
                * 每次释放资源后listIndex减一
                */
                memoryStruct_.listIndexs -= 1;
                lastPointer_ -= memoryStruct_.listPreSize[i];
                memoryStruct_.listPreSize[i] = 0;
          }
      }
      //printf("Finish MarkClear");
  }
  ```

- ```void MemoryManager::markCompress() ```,清理内存时的压缩函数

  ```C++
  void MemoryManager::markCompress() {
      size_t front = 0;
      size_t tail = 0;
      while(front < MAX_OBJECT && tail < MAX_OBJECT){
          while(front < MAX_OBJECT) {
              if(memoryStruct_.list[front] != nullptr) { ++ front; }
              else break;
          }
          while(tail < MAX_OBJECT) {
              if(memoryStruct_.list[tail] == nullptr) { ++ tail; }
              else break;
          }
          /*
           * 当整个内存剩余全为空或全满时结束
           */
          if(tail >= MAX_OBJECT && front >= MAX_OBJECT) { break; }
  
          if(front < tail && front < MAX_OBJECT && tail < MAX_OBJECT) {
              swap(front, tail);
              ++ front;
              ++ tail;
          } else {
              ++ tail;
          }
      }
  }
  //swap(size_t front, size_t tail)
  void MemoryManager::swap(size_t front, size_t tail) {
      memoryStruct_.list[front] = memoryStruct_.list[tail];
      memoryStruct_.list[tail] = nullptr;
  
      memoryStruct_.listPreSize[front] = memoryStruct_.listPreSize[tail];
      memoryStruct_.listPreSize[tail] = 0;
  }
  ```

- ```void MemoryManager::deleteInstance()```,删除单例的类的对象

  ```c++
  void MemoryManager::deleteInstance(){
  #ifdef DEBUG
      std::cout << "~MemoryManager\n";
  #endif
      if(Memory_) {
          ::operator delete(Memory_);
          Memory_ = nullptr;
      }
      if(memoryManager) {
          ::operator delete(memoryManager);
          memoryManager = nullptr;
      }
  
      memoryTrimThread->join();
  }
  ```

#### MemoryBadAlloc.cpp

```c++
const char * MemoryBadAlloc::what() const noexcept {
    return "Does not have enough memory\n";
}
```

#### main.cpp

- 程序入口
- 由于多线程知识不熟练而且确实没时间写多线程，就采用自定义标记删除的方法进行垃圾回收（自定义需要回收的垃圾并完成整套流程）

```C++
class A : public Object {
public:
    explicit A(int a, std::string name) : a_ {a} {
        setObjectName(name);
    }
    explicit A() {}   //防止隐式自动转换
    int a() const { return a_; }
    
private:
    int a_;
};

class B : public A {
public:
    explicit B(int a, std::string name) : a_ {a} {
        setObjectName(name);
    }
private:
    int a_;
};

void testDriver() {
    try{
        A *a1 = new A(5, "a1");
        delete(a1);
        a1 = nullptr;
        A *a2 = new A(2, "a2");
        A *a3 = new A(2, "a3");
        A *a4 = new A(2, "a4");
        delete(a2);
        MemoryManager::memoryTrim();
        delete(a3);
        delete(a4);
        MemoryManager::memoryTrim();
        a2 = nullptr;
        a3 = nullptr;
        a4 = nullptr;
    } catch (MemoryBadAlloc &e) {
        std::cout << e.what();
    }

}

int main() {
    testDriver();
    MemoryManager::deleteInstance();
    return 0;
}
```

#### type_pointer

- 定义了地址指针，并且将内存信息全部展示出来

```C++
#include <stdio.h>
#ifndef TYPE_POINTER_C
#define TYPE_POINTER_C
/*
 * byte_pointer -> unsigned char *
 * 地址指针
 */
typedef unsigned char * byte_pointer;

/*
 * 内存地址输出
 * 应根据大小端机器模式读
 */
void show_bytes(byte_pointer start, size_t len) {
    size_t i;
    for(i = 0; i < len; i++) {
        printf("%.2x ", start[i]);
    }
    printf("\n");
}

void show_pointer(void *x) {
    show_bytes((byte_pointer) &x, sizeof(void *));
}

#endif
```

### 运行结果

#### 运行结果

![image-20221103112850571](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20221103112850571.png)

#### 解释

- 最初new出的一个对象是基类对象，所以先输出的是使用Object基类创建对象，同时开始进程，由于使用了内存管理的单例形式，所以相当于在MemoryManager中创建对象，同时输出new出的对象。

- 在进行内存清理的过程中，由于手动确定需要清理哪一部分，所以在执行delete操作的同时kill掉此时占有的进程同时执行清理操作。
