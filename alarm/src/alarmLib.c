
/*******************Include's declarations********************************************/

#include "portonLib.h"
#include "mgos_i2c.h"
#include "mgos_imu.h"


#define  REINTENTOS 5
#define  SEGUNDOS 30000

#define LISTENER_SPEC "7777"

extern struct mgos_config_wifi_sta cfg;
extern char globalWifiPass[32];

int disable=0;
mgos_timer_id disableTimerId=0;
char topic[21];

static void disable_cb(){
disable=0;
}

static void imu_cb(void *user_data) {
  struct mgos_imu *imu = (struct mgos_imu *)user_data;
  float ax, ay, az;
  if (!imu) return;

  if (mgos_imu_accelerometer_get(imu, &ax, &ay, &az)){
  if((ax>1.5) && !disable)
  {
      disable=1;
      mgos_clear_timer(disableTimerId);
      disableTimerId=mgos_set_timer(2000, true, disable_cb, NULL);
      int val=ACCEL;
      mgos_event_trigger(MQTT_MSG, (void*)&val);

      }
              }
}

void blinky(){
        mgos_gpio_toggle(2);
}

mgos_timer_id id=0;

static void net_changed(int ev, void *evd, void *arg)
{
        struct mgos_net_ip_info ip_info;
        memset(&ip_info, 0, sizeof(ip_info));
        //  char *status = mgos_wifi_get_status_str();
        char *ssid = mgos_wifi_get_connected_ssid();
        char sta_ip[16], ap_ip[16];
        memset(sta_ip, 0, sizeof(sta_ip));
        memset(ap_ip, 0, sizeof(ap_ip));
        if (mgos_net_get_ip_info(MGOS_NET_IF_TYPE_WIFI, MGOS_NET_IF_WIFI_STA,&ip_info))
        {
                mgos_net_ip_to_str(&ip_info.ip, sta_ip);
        }
        if (mgos_net_get_ip_info(MGOS_NET_IF_TYPE_WIFI, MGOS_NET_IF_WIFI_AP,&ip_info))
        {
                mgos_net_ip_to_str(&ip_info.ip, ap_ip);
        }

        switch (ev)
        {
        case MGOS_NET_EV_DISCONNECTED:
                //printf("%s\n", "--------------------------------DISCONNECTED");
                if(reintentosWiFi++==REINTENTOS){
                  //printf("REINICIANDO\n" );
                  mgos_system_restart_after(100);
                }
                mgos_gpio_write(2, 1);


                break;
        case MGOS_NET_EV_CONNECTING:
                //printf("%s\n", "--------------------------------CONNECTING");

                break;
        case MGOS_NET_EV_CONNECTED:
                //printf("%s\n", "--------------------------------CONECTADO");

                break;
        case MGOS_NET_EV_IP_ACQUIRED:
                //printf("%s\n", "--------------------------------IP_ACQUIRED");
                ;
                char *err = NULL;

                if((strlen(ap_ip)!=0) && (strlen(sta_ip)!=0))
                {
                        mgos_sys_config_set_wifi_sta_ssid(ssid);
                        mgos_sys_config_set_wifi_sta_pass(globalWifiPass);
                        mgos_sys_config_set_wifi_sta_enable(1);
                        mgos_sys_config_set_wifi_ap_enable(0);
                        mgos_sys_config_set_dns_sd_enable(true);
                        save_cfg(&mgos_sys_config, &err);

                        //printf("Saving configuration: %s\n", err ? err : "NO ERROR");
                        free(err);
                        free(ssid);



                        mgos_system_restart();
                }
                mgos_gpio_write(2, 0);

                break;
        default:
                //printf("%s\n", "--------------------------------DESCONOCIDO");
                break;
        }

        (void)evd;
        (void)arg;
}

static void set_wifi_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                             struct mg_rpc_frame_info *fi,
                             struct mg_str args)
{

        char *ssid = NULL;
        char *password = NULL;

        if (json_scanf(args.p, args.len, ri->args_fmt, &ssid, &password) != 2)
        {
                mg_rpc_send_errorf(ri, INTERNAL_SERVER_ERROR, ERROR, PRECONDITION_FAILED, ERROR_SSID_AND_PASS_REQUIRED);
                ri = NULL;
                return;
        }

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
                mg_rpc_send_errorf(ri, INTERNAL_SERVER_ERROR, ERROR,CONFLICT,AP_MODE_REQUIRED);
        }
        else if (mg_rpc_send_responsef(ri, "{message: %Q}", "Intentando conectar..."))
        {
                mgos_set_timer(CALLBACK_TIME, false, wifiConnectCb, &cfg);

        }
        //mg_rpc_send_errorf(ri, INTERNAL_SERVER_ERROR, ERROR,200,ERROR_CONNECTING_WIFI);

        ri = NULL;
        (void)cb_arg;
        (void)fi;
}

