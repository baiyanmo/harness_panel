#ifndef _RGB_H_
#define _RGB_H_

#include "iocc2530.h"
#include "OnBoard.h"

#define RGB_MODE_ON   0
#define RGB_MODE_OFF  1
#define RGB_MODE_R    2
#define RGB_MODE_G    3
#define RGB_MODE_B    4

void RGB_Config(void);
void RGB_Mode(uint8 mode);


#endif
