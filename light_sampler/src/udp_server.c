#include "udp_server.h"
#include "sampler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <ctype.h>
#include "dip_detector.h"

static int s_socketFd = -1;
static pthread_t s_udpThread;
static volatile bool s_running = false;

// Keep track of last command for "repeat" functionality
static char s_lastCommand[256] = {0};

// Function prototypes
static void* udpThreadFunction(void* arg);
static void handleCommand(const char* command, struct sockaddr_in* clientAddr, socklen_t clientLen);
static void sendResponse(const char* message, struct sockaddr_in* clientAddr, socklen_t clientLen);
static void trimString(char* str);
static void toLowerCase(char* str);

bool UdpServer_init(void)
{
    // Create UDP socket
    s_socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (s_socketFd < 0) {
        perror("UdpServer_init: socket creation failed");
        return false;
    }

    // Bind to port
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(UDP_PORT);

    if (bind(s_socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("UdpServer_init: bind failed");
        close(s_socketFd);
        s_socketFd = -1;
        return false;
    }

    printf("UDP server listening on port %d\n", UDP_PORT);

    // Start UDP thread
    s_running = true;
    if (pthread_create(&s_udpThread, NULL, udpThreadFunction, NULL) != 0) {
        perror("UdpServer_init: Failed to create UDP thread");
        close(s_socketFd);
        s_socketFd = -1;
        return false;
    }

    return true;
}

void UdpServer_cleanup(void)
{
    if (s_socketFd >= 0) {
        s_running = false;
        
        // Close socket to unblock recvfrom
        close(s_socketFd);
        s_socketFd = -1;
        
        pthread_join(s_udpThread, NULL);
        printf("UDP server stopped\n");
    }
}

static void* udpThreadFunction(void* arg)
{
    (void)arg;
    
    char buffer[UDP_MAX_PACKET_SIZE];
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    printf("UDP thread started\n");

    while (s_running) {
        ssize_t numBytes = recvfrom(s_socketFd, buffer, sizeof(buffer) - 1, 0,
                                     (struct sockaddr*)&clientAddr, &clientLen);
        
        if (numBytes < 0) {
            if (s_running) {
                perror("UDP: recvfrom error");
            }
            break;
        }

        // Null-terminate the received data
        buffer[numBytes] = '\0';
        
        // Remove trailing newline/whitespace
        trimString(buffer);

        printf("UDP received: '%s' from %s:%d\n", 
               buffer,
               inet_ntoa(clientAddr.sin_addr),
               ntohs(clientAddr.sin_port));

        // Handle the command
        handleCommand(buffer, &clientAddr, clientLen);
    }

    printf("UDP thread stopped\n");
    return NULL;
}

static void handleCommand(const char* command, struct sockaddr_in* clientAddr, socklen_t clientLen)
{
    char cmd[256];
    strncpy(cmd, command, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';
    
    // Convert to lowercase for comparison
    toLowerCase(cmd);

    // Handle empty command (repeat last)
    if (strlen(cmd) == 0) {
        if (strlen(s_lastCommand) == 0) {
            sendResponse("Unknown command\n", clientAddr, clientLen);
            return;
        }
        // Repeat last command
        strncpy(cmd, s_lastCommand, sizeof(cmd));
    } else {
        // Save as last command
        strncpy(s_lastCommand, cmd, sizeof(s_lastCommand) - 1);
    }

    // Process commands
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        sendResponse(
            "Accepted command examples:\n"
            "count -- get the total number of samples taken.\n"
            "length -- get the number of samples taken in the previously completed second.\n"
            "dips -- get the number of dips in the previously completed second.\n"
            "history -- get all the samples in the previously completed second.\n"
            "stop -- cause the server program to end.\n"
            "<enter> -- repeat last command.\n",
            clientAddr, clientLen
        );
    }
    else if (strcmp(cmd, "count") == 0) {
        long long total = Sampler_getNumSamplesTaken();
        char response[128];
        snprintf(response, sizeof(response), "# samples taken total: %lld\n", total);
        sendResponse(response, clientAddr, clientLen);
    }
    else if (strcmp(cmd, "length") == 0) {
        int size = Sampler_getHistorySize();
        char response[128];
        snprintf(response, sizeof(response), "# samples taken last second: %d\n", size);
        sendResponse(response, clientAddr, clientLen);
    }
    else if (strcmp(cmd, "dips") == 0) {
    int size;
    double* history = Sampler_getHistory(&size);
    double avgVoltage = Sampler_getAverageReading();
    
    int dips = 0;
    if (history != NULL && size > 0) {
        dips = DipDetector_countDips(history, size, avgVoltage);
        free(history);
    }
    
    char response[128];
    snprintf(response, sizeof(response), "# Dips: %d\n", dips);
    sendResponse(response, clientAddr, clientLen);
}
    else if (strcmp(cmd, "history") == 0) {
        int size;
        double* history = Sampler_getHistory(&size);
        
        if (history == NULL || size == 0) {
            sendResponse("No history available\n", clientAddr, clientLen);
            return;
        }

        // Build response with 10 numbers per line
        char response[UDP_MAX_PACKET_SIZE];
        int offset = 0;
        int numbersOnLine = 0;

        for (int i = 0; i < size; i++) {
            int written = snprintf(response + offset, sizeof(response) - offset,
                                   "%.3f", history[i]);
            offset += written;

            numbersOnLine++;

            // Add comma and space (except for last number)
            if (i < size - 1) {
                if (offset < (int)sizeof(response) - 3) {
                    response[offset++] = ',';
                    response[offset++] = ' ';
                }
            }

            // Newline after 10 numbers
            if (numbersOnLine >= 10 && i < size - 1) {
                if (offset < (int)sizeof(response) - 2) {
                    response[offset++] = '\n';
                    numbersOnLine = 0;
                }
            }

            // Check if we're running out of space
            if (offset >= (int)sizeof(response) - 100) {
                // Send this packet and start a new one
                response[offset++] = '\n';
                response[offset] = '\0';
                sendResponse(response, clientAddr, clientLen);
                offset = 0;
                numbersOnLine = 0;
            }
        }

        // Send remaining data
        if (offset > 0) {
            response[offset++] = '\n';
            response[offset] = '\0';
            sendResponse(response, clientAddr, clientLen);
        }

        free(history);
    }
    else if (strcmp(cmd, "stop") == 0) {
        sendResponse("Program terminating.\n", clientAddr, clientLen);
        printf("Received 'stop' command. Shutting down...\n");
        
        // Signal main program to exit
        extern volatile bool g_running;
        g_running = false;
    }
    else {
        sendResponse("Unknown command\n", clientAddr, clientLen);
    }
}

static void sendResponse(const char* message, struct sockaddr_in* clientAddr, socklen_t clientLen)
{
    ssize_t sent = sendto(s_socketFd, message, strlen(message), 0,
                          (struct sockaddr*)clientAddr, clientLen);
    if (sent < 0) {
        perror("UDP: sendto failed");
    }
}

static void trimString(char* str)
{
    // Trim trailing whitespace
    int len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }

    // Trim leading whitespace
    int start = 0;
    while (str[start] && isspace((unsigned char)str[start])) {
        start++;
    }
    if (start > 0) {
        memmove(str, str + start, strlen(str + start) + 1);
    }
}

static void toLowerCase(char* str)
{
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}
