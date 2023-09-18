/******************************************************************************
* SPINNER driver - SPI Flasher for the Teensy 4.0
*  - Influenced by SPIway/NANDway for the PS4 from judges@eEcho.com
*  - Developed by Wildcard - github.com/VV1LD - @VVildCard777
*  - Backed and supported by Zecoxao - @notnotzecoxao
*******************************************************************************/

#include <Arduino.h>
#include <SPI.h>



//SPI commands (69)
#define SPI_COMMAND_3B_READ				0x03	//
#define SPI_COMMAND_3B_FASTREAD			0x0B	//
#define SPI_COMMAND_3B_2READ			0xBB	//
#define SPI_COMMAND_3B_DREAD			0x3B	//
#define SPI_COMMAND_3B_4READ_BOTTOM		0xEB	//
#define SPI_COMMAND_3B_4READ_TOP		0xEA	//
#define SPI_COMMAND_3B_QREAD			0x6B	//
#define SPI_COMMAND_3B_PAGEPROG			0x02	//
#define SPI_COMMAND_3B_4PAGEPROG		0x38	//
#define SPI_COMMAND_3B_SECTOR_ERASE		0x20	//
#define SPI_COMMAND_3B_BLOCK_ERASE_32K	0x52	//
#define SPI_COMMAND_3B_BLOCK_ERASE_64K	0xD8	//

#define SPI_COMMAND_4B_READ				0x13	//
#define SPI_COMMAND_4B_FASTREAD			0x0C	//
#define SPI_COMMAND_4B_2READ			0xBC	//
#define SPI_COMMAND_4B_DREAD			0x3C	//
#define SPI_COMMAND_4B_4READ			0xEC	//
#define SPI_COMMAND_4B_QREAD			0x6C	//
#define SPI_COMMAND_4B_PAGEPROG			0x12	//
#define SPI_COMMAND_4B_4PAGEPROG		0x3E	//
#define SPI_COMMAND_4B_SECTOR_ERASE		0x21	//
#define SPI_COMMAND_4B_BLOCK_ERASE_32K	0x5C	//
#define SPI_COMMAND_4B_BLOCK_ERASE_64K	0xDC	//

#define SPI_COMMAND_CHIP_ERASE			0x60	//
#define SPI_COMMAND_CHIP_ERASE2			0xC7	//
#define SPI_COMMAND_WREN				0x06	//
#define SPI_COMMAND_WRDI				0x04	//
#define SPI_COMMAND_RDSR				0x05	//
#define SPI_COMMAND_RDCR				0x15	//
#define SPI_COMMAND_WRSR				0x01	//
#define SPI_COMMAND_RDEAR				0xC8	//
#define SPI_COMMAND_WREAR				0xC5	//
#define SPI_COMMAND_WPSEL				0x68	//
#define SPI_COMMAND_EQIO				0x35	//
#define SPI_COMMAND_RSTQIO				0xF5	//
#define SPI_COMMAND_EN4B				0xB7	//
#define SPI_COMMAND_EX4B				0xE9	//
#define SPI_COMMAND_PGM_ERS_SUSPEND		0xB0	//
#define SPI_COMMAND_PGM_ERS_RESUME		0x30	//
#define SPI_COMMAND_DP					0xB9	//
#define SPI_COMMAND_RDP					0xAB	//
#define SPI_COMMAND_SBL					0xC0	//
#define SPI_COMMAND_RDFBR				0x16	//
#define SPI_COMMAND_WRFBR				0x17	//
#define SPI_COMMAND_ESFBR				0x18	//
#define SPI_COMMAND_RDID				0x9F	//
#define SPI_COMMAND_RES					0xAB	//
#define SPI_COMMAND_REMS				0x90	//
#define SPI_COMMAND_QPIID				0xAF	//
#define SPI_COMMAND_RDSFDP				0x5A	//
#define SPI_COMMAND_ENSO				0xB1	//
#define SPI_COMMAND_EXSO				0xC1	//
#define SPI_COMMAND_RDSCUR				0x2B	//
#define SPI_COMMAND_WRSCUR				0x2F	//
#define SPI_COMMAND_GBLK				0x7E	//
#define SPI_COMMAND_GBULK				0x98	//
#define SPI_COMMAND_WRLR				0x2C	//
#define SPI_COMMAND_RDLR				0x2D	//
#define SPI_COMMAND_WRPASS				0x28	//
#define SPI_COMMAND_RDPASS				0x27	//
#define SPI_COMMAND_PASSULK				0x29	//
#define SPI_COMMAND_WRSPB				0xE3	//
#define SPI_COMMAND_ESSPB				0xE4	//
#define SPI_COMMAND_RDSPB				0xE2	//
#define SPI_COMMAND_SPBLK				0xA6	//
#define SPI_COMMAND_RDSPBLK				0xA7	//
#define SPI_COMMAND_WRDPB				0xE1	//
#define SPI_COMMAND_RDDPB				0xE0	//
#define SPI_COMMAND_NOP					0x00	//
#define SPI_COMMAND_RSTEN				0x66	//
#define SPI_COMMAND_RST					0x99	//


