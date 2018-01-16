# Radioino
Arduino (nano) sketch for FM radio chip (RDA5807M) and 1602 LCD using I2C

# Details
This is a full function radio implementation of a RDA5807M chip that uses a LCD display to show the current station information.
Its using a Arduino Nano for controling using I2C.

# Depencies:
aka the giant shoulders we stand on

https://github.com/mathertel/Radio

https://github.com/mathertel/OneButton

https://github.com/marcoschwartz/LiquidCrystal_I2C

# Wiring:

t.b.d.

# Userinterface:
```
+----------------+---------------------+-----------------+---------------------+---------------------------+
| Button \ Event |       click()       | double click()  |     long press      |          comment          |
+----------------+---------------------+-----------------+---------------------+---------------------------+
| VolDwn         | -1 volume           | toggle mute     | each 500ms -1       | if vol eq min then mute   |
| VolUp          | +1 volume           | toggle loudness | each 500ms +1       | if muted the first unmute |
| R3We           | tune 87.8 MHz       |                 | scan to next sender |                           |
| Disp           | toggle display mode |                 |                     | > freq > radio > audio >  |
+----------------+---------------------+-----------------+---------------------+---------------------------+
```
LCD display:
```
+---------------------------------------------------+
|  Programmname  |  (T)uned (S)tereo (M)ute (B)oost |
| > RDS Text > FREQ > RDS Time > Audio > Signal >   |
+---------------------------------------------------+
```

# License
will be free ... but is still under construction :|
