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
#include "wifi_info.h"
#include "esp_log.h"
#include "esp_event.h"
#include <string.h>

/*******************************************************************************
* DEFINES E ENUMS
*******************************************************************************/

/*******************************************************************************
* TYPEDEFS
*******************************************************************************/

/*******************************************************************************
* CONSTANTES
*******************************************************************************/
/* Tag para log */
static const char *TAG = "WIFI_INFO_SERVICE";

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
 * Recupera lista de access-points disponíveis
 * 
 * @param apBufferIn - escrita da lista de pontos de acesso disponíveis
 * @param length - tamanho do buffer de escrita
 * @return uint16_t - quantidade access-points encontrados
 */
uint16_t wifi_info_get_ap_list(wifi_ap_record_t * apBufferInPtr, uint16_t length)
{
  memset(apBufferInPtr, 0, length);
  uint16_t apCount = 0;
  
  esp_wifi_scan_start(NULL, true);
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&length, apBufferInPtr));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&apCount));
  ESP_LOGI(TAG, "Total APs scanned = %u", apCount);

  return apCount;
}

/**
 * Verifica se o SSID fornecido está ao alcance 
 * 
 * @param ssidPtr - nome da rede a ser encontrada
 * @return true - SSID disponível
 * @return false - SSID indisponível
 */
bool wifi_info_ap_available(const char * ssidPtr)
{
  uint16_t size = 20;
  wifi_ap_record_t apList[size];
  memset(apList, 0, sizeof(apList));
  
  esp_wifi_scan_start(NULL, true);
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&size, apList));
  for (uint32_t idx = 0; idx < size; idx++)
  {
    if (strcmp(ssidPtr, &apList[idx].ssid) == 0)
    {
      return true;
    }
  }
  
  return false;
}

/*******************************************************************************
* FUNÇÕES LOCAIS
*******************************************************************************/

/*******************************************************************************
* END OF FILE
*******************************************************************************/
