/**
 * @file app.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-05-17
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "soc/soc.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <WiFi.h>

#include "lora.h"
#include "app.h"

#include "get.h"
#include "post.h"

#define mBitsSet(f,m)       ((f)|=(m))
#define mBitsClr(f,m)       ((f)&=(~(m)))
#define mBitsTgl(f,m)       ((f)^=(m))
#define mBitsMsk(f,m)       ((f)& (m))
#define mIsBitsSet(f,m)     (((f)&(m))==(m))
#define mIsBitsClr(f,m)     (((~(f))&(m))==(m))

static const char *TAG = APP_TAG_STR;

enum e_statusMask{
    ST_ALL_CLEARED                      = 0x00000000,
    ST_ALL_SET                          = 0xffffffff,
    ST_LORA_MODULE_IS_IN_TX_MODE        = 0x00000001,
    ST_LORA_MODULE_TX_DONE_TRIGGERED    = 0x00000002, 
    ST_LORA_MODULE_RX_DONE_TRIGGERED    = 0x00000004,
    ST_LORA_MODULE_REQUEST_ADDRESS      = 0x00000008,

    ST_LORA_STATE_LED                   = 0x00000010,
    ST_LORA_MODULE_LINK_ADDRESS         = 0x00000020,
    ST_VALVE_OPEN                       = 0x00000040,
    ST_VALVE_CLOSE                      = 0x00000080,
};

static struct s_app{
    uint32_t    m_uStatus;
    uint8_t     m_u8HostAddress;
    uint8_t     m_uProbeAddress;
}app={
    ST_ALL_CLEARED,
    APP_LORA_HOST_ADDRESS,
};

/***** Private Function Prototype Declaration/Implementation Section ***************************/

/************************************************************************************************
 * @brief Callback function called on LoRa Rx done event. 
 * 
 * @param iPa Not used.
 */
void _AppLoRaRxCallback(int iPa){
    (void) iPa;
    mBitsSet(app.m_uStatus, ST_LORA_MODULE_RX_DONE_TRIGGERED);
}

/************************************************************************************************
 * @brief Callback function called on LoRa Tx done event.
 */
void _AppLoRaTxCallback(void){
    mBitsSet(app.m_uStatus, ST_LORA_MODULE_TX_DONE_TRIGGERED);
}

/************************************************************************************************
 * @brief Signaling the App state for Tx mode status. Set the Idle state
 *        of the LoRa module.  
 */
void _AppLoRaSetTxMode(void){
    if(mIsBitsSet(app.m_uStatus, ST_LORA_MODULE_IS_IN_TX_MODE)) return;
    ESP_LOGI(TAG, "Setting Tx Mode");
    LoRaIdle();
    mBitsSet(app.m_uStatus, ST_LORA_MODULE_IS_IN_TX_MODE);
}

/************************************************************************************************
 * @brief Signaling the App state for Rx mode status. Set the Rx state
 *        of the LoRa module for receiving incomming data on LoRa radio.
 */
void _AppLoRaSetRxMode(void){
    if(mIsBitsClr(app.m_uStatus, ST_LORA_MODULE_IS_IN_TX_MODE)) return;
    ESP_LOGI(TAG, "Setting Rx Mode");
    LoRaReceive(0);
    mBitsClr(app.m_uStatus, ST_LORA_MODULE_IS_IN_TX_MODE);
}



/************************************************************************************************
 * @brief 
 *     
 */
void _AppLoraSendData(uint8_t destAddr, const char * strMessage){
    uint8_t msgLen;
    _AppLoRaSetTxMode();                    /* signaling the App Tx status and setting LoRa module for Tx action    */

    LoRaWriteByte(destAddr);     /* write the module destination address to LoRa Tx FIFO             */
    LoRaWriteByte(APP_LORA_HOST_ADDRESS);                    /* write the module source address to LoRa Tx FIFO                  */
    LoRaWriteByte(msgLen=(uint8_t)strnlen(strMessage, 250));   /* write the data message string length to the Tx FIFO      */
    for(int k=0; k<msgLen; ++k){                        /* loop for...                                              */
        LoRaWriteByte(strMessage[k]);                          /* ...writing the data message string bytes to the Tx FIFO  */
    }                                                   /*                                                          */
    LoRaWriteByte('\0');        /* write the null string terminator to the LoRa Tx FIFO                             */
    LoRaEndPacket(TRUE);        /* ending the Tx data packet session, triggering LoRa Tx data packet on radio       */
    ESP_LOGI(TAG, "Sent data to 0x%02x [%s]", destAddr, strMessage);
    mBitsSet(app.m_uStatus, ST_LORA_MODULE_TX_DONE_TRIGGERED);
}

