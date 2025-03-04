#include <stdio.h>
#include <stdlib.h>
#include <stdint-gcc.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#define PORT 53
#define MAXLINE 1000

//#include <arpa/inet.h>  // Required for htons()

void increment_id(uint16_t *num) {

    *num = *num + 1;

}


int set_id(uint8_t *header, uint16_t id){

    header[0] = (id >> 8) & 0xFF;
    header[1] = id & 0xFF;

    return 0;

}

int count_sig_bits(int num) {
    int count = 0;
    while (num != 0){
        count++;
        num >>= 1;
    }
    return count;
}

void print_bits(uint8_t byte) {
    for (int i = 7; i >= 0; i--) {
        printf("%d", (byte >> i) & 1); // Extract each bit using bitwise shift and AND
    }
}

//As of right now only works for 1 byte at a time
void write_bits(uint8_t *header, uint8_t bits, int position){

    int bit_len = count_sig_bits(bits);
    int byte = ((position - (position % 8))/8);
    position = (position) % 8;
    uint8_t mask = bits;

    //get to first sig bit
    while ((bits & 128) != 128){
        bits <<= 1;
    }

    while (bit_len > 0){

        mask = bits & 128;
        mask >>= position;
        bits <<= 1;

        header[byte] |= mask;

        bit_len -= 1;

        if ((position+1) > 7){
            position = 0;
            byte += 1;
        } else{
            position += 1;
        }

    }

}

// 0 for query, 1 for response
int set_query_flag(uint8_t *header, uint8_t type){

    uint8_t flag_bit_position = 16;

    // 0 is a query, 1 is a response
    if (type == 1) {

        // Set to response
        write_bits(header,0b1,flag_bit_position);

    } else if (type == 0){ // No need to write if zero as the value will already be zero
        return 0;
    } else {
        return 1;
    }

    return 0;
}

//If the value of the opcode subfield is 0 then it is a standard query.
//The value 1 corresponds to an inverse of query that implies finding the domain name from the IP Address.
//The value 2 refers to the server status request. The value 3 specifies the status reserved and therefore not used.
int set_op_code(uint8_t *header,uint8_t opcode){

    if (opcode == 0){
        return 0;
    }

    write_bits(header,opcode,17);

    return 0;
}

int set_aa(uint8_t *header, uint8_t aa){

    if (aa == 0){
        return 0;
    }

    write_bits(header,aa,21);

    return 0;
}

int set_tc(uint8_t *header, uint8_t tc){

    if (tc == 0){
        return 0;
    }

    write_bits(header,tc,22);

    return 0;
}

int set_rd(uint8_t *header, uint8_t rd){
    if (rd == 0){
        return 0;
    }

    write_bits(header,rd,23);

    return 0;
}

int set_ra(uint8_t *header, uint8_t ra){

    if (ra == 0){
        return 0;
    }

    write_bits(header,ra,24);

    return 0;
}

//These bits are unused for now. Do nothing, just a placeholder function
int set_reserved(uint8_t *header, uint8_t reserved){

    if (reserved == 0){
        return 0;
    }

    //write_bits(header,reserved,25);

    return 0;
}


//The value 0 of rcode indicates no error.
//A value of 1 indicates that there is a problem with the format specification.
//Value 2 indicates server failure.
//Value 3 refers to the Name Error that implies the name given by the query does not exist in the domain.
//Value of 4 indicates that the request type is not supported by the server.
//The value 5 refers to the nonexecution of queries by the server due to policy reasons.
int set_rcode(uint8_t *header, uint8_t rcode){

    if (rcode == 0) {
        return 0;
    }

    write_bits(header,rcode,28);

    return 0;
}


// Number of questions
int set_noq(uint8_t *header, uint16_t noq){
    header[4] = (noq >> 8) & 0xFF;
    header[5] = noq & 0xFF;

    return 0;
}

// Number of response records
int set_norr(uint8_t *header, uint16_t norr){
    header[6] = (norr >> 8) & 0xFF;
    header[7] = norr & 0xFF;

    return 0;
}

int set_nscount(uint8_t *header, uint16_t nscount){
    header[8] = (nscount >> 8) & 0xFF;
    header[9] = nscount & 0xFF;

    return 0;
}

int set_arcount(uint8_t *header, uint16_t arcount){
    header[10] = (arcount >> 8) & 0xFF;
    header[11] = arcount & 0xFF;

    return 0;
}

int create_udp_server(){

    ssize_t valread;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    //char buffer[1024] = { 0 };
    char* hello = "Hello from server";

    int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address,sizeof(address))< 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    int ret;
    #define BUF_SIZE 1024
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUF_SIZE];

    while (1) {
        client_len = sizeof(client_addr);
        ret = recvfrom(server_fd, buffer, BUF_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
        if (ret < 0) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }

        printf("Received message: %s from %s:%d\n", buffer, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Echo back to client
        ret = sendto(server_fd, buffer, ret, 0, (struct sockaddr *)&client_addr, client_len);
        if (ret < 0) {
            perror("sendto failed");
            exit(EXIT_FAILURE);
        }
    }

    return(server_fd);

}

int main(void) {

    uint16_t id = 42900;
    uint8_t header[12];

    //Make sure everything in the header is zero
    for (int i=0;i<12;i++){
        header[i] = 0;
    }

    set_id(header,id);
    set_query_flag(header,0);
    set_op_code(header,0);
    set_aa(header,0);
    set_tc(header,0);
    set_rd(header,1);
    set_ra(header,0);
    set_reserved(header,0);
    set_rcode(header,0);
    set_noq(header,1);
    set_norr(header,0);
    set_nscount(header,0);
    set_arcount(header,0);

    /*
    //Print the header in bit representation
    for (int i=0;i<12;i++){
        print_bits(header[i]);
        printf(" ");
    }
    */

    increment_id(&id);

    // If id reaches the max, set back to zero
    if (id == 65535) {
        id = 0;
    }

    // Convert to Big Endian
    //id = htons(header);

    printf("Listening on port: %d...\n\n",PORT);
    create_udp_server();

    return 0;

}

