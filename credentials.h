#include "profile.h"
/* Setup WiFi Connection 
   for use with WiFi.begin() in setup_wifi() of ino
   see https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html
*/
    #define ssid ""
    #define password ""

/* Setup MQTT Broker Connection 
   public HIVEMQ broker unencrypted unauthenticated: broker.hivemq.com ; TCP Port 1883 ; Websocket Port 8000
   public MOSQUITTO broker unencrypted unauthenticated: test.mosquitto.org ; TCP Port 1883 ; Websocket Port 8080
   private HIVEMQ broker encrypted authenticated: #.s2.eu.hivemq.cloud; TCP Port 8883 ; Websocket Port 8884
   public MOSQUITTO broker encrypted unauthenticated: test.mosquitto.org ; TCP Port 8883 ; Websocket Port 8081
   
   for use with client.connect() 
   see https://pubsubclient.knolleary.net/api
   see also https://www.hivemq.com/docs/hivemq-cloud/introduction.html?utm_campaign=HiveMQ%20Cloud&utm_medium=email&_hsmi=115570127&_hsenc=p2ANqtz-95OzIJCSLsi68QEkTK5kZkUiipt62urAFGCtt2sbE7Tvqx1AFkYTn9l8WG8TdW66gAmz8Nk_J41vizQDf03Nf_TVzZYA&utm_content=115570127&utm_source=hs_automation#get-started
*/  
     
    #define mqtt_server "broker.hivemq.com"                                     //public broker: broker.hivemq.com  or test.mosquitto.org
    #define mqtt_port 1883                                                      //default 1883 ; unencrypted
    #define mqtt_username ""                                                    //default "" ; no authentication
    #define mqtt_password ""                                                    //default "" ; no authentication
   

/*** For Secure TLS/MQTTS, use with certificates in cert.h ***/   
   #ifdef HIVEMQS                                                               //enter private broker: #.s2.eu.hivemq.cloud
    #define mqtt_server "961904.s2.eu.hivemq.cloud"   
    #define mqtt_port 8883                                                      //default 8883 ; encrypted
    #define mqtt_username ""                                                    //enter username for authentication
    #define mqtt_password ""                                                    //enter password for authentication
    #endif
    #ifdef MOSQUITTOS                                                            //public broker: test.mosquitto.org
    #define mqtt_server "test.mosquitto.org"                                                                   
    #define mqtt_port 8883                                                      //default 8883 ; encrypted
    #define mqtt_username ""                                                    //default "" ; no authentication
    #define mqtt_password ""                                                    //default "" ; no authentication
   #endif 
             
