/*
 *    ||          ____  _ __
 * +------+      / __ )(_) /_______________ _____  ___
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2011-2012 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * exptestRR.c - Testing of expansion port.
 */
#define DEBUG_MODULE "ET"

#include <stdint.h>

#include "stm32fxxx.h"
#include "config.h"
#include "debug.h"
#include "deck.h"
#include "deck_test.h"

#include "sensors.h"

// Hardware configuration
#define ET_GPIO_PERIF (RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC)

#define ET_GPIO_PORT_TX2 GPIOA
#define ET_GPIO_PIN_TX2 GPIO_Pin_2
#define ET_GPIO_PORT_RX2 GPIOA
#define ET_GPIO_PIN_RX2 GPIO_Pin_3

#define ET_GPIO_PORT_SDA GPIOB
#define ET_GPIO_PIN_SDA GPIO_Pin_7
#define ET_GPIO_PORT_SCL GPIOB
#define ET_GPIO_PIN_SCL GPIO_Pin_6

#define ET_GPIO_PORT_IO2 GPIOB
#define ET_GPIO_PIN_IO2 GPIO_Pin_5
#define ET_GPIO_PORT_IO3 GPIOB
#define ET_GPIO_PIN_IO3 GPIO_Pin_4
#define ET_GPIO_PORT_IO4 GPIOC
#define ET_GPIO_PIN_IO4 GPIO_Pin_12

#define ET_NBR_PINS 5
#define ET_IO4_PIN (ET_NBR_PINS - 1)

typedef struct _etGpio {
    GPIO_TypeDef* port;
    uint16_t pin;
    char name[6];
} EtGpio;

static EtGpio etRRGpioIn[ET_NBR_PINS] = {{ET_GPIO_PORT_TX2, ET_GPIO_PIN_TX2, "TX2"},
                                         {ET_GPIO_PORT_RX2, ET_GPIO_PIN_RX2, "RX2"},
                                         {ET_GPIO_PORT_IO2, ET_GPIO_PIN_IO2, "IO2"},
                                         {ET_GPIO_PORT_IO3, ET_GPIO_PIN_IO3, "IO3"},
                                         {ET_GPIO_PORT_IO4, ET_GPIO_PIN_IO4, "IO4"}};

static EtGpio etRRGpioSDA = {ET_GPIO_PORT_SDA, ET_GPIO_PIN_SDA, "SDA"};
static EtGpio etRRGpioSCL = {ET_GPIO_PORT_SCL, ET_GPIO_PIN_SCL, "SCL"};

static bool isInit;

static bool exptestRRTestAllPins(bool test);
static bool exptestRRTestPin(EtGpio* etPin, bool test);

static bool exptestRRRun(void) {
    int i;
    volatile int delay;
    bool status = true;
    GPIO_InitTypeDef GPIO_InitStructure;
    GpioRegBuf gpioSaved;

    isInit = true;

    status &= sensorsManufacturingTest();

    // Enable GPIOs
    RCC_AHB1PeriphClockCmd(ET_GPIO_PERIF, ENABLE);

    decktestSaveGPIOStatesABC(&gpioSaved);

    for (i = 0; i < ET_NBR_PINS; i++) {
        // Initialize the pins as inputs
        GPIO_InitStructure.GPIO_Pin = etRRGpioIn[i].pin;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
        GPIO_Init(etRRGpioIn[i].port, &GPIO_InitStructure);
    }

    for (i = 0; i < ET_NBR_PINS && status; i++) {
        // Configure pin as output to poke others
        GPIO_InitStructure.GPIO_Pin = etRRGpioIn[i].pin;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
        GPIO_Init(etRRGpioIn[i].port, &GPIO_InitStructure);

        // Test high
        GPIO_SetBits(etRRGpioIn[i].port, etRRGpioIn[i].pin);
        for (delay = 0; delay < 1000; delay++)
            ;
        if (!exptestRRTestAllPins(1)) {
            status = false;
        }

        // Test low
        GPIO_ResetBits(etRRGpioIn[i].port, etRRGpioIn[i].pin);
        for (delay = 0; delay < 1000; delay++)
            ;
        if (!exptestRRTestAllPins(0)) {
            status = false;
        }

        // Restore
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
        GPIO_Init(etRRGpioIn[i].port, &GPIO_InitStructure);
    }

    decktestRestoreGPIOStatesABC(&gpioSaved);

    if (status) {
        // Configure SDA & SCL to turn on OK leds
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;
        GPIO_InitStructure.GPIO_Pin = etRRGpioSDA.pin;
        GPIO_Init(etRRGpioSDA.port, &GPIO_InitStructure);
        GPIO_InitStructure.GPIO_Pin = etRRGpioSCL.pin;
        GPIO_Init(etRRGpioSCL.port, &GPIO_InitStructure);
        // Turn on OK LEDs.
        GPIO_ResetBits(etRRGpioSDA.port, etRRGpioSDA.pin);
        GPIO_ResetBits(etRRGpioSCL.port, etRRGpioSCL.pin);
    }

    return status;
}

static bool exptestRRTestAllPins(bool test) {
    int i;
    bool status = true;

    for (i = 0; i < ET_NBR_PINS; i++) {
        if (!exptestRRTestPin(&etRRGpioIn[i], test)) {
            status = false;
        }
    }

    return status;
}

static bool exptestRRTestPin(EtGpio* etPin, bool test) {
    if (test == GPIO_ReadInputDataBit(etPin->port, etPin->pin)) {
        return true;
    } else {
        DEBUG_PRINT("Pin:%s != %d [FAIL]\n", etPin->name, test);
        return false;
    }
}

static const DeckDriver exptestRR_deck = {
    .vid = 0xBC,
    .pid = 0xFE,
    .name = "bcExpTestRR",

    .usedGpio = 0, // FIXME: Edit the used GPIOs

    .test = exptestRRRun,
};

DECK_DRIVER(exptestRR_deck);
