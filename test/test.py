#!/usr/bin/python2.7

import sys
import subprocess
import history
from recipe import ioRecipe, sigRecipe

# How to improve this script:
# 	- print a report in HTML
#	- archive previous reports

PIN_PATH 				= "/home/pierre/Documents/pin-2.13-62732-gcc.4.4.7-linux/pin"
TOOL_PATH 				= "/home/pierre/Documents/bacs/tracer/obj-ia32/tracer.so"
WHITE_LIST_PATH 		= "/home/pierre/Documents/bacs/tracer/linux_lib.lst"
MAKEFILE_TRACE_PATH 	= "/home/pierre/Documents/bacs/tracer/"
MAKEFILE_ANAL_PATH 		= "/home/pierre/Documents/bacs/traceAnalysis/Makefile"
MAKEFILE_SIG_PATH 		= "/home/pierre/Documents/bacs/staticSignature/Makefile"
TRACE_PATH				= "/home/pierre/Documents/bacs/test/"
LOG_PATH 				= "/home/pierre/Documents/bacs/test/"
HISTORY_FILE_PATH 		= "/home/pierre/Documents/bacs/test/.testHistory"

if len(sys.argv) != 3:
	print("ERROR: incorrect number of argument")
	print("- 1 arg: recipe file name")
	print("- 2 arg: action type: PRINT or BUILD or TRACE or SEARCH or ALL")
	exit()

file_name = sys.argv[1]
action = sys.argv[2]
if not(action == "PRINT" or action == "BUILD" or action == "TRACE" or action == "SEARCH" or action == "ALL"):
	print("ERROR: incorrect action type")
	exit()

hist = history.history(HISTORY_FILE_PATH)

try:
	file = open(file_name, "r")
	lines= file.readlines()
	file.close()
except IOError:
	print("ERROR: unable to access recipe file: \"" + file_name + "\"")
	exit()

recipe 			= []
line_counter 	= 0;

# VERIFY step
if action == "TRACE" or action == "ALL":
	file = open("/proc/sys/kernel/yama/ptrace_scope", "r")
	if int(file.read()) == 1:
		print("ERROR: the Operating System configuration prevents Pin from using the default (parent) injection mode")
		file.close()
		exit()
	file.close()


# PARSE step
for line in lines:
	name 	= None
	kind 	= None
	build 	= None
	cmd 	= None
	arg 	= None
	crypto 	= None

	escap_char_index = line.find('#')
	if escap_char_index >= 0:
		line = line[:escap_char_index]

	bracket_in = line.find('[')
	bracket_out = line.find(']')
	if bracket_in >= 0 and bracket_out > bracket_in:
		name = line[bracket_in+1:bracket_out].strip()
		line = line[bracket_out+1:]

		bracket_in = line.find('[')
		bracket_out = line.find(']')
		if bracket_in >= 0 and bracket_out > bracket_in:
			kind = line[bracket_in+1:bracket_out].strip()
			line = line[bracket_out+1:]

			bracket_in = line.find('[')
			bracket_out = line.find(']')
			if bracket_in >= 0 and bracket_out > bracket_in:
				build = line[bracket_in+1:bracket_out]
				line = line[bracket_out+1:]

				bracket_in = line.find('[')
				bracket_out = line.find(']')
				if bracket_in >= 0 and bracket_out > bracket_in:
					cmd = line[bracket_in+1:bracket_out]
					line = line[bracket_out+1:]

					bracket_in = line.find('[')
					bracket_out = line.find(']')
					if bracket_in >= 0 and bracket_out > bracket_in:
						arg = line[bracket_in+1:bracket_out]
						line = line[bracket_out+1:]

						bracket_in = line.find('[')
						bracket_out = line.find(']')
						if bracket_in >= 0 and bracket_out > bracket_in:
							crypto = line[bracket_in+1:bracket_out]
							line = line[bracket_out+1:]

	if name != None and kind != None and build != None and cmd != None and arg != None and crypto != None:
		if kind == "io":
			recipe.append(ioRecipe(name, build, cmd, arg, crypto))
		elif kind == "sig":
			recipe.append(sigRecipe(name, build, cmd, arg, crypto))
		else:
			print("ERROR: incorrect recipe type: \"" + kind + "\"")
	elif name == None and kind == None and build == None and cmd == None and arg == None and crypto == None:
		pass
	else:
		print("ERROR: parsing at line " + str(line_counter) + " " + line)

	line_counter = line_counter + 1


# PRINT step
if action == "PRINT":
	for r in recipe:
		print r


# BUILD step
if action == "BUILD" or action == "ALL":
	for r in recipe:
		r.build_prog(LOG_PATH)


# COMPILE TRACE step
if action == "TRACE" or action == "ALL":
	sys.stdout.write("Building Trace program: ... ")
	sys.stdout.flush()
	return_value = subprocess.call(["make", "-C", MAKEFILE_TRACE_PATH])
	if return_value != 0:
		print("ERROR: unable to build Trace program")
		exit()


# TRACE step
if action == "TRACE" or action == "ALL":
	for r in recipe:
		r.trace_prog(hist, LOG_PATH, PIN_PATH, TOOL_PATH, WHITE_LIST_PATH, TRACE_PATH)


# COMPILE SEARCH step
if action == "SEARCH" or action == "ALL":
	if hist.hasFilesChanged([MAKEFILE_ANAL_PATH]):
		makefile = open("makefile_anal", "w")
		return_value = subprocess.call(["sed", "s/^DEBUG[ \t]*:= 1/DEBUG := 0/g; s/^VERBOSE[ \t]*:= 1/VERBOSE := 0/g; s/SRC_DIR[ \t]*:= src/SRC_DIR := ..\/traceAnalysis\/src/g; s/BUILD_DIR[ \t]*:= build/BUILD_DIR := build_anal/g", MAKEFILE_ANAL_PATH], stdout = makefile)
		makefile.close()
		if return_value != 0:
			print("ERROR: unable to create the Makefile")
			exit()
	sys.stdout.write("Building Analysis program: ... ")
	sys.stdout.flush()
	return_value = subprocess.call(["make", "--makefile=makefile_anal", "analysis"])
	if return_value != 0:
		print("ERROR: unable to build Analysis program")
		exit()
	if hist.hasFilesChanged([MAKEFILE_SIG_PATH]):
		makefile = open("makefile_sig", "w")
		return_value = subprocess.call(["sed", "s/^DEBUG[ \t]*:= 1/DEBUG := 0/g; s/^VERBOSE[ \t]*:= 1/VERBOSE := 0/g; s/SRC_DIR[ \t]*:= src/SRC_DIR := ..\/staticSignature\/src/g; s/BUILD_DIR[ \t]*:= build/BUILD_DIR := build_sig/g", MAKEFILE_SIG_PATH], stdout = makefile)
		makefile.close()
		if return_value != 0:
			print("ERROR: unable to create the Makefile")
			exit()
	sys.stdout.write("Building Signature program: ... ")
	sys.stdout.flush()
	return_value = subprocess.call(["make", "--makefile=makefile_sig", "signature"])
	if return_value != 0:
		print("ERROR: unable to build Signature program")
		exit()


# SEARCH step
if action == "SEARCH" or action == "ALL":
	for r in recipe:
		r.search(hist, LOG_PATH)


# CLEAN step
hist.save()