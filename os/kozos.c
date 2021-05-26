#include "kozos.h"
#include "defines.h"
#include "intr.h"
#include "interrupt.h"
#include "syscall.h"
#include "lib.h"

#define THREAD_NUM 6  // TCB の数
#define THREAD_NAME_SIZE 15  // スレッド名の最大長

/* スレッド・コンテキスト */
typedef struct _kz_context {
    uint32 sp;  // スタックポインタ
} kz_context;

/* タスクコントロールブロック TCB */
typedef struct _kz_thread {
    struct _kz_thread *next;  // キューへの接続に利用するポインタ
    char name[THREAD_NAME_SIZE + 1];  // スレッド名
    char *stack;  // スレッドのスタック

    struct { // スレッドのスタートアップ thread_init() に渡すパラメータ
        kz_func_t func;  // スレッドのメイン関数
        int argc;  // スレッドのメイン関数に渡す argc
        char **argv;  // スレッドのメイン関数に渡す argv
    } init;

    struct {  // システムコール用のバッファ
        kz_syscall_type_t type;
        kz_syscall_param_t *param;
    } syscall;

    kz_context context;  // コンテキスト情報
} kz_thread;

/* スレッドのレディーキュー */
static struct {
    kz_thread *head;
    kz_thread *tail;
} readyque;

static kz_thread *current;  // カレントスレッド
static kz_thread threads[THREAD_NUM];  // TCB
static kz_handler_t handlers[SOFTVEC_TYPE_NUM];  // 割込みハンドラ

void dispatch(kz_context *context);  // スレッドのディスパッチ用関数（実体はアセンブラ）

/* カレントスレッドをキューから取り出す */
static int getcurrent() {
    if (current == NULL)
        return -1;

    /* カレントスレッドを先頭から抜き出す */
    readyque.head = current->next;
    if (readyque.head == NULL)
        readyque.tail = NULL;
    return 0;
}

/* カレントスレッドをキューに入れる */
static int putcurrent() {
    if (current == NULL)
        return -1;

    /* キューの末尾に接続 */
    if (readyque.tail) {
        readyque.tail->next = current;
    } else {
        readyque.head = current;
    }
    readyque.tail = current;

    return 0;
}

/* スレッドの終了 */
static void thread_end() {
    kz_exit();
}

/* スレッドのスタートアップ */
static void thread_init(kz_thread *thp) {
    thp->init.func(thp->init.argc, thp->init.argv);  // スレッドのメイン関数を呼び出す
    thread_end();  // メイン関数から戻ってきたら, スレッドを終了
}

/* システムコールの処理 kz_run(): スレッドの生成, 実行 */
static kz_thread_id_t thread_run(kz_func_t func, char *name, int stacksize, int argc, char *argv[]) {
    int i;
    kz_thread *thp;
    uint32 *sp;
    extern char userstack; // リンカスクリプトで定義されているスタック領域
    static char *thread_stack = &userstack;

    /* 空いている TCB の検索 */
    for (i = 0; i < THREAD_NUM; i++ ) {
        thp = &threads[i];
        if (!thp->init.func)  // 見つかった
            break;
    }
    if (i == THREAD_NUM)  // 見つからなかった
        return -1;

    memset(thp, 0, sizeof(*thp));  // TCB をゼロクリア

    /* TCB の各種パラメーターの設定 */
    strcpy(thp->name, name);
    thp->next = NULL;

    thp->init.func = func;
    thp->init.argc = argc;
    thp->init.argv = argv;

    /* スタック領域の獲得 */
    memset(thread_stack, 0, stacksize);
    thread_stack += stacksize;

    thp->stack = thread_stack;  // スタックを設定 

    /* スタックの初期化 */
    sp = (uint32 *)thp->stack;
    *(--sp) = (uint32)thread_end;  // thread_init() からの戻り先として thread_end() を設定

    /* プログラムカウンタを設定 */
    *(--sp) = (uint32)thread_init;  // ディスパッチ時にプログラムカウンタに thread_init() が来るように配置
    *(--sp) = 0;  // ER6
    *(--sp) = 0;  // ER5
    *(--sp) = 0;  // ER4
    *(--sp) = 0;  // ER3
    *(--sp) = 0;  // ER2
    *(--sp) = 0;  // ER1
    /* スレッドのスタートアップ thread_init() に渡す引数 */
    *(--sp) = (uint32)thp;  // ER0

    /* スレッドのコンテキストの設定 */
    thp->context.sp = (uint32)sp;

    /* システムコールを呼び出したスレッドをキューに戻す */
    putcurrent();

    /* 新規生成したスレッドをキューに接続 */
    current = thp;
    putcurrent();

    return (kz_thread_id_t)current;  // 新規作成したスレッドのアドレスをスレッドIDとして返す
}

