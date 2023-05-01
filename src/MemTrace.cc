
#include <stdio.h>

#include <iostream>
#include <fstream>
#include <string>
#include <elfio/elfio.hpp>

using namespace std;

#include "MemTrace.h"
#include "Logger.h"

extern bool verbose;

MemTrace::MemTrace(FstProcess & fstProc, string memInitFileName, int memInitStartAddr,  
        FstSignal clk,
        FstSignal memCmdValid, FstSignal memCmdReady, FstSignal memCmdAddr, FstSignal memCmdSize, FstSignal memCmdWr, FstSignal memCmdWrData,
        FstSignal memRspValid, FstSignal memRspData) :
                    fstProc(fstProc),
                    memInitFileName(memInitFileName),
                    memInitStartAddr(memInitStartAddr),
                    clk(clk),
                    memCmdValid(memCmdValid),
                    memCmdReady(memCmdReady),
                    memCmdAddr(memCmdAddr),
                    memCmdSize(memCmdSize),
                    memCmdWr(memCmdWr),
                    memCmdWrData(memCmdWrData),
                    memRspValid(memRspValid),
                    memRspData(memRspData)
{
    init();
}

static void memChangedCB(uint64_t time, FstSignal *signal, const unsigned char *value, void *userInfo)
{
    MemTrace *memTrace = (MemTrace *)userInfo;
    memTrace->processSignalChanged(time, signal, value);
}

void MemTrace::processSignalChanged(uint64_t time, FstSignal *signal, const unsigned char *value)
{
    if (strstr((char *)value, "x") || strstr((char *)value, "z")){
        return;
    }

    uint64_t valueInt = stol(string((const char *)value), nullptr, 2);

#if 0
    LOG_DEBUG("%ld, %ud, %s, %s, %ld", time, signal->handle, signal->name.c_str(), value, valueInt);
#endif

    if (signal->handle == memCmdValid.handle){
        curMemCmdValid      = valueInt;
        return;
    }

    if (signal->handle == memCmdReady.handle){
        curMemCmdReady      = valueInt;
        return;
    }

    if (signal->handle == memCmdAddr.handle){
        curMemCmdAddr       = valueInt;
        return;
    }

    if (signal->handle == memCmdSize.handle){
        curMemCmdSize       = valueInt;
        return;
    }

    if (signal->handle == memCmdWr.handle){
        curMemCmdWr         = valueInt;
        return;
    }

    if (signal->handle == memCmdWrData.handle){
        curMemCmdWrData     = valueInt;
        return;
    }

    if (signal->handle == memRspValid.handle){
        curMemRspValid      = valueInt;
        return;
    }

    if (signal->handle == memRspData.handle){
        curMemRspData       = valueInt;
        return;
    }

    // All signals changes on the rising edge of the clock. Everything is stable at the falling edge...
    if (signal->handle == clk.handle && valueInt == 0){
        if (curMemCmdValid && curMemCmdReady){
            // For now, only handle memory writes.
            if (curMemCmdWr){
                int byteEna = 0;
                switch(curMemCmdSize){
                case 0:  byteEna     = 1 << (curMemCmdAddr & 3); break;
                case 1:  byteEna     = 3 << (curMemCmdAddr & 3); break;
                default: byteEna     = 15; break;
                }

                for(int byteNr=0; byteNr<4;++byteNr){
                    if (byteEna & (1<<byteNr)){
                        uint64_t byteVal    = (curMemCmdWrData >> (byteNr * 8)) & 255;
                        uint64_t addr       = (curMemCmdAddr & ~3) | byteNr;

                        if (verbose) LOG_INFO("MemWr: 0x%08lx <- 0x%02lx (@%ld)", addr, byteVal, time);

                        MemAccess   ma = { time, curMemCmdWr, addr, byteVal }; 
                        memTrace.push_back(ma);
                    }
                }
            }
        }
    }
}

void MemTrace::init()
{
    if (!memInitFileName.empty()){
        FILE* fp = fopen(memInitFileName.c_str(), "r");
        if(fp) {
            std::array<char, 5> buf;
            auto n = fread(buf.data(), 1, 4, fp);
            fclose(fp);
            if(n != 4)
                throw std::runtime_error("input file has insufficient size");
            buf[4] = 0;
            if(strcmp(buf.data() + 1, "ELF") == 0) {
                // Create elfio reader
                ELFIO::elfio reader;
                // Load ELF data
                if(!reader.load(memInitFileName))
                    throw std::runtime_error("could not process elf file");
                // check elf properties
                if(reader.get_class() != ELFCLASS32)
                    throw std::runtime_error("wrong elf class in file");
                if(reader.get_type() != ET_EXEC)
                    throw std::runtime_error("wrong elf type in file");
                if(reader.get_machine() != EM_RISCV)
                    throw std::runtime_error("wrong elf machine in file");
                for(const auto pseg : reader.segments) {
                    const auto fsize = pseg->get_file_size(); // 0x42c/0x0
                    const auto seg_data = pseg->get_data();
                    if(fsize > 0) {
                        auto addr = pseg->get_physical_address();
                        auto ptr = reinterpret_cast<const uint8_t* const>(seg_data);
                        auto& p = memInitContents(addr / memInitContents.page_size);
                        auto offs = addr & memInitContents.page_addr_mask;
                        std::copy(ptr, ptr + fsize, p.data() + offs);
                    }
                }
            } else {

                LOG_INFO("Loading mem init file: %s", memInitFileName.c_str());
                ifstream initFile(memInitFileName, ios::in | ios::binary);
                if (initFile.fail()){
                    LOG_ERROR("Error opening mem init file: %s (%s)", memInitFileName.c_str(), strerror(errno));
                    exit(1);
                }
                vector<char> content((std::istreambuf_iterator<char>(initFile)), std::istreambuf_iterator<char>());
                auto addr = memInitStartAddr;
                for(auto v:content) {
                    memInitContents[addr++] = v;
                }
            }
        }
    }

    vector<FstSignal *> sigs;

    sigs.push_back(&clk);
    sigs.push_back(&memCmdValid);
    sigs.push_back(&memCmdReady);
    sigs.push_back(&memCmdAddr);
    sigs.push_back(&memCmdSize);
    sigs.push_back(&memCmdWr);
    sigs.push_back(&memCmdWrData);
    sigs.push_back(&memRspValid);
    sigs.push_back(&memRspData);

    bool allSigsFound = fstProc.assignHandles(sigs);
    if (!allSigsFound){
        fstProc.reportSignalsNotFound(sigs);
        exit(-1);
    }

    curMemCmdValid  = false;
    curMemCmdReady  = false;
    curMemCmdAddr   = 0;
    curMemCmdSize   = 0;
    curMemCmdWr     = false;
    curMemCmdWrData = 0;
    curMemRspValid  = false;
    curMemRspData   = 0;

    fstProc.getValueChanges(sigs, memChangedCB, (void *)this);

    LOG_INFO("Nr mem write transactions: %ld", memTrace.size());
}


bool MemTrace::getValue(uint64_t time, uint64_t addr, char *value)
{
    bool valueValid = memInitContents.is_allocated(addr);
    uint64_t val = memInitContents[addr];

    for(auto m: memTrace){
        if (m.time > time)
            break;

        if (m.wr && m.addr == addr){
            valueValid      = true;
            val             = m.value;
        }
    }

    *value      = val;

    return valueValid;
}



