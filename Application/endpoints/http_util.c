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
#include "http_util.h"
#include "json_buffer.h"
/*******************************************************************************
* DEFINES E ENUMS
*******************************************************************************/

/*******************************************************************************
* TYPEDEFS
*******************************************************************************/

/*******************************************************************************
* CONSTANTES
*******************************************************************************/

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
 * Envia resposta no padrão JSON
 * 
 * @param reqPtr      requisição a ser respondida
 * @param httpCode    código HTTP para inserir na reposta
 * @param messagePtr  mensagem a ser inserida no body JSON
 * @return esp_err_t  retorno da operação, sucesso = ESP_OK
 */
esp_err_t http_util_send_response(httpd_req_t * reqPtr, const char * httpCode, const char * messagePtr)
{
  /* Cria body JSON e insere mensagem de retorno */
  cJSON * root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "message", messagePtr);
  close_json(root, json_buffer_get(), json_buffer_get_size());

  /* Define código de retorno e tipo de conteúdo como JSON */
  httpd_resp_set_status(reqPtr, httpCode);
  httpd_resp_set_type(reqPtr, HTTPD_TYPE_JSON);

  /* Envia e retorna resultado da operação */
  return httpd_resp_send(reqPtr, json_buffer_get(), HTTPD_RESP_USE_STRLEN);
}

/**
 * Realiza leitura de um body recebido via HTTP server 
 * 
 * @param req             requisição originária
 * @param bufferOutPtr    buffer de saída para escrever o corpo recebido
 * @param bufferOutSize   tamanho disponível para escrita
 * @return esp_err_t      retorno da operação, sucesso = ESP_OK
 */
esp_err_t http_read_body(httpd_req_t * req, char * bufferOutPtr, size_t bufferOutSize)
{
  memset(bufferOutPtr, 0, bufferOutSize);

  int32_t content_len = req->content_len;

  if (content_len >= bufferOutSize) {
      http_util_send_response(req, HTTPD_500, "Content too long");
      return ESP_FAIL;
  }

  int32_t cur_len = 0;
  int32_t received = 0;

  while (cur_len < content_len) {
      received = httpd_req_recv(req, bufferOutPtr + cur_len, bufferOutSize);
      if (received <= 0) {
          /* Não conseguiu ler corpo recebido */
          http_util_send_response(req, HTTPD_500, "Failed to read control value");
          return ESP_FAIL;
      }
      cur_len += received;
  }

  return ESP_OK;
}

/**
 * Recupera valor em formato de string de um campo JSON
 * 
 * @param root        JSON a ser identificado campo
 * @param key         nome do campo
 * @param fieldPtr    buffer para escrever valor do campo
 * @param sizeField   tamanho disponível para escrita
 * @return esp_err_t  retorno da operação, sucesso = ESP_OK
 */
esp_err_t get_json_string_value(cJSON * root, const char * key, char * fieldPtr, size_t sizeField)
{

  memset(fieldPtr, 0, sizeField);

  cJSON * field = cJSON_GetObjectItem(root, key);
  bool result = ((cJSON_IsString(field)) && (field->valuestring != NULL));
  if (result == false)
  {
    return ESP_FAIL;
  }

  result = strlen(field->valuestring) <= sizeField;
  if (result == false)
  {
    return ESP_FAIL;
  }

  return strncpy(fieldPtr, field->valuestring, strlen(field->valuestring)) != NULL ? ESP_OK : ESP_FAIL;
}

/**
 * Recupera valor em formato numérico de um campo JSON
 * 
 * @param root        JSON a ser identificado campo
 * @param key         nome do campo
 * @param fieldPtr    buffer para escrever valor do campo
 * @return esp_err_t  retorno da operação, sucesso = ESP_OK
 */
esp_err_t get_json_int_value(cJSON * root, const char * key, uint32_t * fieldPtr)
{
  *fieldPtr = NULL;

  cJSON * field = cJSON_GetObjectItem(root, key);

  if (cJSON_IsNumber(field) == false)
  {
    return ESP_FAIL;
  }

  *fieldPtr = field->valueint;
  return ESP_OK;
}

/**
 * Fecha buffer de envio JSON, transformando em texto para envio da resposta 
 * 
 * @param root          JSON original a ser fechado
 * @param bufferOutPtr  buffer de escrita da estrutura de JSON para texto
 * @param sizeBufferOut tamanho disponível para escrita
 * @return true         buffer JSON transformado com sucesso para texto
 * @return false        falha na transformação
 */
bool close_json(cJSON * root, char *bufferOutPtr, size_t sizeBufferOut)
{
  memset(bufferOutPtr, 0, sizeBufferOut);

  char * jsonString = cJSON_Print(root);
  const size_t sizeJsonString = strlen(jsonString);
  const bool result = sizeBufferOut >= sizeJsonString;
  if (result)
  {
    /* Espaço disponível, realiza copia */
    strncpy(bufferOutPtr, jsonString, sizeJsonString);
  }

  /* Limpa body JSON */
	cJSON_Delete(root);
  cJSON_free(jsonString);

  return result;
}
/*******************************************************************************
* FUNÇÕES LOCAIS
*******************************************************************************/

/*******************************************************************************
* END OF FILE
*******************************************************************************/
