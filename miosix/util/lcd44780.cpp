
#include <stdio.h>
#include <stdarg.h>
#include "miosix.h"
#include "interfaces/delays.h"
#include "lcd44780.h"

namespace {

/// Fills data RAM with spaces, sets cursor address to 0, resets scrolling,
/// enables increment mode.
constexpr unsigned char cmdClear=0x01;
/// Sets cursor address to 0 and resets scrolling
constexpr unsigned char cmdHome=0x02;
/// Configures display update behaviour at every data byte sent.
/// \param dir   0: decrement data RAM address at every byte sent
///              1: increment data RAM address at every byte sent 
/// \param shift 0: sending data does not affect scrolling
///              1: sending data scrolls the display left/right depending on
///                 the data RAM address increment/decrement mode
constexpr unsigned char cmdEntryMode(unsigned char dir, unsigned char shift)
{
    return 0x04 | (dir<<1) | shift;
}
/// Enables and disables display layers
/// \param disp  0: turns off the display, 1: turns on the display
/// \param cur   Cursor mode (see Cursor enum)
constexpr unsigned char cmdEnable(unsigned char disp, unsigned char cur)
{
    return 0x08 | (disp<<2) | cur;
}
/// Move the cursor or scroll the display by 1 character forwards or backwards
/// \param shift 0: moves the cursor, 1: scrolls the display
/// \param rl    0: left (increment), 1: right (decrement)
constexpr unsigned char cmdShift(unsigned char shift, unsigned char dir)
{
    return 0x10 | (shift<<3) | (dir<<2);
}
/// Configures the controller for the display
/// \param dl    Data length
///              0: 4 bit, 1: 8 bit
/// \param lines Number of display lines
///              0: 1 line, 1: 2 lines
/// \param font  Size of the font (only for 1 line displays, otherwise ignored)
///              0: 5x8m 1: 5x10
constexpr unsigned char cmdFuncSet(unsigned char dl, unsigned char lines,
                                     unsigned char font)
{
    return 0x20 | (dl<<4) | (lines<<3) | (font<<2);
}
/// Sets the current write address in the font data RAM
constexpr unsigned char cmdCgAddrSet(unsigned char addr)
{
    return 0x40 | addr;
}
/// Sets the current write address in the display data RAM
constexpr unsigned char cmdDdAddrSet(unsigned char addr)
{
    return 0x80 | addr;
}

} //anonymous namespace

namespace miosix {

Lcd44780::Lcd44780(miosix::GpioPin rs, miosix::GpioPin e, miosix::GpioPin d4,
             miosix::GpioPin d5, miosix::GpioPin d6, miosix::GpioPin d7,
             int row, int col) : rs(rs), e(e), d4(d4), d5(d5), d6(d6), d7(d7),
             row(row), col(col)
{
    rs.mode(Mode::OUTPUT);
    e.mode(Mode::OUTPUT);
    d4.mode(Mode::OUTPUT);
    d5.mode(Mode::OUTPUT);
    d6.mode(Mode::OUTPUT);
    d7.mode(Mode::OUTPUT);
    e.low();
    Thread::sleep(50); //Powerup delay
    init();
    clear();
}

void Lcd44780::go(int x, int y)
{
    if(x<0 || x>=col || y<0 || y>=row) return;

    // 4x20 is implemented as 2x40.
    if(y>1) x += col;

    comd(cmdDdAddrSet(((y & 1) ? 0x40 : 0) | x)); //Move cursor
}

int Lcd44780::iprintf(const char* fmt, ...)
{
    va_list arg;
    char line[40];
    va_start(arg,fmt);
    int len=vsniprintf(line,sizeof(line),fmt,arg);
    va_end(arg);
    for(int i=0;i<len;i++) data(line[i]);
    return len;
}

int Lcd44780::printf(const char* fmt, ...)
{
    va_list arg;
    char line[40];
    va_start(arg,fmt);
    int len=vsnprintf(line,sizeof(line),fmt,arg);
    va_end(arg);
    for(int i=0;i<len;i++) data(line[i]);
    return len;
}

void Lcd44780::clear()
{
    comd(cmdClear);
    Thread::sleep(2); //Some displays require this delay
}

void Lcd44780::off()
{
    comd(cmdEnable(0,0));
}

void Lcd44780::on()
{
    comd(cmdEnable(1,cursorState));
}

void Lcd44780::cursor(Cursor cursor)
{
    cursorState=cursor & 0x3;
    comd(cmdEnable(1,cursorState));
}

void Lcd44780::setCgram(int charCode, const unsigned char font[8])
{
    comd(cmdCgAddrSet((charCode & 0x7)<<3));
    for(int i=0;i<8;i++) data(font[i]);
    comd(cmdDdAddrSet(0));
}

void Lcd44780::init()
{
    //On reset the HD44780 controller sets the bus interface to 8 bit mode,
    //and we need to switch it to 4 bit mode.
    //  However we cannot be sure that the controller has not been set to 4 bit
    //mode before (for example if the MCU was reset but not the display).
    //In that case, if we send a 8 bit wide command, it will be interpreted as
    //the first half of a 4 bit command, desynchronizing the bus interface.
    //  As a result we must first ensure the controller is in 8 bit mode.
    //There are 3 different cases:
    //(1) The controller is in 8 bit mode already (default at reset):
    //    - The controller receives three 0x3F, 0x3F, 0x3F commands, which all
    //      set the interface mode to 8 bit. Therefore nothing changes.
    //(2) The controller is in 4 bit mode:
    //    - The controller receives a 0x33 command in 4 bit mode, which sets
    //      the interface to 8 bit mode. Then in 8 bit mode it receives a 0x3F
    //      command and stays in 8 bit mode.
    //(3) The controller is in 4 bit mode and the last byte sent was incomplete
    //    (just the first nybble was received):
    //    - The controller receives two commands in 4 bit mode: 0x-3, 0x33.
    //      The upper nybble of the first command is undefined. However the
    //      second command switches the controller to 8 bit mode.
    rs.low();
    half(cmdFuncSet(1,0,0));
    delayUs(50);
    half(cmdFuncSet(1,0,0));
    delayUs(50);
    half(cmdFuncSet(1,0,0));
    delayUs(50);
    // Now we can safely switch the controller to 4 bit mode
    half(cmdFuncSet(0,0,0));
    rs.high();
    delayUs(50);
    if(row==1) comd(cmdFuncSet(0,0,0)); else comd(cmdFuncSet(0,1,0));
    Thread::sleep(5);  //Initialization delay
    comd(cmdEnable(1,cursorState)); //Display ON, cursor OFF, blink OFF
    comd(cmdEntryMode(1,0));  //Auto increment ON, shift OFF
}

void Lcd44780::half(unsigned char byte)
{
    if(byte & (1<<7)) d7.high(); else d7.low(); //Slow but it works
    if(byte & (1<<6)) d6.high(); else d6.low();
    if(byte & (1<<5)) d5.high(); else d5.low();
    if(byte & (1<<4)) d4.high(); else d4.low();
    delayUs(1);
    e.high();
    delayUs(1);
    e.low();
}

void Lcd44780::data(unsigned char byte)
{
    half(byte);
    byte<<=4;
    half(byte);
    delayUs(50);
}

void Lcd44780::comd(unsigned char byte)
{
    rs.low();
    half(byte);
    byte<<=4;
    half(byte);
    delayUs(50);
    rs.high();
}

} //namespace miosix
