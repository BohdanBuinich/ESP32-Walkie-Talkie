#include <iostream>

#include "driver/i2s.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <cstring>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include "nvs_flash.h"

#include "music.h"


namespace ens
{
    size_t BytesWritten;
    static const int i2s_num = I2S_NUM_0; // i2s port number
    static uint16_t volume_off = 0x255;
    int a = 0;

    const static uint8_t my_mac[6] = {0x10,0x52,0x1c,0x5d,0xf8,0xac};
    const static uint8_t other_mac[6] = {0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
    

    static const i2s_config_t i2s_config = {
        .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, 
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = static_cast<i2s_comm_format_t>(I2S_COMM_FORMAT_STAND_I2S),
        .intr_alloc_flags = 0, 
        .dma_buf_count = 2,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear= false, 
        .fixed_mclk=0
    };

    static const i2s_pin_config_t pin_config = {
        .bck_io_num = 27,                                
        .ws_io_num = 26,                                  
        .data_out_num = 25,                              
        .data_in_num = I2S_PIN_NO_CHANGE                 
    };

    static void wifi_init()
    {
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());

    }

    void onDataRecieve(const unsigned char* mac, const uint8_t* income_message, int len)
    {
        uint8_t *msg = new uint8_t[len];

        memcpy(msg, income_message, len);
        for(int i = 0; i <len; i+= 2)
        {   
            //ens::volume_off = (msg[i] << 8) | (msg[i+1] & 0xff);
            ens::volume_off = msg[i];
            i2s_write(static_cast<i2s_port_t>(ens::i2s_num),&volume_off,1,&BytesWritten,portMAX_DELAY);
        }
        delete msg;
    }

    struct WavHeader_Struct
    {
      //   RIFF Section    
      char RIFFSectionID[4];      // Letters "RIFF"
      uint32_t Size;              // Size of entire file less 8
      char RiffFormat[4];         // Letters "WAVE"
      
      //   Format Section    
      char FormatSectionID[4];    // letters "fmt"
      uint32_t FormatSize;        // Size of format section less 8
      uint16_t FormatID;          // 1=uncompressed PCM
      uint16_t NumChannels;       // 1=mono,2=stereo
      uint32_t SampleRate;        // 44100, 16000, 8000 etc.
      uint32_t ByteRate;          // =SampleRate * Channels * (BitsPerSample/8)
      uint16_t BlockAlign;        // =Channels * (BitsPerSample/8)
      uint16_t BitsPerSample;     // 8,16,24 or 32
    
      // Data Section
      char DataSectionID[4];      // The letters "data"
      uint32_t DataSize;          // Size of the data that follows
    }WavHeader;

    void out(const WavHeader_Struct& wh)
    {
        for(int i = 0; i<4;i++)
            std::cout<<wh.RIFFSectionID[i];
        
        std::cout<<std::endl;

        for(int i = 0; i<4;i++)
            std::cout<<wh.RiffFormat[i];

        std::cout<<std::endl;

        for(int i = 0; i<4;i++)
            std::cout<<wh.FormatSectionID[i];

        std::cout<<std::endl;
        std::cout<<wh.FormatSize<<std::endl;
        std::cout<<wh.FormatID<<std::endl;
        std::cout<<wh.NumChannels<<std::endl;
        std::cout<<wh.SampleRate<<std::endl;
        std::cout<<wh.ByteRate<<std::endl;
        std::cout<<wh.BlockAlign<<std::endl;
        std::cout<<wh.BitsPerSample<<std::endl;

        for(int i = 0; i<4;i++)
            std::cout<<wh.DataSectionID[i];

        std::cout<<std::endl;

        std::cout<<wh.DataSize<<std::endl;
    }

}// namespace ens(esp_now setup)


extern "C" void app_main(void)
{

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(i2s_driver_install(static_cast<i2s_port_t>(ens::i2s_num), &ens::i2s_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(static_cast<i2s_port_t>(ens::i2s_num), &ens::pin_config));

    memcpy(&ens::WavHeader, &melody, 44);
    ens::out(ens::WavHeader);
    const int header_offset = 44;
    static uint8_t ucParameterToPass;
    TaskHandle_t xHandle = NULL;

    uint32_t DataIdx=0; 
    

    while(true) 
    {   
        size_t BytesWritten;                        
        
        i2s_write(static_cast<i2s_port_t>(ens::i2s_num),(melody+(DataIdx+header_offset)),4,&BytesWritten,portMAX_DELAY); 
        DataIdx+=4;               
        if(DataIdx>=ens::WavHeader.DataSize)               
        DataIdx=0;  
    } 
}
