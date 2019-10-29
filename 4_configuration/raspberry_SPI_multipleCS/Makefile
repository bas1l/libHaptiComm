DTSC = dtc
DFLAGS = -I dts -O dtb
SRCFILES = spi-example.dts spi0-1cs-overlay.dts spi0-2cs-overlay.dts spi0-3cs-overlay.dts spi0-4cs-overlay.dts spi0-5cs-overlay.dts spi0-6cs-overlay.dts spi1-3cs-overlay.dts spi1-4cs-overlay.dts spi1-5cs-overlay.dts spi1-6cs-overlay.dts
OBJFILES = ${SRCFILES:-overlay.dts=.dtbo}
DESTDIR = /boot/overlays/

all : $(OBJFILES)


%.dtbo : %-overlay.dts 
	$(DTSC) $(DFLAGS) $< -o $@

.PHONY: install uninstall clean

install : $(OBJFILES)
	cp -rf $(OBJFILES) $(DESTDIR)
	
uninstall : $(OBJFILES)
	rm -f $(DESTDIR)$(OBJFILES)

clean :
	rm -rf *.dtbo *~
	
