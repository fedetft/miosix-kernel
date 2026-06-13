/***************************************************************************
 *   HR_C7000 (CK803S) GPIO interface for modern Miosix.                    *
 *   GPL v2+ with the Miosix linking exception.                            *
 *                                                                          *
 *   Three DesignWare DW_apb_gpio banks (per hd2_regs.h / project_hd2_*):   *
 *     PA = 0x14020000, PB = 0x14100000, PC = 0x14110000                    *
 *   Per-bank registers: DR +0x00 (output data), DDR +0x04 (1=output),      *
 *   EXT_PORT +0x50 (input read).                                          *
 *                                                                          *
 *   Provides the two gpio abstractions Miosix expects: the compile-time    *
 *   Gpio<P,N> template (P = bank base address, N = pin) and the runtime    *
 *   GpioPin (port+pin held as data). getPin() bridges template -> runtime. *
 ***************************************************************************/

#pragma once

namespace miosix {

/// GPIO pin configuration. Minimal: the DW_apb_gpio bank only distinguishes
/// input vs output (DDR bit); pulls live in SOCSYS IOMGR, not exposed here yet.
class Mode
{
public:
    enum Mode_
    {
        INPUT,
        OUTPUT
    };
private:
    Mode(); //Just a wrapper class, disallow creating instances
};

/// GPIO bank base addresses, used as the Gpio<> first template parameter.
const unsigned int PA=0x14020000u;
const unsigned int PB=0x14100000u;
const unsigned int PC=0x14110000u;

/// DW_apb_gpio per-bank register offsets.
inline volatile unsigned int& gpioDr (unsigned int base){ return *reinterpret_cast<volatile unsigned int*>(base+0x00u); }
inline volatile unsigned int& gpioDdr(unsigned int base){ return *reinterpret_cast<volatile unsigned int*>(base+0x04u); }
inline volatile unsigned int& gpioIn (unsigned int base){ return *reinterpret_cast<volatile unsigned int*>(base+0x50u); }

/**
 * Runtime-configurable GPIO pin (port base + pin number held as data).
 */
class GpioPin
{
public:
    /// Default: an invalid pin (getNumber()>=32). Only isValid() is safe.
    GpioPin() : base(PA), n(0xff) {}

    /// \param p bank base (PA/PB/PC), \param num pin number (0..31)
    GpioPin(unsigned int p, unsigned char num) : base(p), n(num) {}

    bool isValid() const { return n<32; }

    void mode(Mode::Mode_ m)
    {
        if(m==Mode::OUTPUT) gpioDdr(base)|=(1u<<n); else gpioDdr(base)&=~(1u<<n);
    }
    void high()  { gpioDr(base)|=(1u<<n); }
    void low()   { gpioDr(base)&=~(1u<<n); }
    int  value() { return (gpioIn(base)>>n)&1; }

    unsigned int  getPort()   const { return base; }
    unsigned char getNumber() const { return n; }

private:
    unsigned int base;   //bank base address
    unsigned char n;     //pin number within the bank
};

/**
 * Compile-time GPIO pin. P = bank base address, N = pin number.
 */
template<unsigned int P, unsigned char N>
class Gpio
{
public:
    static void mode(Mode::Mode_ m)
    {
        if(m==Mode::OUTPUT) gpioDdr(P)|=(1u<<N); else gpioDdr(P)&=~(1u<<N);
    }
    static void high()  { gpioDr(P)|=(1u<<N); }
    static void low()   { gpioDr(P)&=~(1u<<N); }
    static int  value() { return (gpioIn(P)>>N)&1; }

    /// \return this Gpio as a runtime GpioPin
    static GpioPin getPin() { return GpioPin(P,N); }

    unsigned int  getPort()   const { return P; }
    unsigned char getNumber() const { return N; }
private:
    Gpio(); //Only static member functions, disallow creating instances
};

} //namespace miosix
