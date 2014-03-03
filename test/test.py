#!/usr/bin/python2.7

import sys
import subprocess
import re
import time
import history

# How to improve this script:
# 	- print a report in HTML
#	- archive previous reports

PIN_PATH 				= "/home/pierre/Documents/pin-2.13-62732-gcc.4.4.7-linux/pin"
TOOL_PATH 				= "/home/pierre/Documents/bacs/tracer/obj-ia32/tracer.so"
WHITE_LIST_PATH 		= "/home/pierre/Documents/bacs/tracer/linux_lib.lst"
MAKEFILE_TRACE_PATH 	= "/home/pierre/Documents/bacs/tracer/"
MAKEFILE_SEARCH_PATH 	= "/home/pierre/Documents/bacs/traceAnalysis/Makefile"
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

recipe_name 	= []
recipe_build 	= []
recipe_cmd 		= []
recipe_arg 		= []
recipe_crypto 	= []
recipe_log 		= []

line_counter = 0;
recipe_counter = 0;


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

	if name != None and build != None and cmd != None and arg != None and crypto != None:
		recipe_name.append(name)
		recipe_build.append(build)
		recipe_cmd.append(cmd)
		recipe_arg.append(arg)
		recipe_crypto.append(crypto)
		recipe_log.append(None)
		recipe_counter = recipe_counter + 1
	elif name == None and build == None and cmd == None and arg == None and crypto == None:
		pass
	else:
		print("ERROR: parsing at line " + str(line_counter) + " " + line)

	line_counter = line_counter + 1


# PRINT step
if action == "PRINT":
	for i in range(recipe_counter):
		print recipe_name[i], recipe_build[i], recipe_cmd[i], recipe_arg[i], recipe_crypto[i]


# BUILD step
if action == "BUILD" or action == "ALL":
	for i in range(recipe_counter):
		sys.stdout.write("Building " + str(i+1) + "/" + str(recipe_counter) + " " + recipe_name[i] + " ... ")
		sys.stdout.flush()

		if recipe_log[i] == None:
			recipe_log[i] = open(LOG_PATH + recipe_name[i] + ".log", "w")

		recipe_log[i].write("\n\n### BULID STDOUT & STDERR ###\n\n")
		recipe_log[i].flush()

		time_start = time.time()
		return_value = subprocess.call(recipe_build[i].split(' '), stdout = recipe_log[i], stderr = recipe_log[i])
		time_stop  = time.time()
		if return_value == 0:
			sys.stdout.write("\x1b[32mOK\x1b[0m - "+ str(time_stop - time_start) + "s\n")
		else:
			sys.stdout.write("\x1b[31mFAIL\x1b[0m\x1b[0m (return code: " + str(return_value) + ")\n")


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
	for i in range(recipe_counter):
		sys.stdout.write("Tracing " + str(i+1) + "/" + str(recipe_counter) + " " + recipe_name[i] + " ... ")
		sys.stdout.flush()

		if hist.hasFilesChanged([PIN_PATH, TOOL_PATH, WHITE_LIST_PATH, recipe_cmd[i]]):
			if recipe_log[i] == None:
				recipe_log[i] = open(LOG_PATH + recipe_name[i] + ".log", "w")

			recipe_log[i].write("\n\n### TRACE STDOUT & STDERR ###\n\n")
			recipe_log[i].flush()

			time_start = time.time()
			process = subprocess.Popen([PIN_PATH, "-t", TOOL_PATH, "-o", TRACE_PATH + "trace" + recipe_name[i], "-w", WHITE_LIST_PATH, "--", recipe_cmd[i]], stdout = subprocess.PIPE, stderr = subprocess.PIPE)
			process.wait()
			time_stop = time.time()

			output_val = process.communicate()
			recipe_log[i].write(output_val[0])
			recipe_log[i].write(output_val[1])

			if process.returncode == 0:
				sys.stdout.write("\x1b[32mOK\x1b[0m - "+ str(time_stop - time_start) + "s\n")
			else:
				sys.stdout.write("\x1b[31mFAIL\x1b[0m\x1b[0m (return code: " + str(process.returncode) + ")\n")
				print(output_val[1])

			regex = re.compile("ERROR: [a-zA-Z0-9 _,():]*")
			for j in regex.findall(output_val[0]):
				print j.replace("ERROR", "\x1b[35mERROR\x1b[0m")
		else:
			sys.stdout.write("\x1b[36mPASS\x1b[0m\n")


