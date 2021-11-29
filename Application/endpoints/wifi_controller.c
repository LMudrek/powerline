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
#include "wifi_controller.h"
#include "esp_log.h"

#include <string.h>
#include "wifi_info.h"
#include "wifi_config.h"
#include "wifi_app.h"
#include "json_buffer.h"
#include "http_util.h"
/*******************************************************************************
* DEFINES E ENUMS
*******************************************************************************/
/* Quantidade máxima de access-points a serem listados */
#define MAX_AP_LIST_SIZE  5
/*******************************************************************************
* TYPEDEFS
*******************************************************************************/

/*******************************************************************************
* CONSTANTES
*******************************************************************************/
/* Identificador LOG */
static const char *TAG = "WIFI_API_CONTROLLER";

/*******************************************************************************
* VARIÁVEIS
*******************************************************************************/

/*******************************************************************************
* PROTÓTIPOS DE FUNÇÕES
*******************************************************************************/
static bool info_serialize(char * bufferOutPtr, size_t sizeBufferOut);
static bool ap_serialize(char * bufferOutPtr, size_t sizeBufferOut);
static void ap_to_dto(cJSON * root);
static void info_to_dto(cJSON * root);
static esp_err_t dto_to_wifi_config(const char * bufferInPtr, size_t bufferInSize, wifi_config_t * wifiConfigPtr);
/*******************************************************************************
* FUNÇÕES EXPORTADAS
*******************************************************************************/

/**
 * Serviço web para recuperação dos dados do Wi-Fi conectado
 * 
 * @param req         requisição a ser respondida
 * @return esp_err_t  resultado da operação, sucesso = ESP_OK
 */
esp_err_t wifi_controller_get_info(httpd_req_t * req)
{
  
  if (info_serialize(json_buffer_get(), json_buffer_get_size()) == false)
  {
    /* Falha criação dados */
    http_util_send_response(req, HTTPD_500, "Failed to serialize the response");
    return ESP_FAIL;
  };

  httpd_resp_set_type(req, HTTPD_TYPE_JSON);
  httpd_resp_send(req, json_buffer_get(), HTTPD_RESP_USE_STRLEN);

  return ESP_OK;
}

/**
 * Serviço Web para recuperar access-points disponíveis
 * 
 * @param req         requisição a ser respondida
 * @return esp_err_t  resultado da operação, sucesso = ESP_OK
 */
esp_err_t wifi_controller_get_ap(httpd_req_t * req)
{
  
  if (ap_serialize(json_buffer_get(), json_buffer_get_size()) == false)
  {
    /* Falha criação dados */
    http_util_send_response(req, HTTPD_500, "Failed to serialize the response");
    return ESP_FAIL;
  };

  httpd_resp_set_type(req, HTTPD_TYPE_JSON);
  httpd_resp_send(req, json_buffer_get(), HTTPD_RESP_USE_STRLEN);

  return ESP_OK;
}

/**
 * Serviço Web para realizar conexão à uma rede Wi-Fi
 * 
 * @param req         requisição a ser respondida
 * @return esp_err_t  resultado da operação, sucesso = ESP_OK
 */
esp_err_t wifi_controller_post_connect(httpd_req_t * req)
{
  esp_err_t result = http_read_body(req, json_buffer_get(), json_buffer_get_size());

  if (result != ESP_OK)
  {
    /* Falha captação dados */
    http_util_send_response(req, HTTPD_500, "Error reading request body");
    return result;
  }

  wifi_config_t wifiConfig;
  result = dto_to_wifi_config(json_buffer_get(), json_buffer_get_size(), &wifiConfig);

  if (result != ESP_OK)
  {
    /* Body requisitado com formato inválido  */
    http_util_send_response(req, HTTPD_400, "Error decoding request body");
    return result;
  }

  if(wifi_info_ap_available(&wifiConfig.sta.ssid) == false)
  {
    /* Nome da rede não disponível */
    http_util_send_response(req, HTTPD_400, "SSID not available");
    return ESP_FAIL;
  }

  /* Realiza conexão e retorna */
  http_util_send_response(req, HTTPD_200, "Post control value successfully");
  wifi_config_set(&wifiConfig);
  wifi_app_set(WIFI_APP_RESTART_CONFIG_SIGNAL);

  return ESP_OK;
}

/**
 * Serviço Web para exclusão dados Wi-Fi
 * 
 * @param req         requisição a ser respondida
 * @return esp_err_t  resultado da operação, sucesso = ESP_OK
 */
esp_err_t wifi_controller_delete_connect(httpd_req_t * req)
{
  wifi_app_set(WIFI_APP_STA_DEFAULT_SIGNAL);
  http_util_send_response(req, HTTPD_204, "Delete control value successfully");
  return ESP_OK;
}

