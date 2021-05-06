#ifndef LIB_H_
#define LIB_H_

void *memset(void *b, int c, long len);               // メモリを特定のバイトデータで埋める
void *memcpy(void *dst, const void *src, long len);   // メモリのコピー
int memcmp(const void *b1, const void *b2, long len); // メモリの比較
int strlen(const char *s);                            // 文字列の長さ取得
char *strcpy(char *dst, const char *src);             // 文字列コピー
int strcmp(const char *s1, const char *s2);           // 文字列比較
int strncmp(const char *s1, const char *s2, int len); // 長さ指定での文字列比較
int putc(unsigned char c);                            // 1 文字送信
unsigned char getc();                                 // 1 文字受信
int puts(unsigned char *str);                         // 文字列送信
int gets(unsigned char *buf);                         // 文字列受信
int putxval(unsigned long value, int column);         // 数値の16進表示

#endif
