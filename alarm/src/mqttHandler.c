
#include "mqttHandler.h"
#include "userController.h"

#define AUTENTICAR
#define BUFFER_SIZE 600
#define RESULT_SIZE 500
#define ARMING_TIME 10000

extern struct mgos_config_wifi_sta cfg;
extern char globalWifiPass[32];
extern char topic[20];
int flag=0;
enum states alarmState=DISARMED;

void armarRespuesta(struct json_out *out,int code,char * response,char *tag, int id){

        if(code==HTTP_OK) json_printf(out, RESPONSE_OK,response,tag,id);
        else json_printf(out, RESPONSE_ERR,response,tag,id);
}

void armingCb(){
int val=TIMER;
  printf("timer\n" );
  mgos_event_trigger(MQTT_MSG, (void*)&(val));
}

void magnetic_cb(){
int val=MAGNETIC;
  printf("magnetic\n" );
  mgos_event_trigger(MQTT_MSG, (void*)&(val));
}

void deactiveAlarm_cb(){
  printf("boton apretado\n" );
  int val=DESACTIVATE;
  mgos_event_trigger(MQTT_MSG, (void*)&(val));
}

void set_alarm_cb(int ev, void *ev_data, void *userdata) {
 LOG(LL_INFO, ("Going to reboot!"));
int comando = *(int*)ev_data;
printf("event: %d, ev_data: %d\n",ev,comando );

switch (comando) {

  case ACTIVATE:
    if(alarmState!=ALARM){
      alarmState=ARMED;
    }
    break;
  case DESACTIVATE:
    alarmState=DISARMED;
    break;
  case TRIGGER:
      alarmState=ALARM;
    break;
    case ACCEL:
      if(alarmState!=DISARMED){
        alarmState=ALARM;
      }
      break;
  case MAGNETIC:
  if(alarmState==ARMED){
    alarmState=DISARMING;
    mgos_set_timer(ARMING_TIME, 0, armingCb, NULL);
  }
    break;
  case TIMER:
    if(alarmState==DISARMING){
      alarmState=ALARM;
    }
    break;
  default:
    printf("DEFAULT\n" );
    break;
}

printf("%d\n",alarmState );

char actualStatus[15]={0};
snprintf ( actualStatus, 15, "{\"status\":%d}",alarmState );
mgos_mqtt_pub(mgos_sys_config_get_urbit_mqttLwt(), actualStatus,strlen(actualStatus), 1, 1);

 (void) ev;
 (void) ev_data;
 (void) userdata;
}



