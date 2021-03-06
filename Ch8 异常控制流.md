## 异常

控制流中的突变响应处理器状态变化（事件）

发生异常->异常处理->{处理程序返还控制给当前指令 、处理程序返还控制给下一条指令、处理程序终止程序}

### 异常处理

异常表（起始地址：异常表基址寄存器）

### 异常类别

中断（异步，返回下一条指令），陷阱（返回下一条指令），故障（返回当前指令），终止（不返回）

- 中断

来自于处理器外部的I/O设备的信号的结果（通过向处理器芯片上的一个引脚发信号，并将异常号放到系统总线上来触发中断）

对应的异常处理称为：中断处理程序

- 陷阱

在用户程序和内核之间提供一个像过程一样的接口：系统调用

系统调用：运行在内核模式中，允许系统调用执行特权指令，并访问定义在内核中的栈

- 故障

将控制返回到引起故障的指令以重新执行

例如：缺页异常

缺页异常：当指令引用一个虚拟地址而改地址相对应的物理地址不在内存中，必须要从磁盘中取出时，会发生故障

- 终止

硬件错误

### Linux系统中的异常

x86-64

256种不同的异常类型：

- 0-31：Intel架构师定义的异常
- 32-255：操作系统定义的中断和陷阱

例如：

除法错误：0 Floating exeception

一般保护故障：13 （引用未定义的虚拟内存区域、写一个只读文本段等） Segmentation fault

缺页：13

机器检查：18 （不返回控制）

#### 系统调用

syscall 陷阱指令

系统级函数

**（疑问：标准函数中例如fread也需要进入内核？为什么不是系统调用？它底层调用的时read吗？进入内核调用一些限制的指令情况举例？ -> 库函数不依赖于平台）**



## 进程

系统中的每个程序都运行在某个进程的上下文中。

上下文室友程序正确运行所需的状态组成的：

状态包括：存放在内存中的程序的代码和数据、栈、通用目的寄存器的内容、程序计数器、环境变量和打开文件描述符的集合

- 逻辑控制流

- 并发流

- **私有地址空间**

- **用户模式和内核模式**

  作用：限制一个应用可以执行的指令以及它可以访问的地址空间范围

  方式：处理器通常用某个控制寄存器（描述了进程当前享有的特权）中的一个模式位来提供这种功能

  ​	-> 设置后：进程就运行在内核模式中，可以执行指令集中的任何指令，可以访问系统中的任何内存位置

  ​	-> 用户模式 中的进程不允许执行特权指令，不允许直接引用地址空间中内和区域的代码和数据。通过系统调用间接访问

  

  处理异常程序运行在内核模式中，当它返回到应用代码时，处理器就把模式从内核改回用户

  

  **/proc文件系统**：允许用户模式访问内数据结构的内容

  **/sys文件系统**：输出关于系统总线和设备的额外的低层信息

  

- **上下文切换**

  内核为每个进程维护一个**上下文**

  上下文：内核重新启动一个被强占的进程所需的状态。

  所包括的对象：通用目的寄存器，浮点寄存器，程序计数器，用户栈，状态寄存器，内核栈，各种内核数据结构（页表、进程表、文件表等）

  

  **调度**：在程序执行时内核抢占当前进程并重新开始一个先前被抢占了的进程。由内核中的调度器代码处理。

  

  **上下文切换**：1 保存当前进程的上下文  2 恢复某个先前被抢占的进程被保存的上下文  3 将控制传递给这个新恢复的进程

  上下文切换发生的时间：内核代表用户执行系统调用时，中断

  场景描述：开始A在用户空间执行，某一刻发生系统调用，程序A进入内核空间处理，在内核空间中发生磁盘读写操作，此时处理器进行调度。选择内核中调度队列中的程序B开始在内核空间中继续执行，然后B从内核态回到用户态执行。在此期间，A程序中磁盘操作已完成，此时磁盘发送一个中断信号，内核判断B执行了足够的时间然后将控制交给程序A继续执行。



## 进程控制

fork：

**子进程得到与父进程用户级虚拟地址空间相同的一份副本**，包括：**代码和数据段，堆，共享库以及用户栈**。

子进程还获得与父进程任何**打开文件描述符相同的副本**