#define SPI_STATUS_WIP 				0b00000001 // write in progress bit set
#define SPI_STATUS_WEL	 			0b00000010 // write enable bit set
#define SPI_STATUS_BP0	 			0b00000100
#define SPI_STATUS_BP1			 	0b00001000
#define SPI_STATUS_BP2			 	0b00010000
#define SPI_STATUS_BP3			 	0b00100000
#define SPI_STATUS_QE			 	0b01000000 // quad enable bit set
#define SPI_STATUS_SRWD			 	0b10000000 // status register write disable set

#define SPI_SECURITY_OTP			0b00000001 // factory lock bit set
#define SPI_SECURITY_LDSO 			0b00000010 // lock-down bit set (cannot program/erase otp)
#define SPI_SECURITY_PSB 			0b00000100 // program suspended bit set
#define SPI_SECURITY_ESB		 	0b00001000 // erase suspended bit set
#define SPI_SECURITY_RESERVED	 	0b00010000
#define SPI_SECURITY_P_FAIL		 	0b00100000 // program operation failed bit set
#define SPI_SECURITY_E_FAIL		 	0b01000000 // erase operation failed bit set
#define SPI_SECURITY_WPSEL		 	0b10000000 // status register write disable set


#define HOLD_PIN	14
#define WP_PIN		15
#define CS_PIN		10
#define SO_PIN		12
#define SI_PIN		11
#define SCLK_PIN	13

#define SPI_COMMAND_REMS 0x90


#define BUF_SIZE_RW		4096
#define BUF_SIZE_ADDR	4



// Define commands
enum {
	CMD_PING1 = 0,
	CMD_PING2,
	CMD_BOOTLOADER,
	CMD_IO_LOCK,
	CMD_IO_RELEASE,
	CMD_PULLUPS_DISABLE,
	CMD_PULLUPS_ENABLE,
	CMD_SPI_ID,
	CMD_SPI_READBLOCK,
	CMD_SPI_WRITESECTOR,
	CMD_SPI_ERASEBLOCK,
	CMD_SPI_ERASECHIP,
	CMD_SPI_3BYTE_ADDRESS,
	CMD_SPI_4BYTE_ADDRESS,
	CMD_SPI_3BYTE_CMDS,
	CMD_SPI_4BYTE_CMDS,
} cmd_t;



uint32_t 	SPI_BLOCK_SIZE = 0x10000; //64KB block size
uint8_t		SPI_USE_3B_CMDS = 0;
uint8_t		SPI_ADDRESS_LENGTH = 3;

