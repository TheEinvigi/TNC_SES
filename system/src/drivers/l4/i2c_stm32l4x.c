#include "drivers/i2c.h"

#include "main.h"

#include "station_config_target_hw.h"

#include <stm32l4xx.h>
#include <stm32l4xx_ll_gpio.h>
#include <stm32l4xx_ll_i2c.h>

volatile uint16_t i2c_remote_addr = 0;
volatile uint8_t i2c_tx_data[32] = {'\0'};
volatile uint8_t i2c_rx_data[32] = {'\0'};
volatile uint8_t i2c_rxing = 0;
volatile uint8_t i2c_txing = 0;
volatile uint8_t i2c_done = 0;
volatile uint8_t i2c_tx_queue_len = 0;
volatile uint8_t i2c_trx_data_counter = 0;
volatile uint8_t i2c_rx_bytes_number = 0;
volatile uint8_t i2c_error_counter = 0;

volatile i2c_state_t i2c_state;

volatile uint32_t i2cStartTime;

#define MAX_I2C_ERRORS_PER_COMM 5
#define I2C_TIMEOUT 100

void i2cConfigure() {			// funkcja konfiguruje pierwszy kontroler i2c!!!
	LL_I2C_InitTypeDef I2C_InitStructure;
	LL_GPIO_InitTypeDef GPIO_InitTypeDef;

	// disabling I2C in case it was enabled before
	LL_I2C_Disable(I2C1);

	NVIC_EnableIRQ( I2C1_EV_IRQn );
	NVIC_EnableIRQ( I2C1_ER_IRQn );

	GPIO_InitTypeDef.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitTypeDef.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
	GPIO_InitTypeDef.Pin = LL_GPIO_PIN_6;
	GPIO_InitTypeDef.Pull = LL_GPIO_PULL_NO;
	GPIO_InitTypeDef.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitTypeDef.Alternate = LL_GPIO_AF_4;
	LL_GPIO_Init(GPIOB, &GPIO_InitTypeDef);		// SCL

	GPIO_InitTypeDef.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitTypeDef.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
	GPIO_InitTypeDef.Pin = LL_GPIO_PIN_7;
	GPIO_InitTypeDef.Pull = LL_GPIO_PULL_NO;
	GPIO_InitTypeDef.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitTypeDef.Alternate = LL_GPIO_AF_4;
	LL_GPIO_Init(GPIOB, &GPIO_InitTypeDef);		// SDA

//    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
//    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
//    GPIO_InitStruct.Pull = GPIO_PULLUP;
//    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
//    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	//I2C_StructInit(&I2C_InitStructure);

	LL_I2C_StructInit(&I2C_InitStructure);

	/* Konfiguracja I2C */
	I2C_InitStructure.PeripheralMode = LL_I2C_MODE_I2C;
	I2C_InitStructure.Timing = 0x50418AFE;//0x0050D9FF;
	I2C_InitStructure.AnalogFilter = LL_I2C_ANALOGFILTER_DISABLE;
	I2C_InitStructure.DigitalFilter = 0x0;
	I2C_InitStructure.OwnAddress1 = 0x0;
	I2C_InitStructure.TypeAcknowledge = LL_I2C_ACK;
	I2C_InitStructure.OwnAddrSize = LL_I2C_ADDRSLAVE_7BIT;

	/* Potwierdzamy konfigurację przed włączeniem I2C */
	LL_I2C_Init(I2C1, &I2C_InitStructure);

	/* Włączenie I2C */
	LL_I2C_Enable(I2C1);

	LL_I2C_EnableIT_TX(I2C1);
	LL_I2C_EnableIT_RX(I2C1);
	LL_I2C_EnableIT_NACK(I2C1);
	LL_I2C_EnableIT_ERR(I2C1);

	i2c_state = I2C_IDLE;

}

int i2cReinit() {
//	I2C_InitTypeDef I2C_InitStructure;
//
//	I2C1->CR1 |= I2C_CR1_SWRST;
//	I2C1->CR1 &= (0xFFFFFFFF ^ I2C_CR1_SWRST);
//
//	I2C_StructInit(&I2C_InitStructure);
//
//	/* Konfiguracja I2C */
//	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
//	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
//	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
//	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
//	I2C_InitStructure.I2C_ClockSpeed = 50000;
//
//	/* Potwierdzamy konfigurację przed włączeniem I2C */
//	I2C_Init(I2C1, &I2C_InitStructure);
//
//	I2C1->CR2 |= I2C_CR2_ITEVTEN;		// w��czenie generowanie przerwania od zdarzen i2c
//	I2C1->CR2 |= I2C_CR2_ITBUFEN;
//	I2C1->CR2 |= I2C_CR2_ITERREN;


	return 0;
}

