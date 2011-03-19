/*
 Copyright (C) 2011 James Coliz, Jr. <maniacbug@ymail.com>
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include <WProgram.h>
#include <SPI.h>
#include "RF24.h"
#include "nRF24L01.h"

#undef SERIAL_DEBUG
#ifdef SERIAL_DEBUG
#define IF_SERIAL_DEBUG(x) (x)
#else
#define IF_SERIAL_DEBUG(x)
#endif

/******************************************************************/

void RF24::csn(int mode) 
{
  digitalWrite(csn_pin,mode);
}

/******************************************************************/

void RF24::ce(int mode)
{
  digitalWrite(ce_pin,mode);
}

/******************************************************************/

uint8_t RF24::read_register(uint8_t reg, uint8_t* buf, uint8_t len) 
{
  uint8_t status;

  csn(LOW);
  status = SPI.transfer( R_REGISTER | ( REGISTER_MASK & reg ) );
  while ( len-- )
    *buf++ = SPI.transfer(0xff);

  csn(HIGH);

  return status;
}

/******************************************************************/

uint8_t RF24::write_register(uint8_t reg, const uint8_t* buf, uint8_t len)
{
  uint8_t status;

  csn(LOW);
  status = SPI.transfer( W_REGISTER | ( REGISTER_MASK & reg ) );
  while ( len-- )
    SPI.transfer(*buf++);

  csn(HIGH);

  return status;
}

/******************************************************************/

uint8_t RF24::write_register(uint8_t reg, uint8_t value)
{
  uint8_t status;

  csn(LOW);
  status = SPI.transfer( W_REGISTER | ( REGISTER_MASK & reg ) );
  SPI.transfer(value);
  csn(HIGH);

  return status;
}

/******************************************************************/

uint8_t RF24::write_payload(const void* buf)
{
  uint8_t status;

  const uint8_t* current = (const uint8_t*)buf;

  csn(LOW);
  status = SPI.transfer( W_TX_PAYLOAD );
  uint8_t len = payload_size;
  while ( len-- )
    SPI.transfer(*current++);

  csn(HIGH);

  return status;
}

/******************************************************************/

uint8_t RF24::read_payload(void* buf) 
{
  uint8_t status;
  uint8_t* current = (uint8_t*)buf;

  csn(LOW);
  status = SPI.transfer( R_RX_PAYLOAD );
  uint8_t len = payload_size;
  while ( len-- )
    *current++ = SPI.transfer(0xff);
  csn(HIGH);

  return status;
}

/******************************************************************/

uint8_t RF24::flush_rx(void)
{
  uint8_t status;

  csn(LOW);
  status = SPI.transfer( FLUSH_RX );  
  csn(HIGH);

  return status;
}

/******************************************************************/

uint8_t RF24::flush_tx(void)
{
  uint8_t status;

  csn(LOW);
  status = SPI.transfer( FLUSH_TX );  
  csn(HIGH);

  return status;
}

/******************************************************************/

uint8_t RF24::get_status(void) 
{
  uint8_t status;

  csn(LOW);
  status = SPI.transfer( NOP );
  csn(HIGH);

  return status;
}

/******************************************************************/

void RF24::print_status(uint8_t status)
{
  printf("STATUS=%02x: RX_DR=%x TX_DS=%x MAX_RT=%x RX_P_NO=%x TX_FULL=%x\n\r",
  status,
  (status & _BV(RX_DR))?1:0,
  (status & _BV(TX_DS))?1:0,
  (status & _BV(MAX_RT))?1:0,
  ((status >> RX_P_NO) & B111),
  (status & _BV(TX_FULL))?1:0
    );
}

/******************************************************************/

void RF24::print_observe_tx(uint8_t value) 
{
  printf("OBSERVE_TX=%02x: POLS_CNT=%x ARC_CNT=%x\n\r",
  value,
  (value >> PLOS_CNT) & B1111,
  (value >> ARC_CNT) & B1111
    );
}

/******************************************************************/

RF24::RF24(int _cepin, int _cspin): 
  ce_pin(_cepin), csn_pin(_cspin)
{
}

/******************************************************************/

void RF24::setChannel(int channel)
{
  write_register(RF_CH,min(channel,127));  
}

/******************************************************************/

void RF24::setPayloadSize(uint8_t size)
{
  payload_size = min(size,32);
  write_register(RX_PW_P0,min(size,32));
}

/******************************************************************/

uint8_t RF24::getPayloadSize(void) 
{
  return payload_size;
}

/******************************************************************/

void RF24::print_details(void) 
{
  uint8_t buffer[5];
  uint8_t status = read_register(RX_ADDR_P0,buffer,5);
  print_status(status);
  printf("RX_ADDR_P0 = 0x",buffer);
  uint8_t *bufptr = buffer + 5;
  while( bufptr-- > buffer )
    printf("%02x",*bufptr);
  printf("\n\r");

  status = read_register(RX_ADDR_P1,buffer,5);
  printf("RX_ADDR_P1 = 0x",buffer);
  bufptr = buffer + 5;
  while( bufptr-- > buffer )
    printf("%02x",*bufptr);
  printf("\n\r");

  status = read_register(TX_ADDR,buffer,5);
  printf("TX_ADDR = 0x",buffer);
  bufptr = buffer + 5;
  while( bufptr-- > buffer )
    printf("%02x",*bufptr);
  printf("\n\r");

  read_register(EN_AA,buffer,1);
  printf("EN_AA = %02x\n\r",*buffer);

  read_register(EN_RXADDR,buffer,1);
  printf("EN_RXADDR = %02x\n\r",*buffer);

  read_register(RF_CH,buffer,1);
  printf("RF_CH = %02x\n\r",*buffer);  
}

