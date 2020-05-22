#!/usr/bin/python

# A source-tree viewer program (for use with SSI).
# Produces HTML for viewing a .tar.gz file, by
# parsing the data returned from a pipe to gunzip.
# Does not use or require the tar program, since it
# parses the file itself in chunks, for efficiency.
# Bugs: source path name and script URL are hard-coded.
# Features: can sort the listing by date, size or name.

import os, sys, string, cgi, time

this_script = "/cgi-bin/src.cgi"

base_dir = "../root/software/graphapp/"
archive = base_dir + "download/GraphApp.3.tar.gz"
tmp_dir = base_dir + "temp/"
pub_dir = base_dir + "pub/"
old_dir = base_dir + "oldpub/"

gunzip = "gunzip"
chunk  = 512
tar = "tar"
tar_extract = tar + " -zxO"

def main():
	sort = ""
	term = ""

	# Try to fetch query from URI
	try:
		query = os.environ["REQUEST_URI"]
		pos = string.find(query, "/pub/")
		if pos >= 0:
			query = query[pos+5:]
		pos = string.find(query, "?")
		if pos >= 0:
			sort = query[pos+1:]
			query = query[:pos]
			if sort[:3] == "by=":
				sort = sort[3:]
		term = query
	except:
		pass

	# Now try the QUERY_STRING
	try:
		query = os.environ["QUERY_STRING_UNESCAPED"]
		if query[:3] == "by=":
			query = query[3:]
			while query[:1] not in ['&', '']:
				sort = sort + query[:1]
				query = query[1:]
		if query[:1] == '&':
			query = query[1:]
		if query[:4] == "get=":
			term = query[4:]
	except:
		try:
			dict = cgi.parse()
			if "get" in dict.keys():
				term = dict["get"][0]
			if "by" in dict.keys():
				sort = dict["by"][0]
		except:
			pass

	if term == "" or term[-1] == '/':
		print_list(term, sort)
	else:
		base = get_base_dir()
		if term[:len(base)] != base:
			term = base + term
		extract_file(term)

SSI_FILE = """<!--#if expr="ssi-on = ssi-on" -->
<!--#include virtual="/cgi-bin/srcssi.cgi" -->
<!--#else -->
<html>
 <head>
  <title>Redirect In Progress</title>
  <meta http-equiv="refresh" content="2; url=/cgi-bin/src.cgi">
 </head>
 <body bgcolor="white">
 <center>
 Please wait while you are redirected to the
 <a href="/cgi-bin/src.cgi">GraphApp source tree</a>.
 </center>
 </body>
</html>
<!--#endif -->
"""

def build_disk_tree():
	# Build an on-disk tree of the contents of the archive.
	# Remove the old tree then swap to the new one.
	# I.e. "rm -R * pub/; mv temp pub" by smarter than that.
	# We cheat here to save disk space, by using links to
	# a server-side include file which runs this CGI script.
	# Massive cross-linking should reduce disk space usage.

	# Create temporary directory and master SSI file
	os.system("mkdir " + tmp_dir)
	index = "index.html"
	master = tmp_dir + index
	open(master, "w").write(SSI_FILE)
	os.system("chmod 755 " + master)

	# Build linked directory structure
	list = get_list()
	base,date,size = list[0]
	for path,date,size in list[1:]:
		if path[:len(base)] == base:
			path = path[len(base):]
		if path[-1:] == '/':
			folder = tmp_dir + path
			file = folder + index
			os.system("mkdir " + folder)
			os.system("ln " + master + " " + file)
		else:
			file = tmp_dir + path
			os.system("ln " + master + " " + file)

	# Fast, almost atomic, swap
	os.system("mv " + pub_dir + " " + old_dir)
	os.system("mv " + tmp_dir + " " + pub_dir)

	# Delete old directory structure
	os.system("rm -R " + old_dir)

def sort_by_name(s,t):
	if s[0] < t[0]:
		return -1
	elif s[0] > t[0]:
		return 1
	else:
		return 0

def sort_by_oldest(s,t):
	if s[1] < t[1]:
		return -1
	elif s[1] > t[1]:
		return 1
	else:
		return sort_by_name(s,t)

