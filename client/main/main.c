#include "main.h"
//----------------------------------------------------------------
//此为主程序，所有程序的入口
//----------------------------------------------------------------

/*开发板代码开启宏*/
#define GEC6816_CMD 0

int main(int argc, char **argv)
{
#if GEC6816_CMD
    /*硬件初始化*/
    LCD_addr = LCD_init();  //初始化LCD显示
    fd_ts = touch_init("/dev/input/event0");    //初始化触摸屏
    /*字库初始化*/
    Init_Font();
#endif


#if 0 //进入主线程
    while (1)
    {

    }
#endif
    return 0;
}