# COMPILE SEARCH step
if action == "SEARCH" or action == "ALL":
	if hist.hasFilesChanged([MAKEFILE_SEARCH_PATH]):
		makefile = open("Makefile", "w")
		return_value = subprocess.call(["sed", "s/^DEBUG[ \t]*:= 1/DEBUG := 0/g; s/^VERBOSE[ \t]*:= 1/VERBOSE := 0/g; s/SRC_DIR[ \t]*:= src/SRC_DIR := ..\/traceAnalysis\/src/g", MAKEFILE_SEARCH_PATH], stdout = makefile)
		makefile.close()
		if return_value != 0:
			print("ERROR: unable to create the Makefile")
			exit()
	sys.stdout.write("Building Search program: ... ")
	sys.stdout.flush()
	return_value = subprocess.call(["make", "analysis"])
	if return_value != 0:
		print("ERROR: unable to build Search program")
		exit()


# SEARCH step
if action == "SEARCH" or action == "ALL":
	for i in range(recipe_counter):
		sys.stdout.write("Searching " + str(i+1) + "/" + str(recipe_counter) + " " + recipe_name[i] + " ... ")
		sys.stdout.flush()

		ioChecker_file = recipe_arg[i][recipe_arg[i].find("load ioChecker") + 15:]
		ioChecker_file = ioChecker_file[:ioChecker_file.find(",")]
		trace_dir = recipe_arg[i][recipe_arg[i].find("load trace") + 11:]
		trace_dir = trace_dir[:trace_dir.find(",")]

		condition1 = hist.hasFilesChanged(["./analysis", ioChecker_file, trace_dir + "/ins.bin", trace_dir + "/op.bin", trace_dir + "/data.bin"])
		condition2 = hist.hasStringChanged("analysis" + recipe_name[i], recipe_arg[i])

		if condition1 or condition2:
			if recipe_log[i] == None:
				recipe_log[i] = open(LOG_PATH + recipe_name[i] + ".log", "w")

			recipe_log[i].write("\n\n### SEARCH STDOUT & STDERR ###\n\n")
			recipe_log[i].flush()

			cmd = ["./analysis"]
			cmd.extend(recipe_arg[i].split(','))

			time_start = time.time()
			process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
			process.wait()
			time_stop = time.time()

			output_val = process.communicate()
			recipe_log[i].write(output_val[0])
			recipe_log[i].write(output_val[1])

			if process.returncode == 0:
				sys.stdout.write("\x1b[32mOK\x1b[0m - " + str(time_stop - time_start) + "s\n")

				crypto_list = recipe_crypto[i].split(',')

				nb_found 	= []
				nb_expected = []
				crypto_name = []

				for j in crypto_list:
					nb_found.append(0)
					nb_expected.append(int(j[:j.find(':')]))
					crypto_name.append(j[j.find(':')+1:].strip())

				regex = re.compile("\*\*\* IO match for [a-zA-Z0-9 ]* \*\*\*")
				for j in regex.findall(output_val[0]):
					found = 0
					for k in range(len(crypto_list)):
						if crypto_name[k] == j[17:-4].strip():
							nb_found[k] = nb_found[k] + 1
							if nb_found[k] > nb_expected[k]:
								print("\t" + j[17:-4].strip() + " \x1b[33mEXTRA " + str(nb_found[k]) + "/" + str(nb_expected[k]) + "\x1b[0m")
								found = 1
								break
							else:
								print("\t" + j[17:-4].strip() + " \x1b[32mOK " + str(nb_found[k]) + "/" + str(nb_expected[k]) + "\x1b[0m")
								found = 1
								break
					if found == 0:
						print("\t" + j[17:-4].strip() + " \x1b[33mEXTRA ?/?\x1b[0m")

				for j in range(len(crypto_list)):
					if nb_found[j] < nb_expected[j]:
						for k in range(nb_expected[j] - nb_found[j]):
							print("\t" + crypto_name[j].strip() + " \x1b[31mFAIL " + str(nb_found[j] + k + 1) + "/" + str(nb_expected[j]) + "\x1b[0m")
			else:
				sys.stdout.write("\x1b[31mFAIL\x1b[0m\x1b[0m (return code: " + str(process.returncode) + ")\n")
				print(output_val[1])
		else:
			sys.stdout.write("\x1b[36mPASS\x1b[0m\n")


# CLEAN step
for i in range(min(len(recipe_name), len(recipe_cmd), len(recipe_arg), len(recipe_crypto))):
	if recipe_log[i] != None:
		recipe_log[i].close()
		recipe_log[i] = None
hist.save()