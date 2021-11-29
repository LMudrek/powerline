/*******************************************************************************
* Leonardo Mudrek de Almeida
* UTFPR - CT
*
*
* License : CC BY NC SA 4.0
*******************************************************************************/

/*******************************************************************************
* INCLUDES
*******************************************************************************/
#include <string.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "wifi_config_ap.h"
/*******************************************************************************
* DEFINES E ENUMS
*******************************************************************************/
#define WIFI_CONFIG_AP_SSID       "PowerLine"
#define WIFI_CONFIG_AP_PASS       "utfpr_123456"

#define WIFI_CONFIG_AP_MAX_STA    5
#define WIFI_CONFIG_AP_CHANNEL    2
/*******************************************************************************
* TYPEDEFS
*******************************************************************************/

/*******************************************************************************
* CONSTANTES
*******************************************************************************/
static const char *TAG = "WIFI_CONFIG_AP";
/*******************************************************************************
* VARIÁVEIS
*******************************************************************************/

/*******************************************************************************
* PROTÓTIPOS DE FUNÇÕES
*******************************************************************************/

/*******************************************************************************
* FUNÇÕES EXPORTADAS
*******************************************************************************/

/**
 * Inicializa estrutura para modo access-point (AP)
 * 
 */
void wifi_config_ap_init(void)
{
  esp_netif_create_default_wifi_ap();
}

/**
 * Realiza a exposição do ESP como um access-point
 * 
 */
void wifi_config_ap_init_softap(void)
{
  wifi_config_t wifiConfig = {
    .ap = {
      .ssid = WIFI_CONFIG_AP_SSID,
      .ssid_len = strlen(WIFI_CONFIG_AP_SSID),
      .channel = WIFI_CONFIG_AP_CHANNEL,
      .password = WIFI_CONFIG_AP_PASS,
      .max_connection = WIFI_CONFIG_AP_MAX_STA,
      .authmode = WIFI_AUTH_WPA_WPA2_PSK
    },
  };

  if (strlen(WIFI_CONFIG_AP_PASS) == 0)
  {
    /* Caso a senha seja nula, o tipo de autenticação é do tipo rede aberta */
    wifiConfig.ap.authmode = WIFI_AUTH_OPEN;
  }

  esp_wifi_set_mode(WIFI_MODE_APSTA);
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifiConfig));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_softap finished. SSID: %s", WIFI_CONFIG_AP_SSID);
}
/*******************************************************************************
* FUNÇÕES LOCAIS
*******************************************************************************/

/*******************************************************************************
* END OF FILE
*******************************************************************************/
