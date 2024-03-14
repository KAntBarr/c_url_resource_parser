#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <curl/curl.h>

char fileType;
char paused;
int timeout = 100;

// Structure to hold dynamic buffer and its size
typedef struct {
    char *data;
    size_t size;
} MemoryBuffer;

// Function to split a string by a delimiter character
// Returns the number of substrings found
int split_string(const char *str, char delimiter, char ***result) {
    int count = 0;
    const char *start = str;
    const char *end = str;

    // Count the number of substrings
    while (*end) {
        if (*end == delimiter) {
            count++;
            // printf("Count %d, Char %c\n", count, *end);
        }
        end++;
    }
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
        while (*end && *end != delimiter) {
            end++;
        }
        (*result)[i] = (char *)malloc((end - start + 1) * sizeof(char));
        if (!(*result)[i]) {
            // Memory allocation failed, free allocated memory
            for (int j = 0; j < i; j++) {
                free((*result)[j]);
            }
            free(*result);
            return -1;
        }
        strncpy((*result)[i], start, end - start);
        (*result)[i][end - start] = '\0';
        i++;
        if (!*end) {
            break;
        }
        start = end + 1;
    }

    return count;
}

// Function to free memory allocated for the result array
void free_result(char **result, int count) {
    for (int i = 0; i < count; i++) {
        free(result[i]);
    }
    free(result);
}

void handle_text(MemoryBuffer *buffer) {

    char **result;
    // printf("---Reading Text---\n%s\n", buffer->data);
    printf("---Reading Text---\n");

    int count = split_string(buffer->data, '\n', &result);
    if (count == -1) {
        fprintf(stderr, "Error splitting text.\n");
    }

    int line = 0;
    int tempTime = 0;
    while (1) {
        // printf("Time %i\n", tempTime);
        if (line == count) break;
        if (GetAsyncKeyState('q') & 0x8000) {
            printf("Key 'q' is pressed!\n");
        }
        if (GetAsyncKeyState('+') & 0x8000) {
            printf("Key '+' is pressed!\n");
        }
        if (GetAsyncKeyState('-') & 0x8000) {
            printf("Key '-' is pressed!\n");
        }
        if (GetAsyncKeyState(' ') & 0x8000) {
            printf("Key 'space' is pressed!\n");
        }
        if (!paused && tempTime == 0) {
            printf("Line %d: %s\n", line + 1, result[line]);
            line++;
        }
        Sleep(1);
        tempTime = ++tempTime % timeout;
    }

    printf("---End Of Text---\n");


    free_result(result, count);
}

void handle_bytes(MemoryBuffer *buffer) {

}

void check_file_type(int size, char* buffer) {
    char **result;
    char **result2;

    int count = split_string(buffer, ':', &result);

    // printf("%s", result[1]);

    int count2 = split_string(result[1], ';', &result2);

    if (count2 > 1) {
        // printf("Substring: %s", result2[1]);
        if (strncasecmp(result2[1], " charset=UTF-8", strlen(" charset=UTF-8")) == 0) {
            // printf("There is UNICODE!\n");
            fileType = 1;
        }
    }
    else if (strncasecmp(result2[0], " text", strlen(" text")) == 0) {
        // printf("There is text!\n");
        fileType = 1;
    }

    free_result(result, count);
    free_result(result2, count2);
}

// Callback function to handle response headers
size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    // printf("%.*s\n", (int)(size * nitems) - 2, buffer); // 2 for \r\n

    if (strncasecmp(buffer, "Content-Type:", strlen("Content-Type:")) == 0) {
        // printf("%.*s\n", (int)(size * nitems), buffer);
        printf("%.*s\n", (int)(size * nitems) - 2, buffer); // 2 for \r\n
        check_file_type((int)(size * nitems), buffer); // do error handling
    }

    return size * nitems;
}

// Callback function to handle the response data
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t realsize = size * nmemb;
    MemoryBuffer *mem = (MemoryBuffer *)userdata;

    // Reallocate memory to accommodate new data
    char *new_data = realloc(mem->data, mem->size + realsize + 1);
    if (new_data == NULL) {
        fprintf(stderr, "Error reallocating memory\n");
        return 0; // Returning 0 from write callback indicates failure
    }

    // Copy the new data into the buffer
    memcpy(&new_data[mem->size], ptr, realsize);
    // memcpy(new_data, ptr, realsize);
    mem->data = new_data;
    mem->size += realsize;
    mem->data[mem->size] = '\0'; // Null-terminate the string

    // printf("Mem Address %p\n", (void *)mem);

    return realsize;
}

char* check_Url(int argc, char *argv[]) {
    if(argc < 2) {
        return NULL;
    }
    
    return argv[1];
}

int main(int argc, char *argv[]) {

    char *url = check_Url(argc, argv);
    if (!url) {
        fprintf(stderr, "Error: A parameter must be passed in for the url.\n");
        return 1;
    }

    printf("URL - %s\n", url);

    CURL *curl;
    CURLcode res;
    MemoryBuffer buffer = {NULL, 0};

    // printf("Buffer Address %p\n", (void *)&buffer);

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

    // Manipulate the response data in the buffer
    // printf("Response data:\n%s\n", buffer.data);
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