uint8_t		buf_rw[BUF_SIZE_RW];
uint8_t		buf_rw2[BUF_SIZE_RW];
uint8_t		buf_addr[BUF_SIZE_ADDR];



SPISettings spi_settings(4000000, MSBFIRST, SPI_MODE0); 


uint8_t spi_status()
{
	uint8_t status; 

	digitalWrite(CS_PIN, LOW); 
	
	SPI.transfer(SPI_COMMAND_RDSR); 
	status = SPI.transfer(0);
	digitalWrite(CS_PIN, HIGH);

	return status;
}

int spi_busy_wait()
{
	uint8_t status; 

	digitalWrite(CS_PIN, LOW); 

	while (spi_status() & (uint8_t)SPI_STATUS_WIP)
	{
		status = SPI.transfer(0);
		//Serial1.print("status:");
		//Serial1.print(status, HEX);
		//Serial1.print("\n");
	}
	digitalWrite(CS_PIN, HIGH);
}

int spi_write_en()
{

	uint8_t status; 

	do
	{
		digitalWrite(CS_PIN, LOW); 
		SPI.transfer(SPI_COMMAND_WREN);
		digitalWrite(CS_PIN, HIGH);

	}
	while(!(spi_status()) & SPI_STATUS_WEL);
}


int spi_security()
{
	uint8_t sec;

	digitalWrite(CS_PIN, LOW); 
	SPI.transfer(SPI_COMMAND_RDSCUR);
	sec = SPI.transfer(0);
	digitalWrite(CS_PIN, HIGH); 

	return sec;
}



uint8_t maker_code;
uint8_t device_code0;
//uint8_t device_code1;

uint8_t byte_str[10];


// clear all data from rx buffer
void clear_serial_buffer(void)
{
	Serial.flush();
	while(Serial.available() > 0)
	Serial.read();
}

int spi_read_id()
{

  	SPI.beginTransaction(spi_settings);

	digitalWrite(CS_PIN, LOW);
	SPI.transfer(SPI_COMMAND_REMS);
	SPI.transfer(0);
	SPI.transfer(0);
	SPI.transfer(0);

	maker_code = SPI.transfer(0);
	device_code0 = SPI.transfer(0);
	//device_code1 = SPI.transfer(0);

	digitalWrite(CS_PIN, HIGH); 

	return 1;
}



uint8_t spi_read_block(uint32_t address, uint8_t* block_buf) 
{
	uint16_t i;
	uint8_t sreg;

	Serial1.print("reading block\n");

	// command	
	if (SPI_USE_3B_CMDS && SPI_ADDRESS_LENGTH == 4) 
	{
		digitalWrite(CS_PIN, LOW);
		SPI.transfer(SPI_COMMAND_EN4B);
		digitalWrite(CS_PIN, HIGH);
	}


	digitalWrite(CS_PIN, LOW);

	if (SPI_USE_3B_CMDS || SPI_ADDRESS_LENGTH == 3)
	{
		SPI.transfer(SPI_COMMAND_3B_READ);
	}
	else
	{
		SPI.transfer(SPI_COMMAND_4B_READ);
	}	
	
	// address
	if (SPI_ADDRESS_LENGTH == 4) 
	{
		SPI.transfer((address >> 24) & 0xff);
	}

	SPI.transfer((address >> 16) & 0xff);
	SPI.transfer((address >> 8) & 0xff);
	SPI.transfer(address & 0xff);

	for (uint8_t k = 0; k < SPI_BLOCK_SIZE / BUF_SIZE_RW; ++k) 
	{
		for (i = 0; i < BUF_SIZE_RW; ++i) 
		{
			//buf_rw[i] = SPI.transfer(0);
			*(block_buf + i) = SPI.transfer(0);
		}

		for (i = 0; i < BUF_SIZE_RW; ++i) 
		{
			sprintf(byte_str, "%02x", buf_rw[i]);
			Serial.print((char*)&byte_str);
			//Serial.print(buf_rw[i], HEX);
			Serial.print(" ");
		}
	}

	uint16_t rest = SPI_BLOCK_SIZE - ((SPI_BLOCK_SIZE / BUF_SIZE_RW) * BUF_SIZE_RW);
	
	for (i = 0; i < rest; ++i) 
	{
		//buf_rw[i] = SPI.transfer(0);
		*(block_buf + i) = SPI.transfer(0);
	}
	
	for (i = 0; i < rest; ++i) 
	{
		sprintf(byte_str, "%02x", buf_rw[i]);
		Serial.print((char*)&byte_str);
		//Serial.print(buf_rw[i], HEX);
		Serial.print(" ");
	}
	
	digitalWrite(CS_PIN, HIGH);

	return 1;
}



