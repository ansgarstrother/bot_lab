#ifndef _MCP23S17_H
#define _MCP23S17_H

#define MCP23S17_IODIRA    0x00
#define MCP23S17_IODIRB    0x01
#define MCP23S17_IPOLA     0x02
#define MCP23S17_IPOLB     0x03
#define MCP23S17_GPINTENA  0x04
#define MCP23S17_GPINTENB  0x05
#define MCP23S17_DEFVALA   0x06
#define MCP23S17_DEFVALB   0x07
#define MCP23S17_INTCONA   0x08
#define MCP23S17_INTCONB   0x09
#define MCP23S17_IOCON     0x0a
//#define MCP23S17_IOCON 0x00
#define MCP23S17_GPPUA     0x0c
#define MCP23S17_GPPUB     0x0d
#define MCP23S17_INTFA     0x0e
#define MCP23S17_INTFB     0x0f
#define MCP23S17_INTCAPA   0x10
#define MCP23S17_INTCAPB   0x11
#define MCP23S17_GPIOA     0x12
#define MCP23S17_GPIOB     0x13
#define MCP23S17_OLATA     0x14
#define MCP23S17_OLATB     0x15

#include "ssi.h"

uint32_t mcp23s17_read(uint32_t addr);
void mcp23s17_write(uint32_t addr, uint32_t value);


#endif
