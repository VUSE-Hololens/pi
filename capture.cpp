#include "capture.h"

capture::capture()
{
	myCAM = ArduCAM(OV2640,CAM1_CS);
	uint8_t vid, pid;
	wiring_init();
	pinMode(CAM1_CS, OUTPUT);
	// Check if the ArduCAM SPI bus is OK
	myCAM.write_reg(ARDUCHIP_TEST1, 0x72);
	uint8_t temp = myCAM.read_reg(ARDUCHIP_TEST1);
	if(temp != 0x72) {
			printf("SPI interface error!\n");
			exit(EXIT_FAILURE);
	}

	// Change MCU mode
	myCAM.write_reg(ARDUCHIP_MODE, 0x00);
	// Check if the camera module type is OV2640
	myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
	myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);

	if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 ))){
			printf("Can't find OV2640 module!\n");
			exit(EXIT_FAILURE);
	} else {
			printf("OV2640 detected\n");
	}

	myCAM.set_format(JPEG);
	myCAM.InitCAM();
}
capture::~capture()
{

}

void capture::startCaptureAndTransmit() {
	uint8_t* buf;
	while(1) {
		myCAM.flush_fifo();
		// Clear the capture done flag
		myCAM.clear_fifo_flag();
		// Start capture
		myCAM.start_capture();
		while (!(myCAM.read_reg(ARDUCHIP_TRIG) & CAP_DONE_MASK));

		size_t length = myCAM.read_fifo_length();
		if (length >= OV2640_MAX_FIFO_SIZE) {
		 printf("Over size.");
			exit(EXIT_FAILURE);
		} else if (length == 0 ){
			printf("Size is 0.");
			exit(EXIT_FAILURE);
		}

		int32_t i = 0;

		myCAM.CS_HIGH();  //Set CS low

		int size = length;
		buf = new uint8_t[size];

		while ( length-- )
		{
			buf[i++] = myCAM.read_reg(0x3D);
		}
		// Don't need to free buf because transmit class takes care of it
	transmitter.transmitRGBPreCompressed(buf, size);
	}
}

void capture::setImageSize() {
	// TODO: complete me. Will likely need synchronization
}
