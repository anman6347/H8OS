#include "defines.h"
#include "serial.h"
#include "lib.h"
#include "intr.h"
#include "interrupt.h"

static void intr(softvec_type_t type, unsigned long sp) {
    int c;
    static char buf[32]; // 受信バッファ
    static int len; // bss: 0で初期化される

    c = getc(); // 受信割込みが入ったので, 1文字受信

    if (c != '\n') {
        buf[len++] = c;
    } else { // 改行文字を受け取った場合
        buf[len++] = '\0';
        if (!strncmp(buf, "echo", 4)) {
            puts(buf + 4);  // 後続の文字の出力
            puts("\n");
        } else {
            puts("unknown.\n");
        }
        puts("> ");
        len = 0;
    }

}

int main() {
    INTR_DISABLE; // 割込み無効化

    puts("kozos boot succeed!\n");
    puts("----------------------------\n\n");

    softvec_setintr(SOFTVEC_TYPE_SERINTR, intr);  // ソフトウェア・割込みベクタにシリアル割込みのハンドラを設定
    serial_intr_recv_enable(SERIAL_DEFAULT_DEVICE_INDEX); // シリアル受信割込みを有効化

    puts("> ");
    
    INTR_ENABLE; // 割込み有効化

    while (1) {
        asm volatile ("sleep");  // 省電力モードに移行
    }
    return 0;
}