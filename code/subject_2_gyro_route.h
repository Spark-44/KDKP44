#ifndef CODE_SUBJECT_2_GYRO_ROUTE_H_
#define CODE_SUBJECT_2_GYRO_ROUTE_H_

#include "zf_common_typedef.h"

#define SUBJECT_2_GYRO_ROUTE_13 13U
#define SUBJECT_2_GYRO_ROUTE_14 14U

void subject_2_gyro_route_start(uint8 route_number, uint8 reverse);
void subject_2_gyro_route_stop(const char *reason);
void subject_2_gyro_route_task(void);
uint8 subject_2_gyro_route_is_active(void);

#endif
