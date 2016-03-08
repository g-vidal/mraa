/*
 * Author: Brendan Le Foll
 * Copyright (c) 2016 Intel Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <unistd.h>

#include "mraa.h"
#include "mraa/firmata.h"

#define FIRMATA_START_SYSEX 0xF0
#define FIRMATA_END_SYSEX 0xF7
#define FIRMATA_I2C_REPLY 0x77
#define FIRMATA_I2C_REQUEST 0x76
#define I2C_MODE_READ 0x01

void
interrupt(uint8_t* buf, int length)
{
    printf("reg read returned: %d, with buffer size %d\n", ((buf[6] & 0x7f) | ((buf[7] & 0x7f) << 7)), length);
}

int
main()
{
    mraa_init();
    //! [Interesting]

   /**
    * This example reads from a firmata device the check buffer on a BMP085 on
    * 0xD0 which should return 0x55. Obviously I2C_REPLY has to be disabled in
    * firmata_pull for this to work breaking all i2c support for firmata
    */

    mraa_add_subplatform(MRAA_GENERIC_FIRMATA, "/dev/ttyACM0");
    mraa_firmata_context firm = mraa_firmata_init(FIRMATA_I2C_REPLY);
    if (firm == NULL) {
        return EXIT_FAILURE;
    }

    mraa_firmata_response(firm, interrupt);

    uint8_t* buffer = calloc(9, 0);
    if (buffer == NULL) {
        free(firm);
        return EXIT_FAILURE;
    }
    buffer[0] = FIRMATA_START_SYSEX;
    buffer[1] = FIRMATA_I2C_REQUEST;
    buffer[2] = 0x77;
    buffer[3] = I2C_MODE_READ << 3;

    // register to read from
    buffer[4] = 0xD0 & 0x7f;
    buffer[5] = (0xD0 >> 7) & 0x7f;
    // number of bytes
    buffer[6] = 1 & 0x7f;
    buffer[7] = (1 >> 7) & 0x7f;
    buffer[8] = FIRMATA_END_SYSEX;

    mraa_firmata_write_sysex(firm, buffer, 9);

    sleep(1);

    // stop the isr and set it again
    mraa_firmata_response_stop(firm);
    mraa_firmata_response(firm, interrupt);
    mraa_firmata_write_sysex(firm, buffer, 9);

    sleep(1);

    // close everything and try again
    mraa_firmata_close(firm);
    firm = mraa_firmata_init(FIRMATA_I2C_REPLY);
    mraa_firmata_response(firm, interrupt);
    mraa_firmata_write_sysex(firm, buffer, 9);

    sleep(10);

    mraa_firmata_close(firm);
    //! [Interesting]

    return EXIT_SUCCESS;
}
