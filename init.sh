#!/bin/sh

# uncomment if not exists yet. TODO chek automatically
# modprobe libcomposite
# mkdir /sys/kernel/config/usb_gadget/g_multi

cd /sys/kernel/config/usb_gadget/g_multi

# uncommet if not exists yet. TODO check autoamtically
# mkdir configs/c.1
# mkdir strings/0x409
# mkdir configs/c.1/strings/0x409
# echo 0xa4ac > idProduct
# echo 0x0525 > idVendor
# echo "deafbeef1234" > strings/0x409/serialnumber
# echo "tripod.ch" > strings/0x409/manufacturer
# echo "MIDIKeyboard" > strings/0x409/product
# echo "Conf 1" > configs/c.1/strings/0x409/configuration
# echo 120 > configs/c.1/MaxPower

mkdir functions/hid.usb0
echo 1 > functions/hid.usb0/protocol
echo 1 > functions/hid.usb0/subclass
echo 8 > functions/hid.usb0/report_length
# write the keyboard usb page. see https://docs.mbed.com/docs/ble-hid/en/latest/api/md_doc_HID.html#keyboard
echo -ne "\\x05\\x01\\x09\\x06\\xa1\\x01\\x05\\x07\\x\\x19\\xe0\\x29\\xe7\\x15\\x00\\x25\\x01\\x75\\x01\\x95\\x08\\x81\\x02\\x95\\x01\\x\\x75\\x08\\x81\\x03\\x95\\x05\\x75\\x01\\x05\\x08\\x19\\x01\\x29\\x05\\x91\\x02\\x\\x95\\x01\\x75\\x03\\x91\\x03\\x95\\x06\\x75\\x08\\x15\\x00\\x25\\x65\\x05\\x07\\x19\\x00\\x29\\x65\\x81\\x00\\xc0" > functions/hid.usb0/report_desc

# 'install' new device
ln -s functions/hid.usb0 configs/c.1

# enable USB Device Controller. (choose from /sys/class/udc/)
echo musb-hdrc.0 > UDC 