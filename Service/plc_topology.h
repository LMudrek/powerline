/*******************************************************************************
* Leonardo Mudrek de Almeida
* UTFPR - CT
*
*
* License : CC BY NC SA 4.0
*******************************************************************************/
#ifndef PLC_TOPOLOGY_H
#define PLC_TOPOLOGY_H

/*******************************************************************************
* INCLUDES
*******************************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "plc_module_types.h"
/*******************************************************************************
* DEFINES E ENUMS
*******************************************************************************/
/* Maximo de módulos no retorno que a topologia exibe */

/* Concentradores */
#define MAX_CCO_NUM   3
/* Estações */
#define MAX_STA_NUM   10

/*******************************************************************************
* TYPEDEFS
*******************************************************************************/

/* Estrutura de um node e seus campos consumidos pela API */
typedef struct node_t 
{
  /* Endereço MAC do módulo na rede PLC */
  uint8_t mac[6];
  /* ID de contagem */
  uint8_t id;
  /* Tipo módulo */
  nodeRole_t role;
  /* Signal-to-noise ratio */ 
  uint8_t snr;
  /* Atenuação da comunicação */
  uint8_t atenuation;
  /* Fase elétrica módulo */
  uint8_t phase;
} node_t;

/* Estrutura da topologia vista pelo CCO sob controle do ESP */
typedef struct topology_t
{
  /* Vetor armazenamento concentradores */
  node_t cco[MAX_CCO_NUM];
  /* Vetor armazanemento estações */
  node_t sta[MAX_STA_NUM];
  /* Contador tipo módulo concentrador */
  uint32_t ccoCount;
  /* Contador tipo módulo estação */
  uint32_t staCount;
} topology_t;

/*******************************************************************************
* FUNÇÕES EXPORTADAS
*******************************************************************************/
bool plc_topology_get(topology_t * topologyPtr);

/*******************************************************************************
* END OF FILE
*******************************************************************************/
#endif
