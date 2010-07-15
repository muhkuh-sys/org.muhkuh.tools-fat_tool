mkdir tmp
..\msvc8\fat_tool\Release\fat_tool.exe -create 528 8067 0x420000 66000 -writeraw data\netTAP_bsl.bin 0 -mkdir SYSTEM -mkdir PORT_0 -mkdir PORT_1 -mkdir PORT_2 -mkdir PORT_3 -mkdir PORT_4 -mkdir PORT_5 -write data\NTPNSNSC.NXF PORT_0\NTPNSNSC.NXF -write data\ledflash.lua PORT_1\netscrpt.lua -dir \ -r -saveimage tmp\test.bin

rem read everything back and compare
..\msvc8\fat_tool\Release\fat_tool.exe -mount tmp\test.bin 66000 -dir -cd PORT_0 -read NTPNSNSC.NXF tmp\NTPNSNSC.NXF -cd ..\PORT_1 -read netscrpt.lua tmp\ledflash.lua -readraw 0 48796 tmp\netTAP_bsl.bin -cd \ -delete PORT_1\netscrpt.lua -delete PORT_0\NTPNSNSC.NXF -dir -r

fc /b data\netTAP_bsl.bin tmp\netTAP_bsl.bin
fc /b data\NTPNSNSC.NXF tmp\NTPNSNSC.NXF
fc /b data\ledflash.lua tmp\netscrpt.lua
