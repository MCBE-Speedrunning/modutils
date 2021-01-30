#ifndef __RETIME_H_
#define __RETIME_H_

/* Exit status */
#define BAD_YT_DEBUG 2
#define BAD_FPS      3
#define BAD_FLAG     4

/* -h message */
#define HELP_MSG                                                               \
    "Usage: retime [OPTIONS]... \n"                                            \
    "Retime a segment of a youtube video. \n"                                  \
    "Example: retime -mf 30 \n"                                                \
    " \n"                                                                      \
    "Functionality: \n"                                                        \
    "  -b                        bulk retime videos; \n"                       \
    "                              the b and m flags are preserved \n"         \
    "  -f                        set the FPS of the video being retimed \n"    \
    "  -m                        output a mod retime note as opposed to the "  \
    "end duration \n"                                                          \
    " \n"                                                                      \
    "Miscellaneous: \n"                                                        \
    "  -h                        display this help text and exit \n"           \
    "  -v                        display the version information and exit \n"  \
    " \n"                                                                      \
    "Exit status: \n"                                                          \
    " 0  if OK, \n"                                                            \
    " 1  if any sort of non-user related error occured, \n"                    \
    " 2  if invalid youtube debug info, \n"                                    \
    " 3  if invalid fps, \n"                                                   \
    " 4  if invalid flag or missing option. \n"

/* -v message */
#define VERSION_MSG                                                            \
    "retime v1.0 \n"                                                           \
    "License Unlicense: <https://unlicense.org> \n"                            \
    "This is part of the modutils collection; see \n"                          \
    "<https://www.github.com/MCBE-Speedrunning/modutils>"

/**
 * @brief malloc() but with error checking
 * 
 * @param size The number of bytes to allocate
 * @return void* A pointer to the allocated memory
 */
void *smalloc(const size_t size);

/**
 * @brief Convert a string to a double
 * 
 * @param str_time A double in the form of a string
 * @return double The converted double
 */
double str_to_double(const char *str_time);

/**
 * @brief Ensure that the input FPS is valid and convert it from a string to an
 * unsigned int
 * 
 * @param string The FPS in string form
 * @return unsigned int The FPS as an unsigned int
 */
unsigned int check_fps(char *string);

/**
 * @brief Get the total duration of the run
 * 
 * @return double The duration of the run
 */
double get_time(void);

/**
 * @brief Format the runs duration in the form HH:MM:SS.ms
 * 
 * @param time The runs duration in seconds
 * @return char* The formatted duration
 */
char *format_time(const double time);

#endif /* !__RETIME_H_ */