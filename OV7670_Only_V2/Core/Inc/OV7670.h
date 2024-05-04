/*
 * OV7670.h
 *
 *  Created on: Mar 24, 2024
 *      Author: Yavuz
 */

#ifndef INC_OV7670_H_
#define INC_OV7670_H_

#include <OV7670_Reg.h>
#include "stm32f4xx_hal.h"


typedef enum {BYPASS_PLL = 0x00, PLL_x4 = 0x40, PLL_x6 = 0x80, PLL_x8 = 0xC0} PLL_mul;
#define XCLK_DIV(x)	(x-1)

typedef enum {CONTINUOUS=0, SNAPSHOT} Capture_mode;
typedef enum {YUV422=0, RGB565} Camera_format;

typedef enum {QVGA=0, 	// 320x240 px	-->
			  QQVGA, 	// 160x120 px	-->
			  QQQVGA, 	// 80x60   px	-->
			  CIF, 		// 352x288 px	-->
			  QCIF,		// 176x144 px	-->
			  QQCIF		// 88x72   px   -->
} Camera_resolution;


typedef struct{
	Camera_resolution resolution; 	// 0-QVGA, 1-QQVGA, 2-CIF, 3-QCIF, 4-QQCIF
	Camera_format format;			// 0-YUV422, 1-RGB565
}Camera_settings;


/*************************************************************************************************/
								/*** Main Function Prototypes ***/
/*************************************************************************************************/

uint8_t  OV7670_Init(DCMI_HandleTypeDef *p_hdcmi, DMA_HandleTypeDef *p_hdma_dcmi, I2C_HandleTypeDef *p_hi2c);

void OV7670_Config(const uint8_t params[][2]);

void OV7670_Start(Capture_mode mode, uint32_t *capture_address);

void OV7670_Stop();




/*************************************************************************************************/
								/*** Setter Function Prototypes ***/
/*************************************************************************************************/
void OV7670_SetFrameRate(uint8_t div, PLL_mul mul);

void OV7670_SetColorFormat(Camera_format format);

void OV7670_SetFrameControl(uint16_t hstart, uint16_t hstop, uint16_t vstart, uint16_t vstop);

void OV7670_SetResolution(Camera_resolution resolution);

void OV7670_UpdateSettings(Camera_settings OV7670);


/*************************************************************************************************/
								/*** Low-Level Function Prototypes ***/
/*************************************************************************************************/
HAL_StatusTypeDef OV7670_Write(uint8_t regAddr, uint8_t data);

HAL_StatusTypeDef OV7670_Read(uint8_t regAddr, uint8_t *data);


#endif /* INC_OV7670_H_ */