/******************************************************************/

void RF24::begin(void)
{
  pinMode(ce_pin,OUTPUT);
  pinMode(csn_pin,OUTPUT);

  ce(LOW);
  csn(HIGH);

  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV8);

  // Set generous timeouts, to make testing a little easier
  write_register(SETUP_RETR,(B1111 << ARD) | (B1111 << ARC));

  // Reset current status
  write_register(STATUS,_BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );

  // Flush buffers
  flush_rx();
  flush_tx();    

  // Set up default configuration.  Callers can always change it later.
  setChannel(1);
  setPayloadSize(8);
}

/******************************************************************/

void RF24::startListening(void)
{
  write_register(CONFIG, _BV(EN_CRC) | _BV(PWR_UP) | _BV(PRIM_RX));
  write_register(STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );

  // Flush buffers
  flush_rx();

  // Go!  
  ce(HIGH);

  // wait for the radio to come up (130us actually only needed)
  delay(1);
}

/******************************************************************/

void RF24::stopListening(void)
{
  ce(LOW);
}

/******************************************************************/

boolean RF24::write( const void* buf )
{
  boolean result = false;

  // Transmitter power-up
  write_register(CONFIG, _BV(EN_CRC) | _BV(PWR_UP));

  // Send the payload
  write_payload( buf );

  // Allons!
  ce(HIGH);

  // IN the end, the send should be blocking.  It comes back in 60ms worst case, or much faster
  // if I tighted up the retry logic.  (Default settings will be 750us.
  // Monitor the send
  uint8_t observe_tx;
  uint8_t status;
  do
  {
    status = read_register(OBSERVE_TX,&observe_tx,1);
    IF_SERIAL_DEBUG(Serial.print(status,HEX));
    IF_SERIAL_DEBUG(Serial.print(observe_tx,HEX));
  }
  while( ! ( status & ( _BV(TX_DS) | _BV(MAX_RT) ) ) );

  if ( status & _BV(TX_DS) )
    result = true;

  IF_SERIAL_DEBUG(Serial.println(result?"...OK.":"...Failed"));

  // Yay, we are done.
  ce(LOW);

  // Power down
  write_register(CONFIG, _BV(EN_CRC) );

  // Reset current status
  write_register(STATUS,_BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );

  // Flush buffers
  flush_tx();  

  return result;
}

/******************************************************************/

boolean RF24::available(void) 
{
  uint8_t status = get_status();
  boolean result = ( status & _BV(RX_DR) );

  if (result)
  {
    IF_SERIAL_DEBUG(print_status(status));

    // Clear the status bit
    write_register(STATUS,_BV(RX_DR) );
  }

  return result;
}

/******************************************************************/

boolean RF24::read( void* buf ) 
{
  // was this the last of the data available?
  boolean result = false;

  // Fetch the payload
  read_payload( buf );

  uint8_t fifo_status;
  read_register(FIFO_STATUS,&fifo_status,1);
  if ( fifo_status & _BV(RX_EMPTY) )
    result = true;

  return result;
}

/******************************************************************/

void RF24::openWritingPipe(uint64_t value)
{
  // Note that AVR 8-bit uC's store this LSB first, and the NRF24L01
  // expects it LSB first too, so we're good.  
  
  write_register(RX_ADDR_P0, reinterpret_cast<uint8_t*>(&value), 5);
  write_register(TX_ADDR, reinterpret_cast<uint8_t*>(&value), 5);  
}

/******************************************************************/

void RF24::openReadingPipe(uint8_t child, uint64_t value)
{
  const uint8_t child_pipe[] = { 
    RX_ADDR_P1, RX_ADDR_P2, RX_ADDR_P3, RX_ADDR_P4, RX_ADDR_P5   };
  const uint8_t child_payload_size[] = { 
    RX_PW_P1, RX_PW_P2, RX_PW_P3, RX_PW_P4, RX_PW_P5   };
  const uint8_t child_pipe_enable[] = { 
    ENAA_P1, ENAA_P2, ENAA_P3, ENAA_P4, ENAA_P5   };

  if (--child < 5)
  {
    write_register(child_pipe[child], reinterpret_cast<uint8_t*>(&value), 5);    
    write_register(child_payload_size[child],payload_size);  

    // Note this is kind of an inefficient way to set up these enable bits, bit I thought it made
    // the calling code more simple
    uint8_t en_aa;
    read_register(EN_AA,&en_aa,1);
    en_aa |= _BV(child_pipe_enable[child]);
    write_register(EN_AA,en_aa);
  }
}
