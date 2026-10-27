#ifndef APP_H
#define APP_H

void app_init(void); /* call once in main(), after MX_..._Init() */
void app_loop(void); /* call in while(1) */

#endif /* APP_H */
