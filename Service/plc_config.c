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
#include "plc_config.h"
#include "plc_uart.h"
#include <stddef.h>
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
 * Inicializa módulo PLC
 * 
 */
void plc_config_init(void)
{
  plc_uart_init();
}

/**
 * Configura módulo para modo AT
 * 
 * @return true   configuração com sucesso
 * @return false  falha configuração 
 */
bool plc_configure_module(void)
{
  uartPlcResponse_t response;

  bzero(&response, sizeof(uartPlcResponse_t));
  response.result = false;

  /* Altera comando para modelo AT */
  plc_uart_send("++", &response);

  if (response.result == false) {
    return false;
  }

  bzero(&response, sizeof(uartPlcResponse_t));
  response.result = false;
  
  /* Envia comando para definir padrão como AT */
  plc_uart_send("AT+MODE=2\r\n", &response);

  return response.result;
}

/*******************************************************************************
* FUNÇÕES LOCAIS
*******************************************************************************/

/*******************************************************************************
* END OF FILE
*******************************************************************************/
