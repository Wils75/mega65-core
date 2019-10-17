/*
  Simple "colour in the screen in your colour" game as
  demo of C65.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <serial.h>

#define POKE(a,v) *((uint8_t *)a) = (uint8_t)v
#define PEEK(a) ((uint8_t)(*((uint8_t *)a)))

unsigned short i;

struct dmagic_dmalist {

  unsigned char option_0b;
  unsigned char option_80;
  unsigned char source_mb;
  unsigned char option_81;
  unsigned char dest_mb;
  unsigned char end_of_options;

  // F018B format DMA request
  unsigned char command;
  unsigned int count;
  unsigned int source_addr;
  unsigned char source_bank;
  unsigned int dest_addr;
  unsigned char dest_bank;
  unsigned char sub_cmd;  // F018B subcmd
  unsigned int modulo;

};

struct dmagic_dmalist dmalist;
unsigned char dma_byte;

void do_dma(void) {

  // Now run DMA job (to and from anywhere, and list is in low 1MB)
  POKE(0xd702U,0);
  POKE(0xD704U,0x00);
  POKE(0xd701U,(((unsigned int)&dmalist) >> 8));
  POKE(0xd705U,((unsigned int)&dmalist)&0xff); // triggers enhanced DMA

}

void lpoke(long address, unsigned char value) {

  dmalist.option_0b=0x0b;
  dmalist.option_80=0x80;
  dmalist.source_mb=0;
  dmalist.option_81=0x81;
  dmalist.dest_mb=address>>20;
  dmalist.end_of_options=0x00;

  dma_byte = value;
  dmalist.command = 0x00; // copy
  dmalist.sub_cmd = 0;
  dmalist.modulo = 0;
  dmalist.count = 1;
  dmalist.source_addr = (unsigned int)&dma_byte;
  dmalist.source_bank = 0;
  dmalist.dest_addr = address&0xffff;
  dmalist.dest_bank = (address >> 16)&0xf;

  do_dma();
  return;

}

unsigned char lpeek(long address) {

  dmalist.option_0b=0x0b;
  dmalist.option_80=0x80;
  dmalist.source_mb=address>>20;
  dmalist.option_81=0x81;
  dmalist.dest_mb=0;
  dmalist.end_of_options=0x00;

  dmalist.command = 0x00; // copy
  dmalist.count = 1;
  dmalist.source_addr = address&0xffff;
  dmalist.source_bank = (address >> 16)&0x0f;
  dmalist.dest_addr = (unsigned int)&dma_byte;
  dmalist.source_bank = 0;
  dmalist.dest_addr = address&0xffff;
  dmalist.dest_bank = (address >> 16)&0x0f;
  // Make list work on either old or new DMAgic
  dmalist.sub_cmd = 0;
  dmalist.modulo = 0;

  do_dma();
  return dma_byte;

}

void lcopy(long source_address, long destination_address, unsigned int count) {

  dmalist.option_0b=0x0b;
  dmalist.option_80=0x80;
  dmalist.source_mb=source_address>>20;
  dmalist.option_81=0x81;
  dmalist.dest_mb=destination_address>>20;
  dmalist.end_of_options=0x00;

  dmalist.command = 0x00; // copy
  dmalist.count = count;
  dmalist.sub_cmd = 0;
  dmalist.modulo = 0;
  dmalist.source_addr = source_address&0xffff;
  dmalist.source_bank = (source_address >> 16)&0x0f;
  //  if (source_address >= 0xd000 && source_address < 0xe000)
  //    dmalist.source_bank|=0x80;
  dmalist.dest_addr = destination_address&0xffff;
  dmalist.dest_bank = (destination_address >> 16)&0x0f;
  //  if (destination_address>=0xd000 && destination_address<0xe000)
  //    dmalist.dest_bank|=0x80;

  do_dma();
  return;

}

void lfill(long destination_address, unsigned char value, unsigned int count) {

  dmalist.option_0b=0x0b;
  dmalist.option_80=0x80;
  dmalist.source_mb=0;
  dmalist.option_81=0x81;
  dmalist.dest_mb=destination_address>>20;
  dmalist.end_of_options=0x00;

  dmalist.command = 0x03; // fill
  dmalist.sub_cmd = 0;
  dmalist.count = count;
  dmalist.source_addr = value;
  dmalist.dest_addr = destination_address&0xffff;
  dmalist.dest_bank = (destination_address >> 16)&0x0f;
  if (destination_address >= 0xd000 && destination_address < 0xe000)
    dmalist.dest_bank |= 0x80;

  do_dma();
  return;

}

unsigned char offset = 0;
unsigned char v;
unsigned char v1;
unsigned int tempCounter, drawCounter;
unsigned int heartIterator;
unsigned char frame[5];
unsigned char lineColour;
unsigned int heartData[30] = {49,
                             50,
                             53,
                             58,
                             65,
                             74,
                             65,
                             58,
                             53,
                             50,
                             49,
                             48,
                             45,
                             40,
                             33,
                             24,
                             33,
                             40,
                             45,
                             48,
                             49,
                             50,
                             52,
                             53,
                             57,
                             61,
                             57,
                             53,
                             52,
                             50};
unsigned char fnum = 0;
unsigned char flast = 99;
unsigned char spo2;
unsigned short prh;
unsigned short pr;

unsigned long j;

unsigned char needed[25][5] = {{ 0x01, 0x80, 0x80, /*PR*/0x65, 0xC8 },
              /*2*/       { 0x01, 0x80, 0x80, /*PR*/0x65, 0xC8 },
              /*3*/       { 0x01, 0x80, 0x80, /*SPO2*/0x61, 0xC8 },
              /*4*/       { 0x01, 0x80, 0x80, 0x61, 0xC8 },
              /*5*/       { 0x01, 0x80, 0x80, 0x61, 0xC8 },
              /*6*/       { 0x01, 0x80, 0x80, 0x61, 0xC8 },
              /*7*/       { 0x01, 0x80, 0x80, 0x61, 0xC8 },
              /*8*/       { 0x01, 0x80, 0x80, 0x61, 0xC8 },
              /*9*/       { 0x01, 0x80, 0x80, /*SPO2*/0x61, 0xC8 },
              /*10*/      { 0x01, 0x80, 0x80, /*SPO2/PR*/0x64, 0xC8 },
              /*11*/      { 0x01, 0x80, 0x80, /*SPO2*/0x61, 0xC8 },
              /*12*/      { 0x01, 0x80, 0x80, 0x61, 0xC8 },
              /*13*/      { 0x01, 0x80, 0x80, 0x61, 0xC8 },
              /*14*/      { 0x01, 0x80, 0x80, /*PR*/0x65, 0xC8 },
              /*15*/      { 0x01, 0x80, 0x80, /*PR*/0x65, 0xC8 },
              /*16*/      { 0x01, 0x80, 0x80, /*SPO2*/0x61, 0xC8 },
              /*17*/      { 0x01, 0x80, 0x80, /*SPO2*/0x61, 0xC8 },
              /*18*/      { 0x01, 0x80, 0x80, 0x60, 0xC8 },
              /*19*/      { 0x01, 0x80, 0x80, 0x60, 0xC8 },
              /*20*/      { 0x01, 0x80, 0x80, /*PR*/0x65, 0xC8 },
              /*21*/      { 0x01, 0x80, 0x80, /*PR*/0x65, 0xC8 },
              /*22*/      { 0x01, 0x80, 0x80, /*PR*/0x65, 0xC8 },
              /*23*/      { 0x01, 0x80, 0x80, /*PR*/0x65, 0xC8 },
              /*24*/      { 0x01, 0x80, 0x80, 0x64, 0xC8 },
              /*25*/      { 0x01, 0x80, 0x80, 0x64, 0xC8 }};

  unsigned short nums[70][6] = {{102, 102, 102, 32, 32, 102},
                                {102, 102, 32, 32, 32, 102},
                                {102, 32, 32, 32, 32, 102},
                                {102, 102, 102, 32, 32, 102},
                                {102, 102, 102, 32, 32, 102},
                                {102, 102, 102, 32, 32, 102},
                                {102, 102, 102, 32, 32, 102},
                                {102, 32, 32, 32, 32, 102},
                                {32, 32, 102, 102, 32, 32},
                                {102, 102, 102, 102, 32, 32},
                                {102, 102, 102, 32, 32, 102},
                                {102, 102, 32, 32, 102, 102},
                                {102, 32, 32, 102, 102, 102},
                                {32, 32, 32, 32, 32, 32},
                                {102, 32, 32, 32, 32, 102},
                                {32, 32, 102, 102, 32, 32},
                                {102, 102, 102, 102, 32, 32},
                                {102, 102, 32, 32, 32, 102},
                                {102, 102, 102, 102, 32, 32},
                                {32, 32, 102, 102, 32, 32},
                                {102, 32, 32, 32, 32, 102},
                                {102, 102, 102, 32, 32, 32},
                                {102, 102, 32, 32, 32, 32},
                                {102, 32, 32, 102, 32, 32},
                                {32, 32, 102, 102, 32, 32},
                                {32, 32, 32, 32, 32, 32},
                                {102, 102, 102, 102, 32, 32},
                                {102, 102, 102,102, 32, 32},
                                {32, 32, 32, 32, 32, 32},
                                {32, 32, 102, 102, 102, 102},
                                {32, 32, 32, 32, 32, 102},
                                {102, 102, 102, 102, 32, 32},
                                {102, 102, 102, 102, 32, 32},
                                {32, 32, 102, 102, 32, 32},
                                {102, 32, 32, 32, 32, 102},
                                {102, 102, 32, 32, 32, 102},
                                {102, 32, 32, 102, 102, 102},
                                {32, 32, 102, 102, 102, 102},
                                {32, 32, 32, 32, 32, 102},
                                {32, 32, 102, 102, 32, 32},
                                {32, 32, 102, 102, 32, 32},
                                {102, 32, 32, 32, 32, 102},
                                {32, 32, 32, 32, 32, 32},
                                {102, 102, 102, 102, 32, 32},
                                {102, 102, 102, 102, 32, 32},
                                {102, 102, 102, 32, 32, 102},
                                {102, 102, 32, 32, 102, 102},
                                {102, 102, 32, 32, 102, 102},
                                {102, 102, 32, 32, 102, 102},
                                {102, 32, 32, 32, 32, 102},
                                {32, 32, 102, 102, 32, 32},
                                {32, 32, 102, 102, 32, 32},
                                {102, 32, 32, 32, 32, 102},
                                {32, 32, 102, 102, 32, 32},
                                {32, 32, 102, 102, 32, 32},
                                {102, 32, 32, 32, 32, 102},
                                {102, 32, 32, 32, 32, 102},
                                {32, 32, 102, 102, 32, 32},
                                {32, 32, 102, 102, 32, 32},
                                {102, 32, 32, 32, 32, 32},
                                {102, 102, 102, 102, 32, 32},
                                {102, 102, 102, 32, 32, 102},
                                {102, 32, 32, 32, 102, 102},
                                {102, 32, 32, 32, 32, 102},
                                {32, 32, 102, 102, 32, 32},
                                {32, 32, 102, 32, 32, 32},
                                {32, 32, 32, 32, 32, 32},
                                {32, 32, 32, 102, 32, 32},
                                {32, 32, 102, 102, 32, 32},
                                {102, 32, 32, 32, 32, 102}};

  unsigned short *screen = 0xA000U;
  int scr;

  unsigned int x,x1;
  unsigned int y, newX, newY;
  unsigned int heartYPos, pulseYPos, oxyYPos;
  unsigned int flag;
  unsigned int horizontalCounter;
  unsigned int forLoopInt;

  unsigned char* heartString = "  heart  rate  ";
  unsigned char* pulseString = "  pulse  rate  ";
  unsigned char* oxyLevels = " oxygen  level ";

  unsigned char serialData;
  unsigned char c;
  unsigned long a, heartRatePixel, pulseRatePixel,oxyLevelPixel;
  unsigned long heartRatePixelArray[478];
  unsigned long pulseRatePixelArray[478];
  unsigned long oxyLevelPixelArray[478];
  int n = 0;