/*******************************************************************************
* FUNÇÕES LOCAIS
*******************************************************************************/

/**
 * Cria body JSON para informações rede Wi-Fi
 * 
 * @param bufferOutPtr    buffer a ser escrito body JSON
 * @param sizeBufferOut   tamanho disponível para escrita
 * @return true           Criação com sucesso
 * @return false          Falha criação
 */
static bool info_serialize(char * bufferOutPtr, size_t sizeBufferOut)
{
  cJSON * root = cJSON_CreateObject();
  info_to_dto(root);
	return close_json(root, bufferOutPtr, sizeBufferOut);
}

/**
 * Cria body JSON para informações redes disponíveis para conexão
 * 
 * @param bufferOutPtr    buffer a ser escrito body JSON
 * @param sizeBufferOut   tamanho disponível para escrita
 * @return true           criação com sucesso
 * @return false          falha criação
 */
static bool ap_serialize(char * bufferOutPtr, size_t sizeBufferOut)
{
  cJSON * root = cJSON_CreateObject();
  ap_to_dto(root);
	return close_json(root, bufferOutPtr, sizeBufferOut);
}

/**
 * Decodifica body JSON para estrutura de configuração de Wi-fI
 * 
 * @param bufferInPtr     buffer a ser lido com a estrutura JSON
 * @param bufferInSize    tamanho dos dados a serem lidos
 * @param wifiConfigPtr   estrutura de configuração Wi-Fi a ser escrita
 * @return esp_err_t      resultado da operação, sucesso = ESP_OK
 */
static esp_err_t dto_to_wifi_config(const char * bufferInPtr, size_t bufferInSize, wifi_config_t * wifiConfigPtr)
{
  bzero(wifiConfigPtr, sizeof(wifi_config_t));
  cJSON * root = cJSON_Parse(bufferInPtr);
  if (root == NULL)
  {
    return ESP_FAIL;
  }

  /* Recuperação nome da rede a ser conectado */
  if (get_json_string_value(root, "ssid", &wifiConfigPtr->sta.ssid, sizeof(wifiConfigPtr->sta.ssid)) != ESP_OK)
  {
    return ESP_FAIL;
  }

  /* Recuperação senha da da rede a ser conectado */
  if (get_json_string_value(root, "password", &wifiConfigPtr->sta.password, sizeof(wifiConfigPtr->sta.password)) != ESP_OK)
  {
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "ssid: (%s) password: (%s)", wifiConfigPtr->sta.ssid, wifiConfigPtr->sta.password);
  cJSON_Delete(root);

  return ESP_OK;
}

/**
 * Transforma configuração de Wi-Fi em estrutura JSON
 * 
 * @param root  estrutura JSON a ser escrita
 */
static void info_to_dto(cJSON * root)
{
  wifi_config_t wifiConfig;
  wifi_config_get(WIFI_IF_STA, &wifiConfig);

  tcpip_adapter_ip_info_t wifiInterface;
  tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &wifiInterface);

  cJSON_AddStringToObject(root, "ssid", &wifiConfig.sta.ssid);
  cJSON_AddStringToObject(root, "password", &wifiConfig.sta.password);  
  cJSON_AddStringToObject(root, "ip", ip4addr_ntoa(&wifiInterface.ip));
}

/**
 * Transforma leitura de estações disponíveis em estrutura JSON
 * 
 * @param root  estrutura JSON a ser escrita
 */
static void ap_to_dto(cJSON * root)
{
  cJSON * apListJson = cJSON_AddArrayToObject(root, "ap");
  wifi_ap_record_t apList[MAX_AP_LIST_SIZE];
  uint16_t apListSize = wifi_info_get_ap_list(&apList, MAX_AP_LIST_SIZE);  

  for(uint16_t idx = 0; idx < MAX_AP_LIST_SIZE && idx < apListSize; idx++)
  {
    /* Para cada rede encontrada, cria objeto JSON dos dados identificados */
    cJSON * apItem = cJSON_CreateObject();
    cJSON_AddStringToObject(apItem, "ssid", &apList[idx].ssid);
    cJSON_AddNumberToObject(apItem, "strength", apList[idx].rssi);
    cJSON_AddBoolToObject(apItem, "requireAuth", apList[idx].authmode != WIFI_AUTH_OPEN);

    /* Formata MAC */
    char mac[20] = "";
    uint8_t * macPtr = apList[idx].bssid;
    sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X",macPtr[0],macPtr[1],macPtr[2],macPtr[3],macPtr[4],macPtr[5]);
    cJSON_AddStringToObject(apItem, "mac", mac);

    cJSON_AddItemToArray(apListJson, apItem);
  }
}

/*******************************************************************************
* END OF FILE
*******************************************************************************/
