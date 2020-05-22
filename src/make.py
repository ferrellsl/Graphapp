#!/usr/bin/python

# For each source file in GraphApp, compile it thus:
#  gcc -O2 -Wall -I. -Ietc -c -o objects/file.o subdir/file.c
# Then build the static library form of GraphApp, thus:
#  ar rc libapp.a objects/*.o
# Uses full paths.

print 'Compiling GraphApp'

import os, sys, string

target_platform      = 'x11'	# 'win32'

if target_platform == 'win32':
  project_base       = '/apps/app/'
  gcc_base           = '/MinGW/'
elif target_platform == 'x11':
  project_base       = '/home/loki/apps/app/'
  gcc_base           = '/usr/'

source_base          = project_base + 'src/'
object_base          = source_base +       'objects/'
gcc_inc              = gcc_base + 'include/'
gcc_bin              = gcc_base + 'bin/'
gcc                  = gcc_bin +       'gcc'
make_static_lib      = gcc_bin +       'ar rc'
ranlib_static_lib    = gcc_bin +       'ranlib'
gcc_flags            = '-O2 -Wall'
gcc_compile          = '-c'
gcc_outfile          = '-o'
static_lib           = source_base + 'libapp.a'
source_dirs          = [target_platform,
	'utility', 'gui',
	'libz', 'libpng', 'libjpeg', 'libgif', 'imgfmt',
	]
include_dirs         = ['.'] + source_dirs
verbose              = 1

def main():
	# Use full paths unless we're compiling within the source_base dir.
	srcb = source_base
	cwd = os.getcwd()
	if cwd == source_base or cwd == source_base[:-1]:
		srcb = ''
	# Determine include paths.
	inc = ''
	for id in include_dirs:
		inc = inc + '-I' + srcb + id + ' '
	inc = inc + '-I' + gcc_inc
	# Compile
	for sd in source_dirs:
		files = os.listdir(srcb + sd)
		for file in files:
			if file[-2:] == '.c':
				compile(srcb + sd + '/', file[:-2], '.c', inc)

	build_static_lib()
				
def compile(filepath, name, ext, include_paths):
	object_file = object_base + name + '.o'
	source_file = filepath + name + ext
	if newer(object_file, source_file):
		return
	cmd = gcc
	cmd = cmd + ' ' + gcc_flags
	cmd = cmd + ' ' + include_paths
	cmd = cmd + ' ' + gcc_compile
	cmd = cmd + ' ' + gcc_outfile + ' ' + object_file
	cmd = cmd + ' ' + source_file
	perform(cmd)

def newer(fileA, fileB):
	try:
		timestampA = os.stat(fileA).st_mtime
		timestampB = os.stat(fileB).st_mtime
		if timestampA > timestampB:
			return 1
		return 0
	except:
		return 0

def build_static_lib():
	try:
		os.remove(static_lib)
	except:
		pass

	cmd = make_static_lib + ' ' + static_lib + ' ' + object_base + '*.o'
	perform(cmd)

	if ranlib_static_lib:
		cmd = ranlib_static_lib + ' ' + static_lib
		perform(cmd)

def perform(cmd):
	if verbose:
		print cmd
	os.system(cmd)
	
if __name__ == '__main__':
	main()