void plot_pixel() {
  // Integer halving the number with bitwise shift
  x1 = x>>1;

  // Pixel memory address location
  a = 0x40000L +
  // Bitwise AND with 7 to keep resulting value below 4 bits
  (x1&7) +
  // Multiply y with unsigned int 8 because of character size
  (y*8U) +
  // Bitwise shift by 3 (long)
  (x1>>3L) *
  // Screen dimension multiplication
  (50*64L);

  heartRatePixel = 0x40000L +
  (x1&7) +
  (heartYPos*8u) +
  (x1>>3L) *
  (50*64L);

  pulseRatePixel = 0x40000L +
  (x1&7) +
  (pulseYPos*8u) +
  (x1>>3L) *
  (50*64L);

  oxyLevelPixel = 0x40000L +
  (x1&7) +
  (oxyYPos*8u) +
  (x1>>3L) *
  (50*64L);

  /* Assign 2 bytes of memory at address value (of a) before re-assigning to 0. This seems redundant.
  v = lpeek(a); */

  // Assign v to 0
  v = 0;

  // Replacement for modulus (runs statement if odd)
  if (!(x&1)) {
    v &= 0xf0;
    v |= c;
  }
  // Else runs if even
  else {
    v &= 0xf;
    v |= (c<<4);
  }

}

