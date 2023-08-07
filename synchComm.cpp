#include <unistd.h>
#include <fcntl.h>
#include "shareMem.h"
#include "sys/mman.h"
#include <linux/ioctl.h>

synchwriShm::synchwriShm(char *Name) {
    memcpy(name, Name, strlen(Name));
    while (!allocMem(Name)) {
        sleep(SleepTime);
    }
    *status = Waiting;
    *size = 0;
}

bool synchwriShm::allocMem(char *Name) {
    void *addr = NULL;
    int shyper_fd = open("/dev/shyper", O_RDWR);
    // todo 接口
    struct {
        char *name;
        size_t size;
        int sync;
        void** va_ptr;
        int type;
        int *no;
    } ioctl_alloc;
    ioctl_alloc.no = NULL;
    ioctl_alloc.sync = SYNC;
    ioctl_alloc.type = WRITE;
    ioctl_alloc.size = CommSize;
    ioctl_alloc.va_ptr = &addr;
    memcpy(ioctl_alloc.name, Name, strlen(Name));
    ioctl(shyper_fd, IOCTL_ALLOC_CHANNEL, &ioctl_alloc);
    finish = (int *) addr;
    size = (int *) addr + 1;
    status = (int *)addr + 2;
    vaddr = (int *) addr + 10; // 为了跟异步同样的读写空间CommSize
    return true;
}

int synchwriShm::send(void *info, int Size, void *rinfo) {
    int s = Size;
    while (s > 0) {
        if (s > CommSize) {
            int ss = CommSize;
            while (*status != Waiting) {
                sleep(SleepTime);
            }
            memcpy(vaddr, (char *)info + (Size - s), ss);
            *size = ss;
            *finish = 0;
            *status = SendFinish;
            s -= ss;
        } else {
            int ss = s;
            while (*status != Waiting) {
                sleep(SleepTime);
            }
            memcpy(vaddr, (char *)info + (Size - s), ss);
            *size = ss;
            *finish = 1;
            *status = SendFinish;
        }
    }

    int rspOff = 0;
    while(true) {
        while (*status != RespFinish) {
            sleep(SleepTime);
        }
        if (*finish == 1) {
            memcpy((char *)rinfo + rspOff, vaddr, *size);
            rspOff += *size;
            *status = Waiting;
            break;
        } else {
            memcpy((char *)rinfo + rspOff, vaddr,*size);
            rspOff += *size;
            *status = Waiting;
        }
    }
    return rspOff;
}

bool synchwriShm::releaseMem() {
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

synchredShm::synchredShm(char *Name) {
//    this->name = name; // todo 给string是不是不能直接赋值啊？
    memcpy(name, Name, strlen(Name));
    while (!allocMem(Name)) {
        sleep(SleepTime);
    }
}

bool synchredShm::allocMem(char *Name) {
    void *addr = NULL;
    int shyper_fd = open("/dev/shyper", O_RDWR);
    // TODO 接口
    struct {
        char *name;
        size_t size;
        int sync;
        void** va_ptr;
        int type;
    } ioctl_alloc;
    ioctl_alloc.sync = SYNC;
    ioctl_alloc.type = READ;
    ioctl_alloc.size = CommSize;
    ioctl_alloc.va_ptr = &addr;
    memcpy(ioctl_alloc.name, Name, strlen(Name));
    ioctl(shyper_fd, IOCTL_ALLOC_CHANNEL, &ioctl_alloc);
    finish = (int *) addr;
    size = (int *) addr + 1;
    status = (int *)addr + 2;
    vaddr = (int *) addr + 10;
    return true;
}

int synchredShm::rece(void *rinfo) {
    int off = 0;
    while (true) {
        while (*status != SendFinish) {
            sleep(SleepTime);
        }
        if (*finish == 0) {
            memcpy((char *)rinfo + off, vaddr, *size);
            *status = Waiting;
            off += *size;
        } else {
            memcpy((char *)rinfo + off, vaddr, *size);
            *status = RecvFinish;
            off += *size;
            break;
        }
    }
    return off;
}

int synchredShm::send(void *info, int size) {
    int s = size;
    while (s > 0) {
        if (s > CommSize) {
            int ss = CommSize;
            while (*status != Waiting) {
                sleep(SleepTime);
            }
            memcpy(vaddr, (char *)info + (size - s), ss);
            *(this->size) = ss;
            *finish = 0;
            *status = RespFinish;
            s -= ss;
        } else {
            int ss = s;
            while (*status != Waiting) {
                sleep(SleepTime);
            }
            memcpy(vaddr, (char *)info + (size - s), ss); // TODO 大小端
            *(this->size) = ss;
            *finish = 1;
            *status = RespFinish;
        }
    }
    return 0;
}

bool synchredShm::releaseMem() {
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

