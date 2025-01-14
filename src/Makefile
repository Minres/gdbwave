

INC_FILES   = FstProcess.h CpuTrace.h RegFileTrace.h MemTrace.h TcpServer.h Logger.h
OBJ_FILES   = main.o FstProcess.o CpuTrace.o RegFileTrace.o MemTrace.o TcpServer.o Logger.o gdbstub.o gdbstub_sys.o
LIB_FILES   = -lfstapi -lz

UNAME_S         = $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    RISCV_TOOLCHAIN = /opt/riscv64-unknown-elf-toolchain-10.2.0-2020.12.8-x86_64-linux-ubuntu14/bin/
endif
ifeq ($(UNAME_S),Darwin)
    RISCV_TOOLCHAIN = /opt/riscv64-unknown-elf-gcc-10.1.0-2020.08.2-x86_64-apple-darwin/bin
endif

RISCV_PREFIX    = riscv64-unknown-elf-

RISCV_GDB       = $(RISCV_TOOLCHAIN)/$(RISCV_PREFIX)gdb

CXXFLAGS    += --std=c++14 -I. -Wall -pedantic -g -O0 -Wno-format-zero-length
LDFLAGS     += -L./fst -Wall -g 

TEST_FST        = ../test_data/top.fst
TEST_PARAMS     = ../test_data/configParams.txt

all: gdbwave

run: gdbwave
	./gdbwave -w $(TEST_FST) -c $(TEST_PARAMS)

gdbwave: fst/libfstapi.a $(OBJ_FILES) $(INC_FILES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS)  -o $@ $(OBJ_FILES) $(LIB_FILES)

fst/libfstapi.a:
	cd fst && make

$(OBJ_FILES): $(INC_FILES)

gdb: 
	$(RISCV_GDB) ../test_data/progmem.elf -x ./gdb_conf.cmd

wave:
	gtkwave ../test_data/top.fst ../test_data/waves.gtkw

clean:
	\rm -fr gdbwave *.o
	cd fst && make clean