int i2c_send_data(int addr, uint8_t* data, int null) {
	int i;

	i2cVariableReset();

	for (i = 0; (i<32 && *(data+i) != '\0'); i++)
		i2c_tx_data[i]=data[i];
	if (null == 0x01) {					// je�eli do slave trzeba wys�a� 0x00
		i2c_tx_data[0] = 0x00;
		i = 1;
	}
	i2c_tx_queue_len = i;
	i2c_remote_addr = addr;

	i2c_txing = 1;
	i2c_error_counter = 0;

	i2cStartTime = master_time;


	// enable periphal and turn on all interrupts
	i2cStart();

	// set addressing mode
	LL_I2C_SetMasterAddressingMode(I2C1, LL_I2C_ADDRESSING_MODE_7BIT);

	// set slave address to be sent
	LL_I2C_SetSlaveAddr(I2C1, addr & 0xFE);

	// verify that address has been set correctly
	if (LL_I2C_GetSlaveAddr(I2C1) != (addr & 0xFE)) {
		return -1;
	}

	// set transfer direction
	LL_I2C_SetTransferRequest(I2C1, LL_I2C_REQUEST_WRITE);

	// disable reload mode to enable auto-stop after last byte
	LL_I2C_DisableReloadMode(I2C1);

	// enable auto end
	LL_I2C_EnableAutoEndMode(I2C1);

	// set transfer size
	LL_I2C_SetTransferSize(I2C1, i2c_tx_queue_len);

	i2c_state = I2C_TXING;

	I2C1->CR2 |= I2C_CR2_START;			// zadanie warunkow startowych
	return 0;
}

int i2c_receive_data(int addr, int num) {

	i2cVariableReset();

	i2c_rx_bytes_number = num;
	i2c_remote_addr = addr;
	i2c_trx_data_counter = 0;
	i2c_rxing = 1;

	// enable periphal and turn on all interrupts
	i2cStart();

	// set addressing mode
	LL_I2C_SetMasterAddressingMode(I2C1, LL_I2C_ADDRESSING_MODE_7BIT);

	// set slave address to be sent
	LL_I2C_SetSlaveAddr(I2C1, addr);

	// set transfer direction
	LL_I2C_SetTransferRequest(I2C1, LL_I2C_REQUEST_READ);

	// disable reload mode to enable auto-stop after last byte
	LL_I2C_DisableReloadMode(I2C1);

	// enable auto end
	LL_I2C_EnableAutoEndMode(I2C1);

	// set transfer size
	LL_I2C_SetTransferSize(I2C1, num);

	i2cStartTime = master_time;

	i2c_state = I2C_RXING;

	I2C1->CR2 |= I2C_CR2_START;		// zadanie warunkow startowych
	return 0;
}



void i2cVariableReset(void) {
	//I2C1->DR = 0x00;
	i2c_trx_data_counter = 0;
	i2c_tx_queue_len = 0;
	i2c_rx_bytes_number = 0;

	i2c_rxing = 0;
	i2c_txing = 0;
	i2c_tx_queue_len = 0;
	i2c_trx_data_counter = 0;
	i2c_rx_bytes_number = 0;
}

