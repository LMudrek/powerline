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
#include "plc_uart.h"
#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"
/*******************************************************************************
* DEFINES E ENUMS
*******************************************************************************/
#define UART_PLC_NUM            UART_NUM_1
#define UART_PLC_BUFFER_SIZE    1024
#define MAX_UART_SEND_RETRY     5
#define MAX_UART_RESPONSE_RETRY 10
#define WAIT_RESPONSE_TIME      (100 / portTICK_PERIOD_MS)
#define UART_EVENT_QUEUE_SIZE   20
#define UART_PLC_TX_PIN         GPIO_NUM_22
#define UART_PLC_RX_PIN         GPIO_NUM_23
/*******************************************************************************
* TYPEDEFS
*******************************************************************************/
typedef struct queueCommandResponse_t
{
    char * namePtr;
    uint32_t countToWait;
    char * bufferOutPtr;
    uint32_t notificationIndex;
} queueCommandResponse_t;

/*******************************************************************************
* CONSTANTES
*******************************************************************************/
/* Identificador LOG */
static const char *TAG = "PLC_CONFIG_SERVICE";

/*******************************************************************************
* VARIÁVEIS
*******************************************************************************/
static QueueHandle_t uartPlcHandler;
static uint8_t uartPlcBuffer[UART_PLC_BUFFER_SIZE];
static SemaphoreHandle_t uartResponseSemaphore;
static uartPlcResponse_t * uartResponsePtr;
/*******************************************************************************
* PROTÓTIPOS DE FUNÇÕES
*******************************************************************************/
static void plc_uart_task(void *pvParameters);
static bool uart_send(const char *sendBufferPtr, size_t size);
static void parse_uart_data(uint8_t * bufferOutPtr, size_t bytesReceived);
static bool wait_for_response(void);
static bool parse_result(const char * bufferRxPtr, uartPlcResponse_t * uartResponsePtr);
static bool parse_notification(const char * bufferRxPtr);
static void parse_response(const char * bufferRxPtr, uartPlcResponse_t * uartResponsePtr);
static void config_plc_uart(void);
/*******************************************************************************
* FUNÇÕES EXPORTADAS
*******************************************************************************/

/**
 * Inicializa UART e configura sistemas de controle
 * 
 */
void plc_uart_init(void)
{
    /* Inicializa interface ESP */
    config_plc_uart();

    /* Cria semáforo de envio */
    uartResponseSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(uartResponseSemaphore);

    /* Cria task de controle da interface UART */
    xTaskCreate(plc_uart_task, "plc_uart_task", 4096, NULL, 12, NULL);
}

/**
 * Envia comando buffer para a UART PLC
 * 
 * @param sendBufferPtr     buffer a ser enviado
 * @param responsePtr       estrutura de preenchimento da resposta
 */
void plc_uart_send(const void *sendBufferPtr, uartPlcResponse_t * responsePtr)
{
    /* Aguarda não ter um tratamento de comando em andamento */
    xSemaphoreTake(uartResponseSemaphore, portMAX_DELAY);

    /* Copia estrutura de resposta para variável local, a ser escrita de maneira assíncrona */
    uartResponsePtr = responsePtr;

    /* Envia comando */
    bool result = uart_send(sendBufferPtr, strlen(sendBufferPtr));

    if (result == false)
    {
        /* Falha interface UART, finaliza execução */
        uartResponsePtr->result = false;
        xSemaphoreGive(uartResponseSemaphore);
        return;
    }

    /* Comando colocado na fila, aguarda e valida resposta */
    static uint32_t attemptsResponse;
    attemptsResponse = attemptsResponse == 0 ? MAX_UART_SEND_RETRY : attemptsResponse;
    if (wait_for_response() == false)
    {
        /* Tempo para aguarde da resposta esgotado, realiza retry */
        ESP_LOGI(TAG, "Retry TX UART[%d]", UART_PLC_NUM);
        xSemaphoreGive(uartResponseSemaphore);

        if (--attemptsResponse != 0)
        {
            /* Utiliza função de maneira recursiva */
            plc_uart_send(sendBufferPtr, responsePtr);
        }
    }
}

