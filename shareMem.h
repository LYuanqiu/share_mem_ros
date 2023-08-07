#ifndef SHAREMEM_SHAREMEM_H
#define SHAREMEM_SHAREMEM_H
#endif
#include <string>
using namespace std;

#define SYNC 1
#define ASYNC 2
#define IOCTL_RELEASE_CHANNEL 0x2020
#define IOCTL_ALLOC_CHANNEL 0x2021
#define CommSize (4096 - 10 * sizeof(int))
#define SleepTime 0.05
#define WRITE 1
#define READ 0

// TODO 1 大小端
// TODO 2 memcpy各处函数运用是不是对的
// TODO 3 void*类型转换

enum CommStatus {
    Waiting,    // 通道消息为空，等待进程写消息
    SendFinish, // 写完成 等待读取
    RecvFinish, // 读完成
    RespFinish  // 回复完成，
};

/*
 * 同步写
 * 每次调用 send() 函数发送的消息大小不限制 对于较大的消息，while判断多次发送
 */
class synchwriShm {
public:
    synchwriShm(char *name);
                    // 创建一个通信的结构体
                    // 调用allocMem()申请共享内存并完成参数的初始化
    int send(void *info, int Size, void *rinfo);
                        // 发送消息的函数
                        // 参数： 发送消息的地址，消息大小，返回的消息的地址
                        // 返回值：返回消息的大小
    bool releaseMem();
                    // 释放通道的函数

private:
    int *vaddr; // 共享内存写地址
    int *finish; // 消息是否写完的标记位
                 // 当发送消息时，如果当前消息大于一个CommSize，则置0，等读进程读完后再次发送
                 // 当读消息时，如果该标记位为0，则读完后继续等待下一组消息
    int *status; // 通道的状态标记位
                 // 通过该标记位实现读写进程的同步
    int *size;   // 一次发送完成后所发送的消息的大小，
                 // 读取时也读取该大小的消息
    char *name; // 通道的名字
    bool allocMem(char *name);
};

/*
 * 同步读
 * 先调用 rece() 函数读取消息
 * 处理完接收到的消息后调用 send() 函数发送消息
 */
class synchredShm{
public:
    synchredShm(char *name);
    int send(void *info, int size);
                // 发送消息的函数
                // 参数： 发送消息的地址，消息大小，返回的消息的地址
                // 返回值：返回消息的大小
    int rece(void *rinfo);
                // 接收消息的函数
                // 参数：接收消息的地址
                // 返回值：接收到的消息的大小
    bool releaseMem();

private:
    int *vaddr;
    int *status;
    int *finish;
    int *size;
    char *name;
    bool allocMem(char *name);
};

/*
 * 异步写
 *
 */
class asynchwriShm{
public:
    asynchwriShm(char *name);
    int send(void *info, int size); // 发送消息的大小不能大于 CommSize/2
    bool releaseMem();

private:
    int *valid;             // 是否有有效的消息的标记位
    int *start, *end;       // 最新的写完的消息的首地址和尾地址
    int *flag;              // 记录所有读完最新消息的进程
    int *redNode;           //
    int *readAddr;          // 读进程正在读的消息的指针（指向一个数组，每个元素对应序号为该 NO 的读进程正在读的地址）
    int *vaddr;             // 共享内存的地址空间
    char *name;            // 话题名字
    bool isConflict(int, int); // 判断即将写入的地址的是否与正在读进程的地址有冲突
    bool allocMem(char *name); // 申请一块共享内存并初始化相应变量
};

/*
 * 异步读
 */
class asynchredShm{
public:
    asynchredShm (char *name);
    int rece(void *rinfo);
    bool releaseMem();

private:
    char *name;
    int *vaddr;
    int *valid;
    int *flag; // 当发送完一个新消息后 写进程将flag初始化为0
               // 当一个读进程读消息时，需判断(flag & (1 << no))，当为0时可写，否则等新消息发送，
                    // 当读完一个消息时，也应该将这一位置置1
    int *redNode;
    int no;             // 所申请的topic的读进程对应的序号
    int *start, *end;
    int *readAddr;
    bool allocMem(char *name);
};
