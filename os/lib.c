#include "lib.h"
#include "defines.h"
#include "serial.h"

/* メモリを特定のバイトデータで埋める */
void *memset(void *b, int c, long len) { // int が 16bit のため long を使用
    char *p;
    for (p = b; len > 0; len--) {
        *(p++) = c;
    }
    return b;
}

/* メモリのコピー */
void *memcpy(void* dst, const void *src, long len) {
    char *d = dst;
    const char *s = src;
    for (; len > 0; len--) {
        *(d++) = *(s++);
    }
    return dst;
}

/* メモリの比較 */
int memcmp(const void *b1, const void *b2, long len) {
    const char *p1 = b1, *p2 = b2;
    for (; len > 0; len--) {
        if (*p1 != *p2) return (*p1 > *p2) ? 1 : -1;
        p1++;
        p2++;
    }
    return 0;
}

/* 文字列の長さ取得 */
int strlen(const char *s) {
    int len;
    for (len = 0; *s; s++, len++)
        ;
    return len;
}

/* 文字列コピー */
char *strcpy(char *dst, const char *src) {
    char *d = dst;
    for (;; dst++, src++) {
        *dst = *src;
        if (!*src) break;
    }
    return d;
}

/* 文字列比較 */
int strcmp(const char *s1, const char *s2)
{
  while (*s1 || *s2) {
    if (*s1 != *s2) return (*s1 > *s2) ? 1 : -1;
        s1++;
        s2++;
    }
    return 0;
}

/* 長さ指定での文字列比較 */
int strncmp(const char *s1, const char *s2, int len)
{
  while ((*s1 || *s2) && (len > 0)) {
    if (*s1 != *s2) return (*s1 > *s2) ? 1 : -1;
        s1++;
        s2++;
        len--;
    }
    return 0;
}

/* 1 文字送信 */
int putc(unsigned char c) {
    if(c == '\n') {
        serial_send_byte(SERIAL_DEFAULT_DEVICE_INDEX, '\r'); // 改行コード変換 '\n' を '\r' '\n' へ変換
    }
    return serial_send_byte(SERIAL_DEFAULT_DEVICE_INDEX, c);
}

/* 1 文字受信 */
unsigned char getc() {
    unsigned char c = serial_recv_byte(SERIAL_DEFAULT_DEVICE_INDEX);
    c = (c == '\r') ? '\n' : c; // 改行コード変換
    putc(c); // エコーバック
    return c;
}

/* 文字列送信 */
int puts(unsigned char *str) {
    while(*str) {
        putc(*(str++));
    }
    return 0;
}

/* 文字列受信 */
int gets(unsigned char *buf) {
    int i = 0;
    unsigned char c;
    do {
        c = getc();
        if (c == '\n') c = '\0'; // 改行コードを削る
        buf[i++] = c;
    } while (c); // ヌル文字が一つ入ったらループを抜ける
    return i - 1;
}

/* 数値の 16進表示 表示桁数を column で指定できるが、少ない場合は自動補正*/
int putxval(unsigned long value, int column) {
    char buf[9]; // 文字列出力用バッファ

    // 下の桁から処理
    char *p = buf + sizeof(buf) - 1;
    *(p--) = '\0';

    // value を 2進数から16進数表示へ変換 
    if (!value && !column) column++;
    while ( value || column) {
        *(p--) = "0123456789abcded"[value & 0xf];
        value >>= 4;
        if (column) column--; // 0 埋め
    }

    puts(p + 1);

    return 0;
}
