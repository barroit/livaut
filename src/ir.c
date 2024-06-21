/****************************************************************************
**
** Copyright 2024 Jiamu Sun
** Contact: barroit@linux.com
**
** This file is part of livaut.
**
** livaut is free software: you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the
** Free Software Foundation, either version 3 of the License, or (at your
** option) any later version.
**
** livaut is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public License along
** with livaut. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "ir.h"
#include "driver/gpio.h"
#include "helper.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static QueueHandle_t tsop_evt;

static void IRAM_ATTR tsop_isr(void* arg)
{
	xQueueSendFromISR(tsop_evt, arg, NULL);
}

static void tsop_tsk(void* arg)
{
	u8 dmvar;
	while (39) {
		if (xQueueReceive(tsop_evt, &dmvar, portMAX_DELAY))
			putchar(gpio_get_level(TSOP_PIN) + '0');
	}
}

static esp_err_t init_ir_evt(void)
{
	tsop_evt = xQueueCreate(fixed_growth(4), sizeof(u32));
	if (!tsop_evt)
		goto err_que_crt;

	TaskHandle_t tsk = NULL;
	if (!xTaskCreate(tsop_tsk, "tsop_task", 2048, NULL, 10, &tsk))
		goto err_tsk_crt;

	if (ROE_ESP(gpio_install_isr_service(0), "tsop_init"))
		goto err_gpoi_ins_isr;

	return gpio_isr_handler_add(TSOP_PIN, tsop_isr,
					&(int){ 1 } /* dummy var */);

err_gpoi_ins_isr:
	vTaskDelete(tsk);
err_tsk_crt:
	vQueueDelete(tsop_evt);
err_que_crt:
	return -1;
}

esp_err_t init_ir(void)
{
	gpio_config_t conf = {
		.intr_type    = GPIO_INTR_NEGEDGE,
		.mode         = GPIO_MODE_INPUT,
		.pin_bit_mask = PINMASK(TSOP_PIN),
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.pull_up_en   = GPIO_PULLUP_ENABLE,
	};

	gpio_config(&conf);

	return init_ir_evt();
}

void wait_ir_sig(void)
{
	while (39) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
