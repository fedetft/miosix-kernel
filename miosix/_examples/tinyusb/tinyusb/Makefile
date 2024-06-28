##
## Makefile for tinyusb Miosix integration
## This makefile builds libtinyusb.a
##

## KPATH and CONFPATH are forwarded by the parent Makefile
MAKEFILE_VERSION := 1.15
include $(KPATH)/Makefile.kcommon

## List of all tinyusb source files (both .c and .cpp)
## These files will end up in libtinyusb.a
SRC := \
    tinyusb/src/host/hub.c \
    tinyusb/src/host/usbh.c \
    tinyusb/src/portable/nordic/nrf5x/dcd_nrf5x.c \
    tinyusb/src/portable/synopsys/dwc2/dcd_dwc2.c \
    tinyusb/src/portable/valentyusb/eptri/dcd_eptri.c \
    tinyusb/src/portable/ehci/ehci.c \
    tinyusb/src/portable/bridgetek/ft9xx/dcd_ft9xx.c \
    tinyusb/src/portable/nxp/lpc_ip3511/dcd_lpc_ip3511.c \
    tinyusb/src/portable/nxp/khci/dcd_khci.c \
    tinyusb/src/portable/nxp/khci/hcd_khci.c \
    tinyusb/src/portable/nxp/lpc17_40/dcd_lpc17_40.c \
    tinyusb/src/portable/nxp/lpc17_40/hcd_lpc17_40.c \
    tinyusb/src/portable/nxp/transdimension/hcd_transdimension.c \
    tinyusb/src/portable/nxp/transdimension/dcd_transdimension.c \
    tinyusb/src/portable/ohci/ohci.c \
    tinyusb/src/portable/wch/ch32v307/dcd_usbhs.c \
    tinyusb/src/portable/template/dcd_template.c \
    tinyusb/src/portable/mentor/musb/hcd_musb.c \
    tinyusb/src/portable/mentor/musb/dcd_musb.c \
    tinyusb/src/portable/st/synopsys/dcd_synopsys.c \
    tinyusb/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c \
    tinyusb/src/portable/dialog/da146xx/dcd_da146xx.c \
    tinyusb/src/portable/chipidea/ci_hs/dcd_ci_hs.c \
    tinyusb/src/portable/chipidea/ci_hs/hcd_ci_hs.c \
    tinyusb/src/portable/sunxi/dcd_sunxi_musb.c \
    tinyusb/src/portable/microchip/samx7x/dcd_samx7x.c \
    tinyusb/src/portable/microchip/samg/dcd_samg.c \
    tinyusb/src/portable/microchip/pic/dcd_pic.c \
    tinyusb/src/portable/microchip/pic32mz/dcd_pic32mz.c \
    tinyusb/src/portable/microchip/samd/dcd_samd.c \
    tinyusb/src/portable/nuvoton/nuc121/dcd_nuc121.c \
    tinyusb/src/portable/nuvoton/nuc120/dcd_nuc120.c \
    tinyusb/src/portable/nuvoton/nuc505/dcd_nuc505.c \
    tinyusb/src/portable/renesas/usba/dcd_usba.c \
    tinyusb/src/portable/renesas/usba/hcd_usba.c \
    tinyusb/src/portable/espressif/esp32sx/dcd_esp32sx.c \
    tinyusb/src/portable/raspberrypi/rp2040/rp2040_usb.c \
    tinyusb/src/portable/raspberrypi/rp2040/dcd_rp2040.c \
    tinyusb/src/portable/raspberrypi/rp2040/hcd_rp2040.c \
    tinyusb/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c \
    tinyusb/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c \
    tinyusb/src/portable/sony/cxd56/dcd_cxd56.c \
    tinyusb/src/portable/mindmotion/mm32/dcd_mm32f327x_otg.c \
    tinyusb/src/portable/ti/msp430x5xx/dcd_msp430x5xx.c \
    tinyusb/src/common/tusb_fifo.c \
    tinyusb/src/class/cdc/cdc_rndis_host.c \
    tinyusb/src/class/cdc/cdc_host.c \
    tinyusb/src/class/cdc/cdc_device.c \
    tinyusb/src/class/video/video_device.c \
    tinyusb/src/class/net/ncm_device.c \
    tinyusb/src/class/net/ecm_rndis_device.c \
    tinyusb/src/class/dfu/dfu_rt_device.c \
    tinyusb/src/class/dfu/dfu_device.c \
    tinyusb/src/class/midi/midi_device.c \
    tinyusb/src/class/usbtmc/usbtmc_device.c \
    tinyusb/src/class/bth/bth_device.c \
    tinyusb/src/class/audio/audio_device.c \
    tinyusb/src/class/msc/msc_host.c \
    tinyusb/src/class/msc/msc_device.c \
    tinyusb/src/class/hid/hid_device.c \
    tinyusb/src/class/hid/hid_host.c \
    tinyusb/src/class/vendor/vendor_host.c \
    tinyusb/src/class/vendor/vendor_device.c \
    tinyusb/src/device/usbd_control.c \
    tinyusb/src/device/usbd.c \
    tinyusb/src/tusb.c

INCLUDE_DIRS := -I. -I./cmsis_stubs -I./tinyusb/src -I$(CONFPATH)/config

all: $(OBJ)
	$(ECHO) "[AR  ] libtinyusb.a"
	$(Q)$(AR) rcs libtinyusb.a $(OBJ)

clean:
	-rm -f $(OBJ) $(OBJ:.o=.d) libtinyusb.a

# We need to compile all .c files with the C++ compiler for integrating TinyUSB
# without becoming crazy
%.o : %.c
	$(ECHO) "[CXX ] $<"
	$(Q)$(CXX) $(CXXFLAGS) $< -o $@

-include $(OBJ:.o=.d)
