# sldr

DC soldering iron controller for JBC C105, C115 and C210 handles

Accepting fake JCB handles from Ali/Ebay, [example](https://www.ebay.co.uk/itm/166835161395?var=466499866336).

Up to 2A of current.

This board requires two supplies: for stm32 microcontroller (DC 5V) and the iron (variable up to DC 7.5 V / 2A).

JBC C105 cartridge resistance 3.6 Ohm.

JBC C210 cartridge resistance 2.5 Ohm.

Disclaimer: This is DIY contoller board. Use it on your own risk. I'm not responsible for any damage caused buy this product or service.

### Set up compiler
    $ sudo apt install gcc-arm-none-eabi

### Get sources and build the project
    $ git clone --recursive https://github.com/mdepx/sldr
    $ cd sldr
    $ make clean all

Use [STLINK-V3MINIE](https://www.st.com/en/development-tools/stlink-v3minie.html) to program this board.

![c115](https://raw.githubusercontent.com/mdepx/sldr/main/images/iron_c115.jpg)

![alt text](https://raw.githubusercontent.com/mdepx/sldr/main/images/sldr.jpg)