void drawJaggyLines()
{
  lpoke(heartRatePixelArray[x], 0x00);
  lpoke(pulseRatePixelArray[x], 0x00);
  lpoke(oxyLevelPixelArray[x], 0x00);

  if (heartIterator < 29)
  {
    heartYPos = heartData[heartIterator];
    heartIterator += 1;
  }
  else
  {
    heartIterator = 0;
    heartYPos = heartData[heartIterator];
  }

  v = PEEK(0xd012);
  pulseYPos = (v/4) +  124;

  lineColour = 0x22;
  lpoke(heartRatePixel, lineColour);
  heartRatePixelArray[x] = heartRatePixel;
  lineColour = 0x55;
  lpoke(pulseRatePixel, lineColour);
  pulseRatePixelArray[x] = pulseRatePixel;
  lineColour = 0x33;
  lpoke(oxyLevelPixel, lineColour);
  oxyLevelPixelArray[x] = oxyLevelPixel;

  //for (forLoopInt = 0; forLoopInt < 10000; forLoopInt += 1);
}

void drawHorizontalLines()
{
  horizontalCounter = 0;

  while (horizontalCounter < 3)
  {
    plot_pixel();
    lpoke(a, 0x77);
    if (y == 99 || y == 198)
    {
      y += 99;
    }
    else
    {
      y = 99;
    }
    horizontalCounter += 1;
  }
}

