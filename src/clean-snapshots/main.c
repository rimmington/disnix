/*
 * Disnix - A Nix-based distributed service deployment tool
 * Copyright (C) 2008-2015  Sander van der Burg
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <getopt.h>
#include <defaultoptions.h>
#include "clean-snapshots.h"

static void print_usage(const char *command)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s [--interface interface] [--target-property targetProperty] infrastructure_expr\n", command);
    fprintf(stderr, "%s {-h | --help}\n", command);
}

int main(int argc, char *argv[])
{
    /* Declarations */
    int c, option_index = 0;
    struct option long_options[] =
    {
	{"interface", required_argument, 0, 'i'},
	{"target-property", required_argument, 0, 't'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
    };
    char *interface = NULL;
    char *target_property = NULL;
    
    /* Parse command-line options */
    while((c = getopt_long(argc, argv, "h", long_options, &option_index)) != -1)
    {
	switch(c)
	{
	    case 'i':
		interface = optarg;
		break;
	    case 't':
		target_property = optarg;
		break;
	    case 'h':
	    case '?':
		print_usage(argv[0]);
		return 0;
	}
    }
    
    /* Validate options */
    
    interface = check_interface_option(interface);
    target_property = check_target_property_option(target_property);
    
    if(optind >= argc)
    {
	fprintf(stderr, "An infrastructure Nix expression has to be specified!\n");
	return 1;
    }
    else
	return clean_snapshots(interface, target_property, argv[optind]); /* Execute clean snapshots operation */
}