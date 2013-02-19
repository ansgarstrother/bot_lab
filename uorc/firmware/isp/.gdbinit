target remote localhost:3333
#monitor soft_reset_halt

define rr
target remote localhost:3333
monitor soft_reset_halt
symbol-file main_eth.out
thbreak init
continue
end

define regs
info reg
end

define flash
monitor halt
#monitor stellaris mass_erase 0
monitor flash erase_address 0 65536
monitor soft_reset_halt
monitor flash write_image main_eth.out
monitor verify_image main_eth.out
x/4 0
end

define mcm
make clean; make
end

define dis
disassemble $pc-16 $pc+16
end

set print pretty
set confirm off