static void lc_handler(struct mg_connection *nc, int ev, void *ev_data,
                       void *user_data) {
	char addr[32];
	      mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
	                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
  switch (ev) {
    case MG_EV_ACCEPT: {
      LOG(LL_INFO, ("%p Connection from %s", nc, addr));
      mg_printf(nc, mgos_sys_config_get_device_id(), addr);
      nc->flags |= MG_F_SEND_AND_CLOSE;
      break;
    }
    case MG_EV_RECV:
    	LOG(LL_INFO, ("Recibi: %.*s de  %d Bytes desde %s", nc->recv_mbuf.len,nc->recv_mbuf.buf,nc->recv_mbuf.len, addr));

        nc->flags |= MG_F_SEND_AND_CLOSE;
    	break;
    case MG_EV_CLOSE: {
      LOG(LL_INFO, ("%p Connection closed", nc));
      break;
    }
  }
  (void) ev_data;
  (void) user_data;
}

static int init_listener(struct mg_mgr *mgr) {
  struct mg_bind_opts bopts;
  memset(&bopts, 0, sizeof(bopts));
  LOG(LL_INFO, ("Listening on %s", LISTENER_SPEC));
  struct mg_connection *lc =
      mg_bind_opt(mgr, LISTENER_SPEC, lc_handler, NULL, bopts);
  if (lc == NULL) {
    LOG(LL_ERROR, ("Failed to create listener"));
    return 0;
  }
  return 1;
}



bool alarm_init()
{
  if (!init_listener(mgos_get_mgr())) return MGOS_APP_INIT_ERROR;

        initFiles();
        struct mgos_i2c *i2c = mgos_i2c_get_global();
        struct mgos_imu *imu = mgos_imu_create();

        struct mg_rpc *c = mgos_rpc_get_global();

        mgos_gpio_set_mode(LED_PIN, MGOS_GPIO_MODE_OUTPUT);
        mgos_gpio_write(LED_PIN, 0);
        mgos_gpio_set_mode(LED_PIN2, MGOS_GPIO_MODE_OUTPUT);
        mgos_gpio_write(LED_PIN2, 1);

        mg_rpc_add_handler(c, "Admin.Trigger", "{action: %d}", trigger_handler, NULL);
        mg_rpc_add_handler(c, "Admin.SetWifi", "{ssid: %Q,password: %Q}", set_wifi_handler, NULL);
        mg_rpc_add_handler(c, "User.Trigger", "{action: %d}", trigger_handler, NULL);

        mgos_mqtt_add_global_handler(mqtt_ev_handler,NULL);
        mgos_mqtt_sub(mgos_sys_config_get_urbit_mqttSubscribeRequest(), mqtt_handler, NULL); /* Subscribe */

        mgos_event_register_base(MY_EVENT_BASE, "my module foo");
        mgos_event_add_handler(MQTT_MSG, set_alarm_cb, NULL);

        mgos_gpio_set_button_handler(16, MGOS_GPIO_PULL_UP, MGOS_GPIO_INT_EDGE_NEG, 50, deactiveAlarm_cb, NULL);
        mgos_gpio_set_button_handler(17, MGOS_GPIO_PULL_UP, MGOS_GPIO_INT_EDGE_POS, 500, magnetic_cb, NULL);

        mgos_set_timer(5000, MGOS_TIMER_REPEAT, checkHeap, NULL);

        mgos_gpio_set_mode(2, MGOS_GPIO_MODE_OUTPUT);



      if (!i2c) {
        LOG(LL_ERROR, ("I2C bus missing, set i2c.enable=true in mos.yml"));
        return false;
      }

      if (!imu) {
        LOG(LL_ERROR, ("Cannot create IMU"));
        return false;
      }

      if (!mgos_imu_accelerometer_create_i2c(imu, i2c, 0x68, ACC_MPU9250))
        LOG(LL_ERROR, ("Cannot create accelerometer on IMU"));
        mgos_set_timer(100, true, imu_cb, imu);

        return true;
}
