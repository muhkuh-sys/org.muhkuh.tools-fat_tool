
setlocal
set FAT_TOOL=fat_tool.exe

mkdir tmp
rem build image

%FAT_TOOL% -create 528 8067 0x420000 66000 -writeraw data\netTAP_bsl.bin 0 -mkdir SYSTEM -mkdir PORT_0 -mkdir PORT_1 -mkdir PORT_2 -mkdir PORT_3 -mkdir PORT_4 -mkdir PORT_5 -writefile data\NTPNSNSC.NXF PORT_0/NTPNSNSC.NXF -writefile data\ledflash.lua PORT_1/netscrpt.lua -dir / -r -saveimage tmp\test.bin

rem read everything back and compare
%FAT_TOOL% -mount tmp\test.bin 66000 -dir -cd PORT_0 -readfile NTPNSNSC.NXF tmp\NTPNSNSC.NXF -cd ../PORT_1 -readfile netscrpt.lua tmp\ledflash.lua -readraw 0 48796 tmp\netTAP_bsl.bin -cd / -delete PORT_1/netscrpt.lua -delete PORT_0/NTPNSNSC.NXF -dir -r

fc /b data\netTAP_bsl.bin tmp\netTAP_bsl.bin
fc /b data\NTPNSNSC.NXF tmp\NTPNSNSC.NXF
fc /b data\ledflash.lua tmp\ledflash.lua

rem some testing - each test should fail
set FAT_TEST=%FAT_TOOL% -create 528 8067 0x420000 66000

%FAT_TEST% -writeraw data\netTAP_bsl.bin 10000000
%FAT_TEST% -readraw 10000000 1000 data\netTAP_bsl.bin  

%FAT_TEST% -mkdir PORT_0/lalala
%FAT_TEST% -dir
%FAT_TEST% -cd blabla

%FAT_TEST% -writefile bla.bin tra.bin
%FAT_TEST% -readfile bla.bin tra.bin
%FAT_TEST% -delete lala.bin
