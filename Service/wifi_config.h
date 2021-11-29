/*******************************************************************************
* Leonardo Mudrek de Almeida
* UTFPR - CT
*
*
* License : CC BY NC SA 4.0
*******************************************************************************/
#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

/*******************************************************************************
* INCLUDES
*******************************************************************************/
#include "esp_wifi.h"
#include <stdbool.h>
/*******************************************************************************
* DEFINES E ENUMS
*******************************************************************************/

/*******************************************************************************
* TYPEDEFS
*******************************************************************************/
/* Definição callback para notificar status de conectivdade */
typedef void (*wifiConfigCallback_t)(bool);

/*******************************************************************************
* FUNÇÕES EXPORTADAS
*******************************************************************************/
void wifi_config_init(void);
void wifi_config_connect(wifiConfigCallback_t callbackToSet);
void wifi_config_reconnect(wifiConfigCallback_t callbackToSet);
void wifi_config_set(wifi_config_t * wifiConfigPtr);
bool wifi_config_upload(wifi_interface_t interface);
void wifi_config_save(void);
void wifi_config_get(wifi_interface_t interface, wifi_config_t * wifiConfigPtr);
bool wifi_config_delete(void);
/*******************************************************************************
* END OF FILE
*******************************************************************************/
#endif
