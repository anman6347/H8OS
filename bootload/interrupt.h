#ifndef INTERRUPT_H_
#define INTERRUPT_H_

extern char softvec; // リンカスクリプトで定義されているシンボル
#define SOFTVEC_ADDR (&softvec)


typedef short softvec_type_t; // ソフトウェア・割込みベクタの種別を表す型の定義
typedef void (*softvec_handler_t) (softvec_type_t type, unsigned long sp); // 割込みハンドラ

#define SOFTVECS ((softvec_type_t *)SOFTVEC_ADDR) // ソフトウェア・割込みベクタの位置

#define INTR_ENABLE asm volatile ("andc.b #0x3f,ccr") // 割り込み有効化 ccr に 0x00dddddd   d は保持
#define INTR_DISABLE asm volatile ("orc.b #0xc0,ccr") // 割り込み無効化 ccr に 0x11cccccc   c は保持

int softvec_init(); // ソフトウェア・割込みベクタの初期化用関数
int softvec_setintr(softvec_type_t type, softvec_handler_t handler); // ソフトウェア・割込みベクタ設定用関数
void interrupt(softvec_type_t type, unsigned long sp); // ソフトウェア・割込みベクタを処理するための共通ハンドラ

#endif
