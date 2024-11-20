#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#define MAX_NIC_LENGTH 20
#define BUFFER_SIZE 1024

const unsigned char bind_gs_key[] = {
    0x73, 0xDC, 0x25, 0xB6, 0x41, 0xAB, 0x06, 0x15, 0x94, 0x16, 0xED, 0xFB,
    0x89, 0x90, 0x3A, 0x6A, 0xBF, 0x85, 0x5A, 0x1F, 0xCB, 0xFF, 0xF7, 0xFA,
    0xCB, 0x05, 0xC4, 0x8B, 0x6A, 0x11, 0x94, 0x5E, 0xDD, 0xBA, 0xA8, 0x4D,
    0x1A, 0xCE, 0x9F, 0x54, 0x50, 0xCF, 0x68, 0x53, 0xB6, 0x00, 0xDC, 0x3C,
    0xCC, 0x77, 0x1F, 0xC5, 0x33, 0x89, 0x00, 0xF7, 0xD4, 0x95, 0x19, 0xE3,
    0x7F, 0x75, 0x64, 0x2A
};
const size_t bind_gs_key_size = 64;

const unsigned char bind_drone_key[] = {
    0x00, 0x14, 0x9E, 0x86, 0xFC, 0x2D, 0x88, 0x4A, 0x91, 0xA7, 0x5C, 0x51,
    0x44, 0xEB, 0xC9, 0x8F, 0xEE, 0xBB, 0x87, 0x8E, 0x91, 0x88, 0xBA, 0xC3,
    0x0E, 0x1E, 0xE7, 0x56, 0x18, 0xC6, 0x45, 0xEA, 0x74, 0x69, 0x0B, 0xD9,
    0xE2, 0x21, 0xEA, 0xC8, 0x38, 0xDB, 0x6D, 0x88, 0xA9, 0x24, 0x46, 0xA9,
    0x6F, 0x96, 0xEA, 0x1B, 0x6D, 0x85, 0xE7, 0xD8, 0x84, 0xF2, 0x05, 0x1B,
    0xDE, 0xE8, 0x06, 0x11
};
const size_t bind_drone_key_size = 64;

int bind_port = 2463;
const char *bind_key = "/tmp/bind.key";
const char *drone_key = "/etc/drone.key";
const char *gs_key = "/etc/gs.key";

void check_and_generate_keys() {
    // Check if the gs.key file exists
    if (access(gs_key, F_OK) == -1) {
        printf("File %s does not exist. Running wfg_keygen...\n", gs_key);
        // Run the key generation program (wfg_keygen) in the /etc directory
        if (chdir("/etc") != 0) {
            perror("Failed to change directory to /etc");
            exit(EXIT_FAILURE);
        }
        if (system("wfb_keygen") == -1) {
            perror("Failed to execute wfb_keygen");
            exit(EXIT_FAILURE);
        }
    } else {
        printf("File %s exists.\n", gs_key);
    }

}

// Function to get the first NIC from /etc/default/wifibroadcast
int get_gs_nic_from_file(char *nic) {
    FILE *file = fopen("/etc/default/wifibroadcast", "r");
    if (file == NULL) {
        perror("Failed to open file");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "WFB_NICS=", 9) == 0) {
            // Find the value of WFB_NICS (after the '=' character)
            char *nics_value = line + 9;
            
            // Remove the leading double quote if it exists
            if (*nics_value == '"') {
                nics_value++;  // Skip the leading quote
            }

            // Get the first NIC by tokenizing the string
            char *first_nic = strtok(nics_value, " \n");

            if (first_nic != NULL) {
                // Remove the trailing double quote if it exists
                size_t len = strlen(first_nic);
                if (len > 0 && first_nic[len - 1] == '"') {
                    first_nic[len - 1] = '\0';  // Null-terminate at the quote
                }

                // Copy the first NIC to the nic buffer
                strncpy(nic, first_nic, MAX_NIC_LENGTH);
                fclose(file);
                return 0;  // Successfully got the first NIC
            }
        }
    }

    fclose(file);
    return -1;  // No NIC found
}


// Function to get the wlan name from /etc/wfb.conf
int get_drone_nic_from_file(char *wlan) {
    FILE *file = fopen("/etc/wfb.conf", "r");
    if (file == NULL) {
        perror("Failed to open file");
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Check if the line starts with "wlan="
        if (strncmp(line, "wlan=", 5) == 0) {
            // Extract the wlan name (after the "=" character)
            char *wlan_value = line + 5;
            
            // Remove any trailing newline or extra spaces
            char *newline_pos = strchr(wlan_value, '\n');
            if (newline_pos) {
                *newline_pos = '\0';  // Null-terminate at the newline
            }

            // Copy the wlan name to the buffer
            strncpy(wlan, wlan_value, MAX_NIC_LENGTH);
            fclose(file);
            return 0;  // Successfully got the wlan name
        }
    }

    fclose(file);
    return -1;  // wlan not found
}

void create_bind_key(const char *filename, const unsigned char *data, size_t size) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open binary file for writing");
        exit(EXIT_FAILURE);
    }
    if (fwrite(data, 1, size, file) != size) {
        perror("Failed to write binary data to file");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fclose(file);
    printf("Binary data written to %s\n", filename);
}

pid_t start_subprocess(const char *cmd, const char *const args[]) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process: execute the subprocess
        execvp(cmd, (char *const *)args);
        perror("Exec failed");
        exit(EXIT_FAILURE);
    }

    return pid;  // Return the PID of the child process
}