void mqtt_handler(struct mg_connection *c, const char *topic, int topic_len,
                  const char *msg, int msg_len, void *userdata) {
        LOG(LL_INFO, ("Got message on topic %.*s with message: %.*s ", topic_len, topic,msg_len,msg));
        const char* new_msg="nuevo msg";

        int comando=0;
        (void)new_msg;
        char responseTopic[100];
        snprintf ( responseTopic, 100, "%s/response/", mgos_sys_config_get_device_id());

        (void)c;
        (void)userdata;

        char responseBuffer[RESULT_SIZE];
        char msgBuffer[BUFFER_SIZE];

        struct json_out response= JSON_OUT_BUF(responseBuffer, RESULT_SIZE);
        struct json_out result= JSON_OUT_BUF(msgBuffer, BUFFER_SIZE);


        struct json_token t;
        char * method=NULL;
        char * requester=NULL;
        int methodCode=0;
        char * tag=NULL;
        int id=0;
        char name[20];
        int responseCode=0;
        (void)responseCode;

        int i=0;
        if ((i=json_scanf(msg,msg_len, MQTT_PARAMS,&method, &methodCode,&requester, &t, &tag, &id)) != MQTT_PARAMS_QTY) {
                responseCode=BAD_REQUEST;
                json_printf(&result,RESULT_FAIL,BAD_REQUEST,ERROR_BAD_REQUEST);
                printf("error en formato: %d\n",i );
        }
        else{
                #ifdef AUTENTICAR
                if(autenticar(methodCode,requester)!=OK){
                  methodCode=UNAUTHORIZED_USER;
                }
                #endif

                switch (methodCode) {
                case UNAUTHORIZED_USER:
                        responseCode=UNAUTHORIZED_USER;
                        json_printf(&result,RESULT_FAIL,UNAUTHORIZED,ERROR_UNAUTHORIZED);
                        break;
              case ADMIN_SET_WIFI:
                        if (json_scanf(t.ptr, t.len,ADMIN_SET_WIFI_PARAMS, &ssid, &password) != ADMIN_SET_WIFI_PARAMS_QTY)
                        {
                                responseCode=BAD_REQUEST;
                                json_printf(&result,RESULT_FAIL,PRECONDITION_FAILED,ERROR_SSID_AND_PASS_REQUIRED);

                        }
                        else{
                                cfg.ssid = ssid;
                                cfg.pass = password;
                                cfg.enable = 1;

                                strncpy(globalWifiPass, password, sizeof(globalWifiPass));

                                struct mgos_net_ip_info ip_info;
                                memset(&ip_info, 0, sizeof(ip_info));
                                char ap_ip[16];
                                memset(ap_ip, 0, sizeof(ap_ip));

                                if (mgos_net_get_ip_info(MGOS_NET_IF_TYPE_WIFI, MGOS_NET_IF_WIFI_AP,
                                                         &ip_info))
                                {
                                        mgos_net_ip_to_str(&ip_info.ip, ap_ip);
                                }

                                if(strlen(ap_ip)==0) {
                                        responseCode=CONFLICT;
                                        json_printf(&result,RESULT_FAIL,CONFLICT,AP_MODE_REQUIRED);
                                }
                                else
                                {
                                        responseCode=HTTP_OK;
                                        json_printf(&result,"{message:%s}",CONNECTING);
                                        mgos_set_timer(CALLBACK_TIME, false, wifiConnectCb, &cfg);
                                }


                        }
                        break;

                case ADMIN_TRIGGER:
                        printf("ADMIN_TRIGGER\n");
                        if(json_scanf(t.ptr, t.len, "{comando:%d}",&comando)==1){
                        mgos_event_trigger(MQTT_MSG, (void*)&comando);
                        (void)comando;
                        responseCode=HTTP_OK;
                          if(comando==ACTIVATE){
                            json_printf(&result,"{message:%s}","Activada");
                          }else{
                            json_printf(&result,"{message:%s}","Desactivada");
                          }

                        }
                      else{
                        responseCode=BAD_REQUEST;
                        json_printf(&result,RESULT_FAIL,BAD_REQUEST,ERROR_USERNAME_OR_TYPE_WRONG);
                      }

                        break;
                case USER_TRIGGER:
                        if(json_scanf(msg,msg_len, "{comando:%d}",&comando)==1){
                        mgos_event_trigger(MQTT_MSG, (void*)&comando);
                        responseCode=HTTP_OK;
                          if(comando==ACTIVATE){
                            json_printf(&result,"{message:%s}","Activada");
                          }else{
                            json_printf(&result,"{message:%s}","Desactivada");
                          }

                        }
                      else{
                        responseCode=BAD_REQUEST;
                        json_printf(&result,RESULT_FAIL,BAD_REQUEST,ERROR_USERNAME_OR_TYPE_WRONG);
                      }

                        break;
                default:
                        responseCode=BAD_REQUEST;
                        json_printf(&result,RESULT_FAIL,BAD_REQUEST,ERROR_BAD_REQUEST);
                        break;
                }

                armarRespuesta(&response,responseCode,msgBuffer,tag,id);
                strncat(responseTopic,requester,100);

                mgos_mqtt_pub(responseTopic, response.u.buf.buf,response.u.buf.len, 1, 0);

  if(responseCode==HTTP_OK) LOG(LL_INFO, ("Request completed OK"));
  else  LOG(LL_INFO, ("Failed request"));

                free(method);
                free(requester);
        }

}

void mqtt_ev_handler(struct mg_connection *c, int ev, void *p,  void *user_data)
{
        struct mg_mqtt_message *msg = (struct mg_mqtt_message *) p;

        (void) c;
        (void) ev;
        (void) p;
        (void) user_data;

        if (ev == MG_EV_MQTT_DISCONNECT) {
                flag=1;
        }
      else if (ev == MG_EV_MQTT_CONNACK)
      {
                if (msg->connack_ret_code != MG_EV_MQTT_CONNACK_ACCEPTED) {
                        exit(1);
                }
            mgos_mqtt_pub(mgos_sys_config_get_urbit_mqttLwt(), "{\"status\":0}",12, 1, 1);        }
}
