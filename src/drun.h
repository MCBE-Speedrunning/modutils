#ifndef __DRUN_H_
#define __DRUN_H_

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
 * @brief Parse the JSON string to get the video URI
 * 
 * @param json The JSON to parse
 * @return char* The URI of the runs video
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
 */
void dl_json(const char *runid, string_t *json);

#endif