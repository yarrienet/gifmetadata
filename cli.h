// gifmetadata
// Copyright (C) 2025  Harry Stanton
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef GIFMETADATA_CLI_H
#define GIFMETADATA_CLI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @struct cli_user_args
 * @brief Holds the user provided arguments after being parsed.
 *
 * Structure holds the parsed command line arguments provided by the user.
 * Fields are provided for supported flags and an optional filename buffer
 * with size.
 *
 * The presence of a flag or buffer is denoted by a boolean integer or
 * non-null value.
 *
 * @var cli_user_args::all_flag
 * Marks the presence of the 'all' flag (-a / --all): display all GIF blocks,
 * not only the comment.
 *
 * @var cli_user_args::verbose_flag
 * Marks the presence of the 'verbose' flag (-v / --verbose): display more data
 * about the GIF.
 * 
 * @var cli_user_args::dev_flag
 * Marks the presence of the 'dev' flag (-d / --dev): print inner program
 * workings (intended for developers).
 *
 * @var cli_user_args::help_flag
 * Marks the presence of the 'help' flag (-h / --help): skip all other tasks
 * and print program help.
 *
 * @var cli_user_args::filename
 * A filename which may be optionally provided within command arguments by
 * including a loose string. The buffer is dynamically allocated according to
 * the length of the string, size of the buffer is stored within
 * @ref cli_ user_args::filename_size. Must be freed using @ref free_user_args.
 * 
 * @var cli_user_args::filename_size
 * Size of the @ref cli_user_args::filename buffer in bytes.
 *
 * @note @ref cli_user_args::filename is an allocated buffer and needs to be
 *       manually deallocated when deallocating the structure itself. Use
 *       Use helper function @ref cli_free_user_args to deallocate the struct
 *       and all fields.
 *     
 */
typedef struct cli_user_args {
    int all_flag; ///< Marks the presence of the 'all' flag
    int verbose_flag; ///< Marks the presence of the 'verbose' flag
    int dev_flag; ///< Marks the presence of the 'dev' flag
    int help_flag; ///< Mark the presence of the 'help' flag
    char *filename; ///< Optional filename buffer
    size_t filename_size; ///< Size of filename buffer in bytes
} cli_user_args;

/**
 * @brief Frees the memory allocated for the @ref cli_user_args struct.
 *
  Function deallocates the @ref cli_user_args structure and all related fields.
 *
 * @param args A pointer to the @ref cli_user_args struct to be freed.
 */
void cli_free_user_args(cli_user_args *a);
/**
 * @brief Parse command line arguments to produce a structure containing the
 *        presence of supported flags and arguments.
 *
 * Function to parse command line arguments and produce a structure containing
 * the presence of supported flags and arguments. Only embedded and known flags
 * and arguments are supported, the presence of unknowns will produce an error.
 *
 * @param argc Length of command line arguments, usually provided from the main
 *             method.
 *
 * @param argv List of command line arguments, usually provided from the main
 *             method.
 *
 * @return An allocated @ref cli_user_args structure, must be freed before
           program exit. Use helper function @ref free_user_args to free
           structure and allocated fields. On error, null will be returned and 
           message will be printed in stderr.
 */
cli_user_args *cli_parse(int argc, char **argv);

#endif
