# GL Thread Dev Board

Thread Dev Board (TBD) is the end device part of the thread kit developed by GL-iNet. Developers can test thread-related features and develop their own features based on the TBD.

TBD currently implements the following functions:

- Join a thread network as a **Thread Router** / **Thread End Device** / **Thread Sleepy End Device**.
- Real-time collection of current environment information and uploading to S200 gateway via thread.
    - Temperature 
    - Humidity 
    - Air pressure
    - Pyroelectric infrared
    - Rotary encoder status
    - GPIO input
- Receive command and execute operations in real time via thread
    - RGB LED on/off
    - Change RGB LED color
    - GPIO output
- Firmware upgrade
    - DFU by UART
    - DFU by thread

## HW Info

| HW part                             | Info                          |
| ----------------------------------- | ----------------------------- |
| MCU                                 | NRF52840                      |
| Temperature and humidity sensor     | SHTC3                         |
| Light sensor                        | HX3203                        |
| Air pressure and temperature sensor | SPL0601                       |
| Pyroelectric infrared sensor        | XYC-PIR203B-S0                |
| Rotary encoder                      | EC1108                        |
| RGB LED (X2)                        | LC8812B                       |
| SW define button                    | x2                            |
| HW reset button                     | x1                            |
| SW define LED                       | x2                            |
| HW power LED                        | x1                            |
| Power supply                        | Battery base(CR2032) / Type-C |

## Pinout

<img src="./docs/img/gl_thread_dev_board_pinout.jpg" alt="TDB_pinout" style="zoom: 25%;" />

## SW Development Environment

Still under development...

