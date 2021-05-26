#include "defines.h"
#include "lib.h"
#include "interrupt.h"
#include "kozos.h"

/* システムタスクとユーザースレッドの起動 */
static int start_threads(int argc, char *argv[]) {  // 初期スレッドのメイン関数
    kz_run(test08_1_main, "command", 0x100, 0, NULL);  // コマンド処理スレッドの起動
    return 0;
}

int main() {
    INTR_DISABLE;  // 割込み無効化

    puts("kozos boot succeed!\n");
    
    /* OSの動作開始 */
    kz_start(start_threads, "start", 0x100, 0, NULL);

    /* ここには戻ってこない */

    return 0;
}