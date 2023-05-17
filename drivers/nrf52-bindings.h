/*******************************************************************************
 Copyright 2022 GL-iNet. https://www.gl-inet.com/

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ******************************************************************************/

#ifndef _GL_DEV_BOARD_LED_WS2812_H_
#define _GL_DEV_BOARD_LED_WS2812_H_

/*
 * At 4 MHz, 1 bit is 250 ns, so 3 bits is 750 ns.
 *
 * That's cutting it a bit close to the edge of the timing parameters,
 * but it seems to work on AdaFruit LED rings.
 */
#define SPI_FREQ    4000000
#define ZERO_FRAME  0x40
#define ONE_FRAME   0x70

#endif