/* システムコールの処理 kz_exit(): スレッドの終了 */
static int thread_exit() {
    // スタックの解法は省略
    puts(current->name);
    puts(" EXIT.\n");  // 終了時にメッセージを出力

    memset(current, 0, sizeof(*current));  // TCB をゼロクリア
    return 0;
}

/* 割込み処理の入口関数 */
static void thread_intr(softvec_type_t type, unsigned long sp);
/* 割込みハンドラの登録 */
static int setintr(softvec_handler_t type, kz_handler_t handler) {  // 割込みハンドラの登録
    softvec_setintr(type, thread_intr);  // 割込み時にOSハンドラが呼ばれるように割込みベクタを設定
    handlers[(short)type] = handler;  // OS側から呼び出す割込みハンドラを登録
    return 0;
}

/* システムコールの処理関数の呼び出し */
static void call_functions(kz_syscall_type_t type, kz_syscall_param_t *p) {
    /* システムコールの実行中に current が切り替わる */
    switch (type) {
    case KZ_SYSCALL_TYPE_RUN:  /* kz_run() */
        p->un.run.ret = thread_run(p->un.run.func, p->un.run.name, p->un.run.stacksize,
                                    p->un.run.argc, p->un.run.argv);
        break;
    case KZ_SYSCALL_TYPE_EXIT:  /* kz_exit() */
        /* TCB が削除されるので戻り値は書き込まない */
        thread_exit();
        break;
    default:
        break;
    }
}

/* 
 * システムコールを呼び出したスレッドをキューから外した状態で実行する
 * このため,システムコールを呼び出したスレッドを動作継続させるには,
 * 処理関数内部で putcurrent() をする必要がある
 */
static void syscall_proc(kz_syscall_type_t type, kz_syscall_param_t* p) {
    getcurrent();  // カレントスレッドをキューから外す
    call_functions(type, p);  // システムコールの処理関数を呼び出す
}

/* スレッドのスケジューリング */
static void schedule() {
    if (!readyque.head)  // 見つからなかった
        kz_sysdown();
    
    current = readyque.head;  // キューの先頭スレッドをスケジューリング（ラウンド・ロビン式）
}

/* システムコールの呼び出し */
static void syscall_intr() {
    syscall_proc(current->syscall.type, current->syscall.param);
}

/* ソフトウェア・エラーの発生 */
static void softerr_intr() {
    puts(current->name);
    puts(" DOWN.\n");
    getcurrent();  // キューから外す
    thread_exit();  // スレッドを終了する
}


/* 割込み処理の入口関数 */
static void thread_intr(softvec_type_t type, unsigned long sp) {
    /* カレントスレッドのコンテキストを保存する */
    current->context.sp = sp;

    /* 割込みごとの処理を実行 */
    if (handlers[type])
        handlers[type]();
    
    schedule();  // スケジューリング
    
    /* スレッドのディスパッチ */
    dispatch(&(current->context));

    /* ここには返ってこない */
}

/* 初期スレッドを起動し, OSの動作を開始する */
void kz_start(kz_func_t func, char *name, int stacksize, int argc, char *argv[] ) {
    current = NULL;  // current を NULL で初期化

    // 各種データの初期化
    readyque.head = readyque.tail = NULL;
    memset(threads, 0, sizeof(threads));
    memset(handlers, 0, sizeof(handlers));

    /* 割込みハンドラの登録 */
    setintr(SOFTVEC_TYPE_SYSCALL, syscall_intr);  // システムコール
    setintr(SOFTVEC_TYPE_SOFTERR, softerr_intr);  // ダウン要因発生

    /* システムコール発行不可のため直接スレッドを作成する */
    current = (kz_thread *)thread_run(func, name, stacksize, argc, argv);

    /* 最初のスレッドを起動 */
    dispatch(&(current->context));

    /* ここには返ってこない */
}

/* OS内部で致命的エラーが発生したとき呼ぶ関数 */
void kz_sysdown() {
    puts("system error!\n");
    while(1)
        ;
}

/* システムコール呼び出し用ライブラリ関数 */
void kz_syscall(kz_syscall_type_t type, kz_syscall_param_t *param) {
    current->syscall.type = type;
    current->syscall.param = param;
    asm volatile ("trapa #0");  // トラップ割込み発行
}