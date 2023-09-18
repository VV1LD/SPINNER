############################################################################################################
#   
#
#
#
############################################################################################################

import serial, time, datetime, sys, struct
import binascii


class Serial_Error(Exception):
    pass

class Serial(object):

    BUFSIZE = 32768

    def __init__(self, port):
        self.ser = serial.Serial(port, 500000, timeout=500, rtscts=False, dsrdtr=False, xonxoff=False, writeTimeout=500)
        if self.ser is None:
            raise Serial_Error("could not open serial %s")%port
        self.ser.flushInput()
        self.ser.flushOutput()
        self.obuf = bytearray()

    def write(self, s):

        #if isinstance(s,int):
        #    s = chr(s)

        #elif isinstance(s,tuple) or isinstance(s,list):
        #    s = ''.join([chr(c) for c in s])
		
        self.obuf += s
		
        while len(self.obuf) > self.BUFSIZE:
            self.ser.write(self.obuf[:self.BUFSIZE])
            self.obuf = self.obuf[self.BUFSIZE:]

    def flush(self):
		
        if len(self.obuf):
            self.ser.write(self.obuf)
            self.ser.flush()
            self.obuf = ""

    def read(self, size):
        self.flush()
        data = self.ser.read(size)
        return data

    def readbyte(self):
        return ord(self.read(1))

    def close(self):
		
        print()
        print("Closing serial device...")
        if self.ser is None:
            print("Device already closed.")
        else:
            self.ser.close()
            print("Done.")
    

