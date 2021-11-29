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
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

/*******************************************************************************
* DEFINES E ENUMS
*******************************************************************************/
/* Armazenamento NVS */
#define STORAGE_NAMESPACE "storage"

/*******************************************************************************
* TYPEDEFS
*******************************************************************************/

/*******************************************************************************cc
* CONSTANTES
*******************************************************************************/
/* TAG para efeitos de log */
static const char *TAG = "NVS_SERVICE";

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
 * Inicializa Non-Volatile Storage (NVS)
 * 
 */
void nvs_service_init(void)
{
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
}

/**
 * Realiza a leitura de uma string de acordo com a key armazenada para o valor
 * 
 * @param keyPtr - identificador na DB
 * @param bufferOutPtr - buffer a ser escrito o valor encontrado
 * @param length - tamanho do buffer para escrita
 * @return int32_t - Tabela a seguir
 *         0  - Falha na leitura 
 *        -1  - Key não existe na DB
 *         1  - Valor encontrado, buffer com o dado 
 */
int32_t nvs_service_read_string(const char * keyPtr, char * bufferOutPtr, size_t length)
{
  nvs_handle_t nvsHandle;
  /* Comunica interface NVS */
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle);

  /* Inicializa como falha */
  int32_t result = 0;

  /* Valida se sucesso no acesso ao periférico */
  if (err == ESP_OK)
  {
    /* Realiza leitura da DB */
    err = nvs_get_str(nvsHandle, keyPtr, bufferOutPtr, &length);
    switch (err)
    {
      case ESP_OK:
        /* Dado encontrado */
        ESP_LOGI(TAG, "Key (%s) -> value found (%s)", keyPtr, bufferOutPtr);
        result = 1;
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        /* Posição não encontrada na DB */
        ESP_LOGI(TAG, "The key (%s) is not initialized yet!", keyPtr);
        result = -1;
        break;
      default: { }
    }
  }

  /* Finaliza interface e retorna resultado */
  nvs_close(nvsHandle);
  return result;
}

/**
 * Armazena string
 * 
 * @param keyPtr - indexação DB
 * @param bufferInPtr - dado a ser gravado
 * @param length - tamanho do dado a ser gravado
 * @return true - sucesso na escrita
 * @return false - falha na escrita
 */
bool nvs_service_write_string(const char * keyPtr, const char * bufferInPtr, size_t length)
{
  nvs_handle_t nvsHandle;

  /* Valida abertura da interface e, caso sucesso, escreve dado e finaliza escrita */
  bool result = ((nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle) == ESP_OK) &&
                 (nvs_set_str(nvsHandle, keyPtr, bufferInPtr) == ESP_OK) &&
                 (nvs_commit(nvsHandle) == ESP_OK));

  /* Finaliza interface */
  nvs_close(nvsHandle);

  ESP_LOGI(TAG, "Write key (%s) with value (%s) -> result (%s)", keyPtr, bufferInPtr, result ? "true":"false");
  return result;
}

/**
 * Deleta key da base
 * 
 * @param keyPtr - nome da chave a ser excluída
 * @return true - sucesso na remoção
 * @return false - falha na remoção
 */
bool nvs_service_erase_key(const char * keyPtr)
{
  nvs_handle_t nvsHandle;

  /* Valida abertura da interface e, caso sucesso, deleta dado e finaliza escrita */
  bool result = ((nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle) == ESP_OK) &&
                 (nvs_erase_key(nvsHandle, keyPtr) == ESP_OK) &&
                 (nvs_commit(nvsHandle) == ESP_OK));

  /* Finaliza interface */
  nvs_close(nvsHandle);

  ESP_LOGI(TAG, "Delete key (%s) -> result (%s)", keyPtr, result ? "true":"false");
  return result;   
}

/*******************************************************************************
* FUNÇÕES LOCAIS
*******************************************************************************/

/*******************************************************************************
* END OF FILE
*******************************************************************************/
