#ifndef DEFINES_H_
#define DEFINES_H_

#define NULL ((void *)0)
#define SERIAL_DEFAULT_DEVICE_INDEX 1

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;

typedef uint32 kz_thread_id_t;  // スレッドID
typedef int (*kz_func_t) (int argc, char *argv[]);  // スレッドのメイン関数の型
typedef void (*kz_handler_t) (void);  // 割込みハンドラの型
#endif
