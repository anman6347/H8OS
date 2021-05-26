#ifndef SYSCALL_H_
#define SYSCALL_H_

#include "defines.h"

/* システムコール番号の定義 */
typedef enum {
    KZ_SYSCALL_TYPE_RUN = 0,  // kz_run() のシステムコール番号
    KZ_SYSCALL_TYPE_EXIT,  // kz_exit() のシステムコール番号
} kz_syscall_type_t;

/* システムコール呼び出し時のパラメータ格納域の定義 */
typedef struct {
    union {
        /* kz_run() のためのパラメータ */
        struct {
            kz_func_t func;  // メイン関数
            char *name;  // スレッド名
            int stacksize;  // スタックのサイズ
            int argc;  // メイン関数に渡す引数の個数
            char **argv;  // メイン関数に渡す引数
            kz_thread_id_t ret;  // kz_run() の戻り値
        } run;
        /* kz_exit() のためのパラメータ */
        struct {
            int dummy;  // パラメータ無し
        } exit;
    } un;
} kz_syscall_param_t;

#endif
