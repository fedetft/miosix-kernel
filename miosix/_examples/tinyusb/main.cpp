/***************************************************************************
 *   Copyright (C) 2023, 2024 by Daniele Cattaneo                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <cstdio>
#include "miosix.h"
#include "tusb.h"

using namespace std;
using namespace miosix;

namespace usb {

using vbus = Gpio<GPIOA_BASE, 9>;
using id = Gpio<GPIOA_BASE, 10>;
using dm = Gpio<GPIOA_BASE, 11>;
using dp = Gpio<GPIOA_BASE, 12>;

}

void *usbThread(void *unused)
{
    iprintf("USB thread running\n");
    while (!Thread::testTerminate()) tud_task();
    return nullptr;
}

int main()
{
    bool vbusSensing = true;
    {
        FastInterruptDisableLock dLock;
        
        //Turn on USB peripheral
        RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
        RCC_SYNC();
        
        //Configure USB pins. ID is optional (only needed for USB OTG)
        usb::id::alternateFunction(10);
        usb::dm::alternateFunction(10);
        usb::dp::alternateFunction(10);
        usb::id::mode(Mode::ALTERNATE);
        usb::dm::mode(Mode::ALTERNATE);
        usb::dp::mode(Mode::ALTERNATE_OD);
    
        //VBUS sensing allows the USB device to detect connections and
        //disconnections by looking at the 5V coming in from the host. For
        //this feature to work, the USB VCC must be connected to PA9, no
        //other pin works.
        if (vbusSensing)
        {
            usb::vbus::mode(Mode::INPUT);
            USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_NOVBUSSENS;
            USB_OTG_FS->GCCFG |= USB_OTG_GCCFG_VBUSBSEN;
        } else {
            USB_OTG_FS->GCCFG |= USB_OTG_GCCFG_NOVBUSSENS;
            USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_VBUSBSEN;
            USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_VBUSASEN;
        }
    }

    bool r = tusb_init();
    if (!r)
    {
        iprintf("USB initialization error\n");
        return 0;
    }
    Thread::create(usbThread,2048U,Priority(0),nullptr,0);

    for (;;)
    {
        Thread::nanoSleep(1000000);
        if (tud_cdc_n_available(0))
        {
            uint8_t buf[64];
            uint32_t count = tud_cdc_n_read(0, buf, sizeof(buf) - 1);
            for(uint32_t i=0; i<count; i++)
            {
              if(buf[i]!='\r') tud_cdc_n_write_char(0, buf[i]);
              else tud_cdc_n_write_str(0, "\r\n");
            }
            tud_cdc_n_write_flush(0);
        }
    }
}

//
// Interrupt handlers to be forwarded to TinyUSB
//

__attribute__((naked)) void OTG_FS_IRQHandler(void)
{
    saveContext();
    tud_int_handler(0);
    restoreContext();
}

__attribute__((naked)) void OTG_HS_IRQHandler(void)
{
    saveContext();
    tud_int_handler(1);
    restoreContext();
}

//
// TinyUSB callbacks
//

extern "C" void tud_mount_cb(void)
{
    iprintf("USB mounted\n");
}

extern "C" void tud_umount_cb(void)
{
    iprintf("USB unmounted\n");
}

extern "C" void tud_suspend_cb(bool remote_wakeup_en)
{
    iprintf("USB suspended\n");
}

extern "C" void tud_resume_cb(void)
{
    iprintf("USB resumed\n");
}

enum USBStringDescID {
    LocaleIDs = 0,
    None = 0,
    Manufacturer,
    Product,
    Serial
};

// Called when the device descriptor is requested from the host.
uint8_t const *tud_descriptor_device_cb(void)
{
    static const uint16_t vid = 0xcafe;
    // Some OSes (Windows...) remember device configuration based on vid/pid,
    // so if the set of supported interfaces changes, the pid should also
    // change.
    static const uint16_t pid = 0x4001;
    static const uint16_t version = 0x0100;
    static const tusb_desc_device_t desc_device = {
        .bLength = sizeof(tusb_desc_device_t),
        .bDescriptorType = TUSB_DESC_DEVICE,
        .bcdUSB = 0x0200, // Descriptor version number

        // Use Interface Association Descriptor (IAD) for CDC
        //   As required by USB Specs IAD's subclass must be common class (2)
        // and protocol must be IAD (1)
        .bDeviceClass = TUSB_CLASS_MISC,
        .bDeviceSubClass = MISC_SUBCLASS_COMMON,
        .bDeviceProtocol = MISC_PROTOCOL_IAD,

        .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

        .idVendor = vid,
        .idProduct = pid,
        .bcdDevice = version, // Device version number

        .iManufacturer = USBStringDescID::Manufacturer,
        .iProduct = USBStringDescID::Product,
        .iSerialNumber = USBStringDescID::Serial,

        .bNumConfigurations = 0x01
    };
    return reinterpret_cast<uint8_t const *>(&desc_device);
}

// Called when the configuration descriptor is requested from the host.
//   TinyUSB will automatically configure and instantiate the appropriate USB
// drivers depending on the descriptor, therefore this is the main source of
// truth for things like endpoint IDs and so on.
//   Descriptor contents must exist long enough for transfer to complete.
uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    enum USBInterfaceID {
        CDC = 0,
        CDCData, // Implicitly added by the TUD_CDC_DESCRIPTOR macro
        Total
    };
    enum USBEndpointID {
        CDCOut = 0x02,
        CDCNotif = 0x81,
        CDCIn = 0x82
    };
    static const size_t length = TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN;
    static uint8_t const desc_fs_configuration[length] = {
        // Configuration descriptor
        TUD_CONFIG_DESCRIPTOR(
            1,                          // config number
            USBInterfaceID::Total,      // interface count
            USBStringDescID::None,      // string index
            length,                     // total length
            0x00,                       // attribute
            100                         // power in mA
        ),
        // Interface descriptor (macro sets class/subclass automatically)
        TUD_CDC_DESCRIPTOR(
            USBInterfaceID::CDC,        // Interface number
            USBStringDescID::None,      // string index
            USBEndpointID::CDCNotif,    // notification endpoint address
            8,                          // notification endpoint size
            USBEndpointID::CDCOut,      // data out endpoint address
            USBEndpointID::CDCIn,       // data in endpoint address
            64                          // data endpoint size
        ),
    };
    return desc_fs_configuration;
}

struct __attribute__((packed)) STM32DeviceIDFields
{
    uint16_t waferX;
    uint16_t waferY;
    uint8_t wafer;
    char lot[7];
};
#define STM32_DEVICE_ID (reinterpret_cast<STM32DeviceIDFields*>(UID_BASE))

template <size_t L>
struct USBStringDesc
{
    uint8_t size;
    uint8_t type = TUSB_DESC_STRING;
    char16_t str[L];
};

// Invoked when host requests each string descriptor.
// Application returns a pointer to descriptor, whose contents must exist long
// enough for transfer to complete.
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    static const size_t descMaxStrLen=31;
    static USBStringDesc<descMaxStrLen> desc;
    uint8_t length;

    if(index==USBStringDescID::LocaleIDs) // Locale ID
    {
        desc.str[0]=0x0409; // US English
        length=1;
    } else {
        const char *str;
        const int chipid_length = 7+2+4+4;
        char chipid_buf[chipid_length+1];

        if(index == USBStringDescID::Manufacturer) str = "Miosix TinyUSB";
        else if(index == USBStringDescID::Product) str = "USB CDC Example";
        else if(index == USBStringDescID::Serial) { // Serial
            for(int i=0; i<7; i++) chipid_buf[i] = STM32_DEVICE_ID->lot[i];
            siprintf(chipid_buf+7, "%02X%04X%04X",
                STM32_DEVICE_ID->wafer,
                STM32_DEVICE_ID->waferX,
                STM32_DEVICE_ID->waferY);
            str = chipid_buf;
        } else {
            // Do not return anything for other indexes (for example 0xEE which
            // is Windows-proprietary)
            return NULL;
        }
        // copy string into descriptor buffer, lazily converting it to UTF-16
        length = (uint8_t)strlen(str);
        if(length > descMaxStrLen) length = descMaxStrLen;
        for(int i=0; i<length; i++)
            desc.str[i] = str[i];
    }
    desc.size = 2 * length + 2;
    return reinterpret_cast<uint16_t *>(&desc);
}
