#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/vmalloc.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include "ssd1306.h"
#include "OLED_Font.h"

#define IOC_MAGIC 'o'
#define IOC_MAXNR 2
#define NAME "OLED_128_64_1" // 设备节点名字

#define IO_DIRECTLY_ACCESS 1
#define IOCINIT 0

#define DEBUG_ON
#ifdef DEBUG_ON
#define DEBUG(fmt, args...) printk("[FILE = %s][FUNC = %s][LINE = %d] [" fmt "]\n", __FILE__, __FUNCTION__, __LINE__, ##args)
#else
#define DEBUG(fmt, args...) \
    do                      \
    {                       \
    } while (0)
#endif

struct reg_access_transimission
{
    char *buffer;
    unsigned int len;
} reg_access_transimission;

struct ssd1306_dev
{
    unsigned int major;
    unsigned int minor;
    dev_t devid;
    struct class *class; // 设备节点目录
    struct device *dev;  // 设备
    struct i2c_client *client;
    unsigned char pos[2];
    struct cdev cdev;
};

static struct ssd1306_dev ssd1306_device = {0};

// 宣告一个外围设备名字叫做”oled_128_64_1”，设备地址为0x3c
static struct i2c_board_info info = {
    I2C_BOARD_INFO(NAME, 0x3C),
};

static int i2c_dev_write(char *buf, unsigned int count)
{
    int ret;
    ret = i2c_master_send(ssd1306_device.client, buf, count);
    return ret;
}

// static int i2c_dev_read(char *buf, unsigned int count)
// {
//     int ret;
//     ret = i2c_master_recv(ssd1306_device.client, buf, count);
//     return ret;
// }

/**
 * @brief  OLED写命令
 * @param  Command 要写入的命令
 * @retval 无
 */
void OLED_WriteCommand(uint8_t Command)
{
    char buf[] = {0x00, Command};
    i2c_dev_write(buf, 2);
}

/**
 * @brief  OLED写数据
 * @param  Data 要写入的数据
 * @retval 无
 */
void OLED_WriteData(uint8_t Data)
{
    char buf[] = {0x40, Data};
    i2c_dev_write(buf, 2);
}

/**
 * @brief  OLED显示一个字符
 * @param  Line 行位置，范围：1~4
 * @param  Column 列位置，范围：1~16
 * @param  Char 要显示的一个字符，范围：ASCII可见字符
 * @retval 无
 */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    uint8_t i;
    OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8); // 设置光标位置在上半部分
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i]); // 显示上半部分内容
    }
    OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8); // 设置光标位置在下半部分
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]); // 显示下半部分内容
    }
}

/**
 * @brief  OLED次方函数
 * @retval 返回值等于X的Y次方
 */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

/**
 * @brief  OLED显示数字（二进制，正数）
 * @param  Line 起始行位置，范围：1~4
 * @param  Column 起始列位置，范围：1~16
 * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
 * @param  Length 要显示数字的长度，范围：1~16
 * @retval 无
 */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
    }
}

/**
 * @brief  OLED设置光标位置
 * @param  Y 以左上角为原点，向下方向的坐标，范围：0~7
 * @param  X 以左上角为原点，向右方向的坐标，范围：0~127
 * @retval 无
 */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    OLED_WriteCommand(0xB0 | Y);                 // 设置Y位置
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4)); // 设置X位置低4位
    OLED_WriteCommand(0x00 | (X & 0x0F));        // 设置X位置高4位
}

/**
 * @brief  OLED清屏
 * @param  无
 * @retval 无
 */
void OLED_Clear(void)
{
    uint8_t i, j;
    for (j = 0; j < 8; j++)
    {
        OLED_SetCursor(j, 0);
        for (i = 0; i < 128; i++)
        {
            OLED_WriteData(0x00);
        }
    }
}
/**
 * @brief  OLED显示字符串
 * @param  Line 起始行位置，范围：1~4
 * @param  Column 起始列位置，范围：1~16
 * @param  String 要显示的字符串，范围：ASCII可见字符
 * @retval 无
 */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i, j;
    ssd1306_device.pos[0] = Line;
    ssd1306_device.pos[1] = Column;
    for (j = 0; j < 4; j++)
    {
        for (i = 0; String[i + j * 16] != '\0' && i < 16; i++)
        {
            OLED_ShowChar(ssd1306_device.pos[0] + j, ssd1306_device.pos[1] + i, String[i + j * 16]);
        }
        if (String[i + j] == '\0')
            break;
    }
}

