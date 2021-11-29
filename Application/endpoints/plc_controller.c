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
#include "plc_controller.h"
#include "plc_app.h"
#include "plc_uart_model.h"
#include "esp_log.h"
#include "cJSON.h"
#include "json_buffer.h"
#include "plc_uart.h"
#include "http_util.h"
/*******************************************************************************
* DEFINES E ENUMS
*******************************************************************************/

/*******************************************************************************
* TYPEDEFS
*******************************************************************************/

/**
 * Estrutura JSON para recepção de um comando de manipulação de um pino de uma estação (STA)
 * 
 */
typedef struct ioDto_t
{
  char mac [19];
  uint32_t value;
} ioDto_t;

/*******************************************************************************
* CONSTANTES
*******************************************************************************/
/* Identificador LOG */
static const char *TAG = "PLC_CONTROLLER";
/*******************************************************************************
* VARIÁVEIS
*******************************************************************************/

/*******************************************************************************
* PROTÓTIPOS DE FUNÇÕES
*******************************************************************************/
static bool topology_serialize(const topology_t * topologyPtr, char * jsonBufferPtr, size_t sizeJsonBuffer);
static void node_to_dto(cJSON * root, char * keyName, const node_t * nodeBufferPtr, uint32_t nodeCount);
static esp_err_t dto_to_command(const char * bufferInPtr, char * bufferOutPtr, size_t bufferOutSize);
static esp_err_t dto_to_io_command(const char * bufferInPtr, ioDto_t * dtoPtr);
/*******************************************************************************
* FUNÇÕES EXPORTADAS
*******************************************************************************/

/**
 * Serviço Web para recuperar tolologia PLC
 * 
 * @param req         requisição a ser respondida
 * @return esp_err_t  resultado da operação, sucesso = ESP_OK
 */
esp_err_t plc_controller_get_topology(httpd_req_t * req)
{
  topology_t topology;

  plc_topology_get(&topology);

  if (topology_serialize(&topology, json_buffer_get(), json_buffer_get_size()) == false)
  {
    /* Falha em criar objeto de resposta da topologia */
    http_util_send_response(req, HTTPD_500, "Failed to serialize the response");
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, HTTPD_TYPE_JSON);
  httpd_resp_send(req, json_buffer_get(), HTTPD_RESP_USE_STRLEN);

  return ESP_OK;
}

/**
 * Serviço Web para enviar comando para módulo PLC
 * 
 * @param req         requisição a ser respondida
 * @return esp_err_t  resultado da operação, sucesso = ESP_OK
 */
esp_err_t plc_controller_post_command(httpd_req_t * req)
{
  esp_err_t result = http_read_body(req, json_buffer_get(), json_buffer_get_size());

  if (result != ESP_OK)
  {
    /* Falha leitura body do comando a ser utilizado */
    http_util_send_response(req, HTTPD_500, "Error reading request body");
    return result;
  }

  char command[256] = "\0";
  result = dto_to_command(json_buffer_get(), command, sizeof(command));

  if (result != ESP_OK)
  {
    /* Body formatado incorretamente */
    http_util_send_response(req, HTTPD_400, "Error decoding request body");
    return result;
  }

  uartPlcResponse_t response;
  bzero(&response, sizeof(uartPlcResponse_t));
  /* Envia comando para módulo PLC */
  plc_uart_send(command, &response);

  if (response.result == false)
  {
    /* Módulo PLC indisponível */
    http_util_send_response(req, HTTPD_500, "Error sending command to PLC module");
    return result;
  }

  /* Copia resposta */
  strcpy(json_buffer_get(), response.data);
  httpd_resp_sendstr(req, json_buffer_get());
  
  return ESP_OK;
}

/**
 * Serviço Web para chavear carga nas estações
 * 
 * @param req         requisição a ser respondida
 * @return esp_err_t  resultado da operação, sucesso = ESP_OK
 */
esp_err_t plc_controller_post_io(httpd_req_t * req)
{
  esp_err_t result = http_read_body(req, json_buffer_get(), json_buffer_get_size());

  if (result != ESP_OK)
  {
    /* Falha recuperação body */
    http_util_send_response(req, HTTPD_500, "Error reading request body");
    return result;
  }

  ioDto_t dto;
  result = dto_to_io_command(json_buffer_get(), &dto);

  if (result != ESP_OK)
  {
    /* Body formatado incorretamente */
    http_util_send_response(req, HTTPD_400, "Error decoding request body");
    return result;
  }

  /* Envia comando */
  if (plc_uart_model_io(dto.mac, dto.value) == false)
  {
    /* Módulo PLC indisponível */
    http_util_send_response(req, HTTPD_500, "Communication with PLC module failed");
    return ESP_FAIL;
  }

  /* Comunicação OK, envia sucesso */
  http_util_send_response(req, HTTPD_200, "OK");
  return ESP_OK;
}


