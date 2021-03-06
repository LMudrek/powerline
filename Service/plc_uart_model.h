/*******************************************************************************
* Leonardo Mudrek de Almeida
* UTFPR - CT
*
*
* License : CC BY NC SA 4.0
*******************************************************************************/
#ifndef PLC_UART_MODEL_H
#define PLC_UART_MODEL_H

/*******************************************************************************
* INCLUDES
*******************************************************************************/
#include <stddef.h>
#include "plc_topology.h"
/*******************************************************************************
* DEFINES E ENUMS
*******************************************************************************/

/*******************************************************************************
* TYPEDEFS
*******************************************************************************/

/*******************************************************************************
* FUNÇÕES EXPORTADAS
*******************************************************************************/
uint32_t plc_uart_model_get_topology(node_t * nodeBufferPtr, 
                                     size_t nodeBufferSize);
bool plc_uart_model_io(const char * macPtr, const uint32_t value);
/*******************************************************************************
* END OF FILE
*******************************************************************************/
#endif
