#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>

// Callback function to handle the response data
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    FILE *output = (FILE *)userdata;
    return fwrite(ptr, size, nmemb, output);
}

// Callback function to handle response headers
size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    // Print each header to stdout
    printf("%.*s", (int)(size * nitems), buffer);
    return size * nitems;
}

int main() {
    CURL *curl;
    CURLcode res;

    // Initialize libcurl
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error initializing libcurl\n");
        return 1;
    }

    // Set the URL to fetch
    curl_easy_setopt(curl, CURLOPT_URL, "http://youtube.com");

    // Set the callback function to handle the response data
    FILE *output_file = fopen("response.txt", "w");
    if (!output_file) {
        fprintf(stderr, "Error opening output file\n");
        return 1;
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, output_file);

    // Set the callback function to handle response headers
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);

    // Perform the HTTP request
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Error performing HTTP request: %s\n", curl_easy_strerror(res));
        return 1;
    }

    // Clean up
    fclose(output_file);
    curl_easy_cleanup(curl);

    return 0;
}