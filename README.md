MIDI to HID Keyboard Project on BeagleBone Black
================================================

There are many projects that turn a computer keyboard into a MIDI device, e.g. to connect to a MIDI synthesizer.
But there are little projects that work the otherway around. i.e. A hardware solution to map MIDI notes to keyboard strokes. There are software tools available, like the [MIDI Translator](https://www.bome.com/products/mtclassic) from Bone. But of course this limits the use for Windows and might not work in all cases.

This project uses a [BeagleBone Black](https://beagleboard.org/black) which comes with a USB Host **and** a USB client.
The USB host is usually used to connect Keyboard/Mouse to the board, and the USB client is used to connect it to a PC for communication and mass storage device.

Composite Gadget in BBB
------------------------
The mass storage device and the USB ethernet is created using  [GadgetFS](http://www.linux-usb.org/gadget/) using the
[ConfigFS Composite Gadget](https://wiki.tizen.org/USB/Linux_USB_Layers/Configfs_Composite_Gadget).

the script in `/opt/scripts/boot/am335x_evm.sh` sets up the `/sys/kernel/config/usb_gadget/g_multi` gadget with 3 functions:

```
/sys/kernel/config/usb_gadget/g_multi
├── bcdDevice
├── bcdUSB
├── bDeviceClass
├── bDeviceProtocol
├── bDeviceSubClass
├── bMaxPacketSize0
├── configs
│   └── c.1
│       ├── bmAttributes
│       ├── hid.usb0 -> ../../../../usb_gadget/g_multi/functions/hid.usb0
│       ├── MaxPower
│       └── strings
│           └── 0x409
│               └── configuration
├── functions
│   └── hid.usb0
│       ├── dev
│       ├── protocol
│       ├── report_desc
│       ├── report_length
│       └── subclass
├── idProduct
├── idVendor
├── os_desc
│   ├── b_vendor_code
│   ├── qw_sign
│   └── use
├── strings
│   └── 0x409
│       ├── manufacturer
│       ├── product
│       └── serialnumber
└── UDC
```

So it should be easy to add an extra gadget for a [HID Keyboard](https://wiki.tizen.org/USB/Linux_USB_Layers/Configfs_Composite_Gadget/Usage_eq._to_g_hid.ko)

POC
---

### Setup the HID Keyboard device
As first POC, we try to disable the all the gadgets in `g_multi` so we can disable the gadget and add our own (otherwise, we get _Device Busy_ errors).

1. Disable Mass Storage and Tethering
in `/etc/default/bb-boot` uncomment the settings so the Mass Storage Device and the USB Network are disabled:
```env
USB_IMAGE_FILE_DISABLED=yes
USB_NETWORK_DISABLED=yes
```
2. Disable the ACM (USB Serial)
Unfortunately, the acm device is not controlled via flags in the `bb-boot` script, so just uncomment the 2
lines in `/opt/scripts/boot/am335x_evm.sh`
```
# diff -u am335x_evm.sh.ori am335x_evm.sh
--- am335x_evm.sh.ori	2018-10-06 08:54:48.042948584 +0000
+++ am335x_evm.sh	2018-09-30 14:16:05.017540771 +0000
@@ -527,7 +527,7 @@
 			echo ${cpsw_5_mac} > functions/ecm.usb0/dev_addr
 		fi
 
-		mkdir -p functions/acm.usb0
+		# disabled by tripod: mkdir -p functions/acm.usb0
 
 		if [ "x${has_img_file}" = "xtrue" ] ; then
 			echo "${log} enable USB mass_storage ${usb_image_file}"
@@ -549,7 +549,7 @@
 			ln -s functions/rndis.usb0 configs/c.1/
 			ln -s functions/ecm.usb0 configs/c.1/
 		fi
-		ln -s functions/acm.usb0 configs/c.1/
+		# disable by tripod: ln -s functions/acm.usb0 configs/c.1/
 		if [ "x${has_img_file}" = "xtrue" ] ; then
 			ln -s functions/mass_storage.usb0 configs/c.1/
 		fi
```
3. reboot
**NOTE** Be sure to be connected via ethernet and remember the IP. otherwise it's the last thing you do :-)
4. turn off UDC (USB Device Controller)
```
cd /sys/kernel/config/usb_gadget/g_multi
echo "" > UDC
```
5. install HID Keyboad
```
./init.sh
```

### Test!

For testing, compile the `test_gadget.c`, attach to a computer, and play around:

```
cc -o test_gadget test_gadget
sudo ./test_gadget /dev/hidg0 k
....
```

Enabling HID Keyboard alongside USB Network
===========================================

Of course it would be best, if we can keep the USB Network enabled.

so add the init stuff to: `/opt/scripts/boot/am335x_evm.sh`
```
# diff -u am335x_evm.ori am335x_evm.sh 
--- am335x_evm.ori	2018-10-06 08:54:48.042948584 +0000
+++ am335x_evm.sh	2018-10-06 08:59:07.082173424 +0000
@@ -553,6 +553,18 @@
 		if [ "x${has_img_file}" = "xtrue" ] ; then
 			ln -s functions/mass_storage.usb0 configs/c.1/
 		fi
+		# -------------------------------------------
+		# create HID Keyboard
+		mkdir functions/hid.usb0
+		echo 1 > functions/hid.usb0/protocol
+		echo 1 > functions/hid.usb0/subclass
+		echo 8 > functions/hid.usb0/report_length
+		# write the keyboard usb page. see https://docs.mbed.com/docs/ble-hid/en/latest/api/md_doc_HID.html#keyboard
+		echo -ne "\\x05\\x01\\x09\\x06\\xa1\\x01\\x05\\x07\\x\\x19\\xe0\\x29\\xe7\\x15\\x00\\x25\\x01\\x75\\x01\\x95\\x08\\x81\\x02\\x95\\x01\\x\\x75\\x08\\x81\\x03\\x95\\x05\\x75\\x01\\x05\\x08\\x19\\x01\\x29\\x05\\x91\\x02\\x\\x95\\x01\\x75\\x03\\x91\\x03\\x95\\x06\\x75\\x08\\x15\\x00\\x25\\x65\\x05\\x07\\x19\\x00\\x29\\x65\\x81\\x00\\xc0" > functions/hid.usb0/report_desc
+
+		# 'install' new device
+		ln -s functions/hid.usb0 configs/c.1
+		# -------------------------------------------
```

Reading MIDI
============

Attach a MIDI instrument and test if we can read from it:

```
# amidi -l
Dir Device    Name
IO  hw:1,0,0  TD-1 MIDI 1
root@beaglebone:~# amidi -d -p hw:1,0,0

99 26 14
89 26 40
99 30 1C
89 30 40
```

Reading MIDI from C
=====================

**TODO**

http://fundamental-code.com/midi/

Putting it all together
=======================

**TODO**

Misc
====

Upgrade Kernel
--------------

run `/opt/scripts/tools/update_kernel.sh` as root.

before:
`Linux beaglebone 4.14.67-ti-r73 #1 SMP PREEMPT Thu Aug 30 00:08:52 UTC 2018 armv7l GNU/Linux`

after:
`Linux beaglebone 4.14.71-ti-r78 #1 SMP PREEMPT Tue Sep 25 21:14:59 UTC 2018 armv7l GNU/Linux`


Various Links
-------------
- https://docs.mbed.com/docs/ble-hid/en/latest/api/md_doc_HID.html#keyboard
- https://www.usb.org/document-library/hid-usage-tables-112
- https://www.kernel.org/doc/Documentation/usb/gadget_hid.txt

- https://unix.stackexchange.com/questions/388304/no-udc-shows-up-for-usb-mass-storage-gadget-with-configfs/389073
- https://wiki.tizen.org/USB/Linux_USB_Layers/Configfs_Composite_Gadget/General_configuration
- https://stackoverflow.com/questions/33016961/where-is-g-multi-configured-in-beaglebone-black
- https://www.codeproject.com/Questions/1208305/Using-beaglebone-black-as-a-keyboard-device
