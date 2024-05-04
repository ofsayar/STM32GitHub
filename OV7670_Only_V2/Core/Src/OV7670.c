/*
 * OV7670.c
 *
 *  Created on: Mar 24, 2024
 *      Author: Yavuz
 */

#include "OV7670.h"


static DCMI_HandleTypeDef *sp_hdcmi;
static DMA_HandleTypeDef  *sp_hdma_dcmi;
static I2C_HandleTypeDef  *sp_hi2c;

static uint8_t capture_mode = 0;		// By default CONTINOUS Capture Mode
static uint32_t img_address = 0;
static uint16_t img_width	= 320;		// Image Width is 320 pixel
static uint16_t img_height	= 240;		// Image Height is 240 pixel
static uint8_t img_format	= RGB565;	// By default image format is RGB565


extern volatile uint16_t lineCounter[30];
extern volatile uint16_t frameCounter;
extern volatile uint16_t vCounter[30];
extern volatile uint16_t fpsCounter[10];

/*************************************************************************************************/
								/*** High-Level Function Defines ***/
/*************************************************************************************************/

/*
 * @brief Initializes the Camera Module
 * @retval returns number of errors during initialization
 * */
uint8_t OV7670_Init(DCMI_HandleTypeDef *p_hdcmi, DMA_HandleTypeDef *p_hdma_dcmi, I2C_HandleTypeDef *p_hi2c)
{
	sp_hdcmi 	 = p_hdcmi;
	sp_hdma_dcmi = p_hdma_dcmi;
	sp_hi2c		 = p_hi2c;

	HAL_StatusTypeDef status;
	uint8_t errNum = 0;
	uint8_t buffer[4]; //re
	// Software Reset
	status = OV7670_Write(0x12, 0x80);
	errNum += ( status != HAL_OK );
	status = OV7670_Read(0x12, buffer); //re
	errNum += ( status != HAL_OK );		//r
	HAL_Delay(30);

	// Read device ID after reset
	//uint8_t buffer[4];
	status = OV7670_Read(0x0b, buffer);
	errNum += ( status != HAL_OK );

	if(buffer[0] != 0x73)
		return 255;

	return errNum;
}


void OV7670_Config(const uint8_t params[][2])
{
	for(int i = 0; params[i][0] != 0xFF; i++)
	{
		OV7670_Write(params[i][0], params[i][1]);
		HAL_Delay(1);
	}
}

/*
 * @brief Starts the camera capture
 * @param mode Capture_mode enum that is either Continous or Snapshot
 * @param capture_address is the lcd data address
 * @retval None
 * */
void OV7670_Start(Capture_mode mode, uint32_t *capture_address)
{
	capture_mode=mode;
	img_address=(uint32_t)capture_address;

	if(capture_mode == SNAPSHOT)
		HAL_DCMI_Start_DMA(sp_hdcmi, DCMI_MODE_SNAPSHOT, img_address, img_width * img_height/2);
	else
		HAL_DCMI_Start_DMA(sp_hdcmi, DCMI_MODE_CONTINUOUS, img_address, img_width * img_height/2);
}


void OV7670_Stop()
{
	HAL_DCMI_Stop(sp_hdcmi);
}




/*************************************************************************************************/
								/*** Setter Function Defines ***/
/*************************************************************************************************/

/*
 * @brief Set Frame Rate.
 * @param div the number to divide XCLK input by. Use XCLK_DIV(n) macro
 * @param mul the number to multiply with XCLK input.
 * Notes: 	(XCLK / div ) * mul
 * @retval None
 * */
void OV7670_SetFrameRate(uint8_t div, PLL_mul mul)
{
	OV7670_Write(REG_CLKRC, 0x80 | div);
	HAL_Delay(1);
	OV7670_Write(REG_DBLV, 0x0A | mul);
	HAL_Delay(1);
}


void OV7670_SetColorFormat(Camera_format format)
{
	uint8_t temp[2];

	OV7670_Read(REG_COM7, &temp[0]);
	temp[0]&=0b11111010;
	OV7670_Read(REG_COM15, &temp[1]);
	temp[1]&=0b00001111;
	HAL_Delay(10);

	switch(format)
	{
	//According to OV7670/7171 implementation guide v1.0 - Table 2-1
		case YUV422:
			OV7670_Write(REG_COM7, temp[0] | 0x00);
			OV7670_Write(REG_COM15, temp[1] | 0x00);
			img_format=YUV422;
			break;
		case RGB565: //Poor (greenish) image - ???
			OV7670_Write(REG_COM7, temp[0] | 0x04);//RGB
			OV7670_Write(REG_COM15, temp[1] | 0x10);//RGB565
			img_format=RGB565;
			break;
	}
}


