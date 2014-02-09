AT89Loader v0.2

This utility programs firmware into AT89LP6440 microcontrollers using an FTDI USB to
serial cable instead of the parallel cable usualy used to program them. It may work on
similar chips like the AT89LP3240 but it has not been tested. Use it at your own risk.

The program was compiled with Visual C++ 2010. It requires the D2XX header and library
files available here: http://www.ftdichip.com/Drivers/D2XX.htm

These are the default pin connections. They can be changed in software.

MCU SS   <----> FTDI Brown
MCU MOSI <----> FTDI Yellow
MCU MISO <----> FTDI Green
MCU SCK  <----> FTDI Orange

Usage: AT89Loader [options] [filename]

Options:
   -?              See the help page
   -e              Erase all of the flash
   -fxxxxxxxxxxxx  Set fuses. 1, 0, or x for don't change
   -l              Display information while working
   -p              Program flash with filename
   -s              Only display errors
   -v			       Verify flash against filename
   filename        Path of the ihx or hex file to load
   
Known bugs: sometimes flash verification fails. Rerun the verification if this happens.