/*******************************************************************************
* FUNÇÕES LOCAIS
*******************************************************************************/

/**
 * Configura interface baixo nível UART ESP 
 * 
 */
static void config_plc_uart(void)
{
    /* Template de configuração da UART */
    uart_config_t config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_REF_TICK,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_PLC_NUM, &config));

    /* Instala tratamento */
    ESP_ERROR_CHECK(uart_driver_install(
        UART_PLC_NUM,
        UART_PLC_BUFFER_SIZE,
        UART_PLC_BUFFER_SIZE,
        UART_EVENT_QUEUE_SIZE,
        &uartPlcHandler,
        0)
    );

    /* Define pinos para TX e RX */
    ESP_ERROR_CHECK(uart_set_pin(
        UART_PLC_NUM,
        UART_PLC_TX_PIN, UART_PLC_RX_PIN,
        UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE)
    );
}

/**
 * Envia buffer para UART baixo nível ESP 
 * 
 * @param sendBufferPtr     Buffer a ser enviado
 * @param size              Tamanho do buffer para enviar
 * @return true             Bytes colocados na fila para escrita
 * @return false            Falha interface ESP
 */
static bool uart_send(const char *sendBufferPtr, size_t size)
{
    static uint32_t attemptsTx;
    attemptsTx = MAX_UART_SEND_RETRY;
    for (;attemptsTx != 0;)
    {
        int32_t bytesSent = uart_write_bytes(UART_PLC_NUM, sendBufferPtr, strlen(sendBufferPtr));
        if (bytesSent < 0)
        {
            /* Falha colocar na fila, executa loop novamente */
            attemptsTx--;
            continue;
        }

        /* Bytes colocados na fila, retorna sucesso */
        ESP_LOGI(TAG, "TX UART[%d] SENT: %s", UART_PLC_NUM, sendBufferPtr);
        return true;
    }

    ESP_LOGI(TAG, "TX UART[%d] FAIL: %s", UART_PLC_NUM, sendBufferPtr);
    return false;
}

/**
 * Aguarda semaforo de envio e tratamento UART ser liberado 
 * 
 * @return true     Processamento concluído 
 * @return false    Falha no processamento
 */
static bool wait_for_response(void)
{
    static uint32_t attemptsRx;
    attemptsRx = MAX_UART_SEND_RETRY * 2;

    while(xSemaphoreTake(uartResponseSemaphore, WAIT_RESPONSE_TIME) != pdPASS)
    {
        if (--attemptsRx == 0)
        {
            /* Wait loop esgotado */
            return false;
        }
    }

    /* Libera semaforo */
    return (xSemaphoreGive(uartResponseSemaphore) == pdPASS);
}

/**
 * Task de controle eventos UART
 * 
 * @param pvParameters  
 */
static void plc_uart_task(void *pvParameters)
{
    uart_event_t event;
    while (true)
    {
        //Waiting for UART event.
        if (xQueueReceive(uartPlcHandler, (void *)&event, (portTickType)portMAX_DELAY))
        {
            bzero(uartPlcBuffer, UART_PLC_BUFFER_SIZE);
            ESP_LOGI(TAG, "UART[%d] event, size: %u", UART_PLC_NUM, event.size);
            switch (event.type)
            {
                case UART_DATA:
                    parse_uart_data(uartPlcBuffer, event.size);
                    break;
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "HW FIFO overflow");
                    /* Buffer recepção estourado */
                    uart_flush_input(UART_PLC_NUM);
                    xQueueReset(uartPlcHandler);
                    break;
                case UART_BUFFER_FULL:
                    /* Buffer aplicação estourado */
                    ESP_LOGI(TAG, "ring buffer full");
                    uart_flush_input(UART_PLC_NUM);
                    xQueueReset(uartPlcHandler);
                    break;
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
                }
        }
    }
}

/**
 * Realiza leitura do buffer de recepção e trata dados
 * 
 * @param bufferOutPtr  Buffer para tratar
 * @param bytesReceived Tamanho do tratamento
 */