void i2cIrqHandler(void) {

		if ((I2C1->ISR & I2C_ISR_STOPF) == I2C_ISR_STOPF && i2c_txing == 1 && i2c_trx_data_counter >= i2c_tx_queue_len) {
			i2cStop();

			i2c_state = I2C_IDLE;
		}
		else if ((I2C1->ISR & I2C_ISR_STOPF) == I2C_ISR_STOPF) {
			i2cStop();

			i2c_state = I2C_ERROR;
		}
//		if ((I2C1->SR1 & I2C_SR1_ADDR) == I2C_SR1_ADDR && (i2c_txing == 1 || i2c_rxing == 1)) {
//		// After transmitting the slave address      EV6
//			I2C1->SR1 &= (0xFFFFFFFF ^ I2C_SR1_ADDR);
//			I2C1->SR2 &= (0xFFFFFFFF ^ I2C_SR2_TRA);
//
//			if (i2c_rx_bytes_number == 1 && i2c_rxing == 1) {
//				/// EV_6_1
//				// If i2c is on receive mode an only single byte must be received clear the ACK flag
//				// not to set ACK on bus after receiving the byte.
//				I2C1->CR1 &= (0xFFFFFFFF ^ I2C_CR1_ACK);
//				I2C1->CR1 |= I2C_CR1_STOP;
//				}
//
//			if (i2c_rxing == 1) // TODO: this is a bug??
//				I2C1->CR1 |= I2C_CR1_ACK;
//		}
		if ((I2C1->ISR & I2C_ISR_TXIS) == I2C_ISR_TXIS && i2c_txing == 1) {
		// If I2C is in transmission mode and the data buffer is busy
		// put the next in the data register EV_8_1
			I2C1->TXDR = i2c_tx_data[i2c_trx_data_counter];
			i2c_trx_data_counter++;
		}
		if ((I2C1->ISR & I2C_ISR_TC) == I2C_ISR_TC && i2c_txing == 1) {
			// If all data have been transmitted to the slave the stop conditions should be
			// transmitte over the i2c bus
			i2c_txing = 0;

			// stop the i2c
			i2cStop();
		}
//		if ((I2C1->SR1 & I2C_SR1_BTF) == I2C_SR1_BTF && i2c_txing == 1) {
//		// EV_8
//			if ((I2C1->SR1 & I2C_SR1_TXE) == I2C_SR1_TXE && i2c_txing == 1 && i2c_trx_data_counter < i2c_tx_queue_len) {
//			I2C1->DR = i2c_tx_data[i2c_trx_data_counter];
//			i2c_trx_data_counter++;
//			I2C1->SR1 &= (0xFFFFFFFF ^ I2C_SR1_BTF);
//			}
//		}
		if ((I2C1->ISR & I2C_ISR_RXNE) == I2C_ISR_RXNE && i2c_rxing == 1) {
		// EV_7
			*(i2c_rx_data + i2c_trx_data_counter) = I2C1->RXDR & 0xFF;
			i2c_trx_data_counter++;
//			if (i2c_rx_bytes_number-i2c_trx_data_counter == 1) {
//				I2C1->CR1 &= (0xFFFFFFFF ^ I2C_CR1_ACK);	//jezeli odebrano przedostatni bajt to trzeba
//															// zgasic bit ACK zeby nastepnie wyslano NACK na koniec
//			}
			if (i2c_rx_bytes_number-i2c_trx_data_counter == 0) {
				while ((I2C1->ISR & I2C_ISR_STOPF) == 0);		// wait for STOP conditions to be generated automativally
				i2c_rxing = 0;
				//I2C_Cmd(I2C1, DISABLE);
				*(i2c_rx_data + i2c_trx_data_counter) = '\0';
				i2cStop();

			}

		}

}

