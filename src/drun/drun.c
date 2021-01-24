#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <curl/curl.h>

#include "drun.h"
#include "jsmn.h"

char *find_duplicate(FILE *fp, const char *video_uri)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, fp))) {
        if (read == -1) {
            if (feof(fp))
                break;

            perror("drun");
            free(line);
            exit(EXIT_FAILURE);
        }

        /* Match found */
        if (strncmp(line, video_uri, strlen(video_uri)) == 0)
            return line;
    }

    /* No match found */
    free(line);
    return NULL;
}

char *parse_json(string_t *json)
{
    jsmn_parser parser;
    jsmn_init(&parser);

    /* Parse the JSON */
#define TOKBUF 1024
    jsmntok_t tokens[TOKBUF];
    if (tokens == NULL) {
        fputs("Allocation error\n", stderr);
        exit(EXIT_FAILURE);
    }
    int ret = jsmn_parse(&parser, json->ptr, json->len, tokens, TOKBUF);

    /* TODO: Make this macro free run->id */
#define JSON_ERR(STR)                                                          \
    fputs(STR, stderr);                                                        \
    free(json->ptr);                                                           \
    exit(EXIT_FAILURE);

    switch (ret) {
    case JSMN_ERROR_INVAL:
        JSON_ERR("bad token, JSON string is corrupted\n");
    case JSMN_ERROR_NOMEM:
        JSON_ERR("not enough tokens, JSON string is too large\n");
    case JSMN_ERROR_PART:
        JSON_ERR("JSON string is too short, expecting more JSON data\n");
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
                    fputs("Allocation error\n", stderr);
                    free(json->ptr);
                    exit(EXIT_FAILURE);
                }

/*
 * Set the token offset to 6, since the video URI comes 6 tokens after the
 * "videos" key. The json looks like this:
 *
 * "videos": {
 *     "links": [
 *         {
 *             "uri": "https://youtu.be/2vjYnibdCBg"
 *         }
 *     ]
 * },
 *
 * TODO: Make this dynamic maybe
 */
#define TOK_OFFSET 6
                start = tokens[i + TOK_OFFSET].start,
                end = tokens[i + TOK_OFFSET].end;
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
    json->len = 0;
    json->ptr = malloc(json->len + 1);
    if (json->ptr == NULL) {
        fputs("Allocation error\n", stderr);
        exit(EXIT_FAILURE);
    }
    json->ptr[0] = '\0';
    return;
}

size_t write_callback(const void *ptr, const size_t size, const size_t nmemb,
                      string_t *json)
{
    /* Update the length of the JSON, and allocate more memory if needed */
    const size_t new_len = json->len + size * nmemb;
    json->ptr = realloc(json->ptr, new_len + 1);
    if (json->ptr == NULL) {
        fputs("Reallocation error\n", stderr);
        exit(EXIT_FAILURE);
    }

    /* Copy the incoming bytes to `json` */
    memcpy(json->ptr + json->len, ptr, size * nmemb);
    json->ptr[new_len] = '\0';
    json->len = new_len;

    return size * nmemb;
}

char *dl_json(const char *runid, string_t *json)
{
#define BUFSIZE 64
    static char uri[BUFSIZE];
    snprintf(uri, BUFSIZE, "https://www.speedrun.com/api/v1/runs/%s", runid);

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

    return uri;
}

void get_id(run_t *run)
{
    run->id = NULL;
    size_t size = 0;
    ssize_t read;
    if ((read = getline(&run->id, &size, stdin)) == -1) {
        if (feof(stdin))
            exit(EXIT_SUCCESS);

        exit(EXIT_FAILURE);
    }
    run->id[read - 1] = '\0';

    /*
     * Support for URIs with the following formats:
     *  - https://www.speedrun.com/game/run/ID
     *  - www.speedrun.com/game/run/ID
     *
     * the game/ part of the URI is optional
     */
    if (strncmp(run->id, "http", 4) == 0 || strncmp(run->id, "www", 3) == 0) {
        char copy[strlen(run->id) + 1], *token, *prevtoken = run->id;
        strcpy(copy, run->id);

        /* Get the last token */
        token = strtok(copy, "/");
        while ((token = strtok(NULL, "/")))
            prevtoken = token;

        run->id = strcpy(run->id, prevtoken);
    }

    return;
}

int main(int argc, char **argv)
{
    int opt;
    while ((opt = getopt(argc, argv, ":hv")) != -1) {
        switch (opt) {
        case 'h':
            puts(HELP_MSG);
            return EXIT_SUCCESS;
        case 'v':
            puts(VERSION_MSG);
            return EXIT_SUCCESS;
        default:
            fprintf(stderr,
                    "drun: invalid option -- '%c' \nTry 'drun -h' for more "
                    "information.\n",
                    optopt);
            return EXIT_FAILURE;
        }
    }

    run_t run;
    get_id(&run);
    init_string(&run.json);
    dl_json(run.id, &run.json);

    run.vid = parse_json(&run.json);
    if (run.vid == NULL) {
        fputs("No video found\n", stderr);
        goto EXIT;
    }

    /* Differs per system, and there is no easy way to get get the limit */
#ifdef PATH_MAX
#    undef PATH_MAX
#endif
#define PATH_MAX 1028
    const char *HOME = getenv("HOME");
    char xdg_data_home[PATH_MAX - 5], runs_file[PATH_MAX];
    snprintf(xdg_data_home, PATH_MAX - 5, "%s/.local/share/drun", HOME);
    snprintf(runs_file, PATH_MAX, "%s/runs", xdg_data_home);

    /* Create ~/.local/share/drun if it doesn't exist */
    struct stat st = {0};
    if (stat(xdg_data_home, &st) == -1) {
        if (mkdir(xdg_data_home, 0777) == -1) {
            perror("drun");
            exit(EXIT_FAILURE);
        }
    }

    FILE *fp = fopen(runs_file, "a+");
    if (fp == NULL) {
        perror("drun");
        exit(EXIT_FAILURE);
    }

    /*
     * For glibc, the initial file position for reading is at the beginning of
     * the file, but for Android/BSD/MacOS, the initial file position for
     * reading is at the end of the file.
     */
#if defined(__APPLE__) || defined(__FreeBSD__)
    rewind(fp);
#endif

    char *duplicate = find_duplicate(fp, run.vid);
    if (duplicate == NULL) {
        puts("No duplicate found");
        fprintf(fp, "%s https://www.speedrun.com/run/%s\n", run.vid, run.id);
    } else {
        /* Offset the return to get the sr.c run URI */
        printf("Duplicate video found!\n%s", duplicate + strlen(run.vid) + 1);
    }

    fclose(fp);
    if (duplicate != NULL)
        free(duplicate);

EXIT:
    free(run.id);
    free(run.vid);
    free(run.json.ptr);
    return EXIT_SUCCESS;
}
