#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := frodoGo
COMPONENT_SRCDIRS := ./ odroid-go-common

include $(IDF_PATH)/make/project.mk
.build-post: .build-impl
	python /home/Test/esp/esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 --port COM3 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x1000 /c/Users/Test/Documents/NetBeansProjects/goMSX/build/bootloader/bootloader.bin 0x10000 /c/Users/Test/Documents/NetBeansProjects/goMSX/build/goMSX.bin 0x8000 /c/Users/Test/Documents/NetBeansProjects/goMSX/build/partitions_singleapp.bin

flash_COM6:
	python /home/Test/esp/esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 --port COM6 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x1000 /c/Users/Test/Documents/NetBeansProjects/fMSX-go/build/bootloader/bootloader.bin 0x10000 /c/Users/Test/Documents/NetBeansProjects/frodo-go/build/frodoGo.bin 0x8000 /c/Users/Test/Documents/NetBeansProjects/fMSX-go/build/partitions.bin
