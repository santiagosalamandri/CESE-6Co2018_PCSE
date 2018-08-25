#ifndef PORTONLIB_H
#define PORTONLIB_H

/*******************Include's declarations********************************************/
#include "mgos_features.h"
#include "mgos_init.h"

#include "mgos.h"
#include "mgos_rpc.h"
#include "mgos_gpio.h"
#include "mg_rpc_channel_loopback.h"
#include "mgos_net.h"
#include "mgos_net_hal.h"
#include "mgos_wifi.h"
#include "mgos_wifi_hal.h"


#include "userController.h"
#include "systemController.h"
#include "mqttHandler.h"

/**********************Defines declarations*********************************/
#define LED_PIN2 2
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

bool alarm_init();


#endif
