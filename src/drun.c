#define _GNU_SOURCE
#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drun.h"
#include "jsmn.h"

void init_string(string_t *json)
{
    /*
     * Since this function is initializing `json`, we can ignore initialzation
     * warnings
     */
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
    json->len = 0;
    json->ptr = (char *) malloc(json->len + 1);
    if (json->ptr == NULL) {
        fputs("Allocation error", stderr);
        exit(EXIT_FAILURE);
    }
    json->ptr[0] = '\0';
#pragma GCC diagnostic warning "-Wmaybe-uninitialized"
}

size_t write_callback(const void *ptr, const size_t size, const size_t nmemb,
                      string_t *json)
{
    /* Update the length of the JSON, and allocate more memory if needed */
    const size_t new_len = json->len + size * nmemb;
    json->ptr = (char *) realloc(json->ptr, new_len + 1);
    if (json->ptr == NULL) {
        fputs("Reallocation error", stderr);
        exit(EXIT_FAILURE);
    }

    /* Copy the incoming bytes to `json` */
    memcpy((void *) (json->ptr + json->len), ptr, size * nmemb);
    json->ptr[new_len] = '\0';
    json->len = new_len;

    return size * nmemb;
}

void dl_json(const char *runid, string_t *json)
{
    /* Load the full URI into `uri` */
#define BUFSIZE 64
    char uri[BUFSIZE];
    snprintf(uri, BUFSIZE, "https://www.speedrun.com/api/v1/runs/%s", runid);

    /* Initialize curl */
    CURL *curl = curl_easy_init();
    if (curl == NULL)
        exit(EXIT_FAILURE);

    /* Load the contents of the API request to `json` */
    curl_easy_setopt(curl, CURLOPT_URL, uri);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, json);

    CURLcode res;
    if ((res = curl_easy_perform(curl)) != 0) {
        fprintf(stderr,
                "Curl error: %d\nReport this error to whoever you got this "
                "program from\n",
                res);
        curl_easy_cleanup(curl);
        exit(EXIT_FAILURE);
    }
    curl_easy_cleanup(curl);

    return;
}

int main(void)
{
    /* Get the run ID */
    char *line = NULL;
    size_t size = 0;
    ssize_t read;
    if ((read = getline(&line, &size, stdin)) == -1) {
        if (feof(stdin))
            exit(EXIT_SUCCESS);

        exit(EXIT_FAILURE);
    }
    line[read - 1] = '\0';

    /* Get the runs JSON from the sr.c API */
    string_t json;
    init_string(&json);
    dl_json(line, &json);

    /* Cleanup and exit */
    free(line);
    free(json.ptr);
    return EXIT_SUCCESS;
}