父进程和子进程时独立的进程，他们都有自己的私有地址空间

waitpid

…………



## 信号

软件形式的异常

内核为每个进程在pending位向量中维护者待处理信号的集合，在blocked/mask位向量中维护着被阻塞的信号的集合。

### 发送信号

- 进程组（是由一个正整数**进程组ID**来标识的）

  ```c
  pid_t getpgrp(void); // 返回当前进程的进程组ID
  
  int setpgid(pid_t pid, pid_t pgid); // 改变自己或者其他进程的进程组
  ```

- kill 发送信号

- 键盘发送信号

- `int kill(pid_t pid, int sig);`

- `unsigned int alarm(unsigned int secs);`

### 接收信号

### 信号解除

- 隐式阻塞机制

- 显示阻塞机制

```c
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);  // 改变当前阻塞的信号的集合
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signum);
int sigdelset(sigset_t *set, int signum);

int sigismember(const sigset_t *set, int signum);
```

### 信号处理

#### 安全的信号处理

- G0 处理程序尽可能简单

- G1 在处理程序中只调用**异步信号安全**（能够被信号处理程序安全的调用）的函数

  安全的函数：1 要么可重入  2 要么不能被信号处理程序中断 *（LINK - APUE-NOTE：9 并发-信号.md）*

  **(CSAPP - csapp.h - SIO)**：解决输出的异步信号安全的库

- G2 保存和恢复errno

- G3 阻塞所有的信号，保护对**共享全局数据结构**的访问

- G4 使用**volatile**声明全局变量（volatile强迫编译器每次在代码引用变量时到内存中去取值，（因为优化的编译器可能取到缓存在寄存器中值））

- G5 使用 **sig_atomic_t** 声明标志

  使用示例：对变量a的读写保证是原子的（不可中断的），在使用时无需暂时阻塞信号。（只适用于单个的读和写，类似a++这样的多条指令不支持）

  ```c
  sig_atomic_t a=0;
  int main(void){ 
  	/* register a sighandler */ 
  	while(!a); /* wait until a changes in sighandler */ 
  	/* do something after signal arrives */
      return 0;
  }
  ```

#### 正确的信号处理

![](/home/acptek/文档/Server/CSAPP笔记/信号的检测与处理.png)



假设同一个SIGCHLD信号：一个在处理，一个等待处理，之后到来的SIGCHLD信号会直接被丢弃，对应的子进程变成了僵死进程，所以在一次SIGCHLD处理程序中，需要回收尽可能多的僵死子进程

#### 可移植的信号处理

```c
// 允许用户在设置信号处理时，明确指定他们想要的信号的处理语义
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
```

#### 同步流以避免并发错误

竞争情况下，在竞争前屏蔽信号，竞争区域结束后解除信号屏蔽

#### 显示地等待信号

```c
// 等待SIGINT信号  // code - 8.41
while(!pid)
    pause();
// 会出先竞争的情况，操作不原子
// 如果在while测试后和pause之前收到SIGCHLD信号，pause会永远睡眠
```

```c
// 合适的处理方法
int sigsuspend(const sigset_t *mask);

// 等价于以下代码的不可中断版本
sigprocmask(SIG_SETMASK, &mask, &prev);
pause();
sigprocmask(SIG_SETMASK, &prev, NULL);
```



## 非本地跳转

应用1 ：允许从一个深层嵌套的函数调用中立即返回，通常由检测到某个错误情况引起的

应用2 ：使一个信号处理程序分支到一个特殊的代码位置，而不是返回到被信号到达中断了的指令的位置

```c
// 在env缓冲区中保存当前[调用环境]，以供longjmp使用。返回值不能赋值给变量。
// 调用环境：程序计数器，栈指针，通用目的寄存器
int setjmp(jmp_buf env);
int sigsetjmp(sigjmp_buf env, int savesigs);

// longjmp从env缓冲区恢复调用环境，然后触发一个从 [最近一次初始化的env的setjmp调用] 的返回
void longjmp(jmp_buf env, int val);
void siglongjmp(sigjmp_buf env, int val);
```

C++和Java中的软件异常：是C语言中setjmp和longjmp函数的更加结构化的版本。

try:

​	catch -> setjmp

​	throw -> longjmp