void wait_for_subprocess(pid_t pid) {
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("Waitpid failed");
    } else {
        if (WIFEXITED(status)) {
            printf("Subprocess exited with status: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Subprocess terminated by signal: %d\n", WTERMSIG(status));
        } else {
            printf("Subprocess terminated abnormally\n");
        }
    }
}

void gs() {

    check_and_generate_keys();

    char bind_port_str[6];
    snprintf(bind_port_str, sizeof(bind_port_str), "%d", bind_port);  // Convert bind_port to string

    char first_nic[MAX_NIC_LENGTH];
    if (get_gs_nic_from_file(first_nic) != 0) {
        printf("Failed to read NIC from file.\n");
        exit(EXIT_FAILURE);
    }

    const char *const subprocess_args[] = {
        "wfb_tx", "-f", "data", "-p", "255", "-u", bind_port_str, "-K", bind_key, 
        "-B", "20", "-G", "long", "-S", "1", "-L", "1", "-M", "1", "-k", "1",
        "-n", "2", "-T", "0", "-F", "0", "-i", "7669206", "-R", "2097152", 
        "-l", "200", "-C", "0", first_nic, NULL
    };

    // Start the subprocess and get its PID
    printf("Starting subprocess...\n");
    pid_t pid = start_subprocess("wfb_tx", subprocess_args);

    // Create UDP socket
    int sockfd;
    struct sockaddr_in server_addr;
    FILE *file;
    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));

    // Set up the server address (sending to the same port as drone listens)
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Use any available interface
    server_addr.sin_port = htons(bind_port);   // Port 2463

    // Open the drone.key file to read its contents
    file = fopen(drone_key, "rb");
    if (!file) {
        perror("Failed to open input file");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Send data from drone.key over UDP 30 times, once every 1/30th of a second
    for (int i = 0; i < 30; i++) {
        // Rewind the file to send the data from the beginning each time
        fseek(file, 0, SEEK_SET);

        // Read and send the data
        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
            ssize_t n = sendto(sockfd, buffer, bytes_read, 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
            if (n < 0) {
                perror("Send failed");
                break;
            }

            printf("Sent %zd bytes\n", n);
        }

        // Sleep for 1 second
        usleep(1000000);
    }

    printf("Data sent 30 times. Closing...\n");

    // Clean up
    fclose(file);
    close(sockfd);

    // Kill the subprocess using its PID
    printf("Stopping subprocess...\n");
    kill(pid, SIGTERM);  // Send SIGTERM to the specific subprocess
    wait_for_subprocess(pid);  // Wait for the subprocess to exit
}


void drone() {
    char bind_port_str[6];
    snprintf(bind_port_str, sizeof(bind_port_str), "%d", bind_port);  // Convert bind_port to string

    char nic[MAX_NIC_LENGTH];
    
    // Read wlan name from /etc/wfb.conf
    if (get_drone_nic_from_file(nic) != 0) {
        printf("Failed to read nic from file.\n");
        exit(EXIT_FAILURE);
    }    

    const char *const subprocess_args[] = { 
        "wfb_rx", "-p", "255", "-u", bind_port_str, "-K", bind_key, 
        "-i", "7669206", nic, NULL 
    };

    // Start the subprocess and get its PID
    printf("Starting subprocess...\n");
    pid_t pid = start_subprocess("wfb_rx", subprocess_args);

    // Create UDP socket
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    FILE *file;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    // Bind the socket to the specified port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(bind_port);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("UDP receiver started on port %d and nic %s\n", bind_port,nic);

    // Open the output file
    file = fopen(drone_key, "wb");
    if (!file) {
        perror("Failed to open output file");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    size_t total_bytes_received = 0;
    socklen_t len = sizeof(client_addr);

    // Set up the timeout for 30 seconds
    struct timeval timeout;
    timeout.tv_sec = 30;  // 30 seconds
    timeout.tv_usec = 0;

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);

    // Wait for data with a timeout of 30 seconds
    printf("Waiting for data for 30 seconds...\n");
    int select_result = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

    if (select_result == -1) {
        perror("Select failed");
        fclose(file);
        close(sockfd);
        exit(EXIT_FAILURE);
    } else if (select_result == 0) {
        printf("Timeout reached, no data received in 30 seconds.\n");
    } else {
        // Data is available to read, so we proceed with receiving the data
        while (total_bytes_received < bind_drone_key_size) {
            ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &len);
            if (n < 0) {
                perror("Receive failed");
                break;
            }
            fwrite(buffer, 1, n, file);
            total_bytes_received += n;

            printf("Received %zd bytes, total: %zu/%zu\n", n, total_bytes_received, bind_drone_key_size);
        }
    }

    printf("Closing after 30 seconds or data received.\n");

    // Clean up
    fclose(file);
    close(sockfd);

    // Kill the subprocess using its PID
    printf("Stopping subprocess...\n");
    kill(pid, SIGTERM);  // Send SIGTERM to the specific subprocess
    wait_for_subprocess(pid);  // Wait for the subprocess to exit
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [gs|drone]\nVersion: %s\n", argv[0],VERSION_STRING);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "gs") == 0) {
        // Write embedded binary data to a file
        create_bind_key(bind_key, bind_gs_key, bind_gs_key_size);        
        gs();
    } else if (strcmp(argv[1], "drone") == 0) {
        // Write embedded binary data to a file
        create_bind_key(bind_key, bind_drone_key, bind_drone_key_size);        
        drone();
    } else {
        fprintf(stderr, "Invalid option: %s. Use 'gs' or 'drone'.\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    remove(bind_key);
    printf("Exiting program.\n");

}