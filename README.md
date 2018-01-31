# Radioino
Arduino (nano) sketch for FM radio chip (RDA5807M) and 1602 LCD using I2C

# Details
This is a full function FM radio implementation of a RDA5807M chip that uses a LCD display to show the current station information.
It's using a Arduino Nano for controling via I2C.

# Depencies:
aka the giant shoulders we stand on

https://github.com/mathertel/Radio

https://github.com/mathertel/OneButton

https://github.com/marcoschwartz/LiquidCrystal_I2C

# Wiring:

t.b.d.

# Userinterface:
```
+----------------+---------------------+-----------------+---------------------+----------------------------+
| Button \ Event |       click()       | double click()  |    long press()     |          comment           |
+----------------+---------------------+-----------------+---------------------+----------------------------+
| VolDwn         | -1 volume           | toggle mute     | each 500ms -1       | if vol eq 0 then mute      |
| VolUp          | +1 volume           | toggle loudness | each 500ms +1       | if muted then first unmute |
| R3We           | tune 87.8 MHz       | toggle presets  | scan to next sender |                            |
| Disp           | toggle display mode |                 |                     | > freq > radio > audio >   |
+----------------+---------------------+-----------------+---------------------+----------------------------+
```
LCD display:
```
+----------------------------------------------------+
|  Programmname  |  (T)uned (S)tereo (M)ute (B)oost  |
|  > RDS Text > FREQ > RDS Time > Audio > Signal >   |
+----------------------------------------------------+
```

# License
BSD 3-Clause License.

Copyright (c) 2018, WeWeWe - Welle West Wetterau e.V. All rights reserved.

For details see LICENSE.
