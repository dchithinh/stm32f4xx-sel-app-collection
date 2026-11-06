#include "audio_cs43l22.h"
#include "main.h"
#include "hw_def.h"
#include "stm32f4xx_hal.h"

#define CS43L22_I2C_ADDR        0x94U
#define CS43L22_RESET_PORT      GPIOD
#define CS43L22_RESET_PIN       GPIO_PIN_4
#define CS43L22_I2C_SCL_PORT    GPIOB
#define CS43L22_I2C_SCL_PIN     GPIO_PIN_6
#define CS43L22_I2C_SDA_PORT    GPIOB
#define CS43L22_I2C_SDA_PIN     GPIO_PIN_9

#define CS43L22_REG_ID          0x01U
#define CS43L22_REG_POWER_CTL1  0x02U
#define CS43L22_REG_POWER_CTL2  0x04U
#define CS43L22_REG_CLOCKING    0x05U
#define CS43L22_REG_INTERFACE   0x06U
#define CS43L22_REG_PASSTHR_A   0x08U
#define CS43L22_REG_PASSTHR_B   0x09U
#define CS43L22_REG_ANALOG_ZC   0x0AU
#define CS43L22_REG_MISC        0x0EU
#define CS43L22_REG_PLAYBACK    0x0FU
#define CS43L22_REG_PCMA        0x1AU
#define CS43L22_REG_PCMB        0x1BU
#define CS43L22_REG_TONE        0x1FU
#define CS43L22_REG_MASTER_A    0x20U
#define CS43L22_REG_MASTER_B    0x21U
#define CS43L22_REG_HP_A        0x22U
#define CS43L22_REG_HP_B        0x23U

I2S_HandleTypeDef hi2s3;
DMA_HandleTypeDef hdma_i2s3_tx;
static I2C_HandleTypeDef hi2c1;
static volatile uint8_t s_pcm_half;
static bool s_i2s_running;

extern SPI_HandleTypeDef lcd_spi_handle;

static void lcd_wait_idle(void)
{
    uint32_t t;

    if (lcd_spi_handle.Instance != SPI2) {
        return;
    }
    t = HAL_GetTick();
    while (HAL_SPI_GetState(&lcd_spi_handle) != HAL_SPI_STATE_READY) {
        if ((HAL_GetTick() - t) > 50U) {
            break;
        }
    }
}

static void pb9_as_i2c_sda(void)
{
    GPIO_InitTypeDef g = {0};

    lcd_wait_idle();
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);

    g.Pin = CS43L22_I2C_SDA_PIN;
    g.Mode = GPIO_MODE_AF_OD;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(CS43L22_I2C_SDA_PORT, &g);
}

static void pb9_as_lcd_cs(void)
{
    GPIO_InitTypeDef g = {0};

    g.Pin = LCD_CS_PIN;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_CS_PORT, &g);
    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

static HAL_StatusTypeDef codec_write(uint8_t reg, uint8_t val)
{
    HAL_StatusTypeDef st;
    uint8_t buf[2] = {reg, val};

    pb9_as_i2c_sda();
    st = HAL_I2C_Master_Transmit(&hi2c1, CS43L22_I2C_ADDR, buf, 2, 50);
    pb9_as_lcd_cs();
    return st;
}

static void i2s_pll_config(uint32_t audio_hz)
{
    RCC_PeriphCLKInitTypeDef p = {0};

    p.PeriphClockSelection = RCC_PERIPHCLK_I2S;
    if (audio_hz == 44100U || audio_hz == 22050U || audio_hz == 11025U) {
        p.PLLI2S.PLLI2SN = 271;
        p.PLLI2S.PLLI2SR = 2;
    } else {
        p.PLLI2S.PLLI2SN = 192;
        p.PLLI2S.PLLI2SR = 2;
    }
    (void)HAL_RCCEx_PeriphCLKConfig(&p);
}

static uint32_t i2s_freq_const(uint32_t hz)
{
    switch (hz) {
    case 8000:  return I2S_AUDIOFREQ_8K;
    case 11025: return I2S_AUDIOFREQ_11K;
    case 16000: return I2S_AUDIOFREQ_16K;
    case 22050: return I2S_AUDIOFREQ_22K;
    case 32000: return I2S_AUDIOFREQ_32K;
    case 44100: return I2S_AUDIOFREQ_44K;
    default:    return I2S_AUDIOFREQ_48K;
    }
}

