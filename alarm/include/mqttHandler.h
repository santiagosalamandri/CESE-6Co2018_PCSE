#ifndef MQTTHANDLER_H
#define  MQTTHANDLER_H

#include "mgos.h"
#include "mgos_wifi.h"
#include "mgos_wifi_hal.h"
#include "mgos_net.h"
#include "mgos_net_hal.h"
#include "mgos_mqtt.h"
#include "mgos_rpc.h"

#include "systemController.h"
#include "userController.h"
#include "httpCodes.h"

#ifndef FALSE
#define  FALSE 0
#endif

#ifndef TRUE
#define  TRUE 1
#endif


#define MQTT_PARAMS "{method: %Q, method_code: %d,requester: %Q, args: %T, tag: %Q, id: %d}"
#define MQTT_PARAMS_QTY 6
#define ADMIN_SET_WIFI_PARAMS "{ssid: %Q,password: %Q}"
#define ADMIN_SET_WIFI_PARAMS_QTY 2
#define ADMIN_TRIGGER_PARAMS ""
#define WIFI_SCAN_PARAMS "{}"
#define USER_TRIGGER_PARAMS "{c: %B}"


typedef enum {
        UNAUTHORIZED_USER=-1,
        ADMIN_FACTORY_RESET=100,
        ADMIN_SET_WIFI,
        ADMIN_GET_USERS,
        ADMIN_UPDATE_USER,
        ADMIN_DELETE_USER,
        ADMIN_TRIGGER,
        ADMIN_CREATE_USER,
        GUEST_CREATE_USER=200,
        GUEST_USER_STATUS,
        WIFI_SCAN,
        SYS_GET_INFO,
        USER_TRIGGER=300
}METHOD_CODES;


enum states{
DISARMED,
ARMED,
DISARMING,
ALARM
};

enum alarmEvents {
  DESACTIVATE,
  ACTIVATE,
  TRIGGER,
  ACCEL,
  MAGNETIC,
  TIMER
  };

#define MY_EVENT_BASE MGOS_EVENT_BASE('F', 'O', 'O')

enum my_event {
   MQTT_MSG = MY_EVENT_BASE,
   ADC_MSG
  };


void set_alarm_cb(int ev, void *ev_data, void *userdata);
void magnetic_cb();
void deactiveAlarm_cb();

void mqtt_handler(struct mg_connection *c, const char *topic, int topic_len,const char *msg, int msg_len, void *userdata);

void armarRespuesta(struct json_out *out,int code,char* response,char *tag, int id);
void mqtt_ev_handler(struct mg_connection *c, int ev, void *p,  void *user_data);
int autenticar(int methodCode,char* userType);
int deleteInvitations();
#endif
