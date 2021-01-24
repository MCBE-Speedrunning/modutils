#ifndef __DRUN_H_
#define __DRUN_H_

/* -h message */
#define HELP_MSG                                                               \
    "Usage: drun [OPTIONS]... \n"                                              \
    "A speedrun moderation tool to find stolen videos from STDIN \n"           \
    "Example: echo 'https://www.speedrun.com/mcbe/run/yj6wel3z' | drun \n"     \
    "\n"                                                                       \
    "Miscellaneous: \n"                                                        \
    "  -h                        display this help text and exit \n"           \
    "  -v                        display version information and exit \n"      \
    "\n"                                                                       \
    "When a url is read from standard input, it will be compared against the " \
    "\n"                                                                       \
    "database file located in ~/.local/share/drun/runs. \n"                    \
    "If no match is found, the run will be added to the database."

/* -v message */
#define VERSION_MSG                                                            \
    "drun v1.0 \n"                                                             \
    "License Unlicense: <https://unlicense.org> \n"                            \
    "This is part of the modutils collection; see \n"                          \
    "<https://www.github.com/MCBE-Speedrunning/modutils>"

#ifndef size_t
#    include <stdio.h>
#endif

/**
 * @brief A simple struct representing a string, to make working with them a
 * a bit easier
 * 
 * @param ptr A pointer to the beginning of the string
 * @param len The length of the string
 */
typedef struct {
    char *ptr;
    size_t len;
} string_t;

/**
 * @brief A struct containing all the information required for the given run
 * 
 * @param json A string_t containing the result of the API request
 * @param id The ID of the run on sr.c
 * @param vid The uri of the runs video
 */
typedef struct {
    string_t json;
    char *id;
    char *vid;
} run_t;

/**
 * @brief Search the `runs` file for runs with the same video
 * 
 * @param fp Pointer to the `runs` file
 * @param video_uri The uri to look for a duplicate of
 * @return char* The uri of the duplicate run, if none found this is NULL
 */
char *find_duplicate(FILE *fp, const char *video_uri);

/**
 * @brief Parse the JSON string to get the video URI
 * 
 * @param json The JSON to parse
 * @return char* The URI of the runs video, if no video is found the return
 * value is NULL
 */
char *parse_json(string_t *json);

/**
 * @brief Initialze the `string_t` struct
 * 
 * @param json Pointer to the struct to initialze
 */
void init_string(string_t *json);

/**
 * @brief Read the incoming bytes from curl into `json`
 * 
 * @param ptr A pointer to the delivered data
 * @param size 1
 * @param nmemb Number of bytes recieved
 * @param json pointer to the string_t struct where the json will be stored
 * @return size_t The number of bytes taken care of
 */
size_t write_callback(const void *ptr, const size_t size, const size_t nmemb,
                      string_t *json);

/**
 * @brief Download the contents of the API request to `json`
 * 
 * @param runid The ID of the run to get the json of
 * @param json Where to store the json
 * @return char* The sr.c run URI
 */
char *dl_json(const char *runid, string_t *json);

/**
 * @brief Read the run id from stdin into `run->id`
 * 
 * @param run The run_t struct to store the id into 
 */
void get_id(run_t *run);

#endif /* !__DRUN_H_ */