/**
 * @brief  OLED显示数字（十进制，正数）
 * @param  Line 起始行位置，范围：1~4
 * @param  Column 起始列位置，范围：1~16
 * @param  Number 要显示的数字，范围：0~4294967295
 * @param  Length 要显示数字的长度，范围：1~10
 * @retval 无
 */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/**
 * @brief  OLED显示数字（十进制，带符号数）
 * @param  Line 起始行位置，范围：1~4
 * @param  Column 起始列位置，范围：1~16
 * @param  Number 要显示的数字，范围：-2147483648~2147483647
 * @param  Length 要显示数字的长度，范围：1~10
 * @retval 无
 */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    uint8_t i;
    uint32_t Number1;
    if (Number >= 0)
    {
        OLED_ShowChar(Line, Column, '+');
        Number1 = Number;
    }
    else
    {
        OLED_ShowChar(Line, Column, '-');
        Number1 = -Number;
    }
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/**
 * @brief  OLED显示数字（十六进制，正数）
 * @param  Line 起始行位置，范围：1~4
 * @param  Column 起始列位置，范围：1~16
 * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
 * @param  Length 要显示数字的长度，范围：1~8
 * @retval 无
 */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i, SingleNumber;
    for (i = 0; i < Length; i++)
    {
        SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
        if (SingleNumber < 10)
        {
            OLED_ShowChar(Line, Column + i, SingleNumber + '0');
        }
        else
        {
            OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
        }
    }
}

void OLED_Init(void)
{
    OLED_WriteCommand(0xAE); // 关闭显示

    OLED_WriteCommand(0xD5); // 设置显示时钟分频比/振荡器频率
    OLED_WriteCommand(0x80);

    OLED_WriteCommand(0xA8); // 设置多路复用率
    OLED_WriteCommand(0x3F);

    OLED_WriteCommand(0xD3); // 设置显示偏移
    OLED_WriteCommand(0x00);

    OLED_WriteCommand(0x40); // 设置显示开始行

    OLED_WriteCommand(0xA1); // 设置左右方向，0xA1正常 0xA0左右反置

    OLED_WriteCommand(0xC8); // 设置上下方向，0xC8正常 0xC0上下反置

    OLED_WriteCommand(0xDA); // 设置COM引脚硬件配置
    OLED_WriteCommand(0x12);

    OLED_WriteCommand(0x81); // 设置对比度控制
    OLED_WriteCommand(0xCF);

    OLED_WriteCommand(0xD9); // 设置预充电周期
    OLED_WriteCommand(0xF1);

    OLED_WriteCommand(0xDB); // 设置VCOMH取消选择级别
    OLED_WriteCommand(0x30);

    OLED_WriteCommand(0xA4); // 设置整个显示打开/关闭

    OLED_WriteCommand(0xA6); // 设置正常/倒转显示

    OLED_WriteCommand(0x8D); // 设置充电泵
    OLED_WriteCommand(0x14);

    OLED_WriteCommand(0xAF); // 开启显示

    OLED_Clear(); // OLED清屏
}

int OLED_Probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    OLED_Init();
    return 0;
}

int OLED_Remove(struct i2c_client *client)
{
    OLED_Clear();            // OLED清屏
    OLED_WriteCommand(0xAE); // 关闭显示
    return 0;
}

static const struct i2c_device_id oled_id[] = {
    {NAME, 0},
    {},
};

static struct i2c_driver driver = {
    .driver = {
        .name = "oled_128_64_1",
        .owner = THIS_MODULE,
    },
    .probe = OLED_Probe,
    .remove = OLED_Remove,
    .id_table = oled_id,
};

int ssd1306_char_open(struct inode *inode, struct file *file)
{
    OLED_Clear(); 
    return 0;
}

int ssd1306_char_close(struct inode *inode, struct file *file)
{
    return 0;
}

ssize_t ssd1306_char_read(struct file *file, char __user *ubuf, size_t size, loff_t *loff)
{
    return size;
}

ssize_t ssd1306_char_write(struct file *file, const char __user *ubuf, size_t size, loff_t *offs)
{
    unsigned char *buf = vmalloc(size);
    if (copy_from_user(buf, ubuf, size))
    {
        vfree(buf);
        printk("copy data from user error.\n");
        return -E2BIG;
    }
    *(buf + size - 1) = '\0';

    OLED_ShowString(1, 1, buf);
    vfree(buf);
    return size;
}