class SPI_TOOL(Serial):
    CMD_SPI_ID = 1
    CMD_SPI_READBLOCK = 2


    def __init__(self, port):
        if port:
            Serial.__init__(self, port)


    def readid(self):
        self.flush()
        self.ser.write(b'1')

        if(self.read_result() == 0):
            return "error"

        spi_info = self.ser.read(5).decode('utf-8')

        #print(spi_info)        

        self.MF_ID = int(spi_info.split(" ")[0], 16)
        self.DEVICE_ID = int(spi_info.split(" ")[1], 16)

    def read_result(self):
        res = self.ser.read(1).decode('utf-8')

        error_msg = ""

        if(res != 'K'):
            if(res == 'T'):
                error_msg = "RY/BY timeout error while writing!"
            elif(res == 'R'):
                self.close()
                raise Serial_Error("Teensy recieve buffer timeout! Disconnect and reconnect Teensy!")
            elif(res == 'V'):
                error_msg = "Verification error!"
            elif(res == 'P'):
                error_msg = "Device is write-protected!"
            else:
                self.close()
                raise Serial_Error("Received unknown error! (Got %s)"%res)
            
            print(error_msg)
            return 0

        return 1


    def read_block(self, block_index):
        block_data = None

        if self.MF_ID == 0xEF:
            if self.DEVICE_ID == 0x17:

                self.ADDRESS = self.SPI_BLOCK_SIZE * block_index

                #print("address: " + format((self.ADDRESS), '#08x'))
                self.address_string = format((self.ADDRESS), '#08x').split("0x")[1]

                self.flush()

                #print("reading block")
                
                self.ser.write(b'2')
                self.ser.write(b' ')
                #self.ser.write(bytes.fromhex(format(block_index, '#04x').split('0x')[1]))

                self.ser.write(bytes(self.address_string[0:2], 'utf-8'))
                self.ser.write(bytes(self.address_string[2:4], 'utf-8'))
                self.ser.write(bytes(self.address_string[4:6], 'utf-8'))

                if(self.read_result() == 0):
                    return "error"

                block_data = self.ser.read((0x10000*3)).decode('utf-8')
                block_data = block_data.replace(" ", "")
                block_data = binascii.unhexlify(block_data)

                self.flush()
                #self.close()
        return block_data


    def write_sector(self, data, sector):

        if(len(data) != self.SPI_SECTOR_SIZE):
            print("Incorrect data size 0x%x"%(len(data)))
        
        self.ser.write(b'3')
        self.ser.write(b' ')

        self.ADDRESS = sector * self.SPI_SECTOR_SIZE
        #print("address: 0x%x"%self.ADDRESS)
        

        self.address_string = format((self.ADDRESS), '#08x').split("0x")[1]

        self.ser.write(bytes(self.address_string[0:2], 'utf-8'))
        self.ser.write(bytes(self.address_string[2:4], 'utf-8'))
        self.ser.write(bytes(self.address_string[4:6], 'utf-8'))

        for b in data:
            self.ser.write(struct.pack('>B', b))

        result = self.read_result()


        if(result == 0):
            return 0

        return 1


    def erase_chip(self):
        self.ser.write(b'5')
        self.ser.write(b' ')

        if(self.read_result() == 0):
            print("Error erasing chip!")
            return 0

        return 1


    def erase_block(self, block_index):

        self.ADDRESS = self.SPI_BLOCK_SIZE * block_index

        self.address_string = format((self.ADDRESS), '#08x').split("0x")[1]

        self.flush()

        self.ser.write(b'4')
        self.ser.write(b' ')

        self.ser.write(bytes(self.address_string[0:2], 'utf-8'))
        self.ser.write(bytes(self.address_string[2:4], 'utf-8'))
        self.ser.write(bytes(self.address_string[4:6], 'utf-8'))

        if self.read_result() == 0:
            print("Block %d - error erasing block"%(block_index))
            return 0

        return 1

    def program_block(self, block_data, pgblock, verify):
        datasize = len(block_data)

        if(datasize != self.SPI_BLOCK_SIZE):
            print("Incorrect length %d != %d!"%(datasize, self.SPI_BLOCK_SIZE))
            return -1

        sectornr = 0

        while sectornr < self.SPI_SECTORS_PER_BLOCK:
            real_sectornr = (pgblock * self.SPI_SECTORS_PER_BLOCK) + sectornr
            #print("real_sectornr " + str(real_sectornr))

            if(sectornr == 0):
                self.erase_block(pgblock)
            
            self.write_sector(block_data[sectornr * self.SPI_SECTOR_SIZE:(sectornr+1) * self.SPI_SECTOR_SIZE], real_sectornr)
            #while True:
            sectornr += 1


        if(verify == 1):
            if block_data != self.read_block(pgblock):
                print()
                print("Error! Block verification failed (block=%d)."%(pgblock))
                return -1

        return 0

    def printstate(self):

        print("SPI info")
        print("========================")

        self.readid()
        print()

        if(self.MF_ID == 0xEF):
            print("Chip manufacturer: Winbond (0x%02x)"%self.MF_ID)      
            if(self.DEVICE_ID == 0x17):
                print("Chip type:         W25Q128 (0x%02x)"%self.DEVICE_ID)
                self.SPI_BLOCK_COUNT = 256
                self.SPI_SECTORS_PER_BLOCK = 16
                self.SPI_SECTOR_SIZE = 0x1000
                self.SPI_TOTAL_SECTORS = self.SPI_SECTORS_PER_BLOCK * self.SPI_BLOCK_COUNT
                self.SPI_BLOCK_SIZE = self.SPI_SECTORS_PER_BLOCK * self.SPI_SECTOR_SIZE
                self.SPI_ADDRESS_LENGTH = 3
                self.SPI_USE_3BYTE_CMDS = 0          

            else:
                print("Chip type:         unknown (0x%02x)"%self.DEVICE_ID)
                self.close()
                sys.exit(1)

        else:
            print("Chip manufacturer: unknown (0x%02x)"%self.MF_ID)      
            print("Chip type:         unknown (0x%02x)"%self.DEVICE_ID)
            self.close()
            sys.exit(1)

        print()
        if(self.SPI_BLOCK_SIZE * self.SPI_BLOCK_COUNT / 1024) <= 8192:
            print("Chip size:         %d KB"%(self.SPI_BLOCK_SIZE * self.SPI_BLOCK_COUNT / 1024))
        else:
            print("Chip size:         %d MB"%(self.SPI_BLOCK_SIZE * self.SPI_BLOCK_COUNT / 1024 / 1024))     

        print("Sector size:       %d bytes"%(self.SPI_SECTOR_SIZE))
        print("Block size:        %d bytes"%(self.SPI_BLOCK_SIZE))
        print("Sectors per block: %d"%(self.SPI_SECTORS_PER_BLOCK))
        print("Number of blocks:  %d"%(self.SPI_BLOCK_COUNT))
        print("Number of sectors: %d"%(self.SPI_TOTAL_SECTORS))
        
        return



    def dump(self, file, block_offset, nblocks):

        f = open(file, "wb")

        if(nblocks == 0):
            nblocks = self.SPI_BLOCK_COUNT

        if(nblocks > self.SPI_BLOCK_COUNT):
            nblocks = self.SPI_BLOCK_COUNT

        for block in range(block_offset, (block_offset+nblocks), 1):
            block_data = n.read_block(block)
            f.write(block_data)
            print("\r%d KB / %d KB"%((block-block_offset+1)*self.SPI_BLOCK_SIZE/1024, nblocks*self.SPI_BLOCK_SIZE/1024), end ='')
            self.flush()

        f.close()

        return


    def program(self, file, block_offset, nblocks, verify):

        flash_data = open(file, "rb").read()
        datasize = len(flash_data)

        if nblocks == 0:
            nblocks = self.SPI_BLOCK_COUNT - block_offset

        if datasize % self.SPI_BLOCK_SIZE:
            print("Error: expecting file size to be a multiple of the chips block size: %d"%(self.SPI_BLOCK_SIZE))
            return -1
        
        if block_offset + nblocks > datasize/self.SPI_BLOCK_SIZE:
            print("Error: file is 0x%x bytes long and last block is at 0x%x !"%(datasize, (block_offset + nblocks + 1) * self.SPI_BLOCK_SIZE))
            return -1

        if block_offset + nblocks > self.SPI_BLOCK_COUNT:
            print("Error: chip has %d blocks. Writing outside the chips capacity!"%(self.SPI_BLOCK_COUNT, block_offset + nblocks + 1))
            return -1

        block = 0

        print("Writing %d blocks to device (starting at offset 0x%x)..."%(nblocks, block_offset))

        while block < nblocks:
            pgblock = block + block_offset
            self.program_block(flash_data[pgblock * self.SPI_BLOCK_SIZE:(pgblock+1)*self.SPI_BLOCK_SIZE], pgblock, verify)
            print("\r%d KB / %d KB"%( (((block+1)*self.SPI_BLOCK_SIZE)/1024), (nblocks*self.SPI_BLOCK_SIZE)/1024), end ='')
            self.flush()

            block += 1
        
        print()


    def printhelp(self):
        print("Usage:")
        print("spinner.py serial_port command")
        print()
        print("serial_port = eg. COM4, /dev/ttyUSB0")
        print()
        print("Commands:")
        print()
        print("     info")
        print("         - Displays chip information")
        print()
        print("     dump")
        print("         - Dumps to a file at offset/length put in the 2 params after command")
        print()
        print("     write/vwrite")
        print("         - Flashes (v=verify) a file to the chip, using offset/length set in params")
        print()
        print("     erasechip")
        print("         - Erases the entire chip")
        print()
        print("     test *WIP")
        print("         - will read data over and over from the chip and compare. will report OK/NG to rule out wiring issues")
        print()        
        print("* offsets/lengths can be decimal, and offsets supports hex but must be written with the 0x prefix")       
        print("* offsets/lengths will also be block aligned only")       
        print()
        print()
        print("Example usage:")
        print("     spinner.py /dev/ttyUSB0 info")
        print("     spinner.py /dev/ttyUSB0 dump ./spiflash.bin")
        print("     spinner.py /dev/ttyUSB0 dump ./spiflash.bin 0 50")
        print("     spinner.py /dev/ttyUSB0 dump ./spiflash.bin 0x1000 20")
        print("     spinner.py /dev/ttyUSB0 write ./spiflash.bin")
        print("     spinner.py /dev/ttyUSB0 write ./spiflash.bin 0 50")
        print("     spinner.py /dev/ttyUSB0 vwrite ./spiflash.bin")
        print("     spinner.py /dev/ttyUSB0 vwrite ./spiflash.bin 0 50")
        print("     spinner.py /dev/ttyUSB0 erasechip")        
        print("     spinner.py /dev/ttyUSB0 test")        

