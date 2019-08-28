1) Prepare libtool

  Download autoconf, automake and libtools.
  'sudo make install' them.

2) Prepare usb.ids

  Copy from /usr/share/hwdata/ or downloaded from net.

3) Compile with libusb

  $ cd <kernel-src>/tools/usb/usbip/libusb/
  $ ./autogen.sh
  $ ./configure CPPFLAGS=-I/usr/local/include LDFLAGS=-L/usr/local/lib \
                --with-usbids-dir=<dir-of-usb.ids> --with-dummy-driver
  $ make
  $ sudo make install

[EOF]
