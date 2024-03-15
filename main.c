#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#define _POSIX_C_SOURCE 199309L  // Enable POSIX features in time.h
#include <time.h>
#include <curl/curl.h>

char fileType; // flag to determine the file type
char running = 1; // quitting means to stop running the program
char paused; // toggle between executing and stopping the program
float timeout = 100.0; // must be in microseconds when 0.1 and 10 milliseconds when 1??


// Structure to hold dynamic buffer and its size
typedef struct {
    char *data; // pointer to the data  
    size_t size; // the size in bytes
} MemoryBuffer;

// speed up the program by decreasing the timeout
void speed_up() {
    // if (timeout >= 12.5) {
    if (timeout > 2) {
        timeout /= 2;
        // printf("timeout: %f", timeout);
        printf("+Speeding Up+\n");
    }
}

// slow down the program by increasing the timeout
void slow_down() {
    if (timeout < 800) {
        timeout *= 2;
        // printf("timeout: %f", timeout);
        printf("-Slowing Down-\n");
    }
}

// run if paused, pause if running
void toggle_paused() {
    if (paused) { // is currently paused
        paused = 0;
        printf("<|Resuming|>\n");
    } else { // is currently running
        paused = 1;
        printf("*|Stopping|*\n");
    }
}

// Function to split a string by a delimiter character
// Returns the number of substrings found
int split_string(const char *str, char delimiter, char ***result) {
    int count = 0;
    const char *start = str;
    const char *end = str;

    // Count the number of substrings
    while (*end) { // while end is valid(not '\0')
        if (*end == delimiter) {
            count++;
            // printf("Count %d, Char %c\n", count, *end);
        }
        end++;
    }

    // don't include the count if '\n' because
    // we'll get index out of bounds/seg fault
    // when reading past the end of a file
    if (delimiter != '\n') {
        count++; // Count the last substring
    }

    // Allocate memory for the array of pointers
    *result = (char **)malloc(count * sizeof(char *));
    if (!*result) {
        return -1; // Memory allocation failed
    }

    // Extract substrings and store them in the array
    int i = 0;
    while (*start) {
        // printf("Start Char: %c\n", *start);
        end = start;
        while (*end && *end != delimiter) { // move end to the next delimiter
            end++;
        }
        // store the pointer to where the memory is getting allocated
        (*result)[i] = (char *)malloc((end - start + 1) * sizeof(char));
        if (!(*result)[i]) {
            // Memory allocation failed, free allocated memory
            for (int j = 0; j < i; j++) {
                free((*result)[j]);
            }
            free(*result);
            return -1;
        }
        // copy from the start of the current substring
        // until the end of the substring into the 
        // memory that was just allocated
        strncpy((*result)[i], start, end - start);
        (*result)[i][end - start] = '\0'; // terminate the substring in the allocated memory
        i++;
        if (!*end) {
            break;
        }
        start = end + 1; // set start to the beginning of the next substring
    }

    return count;
}

// Function to free memory allocated for the result array
void free_result(char **result, int count) {
    for (int i = 0; i < count; i++) { // free all the substrings
        free(result[i]);
    }
    free(result); // free the array of pointers
}


