#ifndef KOZOS_H_
#define KOZOS_H_

#include "defines.h"
#include "syscall.h"

/* システムコール */

kz_thread_id_t kz_run(kz_func_t func, char *name, int stacksize, int argc, char *argv[]); // スレッド起動のシステムコール
void kz_exit();  // スレッド終了のシステムコール


/* ライブラリ関数 */

void kz_start(kz_func_t func, char *name, int stacksize, int argc, char *argv[]);  // 初期スレッドを起動しOSの動作を開始
void kz_sysdown();  // エラー時に停止
void kz_syscall(kz_syscall_type_t type, kz_syscall_param_t *param);  // システムコールの実行


/* ユーザースレッド */
int test08_1_main(int argc, char *argv[]);  // ユーザースレッドのメイン関数

#endif