void i2cErrIrqHandler(void) {


	if (((I2C1->ISR & I2C_ISR_NACKF) == I2C_ISR_NACKF) && i2c_trx_data_counter == 0 &&  i2c_state == I2C_RXING) {
//		// slave nie odpowiedzia� ack na sw�j adres
//		I2C1->SR1 &= (0xFFFFFFFF ^ I2C_SR1_AF);
//		I2C1->CR1 |= I2C_CR1_STOP;		// zadawanie warunkow STOP i przerywanie komunikacji
//		while ((I2C1->CR1 & I2C_CR1_STOP) == I2C_CR1_STOP);
//		i2c_error_counter++;				// zwieksza wartosc licznika b��d�w transmisji
//		I2C1->CR1 |= I2C_CR1_START;		// ponawianie komunikacji

		i2c_error_counter++;

		// stop the i2c to reset hardware
		i2cStop();

		// enable periphal and turn on all interrupts
		i2cStart();

		// set addressing mode
		LL_I2C_SetMasterAddressingMode(I2C1, LL_I2C_ADDRESSING_MODE_7BIT);

		// set slave address to be sent
		LL_I2C_SetSlaveAddr(I2C1, i2c_remote_addr);

		// set transfer direction
		LL_I2C_SetTransferRequest(I2C1, LL_I2C_REQUEST_READ);

		// disable reload mode to enable auto-stop after last byte
		LL_I2C_DisableReloadMode(I2C1);

		// enable auto end
		LL_I2C_EnableAutoEndMode(I2C1);

		// set transfer size
		LL_I2C_SetTransferSize(I2C1, i2c_rx_bytes_number);

		I2C1->CR2 |= I2C_CR2_START;

		i2cStartTime = master_time;

		i2c_state = I2C_RXING;

	}

	if (((I2C1->ISR & I2C_ISR_NACKF) == I2C_ISR_NACKF) && i2c_trx_data_counter == 0 &&  i2c_state == I2C_TXING) {
		i2c_error_counter++;

		// stop the i2c to reset hardware
		i2cStop();

		// enable periphal and turn on all interrupts
		i2cStart();

		// set addressing mode
		LL_I2C_SetMasterAddressingMode(I2C1, LL_I2C_ADDRESSING_MODE_7BIT);

		// set slave address to be sent
		LL_I2C_SetSlaveAddr(I2C1, i2c_remote_addr);

		// set transfer direction
		LL_I2C_SetTransferRequest(I2C1, LL_I2C_REQUEST_WRITE);

		// disable reload mode to enable auto-stop after last byte
		LL_I2C_DisableReloadMode(I2C1);

		// enable auto end
		LL_I2C_EnableAutoEndMode(I2C1);

		// set transfer size
		LL_I2C_SetTransferSize(I2C1, i2c_tx_queue_len);

		I2C1->CR2 |= I2C_CR2_START;			// zadanie warunkow startowych
	}

	if (((I2C1->ISR & I2C_ISR_NACKF) == I2C_ISR_NACKF) && i2c_trx_data_counter != 0 ) {
		//jezeli slave nie odpowiedzia� ack na wys�any do niego bajt danych
		I2C1->ICR |= I2C_ICR_NACKCF;
		i2c_error_counter++;
		i2c_trx_data_counter--;	// zmniejszanie warto�ci licznika danych aby nadac jeszcze raz to samo

	}

	if (((I2C1->ISR & I2C_ISR_ARLO) == I2C_ISR_ARLO) ) {

		I2C1->ICR |= I2C_ICR_ARLOCF;

		i2c_error_counter = MAX_I2C_ERRORS_PER_COMM + 1;

	}


	if (((I2C1->ISR & I2C_ISR_TIMEOUT) == I2C_ISR_TIMEOUT) ) {

		I2C1->ICR |= I2C_ICR_TIMOUTCF;

		i2c_error_counter = MAX_I2C_ERRORS_PER_COMM + 1;

	}

	if (((I2C1->ISR & I2C_ISR_OVR) == I2C_ISR_OVR) ) {

		I2C1->ICR |= I2C_ICR_OVRCF;

		i2c_error_counter = MAX_I2C_ERRORS_PER_COMM + 1;

	}

	if (((I2C1->ISR & I2C_ISR_BERR) == I2C_ISR_BERR) ) {

		I2C1->ICR |= I2C_ICR_BERRCF;

		i2c_error_counter = MAX_I2C_ERRORS_PER_COMM + 1;

	}

	// if this seems to be some unknow or unhalted error
	i2c_error_counter++;

	if (i2c_error_counter > MAX_I2C_ERRORS_PER_COMM) {
		i2cReinit();
		i2cStop();

		i2c_state = I2C_ERROR;
	}

}

void i2cStop(void) {

	i2c_state = I2C_IDLE;

	LL_I2C_DisableIT_TX(I2C1);
	LL_I2C_DisableIT_RX(I2C1);
	LL_I2C_DisableIT_NACK(I2C1);
	LL_I2C_DisableIT_ERR(I2C1);
	LL_I2C_DisableIT_STOP(I2C1);

	LL_I2C_Disable(I2C1);


}

void i2cStart(void) {

	LL_I2C_Enable(I2C1);

	LL_I2C_EnableIT_TX(I2C1);
	LL_I2C_EnableIT_RX(I2C1);
	LL_I2C_EnableIT_NACK(I2C1);
	LL_I2C_EnableIT_ERR(I2C1);
//	LL_I2C_EnableIT_TXE(I2C1);
//	LL_I2C_EnableIT_TXIS(I2C1);
	LL_I2C_EnableIT_STOP(I2C1);

}

void i2cKeepTimeout(void) {
	if (i2c_state == I2C_RXING || i2c_state == I2C_TXING) {
		if (master_time - i2cStartTime > I2C_TIMEOUT) {
			i2cReinit();
			i2cStop();
			i2c_state = I2C_ERROR;
		}
	}
}