unsigned int arrayOffset;

void setFirstNumberOnRight(unsigned int number,
                           unsigned short offsetX,
                           unsigned short offsetY)
{
  for (newX = (30 + offsetX); newX < (36 + offsetX); newX++)
  {
    for (newY = (0 + offsetY); newY < (7 + offsetY); newY++)
    {
      screen[newX + newY * 45U] = 102;
    }
  }
  switch(number)
  {
    case 1:
      arrayOffset = 0;
      break;
    case 2:
      arrayOffset = 7;
      break;
    case 3:
      arrayOffset = 14;
      break;
    case 4:
      arrayOffset = 21;
      break;
    case 5:
      arrayOffset = 28;
      break;
    case 6:
      arrayOffset = 35;
      break;
    case 7:
      arrayOffset = 42;
      break;
    case 8:
      arrayOffset = 49;
      break;
    case 9:
      arrayOffset = 56;
      break;
    default:
      arrayOffset = 63;
      break;
  }
  for (newX = 0; newX < 6; newX++)
  {
    for (newY = 0; newY < 7; newY++)
    {
      screen[(newX + 30U + offsetX) + ((newY + 1 + offsetY) * 45U)] = nums[newY + arrayOffset][newX];
    }
  }
}

void main(void) {
  tempCounter = 0;
  drawCounter = 1;
  lineColour = 0x55;

  // Fast CPU
  POKE(0,65);

  // Enable access to serial port and other devices
  POKE(53295L,0x47);
  POKE(53295L,0x53);

  // High res in X and Y directions
  POKE(0xD031U,0x88);

  // Set serial port speed to 9600
  POKE(0xd0e6U,0x46);
  POKE(0xd0e7U,0x10);

  // Accessing the VIC registers
  POKE(0xd02f,0x47);
  POKE(0xd02f,0x53);

  // Setting 16 bit character mode
  // also enable full colour chars for chars >$FF
  POKE(0xd054,0x05);

  // Move screen to $A000
  POKE(0xD060,0x00);
  POKE(0xD061,0xA0);

  // Logical lines are 80 bytes long
  POKE(0xD058,90);
  POKE(0xD05E,45);

  // Make screen background black
  POKE(0xD020,0);
  POKE(0xD021,0);

  // Clear colour RAM and set correct bits for showing full colour chars
  lfill(0xff80000L,0x08,45*50*2);

  // Initialise the screen RAM
  n = 0x1000;
  lfill((unsigned long)screen,0,30*50*2);
  for (x=0; x<30; x++) {

    for(y=0; y<50; y++) {

      screen[x + y * 45U] = n;
      n++;
      n=n&0x1fff;

    }

  }

  for (x=30; x<45; x++)
  {
    for(y=0; y<50; y++)
    {
//      v = PEEK(0xd0e0U);
//      v = PEEK(0xd012);
//      if (v != 0)
//      {
//        lineColour = 0x11;
//      }
      screen[x + y * 45U] = 102;
      lpoke(0xff80000L+x*2+y*90U+0,0);
      lpoke(0xff80000L+x*2+y*90U+1,1);
    }
  }

  for (x = 30; x < 45; x++)
  {
    screen[x + 540U] = 32;
    screen[x + 1080U] = 32;
    screen[x + 1665U] = 32;
  }

  for (x = 0; x < 15; x++)
  {
    screen[x + 30 + 405U] = heartString[x];
    lpoke(0xff80000L+((x + 30)*2) + 810U + 1,2);
  }

  for (x = 0; x < 15; x++)
  {
    screen[x + 30 + 990U] = pulseString[x];
    lpoke(0xff80000L+((x + 30)*2) + 1980U + 1,5);
  }

  for (x = 0; x < 15; x++)
  {
    screen[x + 30 + 1530U] = oxyLevels[x];
    lpoke(0xff80000L+((x + 30)*2) + 3060U + 1,3);
  }

  // Clear pixel memory
  lfill(0x50000L,0,0xFFFF);
  lfill(0x40000L,0,0xFFFF);

  x = 0;
  y = 99;
  heartYPos = 24;
  n = 0;
  heartIterator = 0;
  flag = 0;

  while (1)
  {
    tempCounter++;

    x = x + 1;
    if (x > 477) {
      x = 0;
    }

    n++;
    c = n>>8;
    if (tempCounter == 1000)
    {
      if (drawCounter < 9)
      {
        drawCounter++;
      }
      else
      {
          drawCounter = 0;
      }
      setFirstNumberOnRight(drawCounter, 8, 0);
      setFirstNumberOnRight(drawCounter + 1, 1, 0);
      setFirstNumberOnRight(drawCounter + 2, 8, 13);
      setFirstNumberOnRight(drawCounter - 1, 1, 13);
      setFirstNumberOnRight(drawCounter + 3, 8, 25);
      setFirstNumberOnRight(drawCounter - 2, 1, 25);
      setFirstNumberOnRight(drawCounter + 4, 8, 38);
      setFirstNumberOnRight(drawCounter - 3, 1, 38);

//      y = (v/4) + 124;

      tempCounter = 0;
    }

    drawHorizontalLines();
    drawJaggyLines();
  }


  frame[1] = 0x00;
  frame[3] = 0x00;
  frame[4] = 0x00;

  while (1)
  {

    if (x == 24 && y == 5)
    {
      x = 0;
      y = 0;
    }

//    v=PEEK(0xd0e0U);
    v=PEEK(0xd012U);

    for (x=30; x<45; x++) {
      for(y=0; y<50; y++) {
        screen[x + y * 45U] = v;
        //for (forLoopInt = 0; forLoopInt < 1000; forLoopInt++);
      }
    //v = needed[x][y];
  }

    if (v)
    {
      frame[0] = frame[1];
      frame[1] = frame[2];
      frame[2] = frame[3];
      frame[3] = frame[4];
      frame[4] = v;

      if (frame[0] == 0x01)
      {
	      if (frame[2]&0x80)
        {
	        if (!(frame[3]&0x80))
          {
            flast = fnum;

	          if (frame[1]&1)
            {
              fnum = 0;
            }
            else
            {
              fnum++;
            }

	          switch (fnum)
            {
              case 0:
                prh = frame[3];
                break;
              case 1:
		            if (flast == 0)
                {
                  pr = (frame[3]&0x7f); //+((prh&3)<<7);
                }
		            break;
              case 2:
		            if (flast == 1)
                {
                  //printf("pr = %d\n",pr);
                  drawHorizontalLines();
                }
                spo2 = frame[3];
		            break;
	            case 3:
		            if (flast == 2)
                {
		              //printf("spo2 = %d\n",spo2);

                }
                break;
	          }
	        }
        }
      }
    }

    if (y % 5 == 0) {

      x++;
      y = 0;

    }
    y++;
  }
}