/************************************************************************************************
 * @brief The freeRTOS task function for LoRa processing Tx and Rx mechanisms.
 * 
 * @param pV Not used.
 */
void _AppLoRaTask(void*pV){
    (void)pV;
    const char*msg = APP_SENDING_MESSAGE_STR;
    static char buf[128];
    // uint8_t msgLen;
    unsigned long lBaseTime = 0;
    unsigned long lElapsedTime;
    unsigned long lCurrentTime;
    int address[APP_LORA_PROBE_NB];

        ESP_LOGI(TAG, "----------- ENTERING _AppLoRaTask() ------------");
    _AppLoRaSetRxMode();

    for(;;){  /******** freeRTOS task perpetual loop **************************************************************/

        GetLoop();
        /* getting valve order from website */
        int valveOrderLength = GetLoop();
        printf("valveOrderLength : %d\n", valveOrderLength);

        if(valveOrderLength == 1){                             /* if 35 characters, then sending "true" **********/
            ESP_LOGI(TAG, "Ouverture de la vanne");
            mBitsSet(app.m_uStatus, ST_VALVE_OPEN);
        }
        else if(valveOrderLength == 2){                        /* if 36 characters, then sending "false" *********/
            ESP_LOGI(TAG, "Fermeture de la vanne");
            mBitsSet(app.m_uStatus, ST_VALVE_CLOSE);
        }
    
        vTaskDelay(50 / portTICK_PERIOD_MS);    /* the task takes place every 50 ms i.e. task sleeps most of the time           */
        lCurrentTime = (unsigned long)(esp_timer_get_time() / 1000ULL); /* get the current kernel time in milliseconds          */
        lElapsedTime =  lCurrentTime - lBaseTime;                  /* processing the elapsed time since the last task execution */
        
        /******** Tx Task Processing Section ************************************************************************************/
        if(lElapsedTime>=APP_SENDING_INTERVAL_MS){      /* if it's time to process sending task...                              */
             lBaseTime = lCurrentTime;                   /* updating the current time (for the next task execution)              */
            if(LoRaBeginPacket(FALSE)==0){              /* if LoRa module is enabled to process a new Tx packect...             */

                if(mIsBitsSet(app.m_uStatus, ST_LORA_MODULE_LINK_ADDRESS)) {
                    for(int k=2; k<APP_LORA_PROBE_NB; k++){
                        if(address[k] == 0){                             /* saves address if not saved before                    */
                            address[k] = k;                              /* gives last numbers of address                        */
                            app.m_uProbeAddress = k;
                            sprintf(buf, "%d", k);
                            printf("New address (dec): %d\n", k);
                            k = APP_LORA_PROBE_NB+1;                     /* if address saved, then no need to continue           */  
                        }
                    }
                    printf("Address sent : 0x%02x\n", app.m_uProbeAddress);
                    _AppLoraSendData(APP_LORA_REMOTE_ADDRESS, buf);
                    mBitsClr(app.m_uStatus, ST_LORA_MODULE_LINK_ADDRESS);
                }
                else if(mIsBitsSet(app.m_uStatus, ST_VALVE_OPEN)){
                    msg = "20";
                    sprintf(buf, "%s", msg);                     /* building the message string to send back to probe            */
                    mBitsClr(app.m_uStatus, ST_VALVE_OPEN);
                }
                else if(mIsBitsSet(app.m_uStatus, ST_VALVE_CLOSE)){
                    msg = "15";
                    sprintf(buf, "%s", msg);                     /* building the message string to send back to probe            */
                    mBitsClr(app.m_uStatus, ST_VALVE_CLOSE);
                }

                _AppLoraSendData(app.m_uProbeAddress, buf);  /* write the module destination address to LoRa Tx FIFO        */

            }   /*                                                                                                              */
        }   /*                                                                                                                  */
        /************************************************************************************************************************/

        /******* LoRa Tx done event processing **********************************************************************************/
        if(mIsBitsSet(app.m_uStatus, ST_LORA_MODULE_TX_DONE_TRIGGERED)){    /* if LoRa Tx done event has occurred...            */
            mBitsClr(app.m_uStatus, ST_LORA_MODULE_TX_DONE_TRIGGERED);      /* acknowledging this event...                      */
            _AppLoRaSetRxMode();                                            /* and set the App state to Rx state and set the    */
        }                                                                   /* the LoRa module in Rx state too.                 */
        /************************************************************************************************************************/

        /******* LoRa Rx done event processing **********************************************************************************/
        if(mIsBitsSet(app.m_uStatus, ST_LORA_MODULE_RX_DONE_TRIGGERED)){    /* if LoRa Rx done event has occurred...            */
            uint8_t u8DstAddr = LoRaRead();                                             /* get the module destination address   */
            if(u8DstAddr!=app.m_u8HostAddress && u8DstAddr!=APP_LORA_BCAST_ADDRESS){    /* checking destination address...      */
                ESP_LOGI(TAG, "TTGO has received a data frame not for this station!");  /* address not matching host one        */
            }   /*                                                                                                              */
            else{                                           /* address matches local host station or it's the broadcast address */
                ESP_LOGI(TAG, "Received LoRa data: RSSI:%d\tSNR:%f", LoRaPacketRssi(), LoRaPacketSnr()); /* displaying some Rx stats*/
                uint8_t u8SrcAddr = LoRaRead();             /* retrieves the source host station address                        */
                uint8_t u8SzData  = LoRaRead();             /* retrieves the data length                                        */
                char data[u8SzData+1];                      /* allocates a character array for data storage                     */
                for(int k=0; k<u8SzData; ++k){              /* loop for data retrieving...                                      */
                    data[k] = LoRaRead();                   /* peeks byte of data from LoRa Rx FIFO                             */
                }   /*                                                                                                          */
                data[u8SzData]='\0';                        /* placing the null character string terminator                     */
                ESP_LOGI(TAG, "dstAddr: 0x%02X srcAddr: 0x%02X Raw message content: \"%s\"", u8DstAddr, u8SrcAddr, data);   /*  */

                switch (atoi(data)) {
                    case 250:
                        mBitsSet(app.m_uStatus, ST_LORA_MODULE_LINK_ADDRESS);
                        break;
                    default:
                        PostLoop(data);
                        break;
                }
            }   /*                                                                                                              */
            mBitsClr(app.m_uStatus, ST_LORA_MODULE_RX_DONE_TRIGGERED);  /* acknowledging the Rx done event                      */
        }/*                                                                                                                     */
        /************************************************************************************************************************/

    } /* []end of the perpetual loop    */
}

