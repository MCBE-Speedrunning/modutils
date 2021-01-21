#ifndef __DRUN_H_
#define __DRUN_H_

#ifndef size_t
#    include <stdio.h>
#endif

typedef struct {
    char *ptr;
    size_t len;
} string_t;

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