long ssd1306_char_ioctl(struct file *file, unsigned int cmd, long unsigned int args)
{
    int ret = 0;

    /* 检查设备类型 */
    if (_IOC_TYPE(cmd) != IOC_MAGIC)
    {
        pr_err("[%s] command type [%c] error!\n",
               __func__, _IOC_TYPE(cmd));
        return -ENOTTY;
    }

    /* 检查序数 */
    if (_IOC_NR(cmd) > IOC_MAXNR)
    {
        pr_err("[%s] command numer [%d] exceeded!\n",
               __func__, _IOC_NR(cmd));
        return -ENOTTY;
    }

    /* 检查访问模式 */
    if (_IOC_DIR(cmd) & _IOC_READ)
        ret = !access_ok((void __user *)args, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        ret = !access_ok((void __user *)args, _IOC_SIZE(cmd));
    if (ret)
        return -EFAULT;

    switch (cmd)
    {
    /* 初始化设备 */
    case IOCINIT:
        OLED_Init();
        break;

    /* 直接写寄存器 */
    case IO_DIRECTLY_ACCESS:
        ret = copy_from_user(&reg_access_transimission, (char __user *)args, sizeof(reg_access_transimission));
        if (ret)
        {
            pr_err("%s: copy_from_user failed", __func__);
            return ret;
        }
        i2c_dev_write(reg_access_transimission.buffer, reg_access_transimission.len);
        break;

    default:
        return -ENOTTY;
    }
    return 0;
}

const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = ssd1306_char_open,
    .release = ssd1306_char_close,
    .read = ssd1306_char_read,
    .write = ssd1306_char_write,
    .unlocked_ioctl = ssd1306_char_ioctl,
};

static struct miscdevice mdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = NAME,
    .fops = &fops,
};

static int __init ssd1306_Init(void)
{
    int ret = -1;
    struct i2c_adapter *adapter;

    if (ssd1306_device.major)
    {
        ssd1306_device.devid = MKDEV(ssd1306_device.major, 0);
        register_chrdev_region(ssd1306_device.devid, 1, NAME);
    }
    else
    {
        alloc_chrdev_region(&ssd1306_device.devid, 0, 1, NAME);
        ssd1306_device.major = MAJOR(ssd1306_device.devid);
        ssd1306_device.minor = MINOR(ssd1306_device.devid);
    }
    if (ssd1306_device.major < 0)
    {
        DEBUG("register_chrdev faild");
        goto ERR1;
    }
    cdev_init(&ssd1306_device.cdev, &fops);
    cdev_add(&ssd1306_device.cdev, ssd1306_device.devid, 1);

    ssd1306_device.class = class_create(THIS_MODULE, NAME);
    if (IS_ERR(ssd1306_device.class))
    {
        DEBUG("class_create faild");
        goto ERR2;
    }
 
    ssd1306_device.dev = device_create(ssd1306_device.class, NULL, MKDEV(ssd1306_device.major, 0), NULL, NAME);
    if (IS_ERR(ssd1306_device.dev))
    {
        DEBUG("device_create faild");
        ret = PTR_ERR(ssd1306_device.dev);
        goto ERR3;
    }

    adapter = i2c_get_adapter(0);
    ssd1306_device.client = i2c_new_client_device(adapter, &info);
    i2c_put_adapter(adapter);
    i2c_add_driver(&driver);

    misc_register(&mdev);

    OLED_Init();

    return 0;
ERR3:
    class_destroy(ssd1306_device.class);
ERR2:
    unregister_chrdev(ssd1306_device.major, NAME);
ERR1:
    return ret;
}

static void __exit ssd1306_Exit(void)
{
    OLED_Clear();            // OLED清屏
    OLED_WriteCommand(0xAE); // 关闭显示

    misc_deregister(&mdev);

    i2c_unregister_device(ssd1306_device.client);
    i2c_del_driver(&driver);

    device_destroy(ssd1306_device.class, MKDEV(ssd1306_device.major, 0));

    class_destroy(ssd1306_device.class);

    cdev_del(&ssd1306_device.cdev);

    unregister_chrdev_region(ssd1306_device.devid, 1);
    DEBUG("ssd1306_Exit succeed");
}

module_init(ssd1306_Init);
module_exit(ssd1306_Exit);
MODULE_LICENSE("GPL");
