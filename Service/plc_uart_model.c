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
#include "plc_uart_model.h"
#include "plc_uart.h"
#include <string.h>
#include "esp_log.h"
#include <stdio.h>
#include "plc_module_types.h"
/*******************************************************************************
* DEFINES E ENUMS
*******************************************************************************/

/*******************************************************************************
* TYPEDEFS
*******************************************************************************/

/*******************************************************************************
* CONSTANTES
*******************************************************************************/
/* Identificador LOG */
static const char *TAG = "PLC_UART_MODEL";
/*******************************************************************************
* VARIÁVEIS
*******************************************************************************/

/*******************************************************************************
* PROTÓTIPOS DE FUNÇÕES
*******************************************************************************/
static void mac_string_hex_to_bytes (const char * bufferInPtr, uint8_t * bufferOutPtr);
static uint32_t split_convert_to_number (const char * dataToSplitPtr, const char * keySplit, const uint32_t baseConvert);
static void format_mac_only_numbers(char * macPtr);
/*******************************************************************************
* FUNÇÕES EXPORTADAS
*******************************************************************************/

/**
 * Transforma layout recebido pela UART em array nodes 
 * 
 * @param nodeBufferPtr   buffer de nodes a ser escrito
 * @param nodeBufferSize  tamanho disponivel para escrita
 * @return uint32_t       número de módulos recebidos
 */
uint32_t plc_uart_model_get_topology(node_t * nodeBufferPtr, 
                                     size_t nodeBufferSize)
{
  /* Limpa e inicializa estruturas */
  bzero(nodeBufferPtr, nodeBufferSize);
  uartPlcResponse_t response;
  bzero(&response, sizeof(uartPlcResponse_t));

  /* Envia comando ao módulo para captar topologia */
  plc_uart_send("AT+TOPOINFO=1,4\r\n\0", &response);

  for(uint32_t idx = 0; idx < response.lineCounter; idx++)
  {
    char * data = strtok(&response.data[idx][0], ",");
    mac_string_hex_to_bytes(data, nodeBufferPtr[idx].mac);

    nodeBufferPtr[idx].id = split_convert_to_number(NULL, ",", 10);
    split_convert_to_number(NULL, ",", 10);
    split_convert_to_number(NULL, ",", 10);
    nodeBufferPtr[idx].role = split_convert_to_number(NULL, ",", 10) == NODE_ROLE_CCO ? NODE_ROLE_CCO : NODE_ROLE_STA;
    nodeBufferPtr[idx].snr = split_convert_to_number(NULL, ",", 10);
    nodeBufferPtr[idx].atenuation = split_convert_to_number(NULL, ",", 10);
    nodeBufferPtr[idx].phase = split_convert_to_number(NULL, ",", 10);
  }

  return response.lineCounter;
}

/**
 * Manipula carga estação (STA) PLC
 * 
 * @param macPtr  MAC da estação a ser controlada
 * @param value   valor a ser definido na saída do módulo PLC
 * @return true   manipulação com sucesso
 * @return false  falha na operação
 */
bool plc_uart_model_io(const char * macPtr, const uint32_t value)
{
  format_mac_only_numbers(macPtr);

  uartPlcResponse_t response;
  bzero(&response, sizeof(uartPlcResponse_t));
  
  bool result = sprintf(response.command, "AT+IOCTRL=%s,%u,%u\r\n", macPtr, PLC_MODEL_GPIO_LOAD, value) > 0;

  if (result == false)
  {
    return false;
  }

  plc_uart_send(response.command, &response);

  return response.result;
}
/*******************************************************************************
* FUNÇÕES LOCAIS
*******************************************************************************/
/**
 * Converte MAC string em array de bytes com representação em hexa
 * 
 * @param bufferInPtr   MAC string
 * @param bufferOutPtr  MAC hex bytes
 */
static void mac_string_hex_to_bytes (const char * bufferInPtr, uint8_t * bufferOutPtr)
{
  size_t macLength = strlen(bufferInPtr);
  uint32_t counter = 0;
  for (uint32_t stringIdx = 0; stringIdx < macLength; stringIdx += 2)
  {
    /* Realiza leitura dois caracteres string para ter os dados do byte em hex */
    const char macByte[2] = {bufferInPtr[stringIdx], bufferInPtr[stringIdx + 1]};
    /* Transforma valor do texto em byte */
    bufferOutPtr[counter++] = strtol(macByte, NULL, 16);
  }
}

/**
 * Realiza split de uma string e transforma seus valores em uint32_t 
 * 
 * O método utiliza da função "strtok", ou seja, a divisão e andamento
 * é por interações. Na primeira interação deve ser consumido o buffer 
 * a ser realizado o split. Para sequenciar, manter dataToSplitPtr = NULL
 * 
 * @param dataToSplitPtr  String a ser divida por um padrão
 * @param keySplit        padrão a ser utilizado na divisão
 * @param baseConvert     base numérica para conversão
 * @return uint32_t       número convertido a partir da string
 */
static uint32_t split_convert_to_number (const char * dataToSplitPtr, const char * keySplit, const uint32_t baseConvert)
{
  char * data = strtok(dataToSplitPtr, keySplit);
  return data != NULL ? strtol(data, NULL, baseConvert) : 0;
}

/**
 * Remove ":" do endereço MAC string em somente números 
 * 
 * @param macPtr  string MAC a ser formatada
 */
static void format_mac_only_numbers(char * macPtr)
{
  size_t length = strlen(macPtr);
  char macFormated[13] = "\0";
  uint32_t counter = 0;
  for (uint32_t idx = 0; idx < length; idx++)
  {
    if (macPtr[idx] != ':')
    {
      macFormated[counter++] = macPtr[idx];
    }    
  }

  bzero(macPtr, length);
  strncpy(macPtr, macFormated, strlen(macFormated));
}
/*******************************************************************************
* END OF FILE
*******************************************************************************/