static void parse_uart_data(uint8_t * bufferOutPtr, size_t bytesReceived)
{
    uart_read_bytes(UART_PLC_NUM, bufferOutPtr, bytesReceived, portMAX_DELAY);
    char * data = strtok((char *)bufferOutPtr, "\r\n");
    char cmdData[128];
    for (;data != NULL && data[0] != '\0';)
    {
        bzero(cmdData, sizeof(cmdData));
        strcpy(cmdData, data);

        ESP_LOGI(TAG, "TX UART[%u] DATA TO PARSE: %s", UART_PLC_NUM, data);

        if (parse_result(&cmdData[0], uartResponsePtr) == true)
        {
            /* Processou resultado, finaliza tratamento */
            xSemaphoreGive(uartResponseSemaphore);
            break;
        }

        if (parse_notification(&cmdData[0]) == true)
        {
            /* Processou notificação, finaliza tratamento */
            break;
        }

        /* Procesa linha como dado de resposta para um comando */
        parse_response(&cmdData[0], uartResponsePtr);

        /* Processa proxima linha da resposta */
        data = strtok(NULL, "\r\n");
    }
}

/**
 * Realiza tratamento mensagem de resultado
 * 
 * @param bufferRxPtr       Buffer a ser tratado
 * @param uartResponsePtr   Estrutura de resultado
 * @return true             Buffer tratado
 * @return false            Dados não são de resultados. Não tratado
 */
static bool parse_result(const char * bufferRxPtr, uartPlcResponse_t * uartResponsePtr)
{    
    if (strstr(bufferRxPtr, "ERROR") != NULL || strstr(bufferRxPtr, "FAIL") != NULL)
    {
        if (uartResponsePtr != NULL)
        {
            uartResponsePtr->result = false;
        }
        ESP_LOGI(TAG, "Falha notificada pelo módulo");
        /* Processamento com sucesso */
        return true;
    }

    char * resultOk = strstr(bufferRxPtr, "OK");
    if (resultOk != NULL)
    {
        
        if (uartResponsePtr != NULL)
        {
            uartResponsePtr->result = true;
        }
        
        ESP_LOGI(TAG, "Sucesso comando, %s", resultOk);
        /* Processamento com sucesso */
        return true;
    }

    return false;
}

/**
 * Realiza tratamento mensagem de notificação
 * 
 * @param bufferRxPtr       Buffer a ser tratado
 * @return true             Buffer tratado
 * @return false            Dados não são de resultados. Não tratado
 */
static bool parse_notification(const char * bufferRxPtr)
{
    if (strchr(bufferRxPtr, ':') == NULL)
    {
        /* Como não tem dados no tratamento, ":", comando é de notificação */
        ESP_LOGI(TAG, "Notification: %s", bufferRxPtr);
        return true;
    }

    return false;
}

/**
 * Realiza tratamento mensagem de resposta de comando
 * 
 * @param bufferRxPtr       Buffer a ser tratado
 * @param uartResponsePtr   Estrutura de resultado
 */
static void parse_response(const char * bufferRxPtr, uartPlcResponse_t * uartResponsePtr)
{
    /* Recupera posição do separador entre <COMANDO>:<DATA> */
    char * atCmdDividerPtr = strchr(bufferRxPtr, ':');

    ESP_LOGI(TAG, "TX UART[%u] response foo: %s", UART_PLC_NUM, bufferRxPtr);

    if ((uartResponsePtr != NULL) && (atCmdDividerPtr != NULL))
    {
        /* Copia dados de comando, formato -> "XX:yy". Sendo XX = comando respondido */
        strncpy(&uartResponsePtr->command[0], &bufferRxPtr[1], strlen(&bufferRxPtr[1]) - strlen(atCmdDividerPtr));

        ESP_LOGI(TAG, "AT Command: %s", uartResponsePtr->command);

        /* Copia dados de respondidos, formato -> "XX:yy". Sendo yy = dados respondido */
        strcpy(&uartResponsePtr->data[uartResponsePtr->lineCounter][0], &atCmdDividerPtr[1]);

        ESP_LOGI(TAG, "AT Data: %s", uartResponsePtr->data[uartResponsePtr->lineCounter]); 

        /* Linha de resposta tratada */
        uartResponsePtr->lineCounter++;
    }        
}
/*******************************************************************************
* END OF FILE
*******************************************************************************/
