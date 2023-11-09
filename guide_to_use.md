# GL_THREAD_DEV_BOARD_DEMO

- Reference https://docs.gl-inet.com/iot/en/iot_dev_board/ to add TDB(GL Thread DEV Board) to the thread network. After successfully joining the network, you can view the collected data such as temperature reported by the OTB to gl-s200 on the web page, or run commands in the background of gl-s200 to control the OTB.



## Functional description

### GL_NRF52840_DEV_BOARD

#### Button

| Button  | Short Press                  | Long Press                                |
| :------ | :--------------------------- | ----------------------------------------- |
| Button1 | Update its status to S200    | /                                         |
| Button2 | Start joining Thread of S200 | Long press for 3 seconds to factory reset |



#### LED

| LED            | description                                                  |
| -------------- | ------------------------------------------------------------ |
| LED1(WhiteLED) | Blink once when the device reports status or sends a event request. |
| LED2(GreenLED) | Blink indicates that the device has started joining Thread. And LED on indicates that device has joined Thread successfully. |



### DAUGHTER BOARD

#### Infrared sensor

- Infrared sensor is set as button3, when it detects someone nearby, it will be triggered and send a infrared sensor trigger event request to S200.  The action after trigger depends on "Automations" you set, such as turn on the RGB LED on the daughter board.

  ![image-20231109105732970](Instruction_gl_dev_board_demo.assets/image-20231109105732970.png)



#### Knob

- Press knob will send a qdec button trigger event request to S200. The action after trigger depends on "Automations" you set, such as turn on/off the RGB LED on the daughter board.
- Rotate knob will send a qdec rotate trigger event request to S200. The action after trigger depends on "Automations" you set, such as change the color of RGB LED on the daughter board.