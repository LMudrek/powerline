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
#include "plc_topology.h"
#include "plc_uart_model.h"
#include "string.h"
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
static void copy_node(const node_t nodeCopynode, node_t * nodePtr, uint32_t * nodeCounterPtr);

/*******************************************************************************
* FUNÇÕES EXPORTADAS
*******************************************************************************/

/**
 * Montar objeto topologia dos concentradores (CCO) e estações (STA)
 * 
 * @param topologyPtr ponteiro a ser escrita estrutura
 * @return true       encontrou módulos disponíveis
 * @return false      não foram encontrados módulos pela rede elétrica
 */
bool plc_topology_get(topology_t * topologyPtr)
{
  node_t nodes[MAX_CCO_NUM + MAX_STA_NUM];

  bzero(topologyPtr, sizeof(topology_t));

  uint32_t nodeCount = plc_uart_model_get_topology(nodes, sizeof(nodes));

  for (uint32_t idx = 0; idx < nodeCount; idx++)
  {
    nodes[idx].role == NODE_ROLE_CCO ? copy_node(nodes[idx], 
                                                      &topologyPtr->cco[0], 
                                                      &topologyPtr->ccoCount) :
                                       copy_node(nodes[idx], 
                                                      &topologyPtr->sta[0], 
                                                      &topologyPtr->staCount);
  }

  return nodeCount != 0;
}

/*******************************************************************************
* FUNÇÕES LOCAIS
*******************************************************************************/
/**
 * Copia estrutura de um node
 * 
 * @param nodeCopynode    node a ser copiado
 * @param nodePtr         node a ser escrito
 * @param nodeCounterPtr  quantidade do tipo de node copiado, incremento +1
 */
static void copy_node(const node_t nodeCopynode, node_t * nodePtr, uint32_t * nodeCounterPtr)
{
  nodePtr[*nodeCounterPtr] = nodeCopynode;
  *nodeCounterPtr += 1;
}
/*******************************************************************************
* END OF FILE
*******************************************************************************/