/************************************************************************************************
 * @brief Initializes the application App entity
 */
void AppInit(void){
    ESP_LOGI(TAG, "AppInit()");
    esp_err_t err;
    err = gpio_install_isr_service(0);
    ESP_ERROR_CHECK(err);

    /******* Flash led initializing ***************************/
    gpio_reset_pin(APP_FLASH_LED_PIN);
    gpio_set_direction(APP_FLASH_LED_PIN, GPIO_MODE_OUTPUT);


    /******* LoRa module parametrization ************************/
    LoRaBegin(APP_LORA_CARRIER_FREQUENCY_HZ);           /*      */
    LoRaSetSpreadingFactor(APP_LORA_SPREADING_FACTOR);  /*      */
    LoRaSetSignalBandwidth(APP_LORA_BANDWIDTH_FREQUENCY);   /*  */
    LoRaSetCodingRate4(APP_LORA_CODING_RATE);           /*      */
    LoRaOnReceive(_AppLoRaRxCallback);                  /*      */
    LoRaOnTxDone(_AppLoRaTxCallback);                   /*      */
    LoRaEnableCrc();                                    /*      */
    /************************************************************/

    /******* Initializes Wi-Fi **********************************/
    const char* ssid = APP_WIFI_SSID;
    const char* password = APP_WIFI_PASSWORD;
    
    Serial.begin(115200);
    delay(1000);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("\nConnecting to WiFi network");
    int timeout_counter = 0;
    
    while(WiFi.status() != WL_CONNECTED){
        Serial.print(".");
        delay(200);
        timeout_counter++;
        if(timeout_counter >= APP_WIFI_CONNECTION_TIMEOUT*5){
            ESP.restart();  /* restarting if connection time too long */
        }
    }

    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP()); /* getting IP */
    Serial.print("ESP32 MAC address: ");
    Serial.println(WiFi.macAddress());
    /************************************************************/
}

/************************************************************************************************************
 * @brief The main application task, contains the main perpetual task loop
 */
void AppRun(void){
    ESP_LOGI(TAG, "AppRun()");

    LoRaDumpRegisters();    /* displays the content of the entire LoRa Semtech SX127x register bytes array  */
    LoRaDumpFifo();         /* displays the content of the entire LoRa Semtech SX127x fifo data bytes array */

    xTaskCreate(_AppLoRaTask, NULL, 4096, NULL, 5, NULL);   /* creates the freeRTOS LoRa processing task    */

    for(uint32_t k=-1;;){   /* the main perpetual task loop */

        /***** Doing the flashing led processor activity *****************************/
        static const uint32_t ledSeq[]={40,90,40,3500,0};
        if(ledSeq[++k]==0) k=0;
        gpio_set_level(APP_FLASH_LED_PIN, k&0x01);
        vTaskDelay(ledSeq[k] / portTICK_PERIOD_MS);
    }   /* []end of the perpetual main task loop    */

}
/************************************************************************************************************/