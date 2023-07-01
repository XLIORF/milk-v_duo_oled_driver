#ifndef __SSD1306_H__
#define __SSD1306_H__


#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>


// static ssize_t dev_write(struct file *file, const char __user *buf, size_t count, loff_t *fpos);
static int i2c_dev_write(char *buf, unsigned int count);

// static int i2c_dev_read(char *buf, unsigned int count);
void OLED_WriteCommand(uint8_t Command);
void OLED_WriteData(uint8_t Data);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
uint32_t OLED_Pow(uint32_t X, uint32_t Y);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_SetCursor(uint8_t Y, uint8_t X);
void OLED_Clear(void);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_Init(void);
int OLED_Probe(struct i2c_client *client, const struct i2c_device_id *id);
int ssd1306_char_open(struct inode *inode, struct file *file);
int ssd1306_char_close(struct inode *inode, struct file *file);
ssize_t ssd1306_char_read(struct file *file, char __user *ubuf, size_t size, loff_t *loff);
ssize_t ssd1306_char_write(struct file *file, const char __user *ubuf, size_t size, loff_t *offs);
long ssd1306_char_ioctl(struct file *file, unsigned int cmd, long unsigned int args);

#endif