void OV7670_SetFrameControl(uint16_t hstart, uint16_t hstop, uint16_t vstart, uint16_t vstop)
{
	OV7670_Write(REG_HSTART, (hstart >> 3) & 0xff);
	OV7670_Write(REG_HSTOP, (hstop >> 3) & 0xff);
	OV7670_Write(REG_HREF, ((hstop & 0x7) << 3) | (hstart & 0x7));

	OV7670_Write(REG_VSTART, (vstart >> 2) & 0xff);
	OV7670_Write(REG_VSTOP, (vstop >> 2) & 0xff);
	OV7670_Write(REG_VREF,((vstop & 0x3) << 2) | (vstart & 0x3));
}

void OV7670_SetResolution(Camera_resolution resolution)
{

	switch(resolution)
	{
		case QVGA:	//OK
			OV7670_Config(RES_QVGA);
			OV7670_SetFrameControl(168,24,12,492);
			img_width=320;
			img_height=240;
			break;
		case QQVGA:	//OK
			OV7670_Config(RES_QQVGA);
			OV7670_SetFrameControl(174,30,12,492);
			img_width=160;
			img_height=120;
			break;
		case QQQVGA:	//OK
			OV7670_Config(RES_QQQVGA);
			OV7670_SetFrameControl(196,52,12,492);//(196+640)%784=52
			img_width=80;
			img_height=60;
			break;
		case CIF:	//OK
			OV7670_Config(RES_CIF);
			OV7670_SetFrameControl(174,94,12,489); //for vstop=492 image moves out
			img_width=352;
			img_height=288;
			break;
		case QCIF:	//OK
			OV7670_Config(RES_QCIF);
			OV7670_SetFrameControl(454,22,12,492); //for hstart=454, htop=24 incorect last vertical line
			img_width=176;
			img_height=144;
			break;
		case QQCIF: //OK
			OV7670_Config(RES_QQCIF);
			OV7670_SetFrameControl(474,42,12,492); //for hstart=454, htop=24 incorrect first line, incorrect colors
			img_width=88;
			img_height=72;
			break;
	}
}


void OV7670_UpdateSettings(Camera_settings OV7670)
{
	OV7670_Config(defaults);
	HAL_Delay(10);

	OV7670_SetResolution(OV7670.resolution);

	if(OV7670.format)
		OV7670_SetColorFormat(RGB565);
	else
		OV7670_SetColorFormat(YUV422);

}






/*************************************************************************************************/
								/*** Low-Level Function Defines ***/
/*************************************************************************************************/

HAL_StatusTypeDef OV7670_Write(uint8_t regAddr, uint8_t data)
{
  HAL_StatusTypeDef ret;
  do {
	  ret = HAL_I2C_Mem_Write(sp_hi2c, OV7670_SLAVE_ADDR, regAddr, I2C_MEMADD_SIZE_8BIT, &data, 1, HAL_MAX_DELAY);

  } while (ret != HAL_OK && 0);

  return ret;
}


HAL_StatusTypeDef OV7670_Read(uint8_t regAddr, uint8_t *data)
{
  HAL_StatusTypeDef ret;
  do {
	  ret = HAL_I2C_Master_Transmit(sp_hi2c, OV7670_SLAVE_ADDR, &regAddr, 1, 100);
	  ret |= HAL_I2C_Master_Receive(sp_hi2c, OV7670_SLAVE_ADDR, data, 1, 100);
  } while (ret != HAL_OK && 0);

  return ret;
}





/*************************************************************************************************/
								/*** Callback Function Defines ***/
/*************************************************************************************************/


void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)
{
	static int i = 0;

	if(i < 30)
		fpsCounter[i] = HAL_GetTick();

	frameCounter++;

	i++;

//	if(capture_mode==CONTINUOUS)
//		HAL_DMA_Start_IT(hdcmi->DMA_Handle, (uint32_t)&hdcmi->Instance->DR, img_address, img_width * img_height/2);
}


void HAL_DCMI_LineEventCallback(DCMI_HandleTypeDef *hdcmi)
{

	if(frameCounter < 30)
		lineCounter[frameCounter] = HAL_GetTick();

}

void HAL_DCMI_VsyncEventCallback(DCMI_HandleTypeDef *hdcmi)
{

	if(frameCounter < 30)
		vCounter[frameCounter] = HAL_GetTick();
}










