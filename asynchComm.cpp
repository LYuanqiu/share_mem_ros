#include <unistd.h>
#include <fcntl.h>
#include "shareMem.h"
#include "sys/mman.h"
#include <linux/ioctl.h>

asynchwriShm::asynchwriShm(char *Name) {
    memcpy((void *)name, Name, strlen(Name));
    while (!allocMem(Name)) {
        sleep(SleepTime);
    }
    *valid = 0;
    *start = 0;
    *flag = 0;
    *end = 0;
    // TODO 读进程的数量
    for (int i = 0;i < 5;i++) {
        readAddr[i] = -1;
    }
}

bool asynchwriShm::allocMem(char *Name) {
    int mem_fd = open("/dev/share_mem_receive", O_RDWR);
    void *addr = NULL;
    int shyper_fd = open("/dev/shyper", O_RDWR);
    // TODO 接口的结构体的对应顺序
    struct {
        char *name;
        size_t size;
        int sync;
        void** va_ptr;
        int type;
        int *no;
    } ioctl_alloc;
    ioctl_alloc.type = WRITE;
    ioctl_alloc.sync = ASYNC;
    ioctl_alloc.va_ptr = &addr;
    ioctl_alloc.size = CommSize;
    ioctl_alloc.no = NULL;
    memcpy(ioctl_alloc.name, Name, strlen(Name));
    ioctl(shyper_fd, IOCTL_ALLOC_CHANNEL, &ioctl_alloc);
    valid = (int *) addr;
    start = (int *) addr + 1;
    end = (int *) addr + 2;
    flag = (int *)addr + 3;
    readAddr = (int *) addr + 4;
    vaddr = (int *) addr + 10;
    return true;
}

bool asynchwriShm::isConflict(int start, int end) {
    for (int i = 0; i < 5; i++) {
        if (readAddr[i] >= start && readAddr[i] < end) {
            return true;
        }
    }
    return false;
}

int asynchwriShm::send(void *info, int size) {
    // 根据待发送的消息是否跨页分两种情况讨论
    if (*end + size> CommSize) {
        while (isConflict(*end, (size + *end) - CommSize)) {
            sleep(SleepTime);
        }
        memcpy(vaddr + *end, info, CommSize - *end);
        memcpy(vaddr, (char *)info + CommSize - *end, (size + *end) - CommSize);
        *start = *end;
        *end = (size + *end) - CommSize;
        *flag = 0;
        *valid = 1;
    } else {
        while (isConflict(*end, *end + size)) {
            sleep(SleepTime);
        }
        memcpy(vaddr + *end, info, size);
        *start = *end;
        *end = *end + size;
        *flag = 0; // TODO flag 会有加锁的问题
        *valid = 1;
    }
}

bool asynchwriShm::releaseMem() {
    int shyper_fd = open("/dev/shyper", O_RDWR);
    // TODO 接口
    struct {
        char *name;
    } ioctl_release;
    memcpy(ioctl_release.name, name, strlen(name));
    ioctl(shyper_fd, IOCTL_RELEASE_CHANNEL, &ioctl_release);
    close(shyper_fd);
    return true;
}

asynchredShm::asynchredShm(char *Name) {
    memcpy(name, Name, strlen(Name));
    while (!allocMem(Name)) {
        sleep(SleepTime);
    }
}

bool asynchredShm::allocMem(char *Name) {
    int mem_fd = open("/dev/share_mem_receive", O_RDWR);
    void *addr = NULL;
    int shyper_fd = open("/dev/shyper", O_RDWR);
    // TODO 接口顺序
    struct {
        int type;
        char *name;
        size_t size;
        int sync;
        void** va_ptr;
        int *no;
    } ioctl_alloc;
    ioctl_alloc.type = READ;
    ioctl_alloc.no = &no;
    ioctl_alloc.sync = ASYNC;
    ioctl_alloc.va_ptr = &addr;
    ioctl_alloc.size = CommSize;
    memcpy(ioctl_alloc.name, Name, strlen(Name));
    ioctl(shyper_fd, IOCTL_ALLOC_CHANNEL, &ioctl_alloc);
    valid = (int *) addr;
//    redNode = (int *)addr + 1;
    start = (int *) addr + 1;
    end = (int *) addr + 2;
    flag = (int *)addr + 3;
    readAddr = (int *) addr + 4;
    vaddr = (int *) addr + 10;
    return true;
}

int asynchredShm::rece(void *rinfo) {
    while (*valid != 1 || ((*flag & (1 << no)) != 0)) {}
    int s = *start;
    int en = *end;
    readAddr[no] = *start;
    *flag = *flag | (1 << no);
    if (s < en) {
        memcpy(rinfo, vaddr + s, en - s);
    } else {
        memcpy(rinfo, vaddr + s, CommSize - s);
        memcpy((char *)rinfo + (CommSize - s), vaddr, en);
    }
    readAddr[no] = -1;
    return 0;
}

bool asynchredShm::releaseMem() {
    int shyper_fd = open("/dev/shyper", O_RDWR);
    // TODO 接口
    struct {
        char *name;
        int no;
    } ioctl_release;
    ioctl_release.no = no;
    memcpy(ioctl_release.name, name, strlen(name));
    ioctl(shyper_fd, IOCTL_RELEASE_CHANNEL, &ioctl_release);
    close(shyper_fd);
    return true;
}

