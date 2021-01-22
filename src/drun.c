#define _GNU_SOURCE
#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drun.h"
#include "jsmn.h"

char *parse_json(string_t *json)
{
    /* Initialize the JSON parser */
    jsmn_parser parser;
    jsmn_init(&parser);

    /* Parse the JSON */
#define TOKBUF 1024
    jsmntok_t tokens[TOKBUF];
    if (tokens == NULL) {
        fputs("Allocation error", stderr);
        exit(EXIT_FAILURE);
    }
    int ret = jsmn_parse(&parser, json->ptr, json->len, tokens, TOKBUF);

#define JSON_ERR(STR)                                                          \
    fputs(STR, stderr);                                                        \
    free(json->ptr);                                                           \
    exit(EXIT_FAILURE);

    switch (ret) {
    case JSMN_ERROR_INVAL:
        JSON_ERR("bad token, JSON string is corrupted");
    case JSMN_ERROR_NOMEM:
        JSON_ERR("not enough tokens, JSON string is too large");
    case JSMN_ERROR_PART:
        JSON_ERR("JSON string is too short, expecting more JSON data");
    }

    /* Find the "videos" object */
    for (int i = 0; i < ret; i++) {
        if (tokens[i].type == JSMN_STRING) {
            int start = tokens[i].start, end = tokens[i].end;
            if (strncmp(&json->ptr[start], "videos", end - start) == 0) {
                /*
                 * Read in the video URL which is at an offset of 6 from the
                 * "videos" token into `video_uri`
                 */
#define URIBUF 128
                char *video_uri = malloc(sizeof(char) * URIBUF + 1);
                if (video_uri == NULL) {
                    fputs("Allocation error", stderr);
                    free(json->ptr);
                    exit(EXIT_FAILURE);
                }

                start = tokens[i + 6].start, end = tokens[i + 6].end;
                strncpy(video_uri, &json->ptr[start], end - start);
                video_uri[end - start] = '\0';
                return video_uri;
            }
        }
    }

    /* No video found */
    return NULL;
}

void init_string(string_t *json)
{
    /*
     * Since this function is initializing `json`, we can ignore initialzation
     * warnings
     */
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
    json->len = 0;
    json->ptr = malloc(json->len + 1);
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
    json->ptr = realloc(json->ptr, new_len + 1);
    if (json->ptr == NULL) {
        fputs("Reallocation error", stderr);
        exit(EXIT_FAILURE);
    }

    /* Copy the incoming bytes to `json` */
    memcpy(json->ptr + json->len, ptr, size * nmemb);
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
    free(line);

    /* Parse the JSON */
    char *video_uri = parse_json(&json);
    if (video_uri == NULL) {
        fputs("No video found", stderr);
        goto EXIT;
    }
    puts(video_uri);

    /* Cleanup and exit */
    free(video_uri);
EXIT:
    free(json.ptr);
    return EXIT_SUCCESS;
}
