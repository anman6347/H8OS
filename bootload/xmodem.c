#include "defines.h"
#include "serial.h"
#include "lib.h"
#include "xmodem.h"

#define XMODEM_SOH 0x01
#define XMODEM_STX 0x02
#define XMODEM_EOT 0x04
#define XMODEM_ACK 0x06
#define XMODEM_NAK 0x15
#define XMODEM_CAN 0x18
#define XMODEM_EOF 0x1a

#define XMODEM_BLOCK_SIZE 128

/* 受信開始されるまで送信要求を出す */
static int xmodem_wait() {
    long cnt = 0;

    while(!serial_is_recv_enable(SERIAL_DEFAULT_DEVICE_INDEX)) {
        if(++cnt >= 2000000) {
            cnt = 0;
            serial_send_byte(SERIAL_DEFAULT_DEVICE_INDEX, XMODEM_NAK); // 受信開始まで NAK を定期的に送信
        }
    }
    return 0;
}

/* ブロックの単位での受信 */
static int xmodem_read_block(unsigned char block_number, char *buf) {
    unsigned char c, block_num, check_sum;
    int i;

    block_num = serial_recv_byte(SERIAL_DEFAULT_DEVICE_INDEX); // ブロック番号の受信
    if (block_num != block_number)
        return -1;
    
    block_num ^= serial_recv_byte(SERIAL_DEFAULT_DEVICE_INDEX); // 反転したブロック番号の受信
    if (block_num != 0xff)
        return -1;
    
    // 128 バイトデータ受信
    check_sum = 0;
    for (i = 0; i < XMODEM_BLOCK_SIZE; i++) {
        c = serial_recv_byte(SERIAL_DEFAULT_DEVICE_INDEX);
        *(buf++) = c;
        check_sum += c;
    }

    check_sum ^= serial_recv_byte(SERIAL_DEFAULT_DEVICE_INDEX); // チェックサム受信
    if (check_sum)
        return -1;

    return i;
}


long xmodem_recv(char *buf) {
    int r, recieving = 0;
    long size = 0;
    unsigned char c, block_number = 1;

    while(1) {
        if (!recieving) xmodem_wait(); // 受信されるまで送信要求を出す

        c = serial_recv_byte(SERIAL_DEFAULT_DEVICE_INDEX);  // 1 文字の受信

        if (c == XMODEM_EOT) { // EOT 受信
            serial_send_byte(SERIAL_DEFAULT_DEVICE_INDEX, XMODEM_ACK); // ACK 送信で終了
            break;
        } else if (c == XMODEM_CAN) { // CAN 受信で受信中断
            return -1;
        } else if (c == XMODEM_SOH) { // SOH 受信で受信開始
            recieving++;
            r = xmodem_read_block(block_number, buf); // ブロック単位での受信
            if (r < 0) { // 受信エラー
                serial_send_byte(SERIAL_DEFAULT_DEVICE_INDEX, XMODEM_NAK); // エラー時は NAK を返す
            } else { // 正常終了
                block_number++;
                size += r;
                buf += r;
                serial_send_byte(SERIAL_DEFAULT_DEVICE_INDEX, XMODEM_ACK);
            }
        } else{
            if(recieving)
                return -1;
        }
    }
    return size;
}