// when the file type is determined to be text
// handle the text here by printing each line
// by the default timeout and listen for 
// keyboard events: Q/q, +, -, and space
void handle_text(MemoryBuffer *buffer) {

    char **result; // an array to hold all the pointers for each line

    printf("\n---Reading Text---\n");

    // split the incoming data by '\n'
    // and store each line into the result array
    int count = split_string(buffer->data, '\n', &result);
    if (count == -1) {
        fprintf(stderr, "Error splitting text.\n");
    }

    int line = 0; // the current line
    int currentTime = 0; // the current time, should never be greater than the timeout

    // set up for using nanosleep()
    struct timespec req, rem;
    req.tv_sec = 0;
    req.tv_nsec = 1000000;  // 1 milliseconds, allegedely??

    // set up for detecting keyboard events
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD numEvents;
    INPUT_RECORD inputRecord;

    // Enable keyboard input events
    SetConsoleMode(hInput, ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT);

    while (running) {
        if (line >= count) break; // when the count is reached, end the loop
        
        // Check for available input events
        if (PeekConsoleInput(hInput, &inputRecord, 1, &numEvents) && numEvents > 0) {
            // Read input events
            ReadConsoleInput(hInput, &inputRecord, 1, &numEvents);

            // Process input events
            if (inputRecord.EventType == KEY_EVENT && inputRecord.Event.KeyEvent.bKeyDown) {
                // Key press event
                switch ((char)inputRecord.Event.KeyEvent.uChar.AsciiChar) {
                    case 'q':
                    case 'Q':
                        printf("Quitting\n");
                        running = 0;
                        break;
                    case '+':
                        speed_up();
                        break;
                    case '-':
                        slow_down();
                        break;
                    case ' ':
                        toggle_paused();
                        break;
                    default: // do nothing if other keys are pressed
                }
            }
        }

        // only print when running and currentTime is a certain value
        // currentTime being a certain value allows for the line to
        // only be printed once per timeout
        if (!paused && currentTime == 0) {
            printf("Line %d: %s\n\n", line + 1, result[line]);
            line++;
        }

        // Sleep(1);
        nanosleep(&req, &rem);

        // only increment the currentTime
        // if the program is running
        if (!paused) {
            currentTime = ++currentTime % (int)timeout;
        }
    }

    // this condition is needed for
    // when the user wants to quit
    if (running) {
        printf("---End Of Text---\n");
    }

    free_result(result, count);
}

// when the file type is determined to be not text
// handle the bytes here by printing every 16 bytes
// by the default timeout and listen for 
// keyboard events: Q/q, +, -, and space
void handle_bytes(MemoryBuffer *buffer) {
    long start = 0; // long in case the buffer is too big for int
    int currentTime = 0;

    // set up for using nanosleep()
    struct timespec req, rem;
    req.tv_sec = 0;
    req.tv_nsec = 1000000;

    // set up for detecting keyboard events
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    DWORD numEvents;
    INPUT_RECORD inputRecord;

    // Enable keyboard input events
    SetConsoleMode(hInput, ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT);
    printf("\n---Reading Bytes---\n");

    while (running) {
        // once the end of the buffer is reached
        // break from the loop
        if (start >= buffer->size) break;

        // Check for available input events
        if (PeekConsoleInput(hInput, &inputRecord, 1, &numEvents) && numEvents > 0) {
            // Read input events
            ReadConsoleInput(hInput, &inputRecord, 1, &numEvents);

            // Process input events
            if (inputRecord.EventType == KEY_EVENT && inputRecord.Event.KeyEvent.bKeyDown) {
                // Key press event
                switch ((char)inputRecord.Event.KeyEvent.uChar.AsciiChar) {
                    case 'q':
                    case 'Q':
                        printf("Quitting\n");
                        running = 0;
                        break;
                    case '+':
                        speed_up();
                        break;
                    case '-':
                        slow_down();
                        break;
                    case ' ':
                        toggle_paused();
                        break;
                    default:
                }
            }
        }

        if (!paused && currentTime == 0) {
            // offset from start by 0-15 to get 16 bytes
            printf("0x%02x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n\n",\
                    start, (unsigned char)buffer->data[start], (unsigned char)buffer->data[start+1],\
                            (unsigned char)buffer->data[start+2], (unsigned char)buffer->data[start+3],\
                            (unsigned char)buffer->data[start+4], (unsigned char)buffer->data[start+5],\
                            (unsigned char)buffer->data[start+6], (unsigned char)buffer->data[start+7],\
                            (unsigned char)buffer->data[start+8], (unsigned char)buffer->data[start+9],\
                            (unsigned char)buffer->data[start+10], (unsigned char)buffer->data[start+11],\
                            (unsigned char)buffer->data[start+12], (unsigned char)buffer->data[start+13],\
                            (unsigned char)buffer->data[start+14], (unsigned char)buffer->data[start+15]);
            
            // increase start by 16
            start += 0x10;
        }

        // Sleep(1);
        nanosleep(&req, &rem);

        if (!paused) {
            currentTime = ++currentTime % (int)timeout;
        }
    }



    if (running) {
        printf("---End Of Bytes---\n");
    }

}