uint8_t spi_erase_block(uint32_t address) 
{
	uint16_t i;
	uint8_t sreg;

	Serial1.print("erasing block\n");

	// 4b support	
	if (SPI_USE_3B_CMDS && SPI_ADDRESS_LENGTH == 4) 
	{
		digitalWrite(CS_PIN, LOW);
		SPI.transfer(SPI_COMMAND_EN4B);
		digitalWrite(CS_PIN, HIGH);
	}

	digitalWrite(WP_PIN, HIGH);
	spi_write_en();

	digitalWrite(CS_PIN, LOW);


	// command
	if (SPI_USE_3B_CMDS || SPI_ADDRESS_LENGTH == 3)
	{
		SPI.transfer(SPI_COMMAND_3B_BLOCK_ERASE_64K);
	}
	else
	{
		SPI.transfer(SPI_COMMAND_4B_BLOCK_ERASE_64K);
	}	
	
	// address
	if (SPI_ADDRESS_LENGTH == 4) 
	{
		SPI.transfer((address >> 24) & 0xff);
	}

	SPI.transfer((address >> 16) & 0xff);
	SPI.transfer((address >> 8) & 0xff);
	SPI.transfer(address & 0xff);


	digitalWrite(CS_PIN, HIGH);

	spi_busy_wait();
	
	digitalWrite(WP_PIN, LOW);


	return 1;
}


static uint8_t block_data[0x1000];

uint8_t spi_write_block(uint32_t address) 
{
	uint8_t in_data;
	int ret = 1;
	uint8_t byte_str[10];
	int available = 0;


	Serial1.print("writing block\n");

	Serial1.print("parsing block data\n");


	for(int i = 0; i < 0x1000; i++)
		block_data[i] = 0;


	uint32_t j = 0;
	
	while(j < 0x1000)
	{
		while(Serial.available() > 0) 
		{
			block_data[j] = Serial.read();
			j++;
		}
	}


	// 4b support	
	if (SPI_USE_3B_CMDS && SPI_ADDRESS_LENGTH == 4) 
	{
		digitalWrite(CS_PIN, LOW);
		SPI.transfer(SPI_COMMAND_EN4B);
		digitalWrite(CS_PIN, HIGH);
	}

	for (uint8_t k = 0; k < 16; k++) // 16
	{

		digitalWrite(WP_PIN, HIGH);
		spi_write_en();
		digitalWrite(CS_PIN, LOW);

		// command
		if (SPI_USE_3B_CMDS || SPI_ADDRESS_LENGTH == 3)
		{
			SPI.transfer(SPI_COMMAND_3B_PAGEPROG);
		}
		else
		{
			SPI.transfer(SPI_COMMAND_4B_PAGEPROG);
		}	

		// address
		if (SPI_ADDRESS_LENGTH == 4) 
		{
			SPI.transfer((address >> 24) & 0xff);
		}

		SPI.transfer((address >> 16) & 0xff);
		SPI.transfer(((address >> 8) & 0xff) | k);
		SPI.transfer(address & 0xff);

		for (uint16_t i = 0; i < 0x100 ; i++) // 0x1000
		{			
			SPI.transfer(block_data[(k * 0x100) + i]);
		}

		digitalWrite(CS_PIN, HIGH);
		spi_busy_wait();
		digitalWrite(WP_PIN, LOW);

	}

	Serial1.printf("here\n");
	Serial.print('K');
	clear_serial_buffer();


	Serial1.print("exiting write\n");

	return 1;
}


