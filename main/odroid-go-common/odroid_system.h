#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void odroid_system_application_set(int slot);
void odroid_system_sleep();
void odroid_system_init();
void odroid_system_led_set(int value);
#ifdef __cplusplus
}
#endif