// check the file type from the provided string
// then set the fileType flag accordingly
void check_file_type(int size, char* buffer) {
    char **result; // this will be the first split 'x: y' by ':'
    char **result2; // this will split y by ';' to get the charset if present

    int count = split_string(buffer, ':', &result); // split 'Content-Type: y'

    // if charset is present, split y by ';'
    // and the count will be greater than 1
    int count2 = split_string(result[1], ';', &result2); 

    if (count2 > 1) {
        // check to see if the second substring is for the utf-8 charset
        if (strncasecmp(result2[1], " charset=UTF-8", strlen(" charset=UTF-8")) == 0) {
            // printf("There is UNICODE!\n");
            fileType = 1;
        }
    }
    // if charset is not present, check to see if text was provided
    else if (strncasecmp(result2[0], " text", strlen(" text")) == 0) {
        // printf("There is text!\n");
        fileType = 1;
    }
    // else leave fileType to be 0 which
    // will be trigger bytes to be read

    free_result(result, count);
    free_result(result2, count2);
}

// Callback function to handle response headers
size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    // check to see if the current header being processed is 'Content-Type'
    if (strncasecmp(buffer, "Content-Type:", strlen("Content-Type:")) == 0) {
        printf("%.*s\n", (int)(size * nitems) - 2, buffer); // 2 for \r\n
        // check the file type by checking
        // against the 'Content-Type'
        check_file_type((int)(size * nitems), buffer);
    }

    return size * nitems;
}

// Callback function to handle the response data
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    // set up for copying the incoming data into a buffer
    size_t realsize = size * nmemb; // size of incoming data
    MemoryBuffer *mem = (MemoryBuffer *)userdata; // the buffer pointer that was passed in

    // Reallocate memory to accommodate new data
    char *new_data = realloc(mem->data, mem->size + realsize + 1);
    if (new_data == NULL) {
        fprintf(stderr, "Error reallocating memory\n");
        return 0; // Returning 0 from write callback indicates failure
    }

    // Copy the new data into the buffer
    memcpy(&new_data[mem->size], ptr, realsize);
    mem->data = new_data; // point to the filled buffer
    mem->size += realsize;
    mem->data[mem->size] = '\0'; // Null-terminate the string

    return realsize;
}

// check to make sure that a second 
// argument was provided, it could
// still be an invalid url
char* check_Url(int argc, char *argv[]) {
    if(argc < 2) {
        return NULL;
    }
    
    return argv[1]; // return the "url"
}

int main(int argc, char *argv[]) {

    char *url = check_Url(argc, argv); // check for url
    if (!url) {
        fprintf(stderr, "Error: A parameter must be passed in for the url.\n");
        return 1;
    }

    printf("URL - %s\n", url);

    // set up to use curl
    CURL *curl;
    CURLcode res;
    MemoryBuffer buffer = {NULL, 0}; // buffer(pointer) to access incoming data

    // Initialize libcurl
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error initializing libcurl\n");
        // Clean up and free memory
        free(buffer.data);
        curl_easy_cleanup(curl);
        return 1;
    }

    // Set the URL to fetch
    curl_easy_setopt(curl, CURLOPT_URL, url);

    // Set the callback function to handle response headers
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);

    // Set the callback function to handle the response data
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

    // Perform the HTTP request
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Error performing HTTP request: %s\n", curl_easy_strerror(res));
        // Clean up and free memory
        free(buffer.data);
        curl_easy_cleanup(curl);
        return 1;
    }

    printf("Press Q/q to quit at anytime.\n");
    // Manipulate the response data in the buffer
    if (fileType) {
        printf("Text was detected.\n");
        handle_text(&buffer);
    } else {
        printf("Deferring to bytes.\n");
        handle_bytes(&buffer);
    }

    // Clean up and free memory
    free(buffer.data);
    curl_easy_cleanup(curl);

    return 0;
}