#include "defines.h"

#define SERIAL_SCI_NUM 3

#define H8_3069F_SCI0 ((volatile struct h8_3069f_sci*)0xffffb0)
#define H8_3069F_SCI1 ((volatile struct h8_3069f_sci*)0xffffb8)
#define H8_3069F_SCI2 ((volatile struct h8_3069f_sci*)0xffffc0)

/* SCIのレジスタを管理する構造体 */
struct h8_3069f_sci {
    volatile uint8 smr;  // モード設定を管理するレジスタ
    volatile uint8 brr;  // ボーレート設定を管理するレジスタ
    volatile uint8 scr;  // 送受信と割り込み設定を管理するレジスタ
    volatile uint8 tdr;  // 送信データを管理するレジスタ
    volatile uint8 ssr;  // ステータスを管理するレジスタ
    volatile uint8 rdr;  // 受信データを管理するレジスタ
    volatile uint8 scmr; // スマートカードを管理するレジスタ
};

/* シリアルモードレジスタ SMR */
#define H8_3069F_SCI_SMR_CKS0 (0 << 0)
#define H8_3069F_SCI_SMR_CKS1 (1 << 0)
#define H8_3069F_SCI_SMR_MP   (1 << 2)
#define H8_3069F_SCI_SMR_STOP (1 << 3)
#define H8_3069F_SCI_SMR_OE   (1 << 4)
#define H8_3069F_SCI_SMR_PE   (1 << 5)
#define H8_3069F_SCI_SMR_CHR  (1 << 6)
#define H8_3069F_SCI_SMR_CA   (1 << 7)

/* シリアルコントロールレジスタ SCR*/
#define H8_3069F_SCI_SCR_CKE0 (1 << 0)
#define H8_3069F_SCI_SCR_CKE1 (1 << 1)
#define H8_3069F_SCI_SCR_TEIE (1 << 2)
#define H8_3069F_SCI_SCR_MPIE (1 << 3)
#define H8_3069F_SCI_SCR_RE   (1 << 4) /* 受信有効 */
#define H8_3069F_SCI_SCR_TE   (1 << 5) /* 送信有効 */
#define H8_3069F_SCI_SCR_RIE  (1 << 6) /* 受信割込み有効 */
#define H8_3069F_SCI_SCR_TIE  (1 << 7) /* 送信割込み有効 */

/* シリアルステータスレジスタ SSR */
#define H8_3069F_SCI_SSR_MPBT   (1 << 0)
#define H8_3069F_SCI_SSR_MPB    (1 << 1)
#define H8_3069F_SCI_SSR_TEND   (1 << 2)
#define H8_3069F_SCI_SSR_PER    (1 << 3)
#define H8_3069F_SCI_SSR_FERERS (1 << 4)
#define H8_3069F_SCI_SSR_ORER   (1 << 5)
#define H8_3069F_SCI_SSR_RDRF   (1 << 6) /* 受信完了 */
#define H8_3069F_SCI_SSR_TDRE   (1 << 7) /* 送信完了 */


/* 3チャネルの SCI の先頭アドレスをメンバ変数として格納した構造体配列*/
static struct {
    volatile struct h8_3069f_sci* sci;
} regs[SERIAL_SCI_NUM] = {
    H8_3069F_SCI0,
    H8_3069F_SCI1,
    H8_3069F_SCI2};



/* デバイス初期化 */
int serial_init(int index) {
    volatile struct h8_3069f_sci* sci = regs[index].sci; // index チャネルの SCI のアドレス

    sci->scr = 0;
    sci->smr = 0; 
    sci->brr = 64; // 20MHz クロック 分周比 1 ボーレート 9600bps
    sci->scr = H8_3069F_SCI_SCR_RE | H8_3069F_SCI_SCR_TE; // 送受信可能に
    sci->ssr = 0;

    return 0;
}

/* 送信可能か */
int serial_is_send_enable(int index) {
    volatile struct h8_3069f_sci* sci = regs[index].sci; // index チャネルの SCI のアドレス
    return (sci->ssr & H8_3069F_SCI_SSR_TDRE);
}

/* 1 byte 送信 */
int serial_send_byte(int index, unsigned char c) {
    volatile struct h8_3069f_sci* sci = regs[index].sci; // index チャネルの SCI のアドレス

    /* 送信可能になるまで待機 */
    while(!serial_is_send_enable(index))
        ;
    sci->tdr = c;
    sci->ssr &= ~H8_3069F_SCI_SSR_TDRE; // 送信開始

    return 0;
}

/* 受信可能か */
int serial_is_recv_enable(int index) {
    volatile struct h8_3069f_sci *sci = regs[index].sci;
    return (sci->ssr & H8_3069F_SCI_SSR_RDRF);
}

/* 1 文字受信 */
unsigned char serial_recv_byte(int index) {
    volatile struct h8_3069f_sci *sci = regs[index].sci;
    unsigned char c;

    /* 受信データ取得まで待機 */
    while(!serial_is_recv_enable(index))
        ;
    c = sci->rdr;
    sci->ssr &= ~H8_3069F_SCI_SSR_RDRF; /* 次のデータを受信可能にする */
    
    return c;
}

/* 送信割込み有効か */
int serial_intr_is_send_enable(int index) {
    volatile struct h8_3069f_sci *sci = regs[index].sci;
    return (sci->scr & H8_3069F_SCI_SCR_TIE) ? 1 : 0;
}

/* 送信割込み有効化 */
void serial_intr_send_enable(int index) {
    volatile struct h8_3069f_sci *sci = regs[index].sci;
    sci->scr |= H8_3069F_SCI_SCR_TIE;
}

/* 送信割込み無効化 */
void serial_intr_send_disable(int index) {
    volatile struct h8_3069f_sci *sci = regs[index].sci;
    sci->scr &= ~H8_3069F_SCI_SCR_TIE;
}

/* 受信割込み有効か */
int serial_intr_is_recv_enable(int index) {
    volatile struct h8_3069f_sci *sci = regs[index].sci;
    return (sci->scr & H8_3069F_SCI_SCR_RIE) ? 1 : 0;
}

/* 受信割込み有効化 */
void serial_intr_recv_enable(int index) {
    volatile struct h8_3069f_sci *sci = regs[index].sci;
    sci->scr |= H8_3069F_SCI_SCR_RIE;
}

/* 受信割込み無効化 */
void serial_intr_recv_disable(int index) {
    volatile struct h8_3069f_sci *sci = regs[index].sci;
    sci->scr &= ~H8_3069F_SCI_SCR_RIE;
}