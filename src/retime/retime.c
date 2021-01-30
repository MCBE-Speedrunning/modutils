#define _GNU_SOURCE
#define _XOPEN_SOURCE
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#    include "getline.h"
#endif
#include "jsmn.h"
#include "retime.h"

void *smalloc(const size_t size)
{
    void *ret = malloc(size);
    if (ret == NULL) {
        fputs("Allocation error\n", stderr);
        exit(EXIT_FAILURE);
    }
    return ret;
}

double str_to_double(const char *str_time)
{
    unsigned int i = 0, sigfig = 1;
    double time = 0;

    while (str_time[i] != '.') {
        if (str_time[i] == '\0')
            return time;

        time = time * 10 + str_time[i++] - '0';
    }

    i++;
    while (str_time[i] != '\0')
        time = time + (str_time[i++] - '0') / pow(10, sigfig++);

    return time;
}

unsigned int check_fps(char *string)
{
    /* Make sure fps is numeric */
    for (int i = 0, len = strlen(string); i < len; i++)
        if (!isdigit(string[i]))
            goto INVALID_FPS;

    const unsigned int fps = (unsigned int) str_to_double(string);

    if (fps > 60 || fps < 1)
        goto INVALID_FPS;

    return fps;

INVALID_FPS:
    fputs("retime: 'f' option must be a whole number from 1 to 60\n", stderr);
    exit(BAD_FPS);
}

double get_time(void)
{
    char *string = NULL;
    size_t size = 0;
    ssize_t read;

    if ((read = getdelim(&string, &size, '}', stdin)) == -1) {
        perror("retime");
        free(string);
        exit(EXIT_FAILURE);
    }

    jsmn_parser parser;
    jsmn_init(&parser);

    /* Parse the JSON */
#define TOKBUF 1024
    jsmntok_t tokens[TOKBUF];
    if (tokens == NULL) {
        fputs("Allocation error\n", stderr);
        exit(EXIT_FAILURE);
    }
    int ret = jsmn_parse(&parser, string, strlen(string), tokens, TOKBUF);

#define JSON_ERR(STR)                                                          \
    fputs(STR, stderr);                                                        \
    free(string);                                                              \
    exit(EXIT_FAILURE);

    switch (ret) {
    case JSMN_ERROR_INVAL:
        JSON_ERR("bad token, JSON string is corrupted\n");
    case JSMN_ERROR_NOMEM:
        JSON_ERR("not enough tokens, JSON string is too large\n");
    case JSMN_ERROR_PART:
        JSON_ERR("JSON string is too short, expecting more JSON data\n");
    }

    /* Find the value of "cmt" */
    for (int i = 0; i < ret; i++) {
        if (tokens[i].type == JSMN_STRING) {
            int start = tokens[i].start, end = tokens[i].end;
            if (strncmp(&string[start], "cmt", end - start) == 0) {
                char *time = smalloc(end - start + 1);
                start = tokens[i + 1].start, end = tokens[i + 1].end;
                strncpy(time, &string[start], end - start);
                time[end - start] = '\0';

                free(string);
                return str_to_double(time);
            }
        }
    }

    return -1;
}

char *format_time(const double time)
{
#define FTIME_BUF 32
    char *formatted_time = smalloc(FTIME_BUF);
    const unsigned int hours = time / 3600,
                       minutes = fmod(time, (double) 3600) / 60,
                       seconds = fmod(time, (double) 60),
                       milliseconds = fmod(round(time * 1000), (double) 1000);

    if (!hours) {
        if (!minutes)
            snprintf(formatted_time, FTIME_BUF, "%u.%03u", seconds,
                     milliseconds);
        else
            snprintf(formatted_time, FTIME_BUF, "%u:%02u.%03u", minutes,
                     seconds, milliseconds);
    } else {
        snprintf(formatted_time, FTIME_BUF, "%u:%02u:%02u.%03u", hours, minutes,
                 seconds, milliseconds);
    }

    return formatted_time;
}

int main(int argc, char **argv)
{
    bool bflag = false, mflag = false;
    unsigned int fps = 0;

    int opt;
    while ((opt = getopt(argc, argv, ":bf:mhv")) != -1) {
        switch (opt) {
        case 'b':
            bflag = true;
            break;
        case 'f':
            fps = check_fps(optarg);
            break;
        case 'm':
            mflag = true;
            break;
        case 'h':
            fputs(HELP_MSG, stderr);
            return EXIT_SUCCESS;
        case 'v':
            fputs(VERSION_MSG, stderr);
            return EXIT_SUCCESS;
        default:
            if (optopt == 'f') {
                fputs("retime: option requires an argument -- 'f'\nTry "
                      "'retime -h' for more information\n",
                      stderr);
                return BAD_FPS;
            }

            fprintf(stderr,
                    "retime: invalid option -- '%c'\nTry 'retime -h' for "
                    "more information.\n",
                    optopt);
            return BAD_FLAG;
        }
    }

LOOP:
    if (!fps) {
        char *fpsstr = NULL;
        size_t size = 0;
        ssize_t read;

        fputs("Video Framerate: ", stderr);
        if ((read = getline(&fpsstr, &size, stdin)) == -1) {
            free(fpsstr);
            if (feof(stdin))
                exit(EXIT_SUCCESS);

            perror("retime");
            exit(EXIT_FAILURE);
        } else {
            fpsstr[read - 1] = '\0';
        }

        fps = check_fps(fpsstr);
        free(fpsstr);
    }

    /* Prompt the user for the start and end of the run */
    puts("Paste the debug info of the start of the run:");
    const unsigned int start_time = get_time() * fps / 1;
    puts("Paste the debug info of the end of the run:");
    const unsigned int end_time = get_time() * fps / 1;

    /* Clear the screen */
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    const double duration = (end_time - start_time) / (double) fps;
    char *formatted_duration = format_time(duration);

    if (mflag)
        printf("Mod Note: Retimed (Start: Frame %u, End: Frame %u, FPS: "
               "%u, "
               "Total Time: %s)\n",
               start_time, end_time, fps, formatted_duration);
    else
        printf("Final Time: %s\n", formatted_duration);

    free(formatted_duration);

    /* Loop when bulk_retime is true */
    if (bflag) {
        getchar();
        fps = 0;
        goto LOOP;
    }

    return EXIT_SUCCESS;
}
