#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_DATA_LENGTH 16
#define STOP_CHAR '\0'

typedef struct {
    char start;
    char address[2];
    char command[2];
    char data[16];
    char checksum[2];
    char end;
} Frame;

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

int recv_frame(char* buf) {
    if (buf[0] != ':') {
        printf("Invalid start character: %c\n", buf[0]);
        return 1;
    }
    // buf[1] ignored, always '0'
    uint8_t address = hex_char_to_uint8(buf[2]);
    if (address == 255) {
        printf("Invalid address character: %c\n", buf[2]);
        return 2;
    }

    char command_type = buf[3];
    uint8_t command_target = hex_char_to_uint8(buf[4]);

    if (command_type != 'W' && command_type != 'R' && command_type != 'N') {
        printf("Invalid command type: %c\n", command_type);
        return 3;
    }

    uint8_t computed_checksum = buf[1] + buf[2] + buf[3] + buf[4];
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

    char* data = malloc(data_len + 1);
    memcpy(data, &buf[5], data_len);
    data[data_len] = 0;

    printf("Address: %x, Command: %c, Register: %x, Data length: %d, Checksum: read %x computed %x\n",
        address, command_type, command_target, data_len, read_checksum, computed_checksum);
    printf("Data: %s\n", data);
    free(data);
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

    uint8_t checksum = 0;
    for (int i = 1; i < data_len + 5; ++i) {
        checksum += buf[i];
    }

    sprintf(&buf[data_len + 5], "%02x", checksum);

    buf[data_len + 7] = STOP_CHAR;

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
	printf("Data (max length 16): ");
	scanf("%s", data_input);
	printf("%s\n", data_input);

	char frame_buffer[25];
	// canary just in case
	frame_buffer[24] = 'z';

	error = make_frame(address, command_input, reg, data_input, frame_buffer);
    if (error) {
    	exit(error);
    }
    printf("%s\n", frame_buffer);


    // sending frame over serial
    int serial = open("/dev/ttyS1", O_RDWR);
    if (serial < 0) {
    	puts("Error opening serial port");
    	exit(-2);
    }
    write(serial, frame_buffer, strlen(frame_buffer));


    error = recv_frame(frame_buffer);
    if (error) {
    	exit(error);
    }


    if (frame_buffer[24] != 'z') {
    	printf("buffer corrupted, canary dead, should be 'z' now is: '%c'\n", frame_buffer[24]);
    }
}
void flush() {
	int c;
	while ((c = getchar()) != '\n' && c != EOF)
		;
}