def sort_by_newest(s,t):
	if s[1] > t[1]:
		return -1
	elif s[1] < t[1]:
		return 1
	else:
		return sort_by_name(s,t)

def sort_by_smallest(s,t):
	if s[2] < t[2]:
		return -1
	elif s[2] > t[2]:
		return 1
	else:
		return sort_by_name(s,t)

def sort_by_largest(s,t):
	if s[2] > t[2]:
		return -1
	elif s[2] < t[2]:
		return 1
	else:
		return sort_by_name(s,t)

def print_list(term="", by=""):
	# Print a summary listing of the files.

	print 'Content-type: text/html'
	print
	print '<HTML>'
	print '<HEAD><TITLE>%s: %s</TITLE></HEAD>' % ("GraphApp 3 Source", term)
	print '<BODY BGCOLOR="white">'
	list = get_list()
	base = ""
	if list != []:
		path,date,size = list[0]
		base = path
		if term == "":
			term = path
		elif term[:len(base)] != base:
			term = base + term
		print '<H1>' + term + '</H1>'

	# Sort the list. Default is sorted by name.
	if by == "oldest":
		list.sort(sort_by_oldest)
	elif by == "newest":
		list.sort(sort_by_newest)
	elif by == "smallest":
		list.sort(sort_by_smallest)
	elif by == "largest":
		list.sort(sort_by_largest)

	# Find longest filename, to use in spacing.
	longest = 0
	for path,date,size in list[1:]:
		if path[:len(term)] == term:
			short = path[len(term):]
			if is_in_sub_dir(short):
				continue
			if len(short) > longest:
				longest = len(short)
	if longest < 15:
		longest = 15
	print '<PRE>'
	line = ' <IMG SRC="/images/icons/blank.png"  WIDTH="20" HEIGHT="22"> '

	# Print name
	if by in ["", "name"]:
		line = line + "<B>"
	line = line + '<A HREF="">Name</A>'	# this directory
	if by in ["", "name"]:
		line = line + "</B>"
	line = line + ' ' * longest

	# Print modification date
	if by in ["oldest", "newest"]:
		line = line + "<B>"
	if by == "oldest":
		line = line + '<A HREF="%s?by=newest&get=%s">Oldest Modification</A>' % (this_script, term)
	elif by == "newest":
		line = line + '<A HREF="%s?by=oldest&get=%s">Newest Modification</A>' % (this_script, term)
	else:
		line = line + '<A HREF="%s?by=newest&get=%s">Last Modification</A>' % (this_script, term)
	if by in ["oldest", "newest"]:
		line = line + "</B>"
		line = line + ' ' * 10
	else:
		line = line + ' ' * 12

	# Print size
	if by in ["smallest", "largest"]:
		line = line + "<B>"
	if by == "largest":
		line = line + '<A HREF="%s?by=smallest&get=%s">Size</A>' % (this_script, term)
	else:
		line = line + '<A HREF="%s?by=largest&get=%s">Size</A>' % (this_script, term)
	if by in ["smallest", "largest"]:
		line = line + "</B>"
	print line

	print '<HR NOSHADE SIZE="1">'

	for path,date,size in list[1:]:
		if path[:len(term)] == term:
			short = path[len(term):]
			path = path[string.rfind(path[:-1], "/")+1:]
			if is_in_sub_dir(short):
				continue
			line = " "
			if ends_with(path, '/'):
				line = line + '<IMG SRC="/images/icons/folder.png" WIDTH="20" HEIGHT="22">'
			elif ends_with(path, [".png", ".gif", ".jpg"]):
				line = line + '<IMG SRC="/images/icons/image.png"  WIDTH="20" HEIGHT="22">'
			elif ends_with(path, [".c", ".h"]):
				line = line + '<IMG SRC="/images/icons/c.png"      WIDTH="20" HEIGHT="22">'
			else:
				line = line + '<IMG SRC="/images/icons/text.png"   WIDTH="20" HEIGHT="22">'
			line = line + ' <A HREF="'
			if by != "":
				line = line + this_script + "?"
				line = line + "by=" + by + "&"
				line = line + "get="
			line = line + path
			line = line + '">' + short + '</A>'
			line = line + ' ' * (longest - len(short))
			line = line + ' ' * 4 + time.asctime(time.localtime(date))
			line = line + ' ' * 5
			if size == 0:
				line = line + "   - "
			elif size < 1024:
				line = line + "%4d" % size + " "
			elif size < 1024*1024:
				line = line + "%4d" % (size/1024) + "k"
			else:
				line = line + "%4d" % (size/1024/1024) + "M"
			print line
	print '<HR NOSHADE SIZE="1">'
	print '</PRE>'
	print '</BODY>'
	print '</HTML>'

