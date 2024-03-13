#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>


// Structure to hold dynamic buffer and its size
typedef struct {
    char *data;
    size_t size;
} MemoryBuffer;

// Callback function to handle response headers
size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
        printf("%.*s\n", (int)(size * nitems) - 2, buffer); // 2 for \r\n

    if (strncasecmp(buffer, "Content-Type:", strlen("Content-Type:")) == 0) {
        // printf("%.*s\n", (int)(size * nitems), buffer);
        // printf("%.*s\n", (int)(size * nitems) - 2, buffer); // 2 for \r\n
        
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

char* checkUrl(int argc, char *argv[]) {
    if(argc < 2) {
        return NULL;
    }
    
    return argv[1];
}

int main(int argc, char *argv[]) {

    char *url = checkUrl(argc, argv);
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
    printf("Response data: %s\n", buffer.data);

    // Clean up and free memory
    free(buffer.data);
    curl_easy_cleanup(curl);

    return 0;
}