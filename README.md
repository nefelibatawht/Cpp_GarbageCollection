# Cpp_GarbageCollection
java课后实验，C++实现的垃圾回收机制

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
