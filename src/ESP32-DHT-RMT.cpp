// https://github.com/joergtiedemann/ESP32-DHT-RMT

#include <Arduino.h>
#include "ESP32-DHT-RMT.h"
#include "sdkconfig.h"

CDHT22::CDHT22() 
{
	m_minpulse = 130;
	m_maxpulse = 180;
	m_duration0pulse = 0;
	m_duration1pulse = 0;
}

void CDHT22::get_nakduration(float &duration0, float &duration1) 
{
	duration0 = m_duration0pulse;
	duration1 = m_duration1pulse;
}

void CDHT22::set_minmaxpulse(int minpulse, int maxpulse) 
{
	m_minpulse =minpulse;
	m_maxpulse = maxpulse;
}


/*
Auszug aus den Rohdaten die empfangen wurden
DHT22: Received 43 symbols
DHT22: Received Symbol:0, Duration0:21 Level0:1, Duration1:79 Level1:0 -> das ist der High Puls nachdem die Leitung vom ESP auf High gezogen wurde und der Lowpuls von 80us der vom DHT22 kommt
DHT22: Received Symbol:1, Duration0:82 Level0:1, Duration1:54 Level1:0 -> das ist der High Puls von 80us der vom DHT22 kommt und der Lowpuls des 1. Bits der Daten
DHT22: Received Symbol:2, Duration0:26 Level0:1, Duration1:54 Level1:0 -> das ist der High Puls des 1. Bits der Datenund der Lowpuls des 2. Bits der Daten vom DHT22 usw. 
DHT22: Received Symbol:3, Duration0:26 Level0:1, Duration1:54 Level1:0
DHT22: Received Symbol:4, Duration0:26 Level0:1, Duration1:54 Level1:0
DHT22: Received Symbol:5, Duration0:26 Level0:1, Duration1:54 Level1:0
DHT22: Received Symbol:6, Duration0:26 Level0:1, Duration1:54 Level1:0
DHT22: Received Symbol:7, Duration0:26 Level0:1, Duration1:54 Level1:0
DHT22: Received Symbol:8, Duration0:74 Level0:1, Duration1:54 Level1:0
DHT22: Received Symbol:9, Duration0:25 Level0:1, Duration1:67 Level1:0
wir koennen nach dem xQueueReceive(... nix in den Serial Port schreiben entweder weil wir in einer ISR sind oder weil wir zu wenig Stack haben
daher die Daten als globale Member Variablen speichern und dann in der Task ausgeben
*/
uint8_t CDHT22::read_dht(float &temperature, float &humidity, uint8_t pin, uint8_t dhttype) {
	gpio_num_t dhtpin = static_cast<gpio_num_t>(pin);
	rmt_channel_handle_t rx_channel = NULL;
	
	rmt_receive_config_t rx_config = {
		.signal_range_min_ns = 3000,
		.signal_range_max_ns = 150000,
	};
	
	rmt_rx_channel_config_t rx_ch_conf = {
		.gpio_num = dhtpin,
		.clk_src = RMT_CLK_SRC_DEFAULT,
		.resolution_hz = 1000000,
		.mem_block_symbols = MAX_BLOCKS,
	};
	uint8_t error = DHT_OK;
	if(rmt_new_rx_channel(&rx_ch_conf, &rx_channel) != ESP_OK) {
		return DHT_DRIVER;
	}
	QueueHandle_t rx_queue = xQueueCreate(1, sizeof(rx_data));
	assert(rx_queue);
	rmt_rx_event_callbacks_t cbs = {
		.on_recv_done = dhtrx_done,
	};

	rmt_rx_register_event_callbacks(rx_channel, &cbs, rx_queue);

	gpio_set_level(dhtpin, 1);
	gpio_pullup_dis(dhtpin);
	gpio_pulldown_dis(dhtpin);
	gpio_set_direction(dhtpin, GPIO_MODE_INPUT_OUTPUT_OD);
	gpio_set_intr_type(dhtpin, GPIO_INTR_DISABLE);
	// 3. Leitung auf Low ziehen (Start-Impuls beginnen)
	gpio_set_level(dhtpin, 0);

	// 4. RMT scharfschalten, WÄHREND die Leitung noch Low ist! -> RMT Receive startet, sobald die Leitung wieder High geht
	if (rmt_enable(rx_channel) != ESP_OK || rmt_receive(rx_channel, symbols, sizeof(symbols), &rx_config) != ESP_OK) {
		return DHT_DRIVER;
	}
	// 5. Restliche Wartezeit für das Startsignal abwarten  und Leitung freigeben
	vTaskDelay((dhttype == DHT22 ? 2 : 22) / portTICK_PERIOD_MS);
	gpio_set_level(dhtpin, 1);
	
	if (xQueueReceive(rx_queue, &rx_data, pdMS_TO_TICKS(100)) == pdPASS) {
		size_t len = rx_data.num_symbols;
		uint32_t code = 0;
		rmt_symbol_word_t *cur = rx_data.received_symbols;

		// Handshake-Puls (ACK): 80us Low (cur[0].duration1) + 80us High (cur[1].duration0)
		uint16_t pulse = cur[0].duration1 + cur[1].duration0;

		m_duration0pulse = cur[0].duration1;
		m_duration1pulse = cur[1].duration0;

		if (len == 1) {
			error = DHT_TIMEOUT1;
			m_errorlen = len;
		} else if (len < 42) {
			error = DHT_UNDERFLOW;
			m_errorlen = len;
		} else if (len > 43) {
			error = DHT_OVERFLOW;
			m_errorlen = len;
		} else if (pulse < m_minpulse || pulse > m_maxpulse) {
			m_errorduration0pulse = cur[0].duration1;
			m_errorduration1pulse = cur[1].duration0;
			error = DHT_NACK;
		} else {
			uint8_t data[6] = {0}; // WICHTIG: Array mit 0 initialisieren!
			for (uint8_t i = 0; i < 40; i++) {
				// Jedes Bit besteht aus Low-Phase (cur[i+1].duration1) + High-Phase (cur[i+2].duration0)
				pulse = cur[i + 1].duration1 + cur[i + 2].duration0;
				
				if (pulse > 55 && pulse < 145) {
					data[i / 8] <<= 1;
					if (pulse > 110) {
						data[i / 8] |= 1;
					}
				} else {
					error = DHT_BAD_DATA;
					m_errordataduration0pulse = cur[i + 1].duration1;
					m_errordataduration1pulse = cur[i + 2].duration0;
					break; // Bei Fehler Schleife direkt abbrechen, damit Fehlerwerte nicht überschrieben werden
				}
			}	
			// Hier folgt deine Checksummen- und Temperatur-/Feuchtigkeitsberechnung mit data[0..4]
			if(!error){
				uint8_t total = data[0] + data[1] + data[2] + data[3];
				if(data[4] == total){
					if(dhttype == DHT22){
						humidity = (data[0] * 256 + data[1]) * 0.1;
						temperature = ((data[2] & 0x7f) * 256 + data[3]) * 0.1;
						if(data[2] & 0x80){temperature = -temperature;}
					}else{
						humidity = data[0];
						temperature = data[2];
					}
				}else{
					error = DHT_CHECKSUM;
				}
			}
		}
	} else {
		error = DHT_TIMEOUT;
	}

	gpio_set_level(dhtpin, 1);
	vQueueDelete(rx_queue);
	rmt_disable(rx_channel);
	rmt_del_channel(rx_channel);
	return error;
}


String CDHT22::errorToString(uint8_t error) {
  switch (error) {
    case DHT_OK:        return "OK";
    case DHT_TOO_SOON:  return "Too soon";
    case DHT_DRIVER:    return "RMT driver error";
    case DHT_TIMEOUT:   return "Sensor timeout";
    case DHT_TIMEOUT1:  return "Sensor timeout, 1 Bit received";
    case DHT_NACK:      return "Invalid ACK! Pulseduration0:" + String(m_errorduration0pulse) + "us, Pulseduration1:" + String(m_errorduration1pulse)+"us";
    case DHT_BAD_DATA:  return "Bad data pulse! Pulseduration0:" + String(m_errordataduration0pulse) + "us, Pulseduration1:" + String(m_errordataduration1pulse)+"us";
    case DHT_CHECKSUM:  return "Checksum error";
    case DHT_UNDERFLOW: return "Too few bits! Received bits: " + String(m_errorlen);
    case DHT_OVERFLOW:  return "Too many bits! Received bits: " + String(m_errorlen);
    default:            return "Unknown error";
  }
}