int spi_erase_chip(void)
{
	Serial1.print("erasing chip\n");

	digitalWrite(WP_PIN, HIGH);
	spi_write_en();

	digitalWrite(CS_PIN, LOW);
	SPI.transfer(SPI_COMMAND_CHIP_ERASE);
	digitalWrite(CS_PIN, HIGH);

	spi_busy_wait();
	digitalWrite(WP_PIN, LOW);

	// TODO: check status
	uint8_t sec = spi_security();
	Serial1.print("sec:");
	Serial1.print(sec, HEX);
	Serial1.print("\n");

	return 1;
}


extern "C" int main(void)
{


	int16_t command = 0xaa;
	uint16_t freemem;
	uint8_t space;
	uint8_t addr_str[10];


	int block_index;


	Serial.begin(500000); 
	Serial1.begin(500000);

	Serial1.print("debug start\n");

	/*
		Teensy 4.0

		CS = pin10
		SI = pin11
		SO = pin12
		SCLK = pin13

		HOLD/RESET = pin14
		WP = pin15
	*/

	pinMode(CS_PIN, OUTPUT);
	pinMode(SI_PIN, OUTPUT);
	pinMode(SO_PIN, INPUT);
	pinMode(SCLK_PIN, OUTPUT);

	pinMode(HOLD_PIN, OUTPUT);
	pinMode(WP_PIN, OUTPUT);


	SPI.begin(); 


	delay(1000); // delay for 1 second

	while(1)
	{
		if(Serial.available() > 0) 
		{
			command = Serial.read();
			Serial1.print("command: ");
			Serial1.print(command, HEX);
			Serial1.print("\n");

			if(command == 0x32 || command == 0x33 || command == 0x34)
			{
				while(Serial.available() < 7)
				{
				}
			}

			if(command == 0x32 || command == 0x33 || command == 0x34)
			{
				
				space = Serial.read();

				if(space != 0x20)
					break;
				
				Serial1.print("space: ");
				Serial1.print(space, HEX);
				Serial1.print("\n");			

				for(int i = 0; i < SPI_ADDRESS_LENGTH * 2; i++)
				{
					addr_str[i] = Serial.read();	
				}

				Serial1.print("decoded block index: ");

				block_index = strtol(addr_str, NULL, 16);
				Serial1.print(block_index, HEX);
				Serial1.print("\n");			
			}

			else if(command == 0x35)
			{
				space = Serial.read();
				if(space != 0x20)
					break;
				
				Serial1.print("space: ");
				Serial1.print(space, HEX);
				Serial1.print("\n");	
			}
		}

		switch(command)
		{
			case 0x31:
				Serial.print('K');
				spi_read_id();
				sprintf(byte_str, "%02x", maker_code);
				Serial.print((char*)&byte_str);
				Serial.print(" ");
				sprintf(byte_str, "%02x", device_code0);
				Serial.print((char*)&byte_str);
				Serial1.print("=======================\n\n");
				break;
			
			case 0x32:
				Serial.print('K');
				spi_read_block(block_index, (uint8_t*)&buf_rw);
				break;

			case 0x33:
				clear_serial_buffer();
				spi_write_block(block_index);
				//Serial.print('K');
				clear_serial_buffer();

				break;
			
			case 0x34:
				spi_erase_block(block_index);
				Serial.print('K');
				break;

			case 0x35:
				spi_erase_chip();
				Serial.print('K');
				break;			



			default:
				//Serial1.print("command not supported\n");
				break;

		}
		command = -1;

	}

}