if __name__ == "__main__":


    print("SPINNER - SPI Flash Tool")
    print("Developed by Wildcard")
    print("2023 - github.com/VV1LD @VVildCard777")
    print("Backed by Zecoxao @notnotzecoxao")
    print()

    if(len(sys.argv) < 1):
        n = SPI_TOOL("")
        n.printhelp()
        sys.exit(0)

    elif(len(sys.argv) > 1):
        n = SPI_TOOL(sys.argv[1])

        tStart = time.time()

        if len(sys.argv) == 3 and sys.argv[2] == "info":
            n.printstate()
            print()
            sys.exit(0)

        if len(sys.argv) in (4,5,6) and sys.argv[2] == "dump":
            n.printstate()

            print()
            print("Dumping...")
            sys.stdout.flush()
            print()

            block_offset = 0
            nblocks = 0

            if len(sys.argv) == 5:
                block_offset=int(sys.argv[4])
            elif len(sys.argv) == 6:
                block_offset=int(sys.argv[4])
                nblocks=int(sys.argv[5])

            n.dump(sys.argv[3], block_offset, nblocks)

            print()
            print("Done. [%s]"%(datetime.timedelta(seconds=time.time() - tStart)))
        

        elif len(sys.argv) in (4,5,6) and sys.argv[2] == "write" or sys.argv[2] == "vwrite":
            n.printstate()

            print()
            print("Writing...")
            sys.stdout.flush()
            print()

            block_offset = 0
            nblocks = 0
            verify = 0

            if(sys.argv[2] == "vwrite"):
                verify = 1


            if len(sys.argv) == 5:
                block_offset=int(sys.argv[4])
            elif len(sys.argv) == 6:
                block_offset=int(sys.argv[4])
                nblocks=int(sys.argv[5])

            n.program(sys.argv[3], block_offset, nblocks, verify)

            print()
            print("Done. [%s]"%(datetime.timedelta(seconds=time.time() - tStart)))


        elif len(sys.argv) == 3 and sys.argv[2] == "erasechip":
            n.printstate()
            print()
            print("Erasing chip...")
            n.erase_chip()
            print()
            print("Done. [%s]"%(datetime.timedelta(seconds=time.time() - tStart)))

        else:
            n.printhelp()
            sys.exit(0)

        #self.close()
        sys.exit()

    

