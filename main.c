#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_DATA_LENGTH 16
#define STOP_CHAR '\n'

typedef struct {
	uint8_t address;
    char command_type;
    uint8_t reg;
    char data[17];
    uint8_t checksum;
} FrameData;

// range 0-15, 255 on invalid hex digit
uint8_t hex_char_to_uint8(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return 255;
}

char uint8_to_hex_char(uint8_t n) {
	if (n < 10) {
		return n + '0';
	}
	if (n < 16) {
		return n + 'a';
	}
	return '-';
}

int recv_frame(char* buf, FrameData* frame_data) {
    if (buf[0] != ':') {
        printf("Invalid start character: %c\n", buf[0]);
        return 1;
    }
    // buf[1] ignored, always '0'
    frame_data->address = hex_char_to_uint8(buf[2]);
    if (frame_data->address == 255) {
        printf("Invalid address character: %c\n", buf[2]);
        return 2;
    }

    frame_data->command_type = buf[3];
    frame_data->reg = hex_char_to_uint8(buf[4]);

    if (frame_data->command_type != 'W' && frame_data->command_type != 'R' && frame_data->command_type != 'N') {
        printf("Invalid command type: %c\n", frame_data->command_type);
        return 3;
    }

    uint8_t computed_checksum = buf[0] + buf[1] + buf[2] + buf[3] + buf[4];
    uint8_t data_i = 0;
    while (1) {
        if (buf[5 + data_i] == STOP_CHAR) {
            break;
        }
        if (data_i == MAX_DATA_LENGTH + 3) {
            printf("Data too long, stop char not found\n");
            return 4;
        }
        computed_checksum += buf[5 + data_i];
        ++data_i;
    }

    uint8_t data_len = data_i - 2;
    char read_checksum_higher = buf[5 + data_len];
    char read_checksum_lower = buf[6 + data_len];
    computed_checksum -= read_checksum_higher + read_checksum_lower;
    uint8_t read_checksum = 16 * hex_char_to_uint8(read_checksum_higher) + hex_char_to_uint8(read_checksum_lower);
    if (read_checksum != computed_checksum) {
        printf("Invalid checksum. read: %x, computed: %x\n", read_checksum, computed_checksum);
        return 5;
    }
    frame_data->checksum = read_checksum;

//    char* data = malloc(data_len + 1);
    memcpy(frame_data->data, &buf[5], data_len);
    frame_data->data[data_len] = 0;

    printf("Received frame\nAddress: %x, Command: %c, Register: %x, Data length: %d, Checksum: read %x computed %x\n",
        frame_data->address, frame_data->command_type, frame_data->reg, data_len, read_checksum, computed_checksum);
    printf("Data: %s\n", frame_data->data);


//    free(data);
    return 0;
}

int make_frame(uint8_t address, char action, uint8_t reg, char* data, char* buf) {
    size_t data_len = strlen(data);
    if (data_len > MAX_DATA_LENGTH) {
        printf("Data too long: %ld\n", data_len);
        return 1;
    }

//    char* buf = malloc(data_len + 8);

    if (address > 0xf) {
        printf("Invalid address %x\n", address);
//        free(buf);
        return 2;
    }

    sprintf(buf, ":0%x%c%x%s", address, action, reg, data);

    uint8_t checksum = buf[0];
    for (int i = 1; i < data_len + 5; ++i) {
        checksum += buf[i];
    }

    sprintf(&buf[data_len + 5], "%02x", checksum);
//    buf[data_len + 5] = '0';
//    buf[data_len + 6] = uint8_to_hex_char();

    buf[data_len + 7] = STOP_CHAR;
    buf[data_len + 8] = '\0';

//    free(buf);
    return 0;
}

void flush();

int main() {
	int error;

	char address_input;
	printf("Address (1-f): ");
	address_input = getchar();
	flush();
	uint8_t address = hex_char_to_uint8(address_input);
	if (address == 255 || address == 0) {
		puts("Invalid address number");
		exit(-1);
	}

	char command_input;
	printf("Command (W/R/N): ");
	command_input = getchar();
	if (command_input != 'W' && command_input != 'R' && command_input != 'N') {
		puts("Invalid command");
		exit(-1);
	}
	flush();

	char register_input;
	printf("Register (0-f): ");
	register_input = getchar();
	flush();
	uint8_t reg = hex_char_to_uint8(register_input);
	if (address == 255) {
		puts("Invalid register number!");
		exit(-1);
	}

	char data_input[17];
	if (command_input == 'W') {
		printf("Data (max length 16): ");
		scanf("%s", data_input);
		printf("%s\n", data_input);
	}
	else {
		data_input[0] = 0;
	}


	char frame_buffer[26];
	// canary just in case
	frame_buffer[25] = 'z';

	error = make_frame(address, command_input, reg, data_input, frame_buffer);
    if (error) {
    	exit(error);
    }
    printf("%s", frame_buffer);


    // sending frame over serial
    int serial = open("/dev/ttyS1", O_RDWR);
    if (serial < 0) {
    	puts("Error opening serial port");
    	exit(-2);
    }
    write(serial, frame_buffer, strlen(frame_buffer));


    FrameData frame_data;
    error = recv_frame(frame_buffer, &frame_data);
    if (error) {
    	exit(error);
    }

    if (frame_data.command_type == 'W') {
    	printf("Write %s to register %x\nResponse: %s", frame_data.data, frame_data.reg, frame_buffer);
    }

    if (frame_data.command_type == 'R') {
    	printf("Read %s from register %x\n", frame_data.data, frame_data.reg);
    }

    if (frame_data.command_type == 'N') {
    	printf("Received CRC error frame\n");
    }


    if (frame_buffer[25] != 'z') {
    	printf("buffer corrupted, canary dead, should be 'z' now is: '%c'\n", frame_buffer[25]);
    }
}

void flush() {
	int c;
	while ((c = getchar()) != '\n' && c != EOF)
		;
}
