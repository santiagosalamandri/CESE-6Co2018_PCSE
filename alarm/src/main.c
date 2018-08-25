#include "mgos.h"
#include "mgos_rpc.h"
#include "alarmLib.h"



enum mgos_app_init_result mgos_app_init(void) {
								alarm_init();
								return MGOS_APP_INIT_SUCCESS;
}