def ends_with(str, suffixes):
	if type(suffixes) == type(""):
		suffixes = [suffixes]
	for suffix in suffixes:
		if string.lower(str[-len(suffix):]) == string.lower(suffix):
			return 1
	return 0

def get_base_dir():
	# Get base directory only from the archive.

	cmd = "%s < %s" % (gunzip, archive)
	try:
		f = os.popen(cmd)
	except IOError:
		return []
	data = f.read(chunk)
	base = ""
	for ch in data:
		if ch == '\0':
			break
		base = base + ch
	f.close()
	return base

def get_list():
	# Open a pipe to gunzip to extract the data nicely.

	cmd = "%s < %s" % (gunzip, archive)
	try:
		f = os.popen(cmd)
	except IOError:
		return []
        list = list_tar(f)
	return fix_directory_timestamps(list)

def list_tar(file):
	# Read the tar file in chunks and look for the
	# base file name (first one listed in the file)
	# to determine where the filenames and timestamps are located.

	data = file.read(chunk)
	base = ""
	for ch in data:
		if ch == '\0':
			break
		base = base + ch

	list = []
	pos = string.find(data, base)
	while 1:
		while pos < 0:
			data = file.read(chunk)
			if data == "":
				break
			pos = string.find(data, base)
		if data == "":
			break

		# Find next pathname listed in the tar file
		i = pos
		path = ""
		while data[i] != '\0':
			path = path + data[i]
			i = i + 1
		# Read the timestamp/permissions info associated with it
		info = [0.0] * 7
		for n in range(len(info)):
			while data[i] == '\0':	# skip NULs
				i = i + 1
			while data[i] != '\0':
				info[n]  = info[n] * 8 + ord(data[i]) - ord('0')
				i = i + 1
		list.append((path, info[4], int(info[3])))

		# Go to next occurrence
		data = data[i:]
		pos = string.find(data, base)

	list.sort()
	return list

def fix_directory_timestamps(list):
	# Problem: directory timestamps are often newer than
	# the files within. Why? Because there might have been
	# objects compiled within which caused an update of
	# the directory but were not included in the tar file.
	#
	# Solution: a directory's timestamp is equal to its
	# most recent constituent file or directory (recursive).
	# If the directory has no constituents, leave its
	# timestamp alone.
	#
	# This function performs this adjustment on the list.

	update = {}
	list.reverse()
	for path,date,size in list:
		pos = string.rfind(path[:-1], '/')
		parent = path[:pos+1]

		if path[-1:] == '/':		# this is a folder
			try:			# get date from children
				date = update[path]
			except:
				pass		# else from self
		try:
			max = update[parent]	# get parent's max date
		except:
			max = date		# else use this child's
		if max < date:
			max = date		# use more recent date
		update[parent] = max

	list.reverse()
	newlist = []
	for path,date,size in list:
		try:
			date = update[path]
		except:
			pass
		newlist.append((path, date, size))

	return newlist

def extract_file(term):
	# Open a pipe to tar+gunzip and extract the correct file.

	type = 'text/plain'
	if term[-4:] in ['.png', '.gif']:
		type = 'image/' + term[-3:]
	elif term[-4:] == '.jpg':
		type = 'image/jpeg'
	elif term[-5:] == '.html':
		type = 'text/html'
	elif term[-4:] == '.htm':
		type = 'text/html'
	print 'Content-type: ' + type
	print
	sys.stdout.flush()
	cmd = "%s %s < %s" % (tar_extract, term, archive)
	f = os.popen(cmd)
	sys.stdout.write(f.read())
	f.close()

def is_in_sub_dir(item):
	if item == "":
		return 1
	if '/' in item:
		pos = string.find(item, '/')
		if pos != len(item)-1:
			return 1
	return 0

if __name__ == '__main__':
	main()

