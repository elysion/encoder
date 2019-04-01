# Interface module for modular controllers.

The module can be used for converting encoder, pot and button movement
into messages sent over TWI / I2C. You can e.g. combine up to 5 encoders 
(3 of which can have push buttons) and connect them to a single 
microcontroller (ATMega168p) using a single PCB design. This helps to bring down the 
costs of the BOM and of the boards. The design also includes a configurable
LED ring that can be used to visualize the positions of the encoders.  
