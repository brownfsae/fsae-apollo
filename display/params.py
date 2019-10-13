from enum import Enum
from pint import UnitRegistry
from dataTransfer.dataProcessing import units

# Data pin constants
DATA_PIN = 10  # MOSI pin (master out slave in)
CLOCK_PIN = 11  # SCLK pin (serial clock)

# LED display settings
LED_MIN_RPM = 7000 * units.rpm  # smallest rpm to display on the LEDs
SHIFT_RPM = 13000 * units.rpm  # rpm at which to turn on last LED
REDLINE_RPM = 13500 * units.rpm  # rpm where engine redlines

LED_BRIGHTNESS = 20 * units.percent


# LED color hex values
class LEDColor(Enum):
    OFF = 0x000000
    RED = 0xFF0000
    ORANGE = 0xFF7F00
    YELLOW = 0xFFFF00
    GREEN = 0x00FF00
    BLUE = 0x0000FF
    PURPLE = 0x4B0082
    VIOLET = 0x9400D3