/*******************************************************************************
* FUNÇÕES LOCAIS
*******************************************************************************/

/**
 * Transforma estrutura topologia em body JSON
 * 
 * @param topologyPtr       estrutura a ser manipulada
 * @param jsonBufferPtr     buffer escrita JSON
 * @param sizeJsonBuffer    tamanho disponível para escrita
 * @return true             sucesso na transformação
 * @return false            falha transformação
 */
static bool topology_serialize(const topology_t * topologyPtr, char * jsonBufferPtr, size_t sizeJsonBuffer)
{
  cJSON * root = cJSON_CreateObject();
  /* Trata módulos do tipo concentrador (CCO) */
  node_to_dto(root, "cco", &topologyPtr->cco, topologyPtr->ccoCount);
  /* Trata módulos do tipo estação (STA) */
  node_to_dto(root, "sta", &topologyPtr->sta, topologyPtr->staCount);
	return close_json(root, jsonBufferPtr, sizeJsonBuffer);
}

/**
 * Cria array de objetos JSON para certo tipo de módulo 
 *      
 * @param root            estrutura JSON a ser utilizada para inserção do array de objeto 
 * @param keyName         nome a ser dado para array
 * @param nodeBufferPtr   buffer de node a ser consumido
 * @param nodeCount       quantidade de nodes a serem expostos 
 */
static void node_to_dto(cJSON * root, char * keyName, const node_t * nodeBufferPtr, uint32_t nodeCount)
{
  cJSON * nodeListJson = cJSON_AddArrayToObject(root, keyName);

  for(uint32_t idx = 0; idx < nodeCount; idx++)
  {
    cJSON * nodeItem = cJSON_CreateObject();

    char mac[20] = "";
    uint8_t * macPtr = nodeBufferPtr[idx].mac;
    /* Formata MAC */
    sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X",macPtr[0],macPtr[1],macPtr[2],macPtr[3],macPtr[4],macPtr[5]);
    cJSON_AddStringToObject(nodeItem, "mac", mac);

    cJSON_AddNumberToObject(nodeItem, "id", nodeBufferPtr[idx].id);
    cJSON_AddNumberToObject(nodeItem, "atenuation", nodeBufferPtr[idx].atenuation);
    cJSON_AddNumberToObject(nodeItem, "snr", nodeBufferPtr[idx].snr);
    cJSON_AddNumberToObject(nodeItem, "phase", nodeBufferPtr[idx].phase);
    cJSON_AddItemToArray(nodeListJson, nodeItem);
  }
}

/**
 * Transformação do body JSON recebido para identificação de um comando UART
 * 
 * @param bufferInPtr     estrutura JSON a ser lida
 * @param bufferOutPtr    buffer de escrita do comando recebido
 * @param bufferOutSize   tamanho do buffer disponível para escrita 
 * @return esp_err_t      resultado da operação, sucesso = ESP_OK
 */
static esp_err_t dto_to_command(const char * bufferInPtr, char * bufferOutPtr, size_t bufferOutSize)
{
  cJSON * root = cJSON_Parse(bufferInPtr);
  if (root == NULL)
  {
    return ESP_FAIL;
  }

  return get_json_string_value(root, "command", bufferOutPtr, bufferOutSize);
}


/**
 * Transformação do body JSON recebido para identificação da manipulação de carga nas estações
 * 
 * @param bufferInPtr     estrutura JSON a ser lida
 * @param dtoPtr    buffer de escrita do comando recebido
 * @return esp_err_t      resultado da operação, sucesso = ESP_OK
 */
static esp_err_t dto_to_io_command(const char * bufferInPtr, ioDto_t * dtoPtr)
{
  cJSON * root = cJSON_Parse(bufferInPtr);
  if (root == NULL)
  {
    return ESP_FAIL;
  }

  /* Recupera MAC da estação a ser manipulada */
  esp_err_t result = get_json_string_value(root, "mac", dtoPtr->mac, sizeof(dtoPtr->mac));

  if (result != ESP_OK)
  {
    return result;
  }

  /* Recupera valor a ser enviado ao pino da estação */
  result = get_json_int_value(root, "value", &dtoPtr->value); 

  if (result != ESP_OK)
  {
    return result;
  }

  /* Valida se valor do pino está no range adequado */
  return (dtoPtr->value >= 0 && dtoPtr->value <= 100) ? ESP_OK : ESP_FAIL;
}


/*******************************************************************************
* END OF FILE
*******************************************************************************/
