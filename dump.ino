const int wr  = 2;
const int rd  = 3;
const int cs  = 4;
const int _A0 = 22;
const int _D0 = 38;

void setup()
{
  for(int i=0; i<16; i++)
  {
    pinMode(_A0+i, OUTPUT);
  }

  for(int i=0; i<8; i++)
  {
    pinMode(_D0+i, INPUT);
  }

  pinMode(wr, OUTPUT);
  digitalWrite(wr, HIGH);

  pinMode(rd, OUTPUT);
  digitalWrite(rd, HIGH);

  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);

  Serial.begin(115200);
/*
  for(int i=0; i<0x16; i++)
  {
    byte data = readdata(i);
    Serial.print(i, HEX);
    Serial.print(" : ");
    Serial.println(data, HEX);
  }
  for(;;){}
*/
  // read header
  readdata_range(0x0000, 0x0150);
}

void setaddress(unsigned int addr)
{
  for(int i=0; i<16; i++)
  {
    digitalWrite(_A0+i, (addr & (1 << i)) ? HIGH : LOW);
  }
}

byte readdata(unsigned int addr)
{
  digitalWrite(rd, LOW);
  digitalWrite(cs, LOW);

  setaddress(addr);
  delayMicroseconds(200);

  byte ret = 0;

  for(int i=0; i<8; i++)
  {
    if(digitalRead(_D0+i) == HIGH)
    {
      ret |= (1 << i);
    }
  }

  digitalWrite(rd, HIGH);
  digitalWrite(cs, HIGH);

  return ret;
}

void readdata_range(unsigned int addr_start,unsigned int addr_end)
{
  for(unsigned int addr=addr_start; addr<=addr_end; addr++)
  {
    byte val = readdata(addr);
    Serial.write(val);
  }
}

void writedata(unsigned int addr, unsigned int value)
{
  setaddress(addr);

  for(int i=0; i<8; i++)
  {
    pinMode(_D0+i, OUTPUT);
    digitalWrite(_D0+i, (value & (1 << i)) ? HIGH : LOW);
  }

  digitalWrite(wr, LOW);
  delay(50);
  digitalWrite(wr, HIGH);
  delay(50);

  for(int i=0; i<8; i++)
  {
    pinMode(_D0+i, INPUT);
  }
}

void dumprom()
{
  unsigned int carttype   = readdata(0x0147);
  unsigned int romsize    = readdata(0x0148);
  unsigned int banksize   = (0b100000 << romsize) / 16;
  unsigned int start_addr = 0x0000;

  switch(carttype)
  {
  // MBC1
  case 0x01:
  case 0x02:
  case 0x03:
    for(unsigned int i=1; i<banksize; i++)
    {
      writedata(0x6000, 0);
      writedata(0x4000, (i >> 5) & 0b11);
      writedata(0x2000, i & 0b11111);

      if(i > 1)
      {
        start_addr=0x4000;
      }
      readdata_range(start_addr, 0x7FFF);
    }
    break;

  // MBC2
  case 0x05:
  case 0x06:
    for(int i=1; i<banksize; i++)
    {
      writedata(0x2100, i);
      
      if(i > 1)
      {
        start_addr=0x4000;
      }
      readdata_range(start_addr, 0x7FFF);
    }
    break;

  // MBC3
  case 0x0F:
  case 0x10:
  case 0x11:
  case 0x12:
  case 0x13:
    for(unsigned int i=1; i<banksize; i++)
    {
      writedata(0x2000, i & 0b1111111);

      if(i > 1)
      {
        start_addr=0x4000;
      }
      readdata_range(start_addr, 0x7FFF);
    }
    break; 

  // MBC5 or Pocket camera
  case 0x19:
  case 0x1A:
  case 0x1B:
  case 0x1C:
  case 0x1D:
  case 0x1E:
  case 0xFC:
    for(unsigned int i=1; i<banksize; i++)
    {
      writedata(0x3000, (i >> 8) & 0b1);
      writedata(0x2000, i & 0b11111111);

      if(i > 1)
      {
        start_addr=0x4000;
      }
      readdata_range(start_addr, 0x7FFF);
    }  
    break;

  default:
    Serial.println("unknown type");
    break;
  }
}

void dumpram()
{
  unsigned int carttype = readdata(0x0147);
  unsigned int ramsize  = readdata(0x0149);
  unsigned int banksize = 0;

  // Initialise MBC
  writedata(0x0000, 0xA);

  if(ramsize == 0x00)
  {
    banksize = 0;
  }
  else if (ramsize == 0x01)
  {
    banksize = 2 / 8;
  }
  else if(ramsize == 0x02)
  {
    banksize = 8 / 8;
  }
  else if(ramsize == 0x03)
  {
    banksize = 32 / 8;
  }
  else if(ramsize == 0x04)
  {
    banksize = 128 / 8;
  }
  else if(ramsize == 0x05)
  {
    banksize = 64 / 8;
  }

  if(banksize==0 && !(carttype==0x05 || carttype==0x06))
  {
    Serial.println("no ram exists.");
    return;
  }

  switch(carttype)
  {
  // MBC1
  case 0x01:
  case 0x02:
  case 0x03:
    writedata(0x6000, 1);

    for(unsigned int i=0; i<banksize; i++)
    {
      writedata(0x4000, i & 0b11);
      readdata_range(0xA000, 0xBFFF);
    }    
    break;

  // MBC2
  case 0x05:
  case 0x06:
    readdata_range(0xA000, 0xA1FF);
    break;

  // MBC3
  case 0x0F:
  case 0x10:
  case 0x11:
  case 0x12:
  case 0x13:
    for(unsigned int i=0; i<banksize; i++)
    {
      writedata(0x4000, i&0b11);
      readdata_range(0xA000, 0xBFFF);
    }    
    break;

  // MBC5 or Pocket camera
  case 0x19:
  case 0x1A:
  case 0x1B:
  case 0x1C:
  case 0x1D:
  case 0x1E:
  case 0xFC:
    for(unsigned int i=0; i<banksize; i++)
    {
      writedata(0x4000, i&0b11111111);
      readdata_range(0xA000, 0xBFFF);
    }
  break;

  default:
    Serial.println("unknown type");
    break;
  }

  // Disable RAM
  writedata(0x0000, 0x0);
}

void loop()
{
  if(Serial.available() <= 0)
  {
    return;
  }

  int data = Serial.read();

  if(data == '1')
  {
    dumprom();
  }
  else if(data == '2')
  {
    dumpram();
  }
  else
  {
    Serial.println("I could not read.");
  }

  while(Serial.available() > 0)
  {
    Serial.read();
  }
}
