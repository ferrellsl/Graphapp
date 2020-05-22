#!/usr/local/bin/python

# This program generates the graphapp.h header file from
# the app.h header file.
#
# It works by finding all instances of function names
# which start with "app_", and writing a set of translations
# for example:
#   #define new_window app_new_window
#   #define is_enabled app_is_enabled
#   etc
#
# This means a program can include graphapp.h
# and can then use function names like "is_enabled" instead
# of the longer "app_is_enabled". This can make code a lot
# easier to understand.
#
# The reason why the C implementation of GraphApp has
# "app_" before each function name is to provide proper
# scope. But in practical use, it is sometimes a lot easier
# and clearer to omit the "app_" prefix.

import os, sys, string

MAX_NAME_LEN = 16

header = """/*
 *  GraphApp programming interface.
 *
 *  Include this header file to use the abbreviated
 *  function names.
 */

#include <app.h>

"""

additional = [
  ["malloc",			"app_alloc"],
  ["parent_window(c)",		"((c)->win)"],
  ["set_color",			"app_set_rgb"],
  ["set_colour",		"app_set_rgb"],
  ["new_color",			"app_new_rgb"],
  ["new_colour",		"app_new_rgb"],
  ["colors_equal",		"app_rgbs_equal"],
  ["colours_equal",		"app_rgbs_equal"],
  ["copy_rect(g,dp,src,sr)",	"((g)->copy_rect((g),(dp),(src),(sr)))"],
  ["fill_rect(g,r)",		"((g)->fill_rect((g),(r)))"],
  ["draw_utf8(g,p,utf8,nb)",	"((g)->draw_utf8((g),(p),(utf8),(nb)))"],
  ["draw_line(g,p1,p2)",	"((g)->draw_line((g),(p1),(p2)))"],
]

valid = [
	"alloc",	"malloc",
	"free",		"realloc",
	"arm",		"disarm",
	"check",	"uncheck",
	"disable",	"enable",
	"highlight",	"unhighlight",
	"exec",
	"delay",
	"beep",
	"error",
]

def main():
	# Read all lines from app.h
	try:
		lines = open("app.h").readlines()
	except:
		print "Could not open app.h"
		return

	translation = []

	# Build a translation table.
	for line in lines:
		pos = string.find(line, "app_")
		if pos < 0:
			continue
		if string.find(line, "#define") >= 0:
			continue
		name = line[pos:]
		for terminator in ["(", " ", "\n"]:
			pos  = string.find(name, terminator)
			if pos > 0:
				name = name[:pos]
		translation.append([name[4:], name])

	# Include any additional required translations.
	translation = translation + additional

	# Determine the width of the largest name.
	largest = 0
	for t in translation:
		if len(t[0]) > largest:
			largest = len(t[0])

	# Sort the table alphabetically.
	translation.sort()

	# Make a clash table. Clashes occur if MAX_NAME_LEN chars are
	# insufficient to distinguish all names.
	clashes = {}

	# Write the new graphapp.h file.
	f = open("graphapp.h", "w")
	f.write(header)
	for t in translation:
		f.write('#define ')
		f.write(("%-" + str(largest) + "s") % t[0])
		f.write(' ')
		f.write(t[1])
 		f.write('\n')
		try:
			reduced = ('app_' + t[0])[:MAX_NAME_LEN]
			clash = clashes[reduced]
			clash.append('app_' + t[0])
		except:
			clashes[reduced] = ['app_' + t[0]]
	f.close()

	clash_keys = clashes.keys()
	clash_keys.sort()
	for clash in clash_keys:
		if len(clashes[clash]) > 1:
			for name in clashes[clash]:
				sys.stderr.write('Warning: length %d exceeded: %s\n' % (MAX_NAME_LEN, name))

	# Report success.
	print "graphapp.h successfully created"

	verify = []
	for t in translation:
		if "_" not in t[0]:
			if t[0] not in valid:
				verify.append(t[0])

	if len(verify) > 0:
		print "please verify the following definitions are valid:"
		for v in verify:
			print '\t' + v

if __name__ == '__main__':
	main()

