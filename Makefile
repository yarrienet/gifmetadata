CC=gcc
VERSION=v0.0.1
PRODUCT=gifmetadata
SOURCE=.gitignore Makefile main.c README.md

main: main.c
	$(CC) main.c -lm -o $(PRODUCT)

mac_x86_64: main.c
	mkdir -p build/mac_x86_64
	$(CC) main.c -o build/mac_x86_64/$(PRODUCT) -target x86_64-apple-macos10.4
	
mac_arm: main.c
	mkdir -p build/mac_arm
	$(CC) main.c -o build/mac_arm/$(PRODUCT) -target arm64-apple-macos11
	
mac_universal: mac_x86_64 mac_arm
	mkdir -p build/mac_universal
	lipo -create -output build/mac_universal/$(PRODUCT) build/mac_x86_64/$(PRODUCT) build/mac_arm/$(PRODUCT)
	
host_64: main.c
	mkdir -p build/host_64
	$(CC) main.c -o build/host_64/$(PRODUCT) -m64
	
host_32: main.c
	mkdir -p build/host_32
	$(CC) main.c -o build/host_32/$(PRODUCT) -m32
	
clean:
	rm -rf *.o *.tar.gz build/

source_tar: $(SOURCE)
	tar -czvf $(PRODUCT)_$(VERSION)_source.tar.gz $(SOURCE)