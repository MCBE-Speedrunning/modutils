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

float str_to_float(const char *str_time)
{
    unsigned int i = 0, sigfig = 0;
    float time = 0;

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

    const unsigned int fps = (unsigned int) str_to_float(string);

    if (fps > 60 || fps < 1)
        goto INVALID_FPS;

    return fps;

INVALID_FPS:
    fputs("retime: 'f' option must be a whole number from 1 to 60\n", stderr);
    exit(BAD_FPS);
}

float get_time(void)
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
#define CMTBUF 16
                char *time = smalloc(CMTBUF);
                start = tokens[i + 1].start, end = tokens[i + 1].end;
                strncpy(time, &string[start], end - start);
                time[end - start] = '\0';

                free(string);
                return str_to_float(time);
            }
        }
    }

    return -1;
}

char *format_time(const float time)
{
/*
 * 14 is the max length of a time that can be input into speedrun.com, +1 for 
 * '\0'
 */
#define FTIME_BUF 15
    char *formatted_time = smalloc(FTIME_BUF);
    const unsigned int hours = time / 3600, minutes = fmod(time, 3600) / 60,
                       seconds = fmod(time, 60),
                       milliseconds = fmod(trunc(round(time * 1000)), 1000);

    if (!hours) {
        if (!minutes)
            snprintf(formatted_time, FTIME_BUF, "%d.%03d", seconds,
                     milliseconds);
        else
            snprintf(formatted_time, FTIME_BUF, "%d:%02d.%03d", minutes,
                     seconds, milliseconds);
    } else {
        snprintf(formatted_time, FTIME_BUF, "%d:%02d:%02d.%03d", hours, minutes,
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
    if (fps == 0) {
        char *fpsstr = NULL;
        size_t size = 0;
        ssize_t read;

        fputs("Video Framerate: ", stderr);
        if ((read = getline(&fpsstr, &size, stdin)) == -1) {
            perror("retime");
            free(fpsstr);
            exit(EXIT_FAILURE);
        } else {
            fpsstr[read - 1] = '\0';
        }

        fps = check_fps(fpsstr);
        free(fpsstr);
    }

    /* Prompt the user for the start and end of the run */
    fputs("Paste the debug info of the start of the run:\n", stderr);
    size_t start_time = trunc(get_time() * fps);
    fputs("Paste the debug info of the end of the run:\n", stderr);
    size_t end_time = trunc(get_time() * fps);

    /* Clear the screen */
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    float duration = (end_time - start_time) / (float) fps;
    char *formatted_duration = format_time(duration);

    if (mflag)
        printf("Mod Note: Retimed (Start: Frame %zu, End: Frame %zu, FPS: "
               "%d, "
               "Total Time: %s)\n",
               start_time, end_time, fps, formatted_duration);
    else
        printf("Final Time: %s\n", formatted_duration);

    free(formatted_duration);

    /* Loop when bulk_retime is true */
    if (bflag) {
        fps = 0;
        fflush(stdin);
        goto LOOP;
    }

    return EXIT_SUCCESS;
}