int audio_codec_init(void)
{
    GPIO_InitTypeDef g = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    HAL_GPIO_WritePin(CS43L22_RESET_PORT, CS43L22_RESET_PIN, GPIO_PIN_RESET);
    g.Pin = CS43L22_RESET_PIN;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(CS43L22_RESET_PORT, &g);
    HAL_Delay(5);
    HAL_GPIO_WritePin(CS43L22_RESET_PORT, CS43L22_RESET_PIN, GPIO_PIN_SET);
    HAL_Delay(5);

    g.Pin = CS43L22_I2C_SCL_PIN;
    g.Mode = GPIO_MODE_AF_OD;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(CS43L22_I2C_SCL_PORT, &g);

    pb9_as_i2c_sda();

    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        pb9_as_lcd_cs();
        return -1;
    }

    /* Required CS43L22 bring-up sequence from the datasheet. */
    (void)codec_write(0x00U, 0x99U);
    (void)codec_write(0x47U, 0x80U);
    (void)codec_write(0x32U, 0x80U);
    (void)codec_write(0x32U, 0x00U);
    (void)codec_write(0x00U, 0x00U);

    (void)codec_write(CS43L22_REG_POWER_CTL1, 0x01U);
    (void)codec_write(CS43L22_REG_POWER_CTL2, 0xAFU); /* headphones on, speaker off */
    (void)codec_write(CS43L22_REG_CLOCKING, 0x81U);
    (void)codec_write(CS43L22_REG_INTERFACE, 0x04U);  /* I2S, 16-bit */
    (void)codec_write(CS43L22_REG_ANALOG_ZC, 0x00U);
    (void)codec_write(CS43L22_REG_MISC, 0x04U);
    (void)codec_write(CS43L22_REG_PLAYBACK, 0x00U);
    (void)codec_write(CS43L22_REG_TONE, 0x0FU);
    (void)codec_write(CS43L22_REG_PCMA, 0x0AU);
    (void)codec_write(CS43L22_REG_PCMB, 0x0AU);
    (void)codec_write(CS43L22_REG_HP_A, 0x00U);
    (void)codec_write(CS43L22_REG_HP_B, 0x00U);
    audio_codec_set_volume(70);
    (void)codec_write(CS43L22_REG_POWER_CTL1, 0x9EU);

    return 0;
}

void audio_codec_set_volume(uint8_t percent)
{
    uint8_t converted;
    uint8_t tmp;

    if (percent > 100U) {
        percent = 100U;
    }
    converted = (uint8_t)((percent * 255U) / 100U);
    if (converted > 0xE6U) {
        tmp = (uint8_t)(converted - 0xE7U);
    } else {
        tmp = (uint8_t)(converted + 0x19U);
    }
    (void)codec_write(CS43L22_REG_MASTER_A, tmp);
    (void)codec_write(CS43L22_REG_MASTER_B, tmp);
}

void audio_codec_play(void)
{
    (void)codec_write(CS43L22_REG_MISC, 0x06U);
    (void)codec_write(CS43L22_REG_POWER_CTL1, 0x9EU);
    (void)codec_write(CS43L22_REG_POWER_CTL2, 0xAFU);
}

void audio_codec_pause(void)
{
    audio_codec_mute(true);
}

void audio_codec_mute(bool mute)
{
    (void)codec_write(CS43L22_REG_POWER_CTL2, mute ? 0xFFU : 0xAFU);
}

void audio_codec_stop(void)
{
    audio_codec_pause();
    (void)codec_write(CS43L22_REG_MISC, 0x04U);
}

void HAL_I2S_MspInit(I2S_HandleTypeDef *hi2s)
{
    GPIO_InitTypeDef g = {0};

    if (hi2s->Instance != SPI3) {
        return;
    }

    __HAL_RCC_SPI3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    g.Pin = GPIO_PIN_4;
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOA, &g);

    g.Pin = GPIO_PIN_7 | GPIO_PIN_10 | GPIO_PIN_12;
    HAL_GPIO_Init(GPIOC, &g);

    hdma_i2s3_tx.Instance = DMA1_Stream7;
    hdma_i2s3_tx.Init.Channel = DMA_CHANNEL_0;
    hdma_i2s3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_i2s3_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_i2s3_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_i2s3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_i2s3_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_i2s3_tx.Init.Mode = DMA_CIRCULAR;
    hdma_i2s3_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    hdma_i2s3_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_i2s3_tx) != HAL_OK) {
        Error_Handler();
    }
    __HAL_LINKDMA(hi2s, hdmatx, hdma_i2s3_tx);

    HAL_NVIC_SetPriority(DMA1_Stream7_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream7_IRQn);
}

void HAL_I2S_MspDeInit(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance != SPI3) {
        return;
    }
    HAL_NVIC_DisableIRQ(DMA1_Stream7_IRQn);
    (void)HAL_DMA_DeInit(&hdma_i2s3_tx);
}

int audio_i2s_start(int16_t *pcm, uint32_t n_samples, uint32_t audio_hz)
{
    i2s_pll_config(audio_hz);

    if (hi2s3.Instance == SPI3) {
        (void)HAL_I2S_DeInit(&hi2s3);
    }

    hi2s3.Instance = SPI3;
    hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
    hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
    hi2s3.Init.DataFormat = I2S_DATAFORMAT_16B;
    hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
    hi2s3.Init.AudioFreq = i2s_freq_const(audio_hz);
    hi2s3.Init.CPOL = I2S_CPOL_LOW;
    hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
    hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
    if (HAL_I2S_Init(&hi2s3) != HAL_OK) {
        return -1;
    }

    s_pcm_half = 0;
    s_i2s_running = true;
    if (HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t *)pcm, (uint16_t)n_samples) != HAL_OK) {
        s_i2s_running = false;
        return -1;
    }
    audio_codec_play();
    return 0;
}

void audio_i2s_stop(void)
{
    if (!s_i2s_running) {
        return;
    }
    audio_codec_pause();
    (void)HAL_I2S_DMAStop(&hi2s3);
    (void)HAL_I2S_DeInit(&hi2s3);
    s_i2s_running = false;
    s_pcm_half = 0;
}

bool audio_i2s_is_running(void)
{
    return s_i2s_running;
}

void audio_pcm_mark_half(uint8_t half)
{
    s_pcm_half = half;
}

uint8_t audio_pcm_take_half(void)
{
    uint8_t h = s_pcm_half;
    s_pcm_half = 0;
    return h;
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    (void)hi2s;
    s_pcm_half = 1;
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    (void)hi2s;
    s_pcm_half = 2;
}
