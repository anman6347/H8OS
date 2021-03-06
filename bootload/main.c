#include "defines.h"
#include "interrupt.h"
#include "serial.h"
#include "lib.h"
#include "xmodem.h"
#include "elf.h"

static int init(void) {
    extern int erodata, data_start, edata, bss_start, ebss;

    /* data 領域と bss 領域の初期化 */
    memcpy(&data_start, &erodata, (long)&edata - (long)&data_start); // ROM の data セクションを RAM の data セクションにコピー
    memset(&bss_start, 0, (long)&ebss - (long)&bss_start); // RAM の bss セクションを 0 埋め

    /* ソフトウェア・割込みベクタの初期化 */
    softvec_init();

    /* シリアルの初期化 */
    serial_init(SERIAL_DEFAULT_DEVICE_INDEX);
    return 0;
}

static int dump(char *buf, long size) {
    long i;

    if (size < 0) {
        puts("no data.\n");
        return -1;
    }

    for(i = 0; i < size; i++) {
        putxval(buf[i], 2);
        if ((i & 0xf) == 15) {
            puts("\n");
        } else {
            if ((i & 0xf) == 7) puts(" ");
            puts(" ");
        }
    }
    puts("\n");
    return 0;
}

static void wait() {
    volatile long i;
    for(i = 0; i < 300000; i++)
        ;
}

int main(void) {

    static char buf[16];
    static long size = -1;
    static unsigned char *loadbuf = NULL;
    char *entry_point;
    void (*f)(void);

    extern int buffer_start; // リンカスクリプトで定義されているバッファ。直接アドレスがわかる

    INTR_DISABLE; // 割込み無効
    init(); // data, bss領域の処理や、ソフトウェア・割込み、シリアルの初期化

    puts("kozos boot loader started.\n");

    while(1) {
        puts("kzload> "); // プロンプト表示
        gets(buf); // シリアルからのコマンド受信

        if (!strcmp(buf, "load")) { // XMODEM でファイルダウンロード
            loadbuf = (char *) (&buffer_start);
            size = xmodem_recv(loadbuf);
            wait();

            if (size < 0) {
                puts("\nXMODEM receive error!\n");
            } else {
                puts("\nXMODEM receive succeed.\n");
            }
        } else if (!strcmp(buf, "dump")) { // メモリの 16進ダンプ出力
            puts("size: ");
            putxval(size, 0);
            puts("\n");
            dump(loadbuf, size);
        } else if (!strcmp(buf, "run")) { // ELF 形式ファイルの実行
            entry_point = elf_load(loadbuf); // メモリ上に展開（ロード）
            if (!entry_point) puts("run error!\n");
            else {
                puts("starting from entry point: ");
                putxval((unsigned long)entry_point, 0);
                puts("\n");
                f = (void (*)(void))entry_point;
                f(); // ここで、ロードしたプログラムに処理を渡す
                /* ここには返ってこない */
            }
        } else {
            puts("unknown.\n");
        }
    }

    return 0;
}
