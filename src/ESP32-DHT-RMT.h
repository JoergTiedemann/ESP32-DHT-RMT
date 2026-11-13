// https://github.com/joergtiedemann/ESP32-DHT-RMT
#pragma once

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/rmt_rx.h"
#include "driver/gpio.h"

#define DHT11	0
#define DHT21	1
#define DHT22	1
#define AM2301	1
#define AM2302	1

#define DHT_OK			0
#define DHT_TOO_SOON	1
#define DHT_DRIVER		2
#define DHT_TIMEOUT		3
#define DHT_NACK		4
#define DHT_BAD_DATA	5
#define DHT_CHECKSUM	6
#define DHT_UNDERFLOW	7
#define DHT_OVERFLOW	8

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
#define MAX_BLOCKS	64
#else
#define MAX_BLOCKS	48
#endif

class CDHT22 {
public:
    CDHT22();
    uint8_t read_dht(float &temperature, float &humidity, uint8_t pin, uint8_t dhttype);
    void get_nakduration(float &duration0, float &duration1); 
    void set_minmaxpulse(int minpulse, int maxpulse); 
    /**
     * Get human-readable error string.
     */
    String errorToString(uint8_t error);

    	
    rmt_symbol_word_t symbols[MAX_BLOCKS];
	rmt_rx_done_event_data_t rx_data;

    private:
    int m_minpulse;
    int m_maxpulse;
    int	m_duration0pulse;
    int m_duration1pulse;
	int m_errorduration0pulse;
	int m_errorduration1pulse;
	int m_errordataduration0pulse;
	int m_errordataduration1pulse;
    size_t m_errorlen;
};


static bool IRAM_ATTR dhtrx_done(rmt_channel_handle_t ch, const rmt_rx_done_event_data_t *edata, void *udata){
	BaseType_t w = pdFALSE;
	QueueHandle_t d = (QueueHandle_t)udata;
	xQueueSendFromISR(d, edata, &w);
	return w == pdTRUE;
}

