#!/usr/bin/python2.7

import sys
import subprocess
import re

PIN_PATH 		= "/home/pierre/Documents/pin-2.13-61206-gcc.4.4.7-linux/pin"
TOOL_PATH 		= "/home/pierre/Documents/bacs/tracer/obj-ia32/tracer.so"
WHITE_LIST_PATH = "/home/pierre/Documents/bacs/tracer/linux_lib.lst"
MAKEFILE_PATH 	= "/home/pierre/Documents/bacs/traceAnalysis/Makefile"
TRACE_PATH		= "/home/pierre/Documents/bacs/test/"
LOG_PATH 		= "/home/pierre/Documents/bacs/test/"

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


file = open(file_name, "r")
lines= file.readlines()
file.close();

recipe_name 	= []
recipe_build 	= []
recipe_cmd 		= []
recipe_arg 		= []
recipe_crypto 	= []
recipe_log 		= []

line_counter = 0;
recipe_counter = 0;

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
		if recipe_log[i] == None:
			recipe_log[i] = open(LOG_PATH + recipe_name[i] + ".log", "w")

		recipe_log[i].write("\n\n### BULID STDOUT & STDERR ###\n\n")
		recipe_log[i].flush()

		sys.stdout.write("Building " + str(i+1) + "/" + str(recipe_counter) + " " + recipe_name[i] + " ... ")
		sys.stdout.flush()

		return_value = subprocess.call(recipe_build[i].split(' '), stdout = recipe_log[i], stderr = recipe_log[i])
		if return_value == 0:
			sys.stdout.write("\x1b[32mOK\x1b[0m\n")
		else:
			sys.stdout.write("\x1b[31mFAIL\x1b[0m\x1b[0m (return code: " + str(return_value) + ")\n")

# TRACE step
if action == "TRACE" or action == "ALL":
	for i in range(recipe_counter):
		if recipe_log[i] == None:
			recipe_log[i] = open(LOG_PATH + recipe_name[i] + ".log", "w")

		recipe_log[i].write("\n\n### TRACE STDOUT & STDERR ###\n\n")
		recipe_log[i].flush()

		sys.stdout.write("Tracing " + str(i+1) + "/" + str(recipe_counter) + " " + recipe_name[i] + " ... ")
		sys.stdout.flush()

		return_value = subprocess.call([PIN_PATH, "-t", TOOL_PATH, "-o", TRACE_PATH + "trace" + recipe_name[i], "-w", WHITE_LIST_PATH, "--", recipe_cmd[i]], stdout = recipe_log[i], stderr = recipe_log[i])
		if return_value == 0:
			sys.stdout.write("\x1b[32mOK\x1b[0m\n")
		else:
			sys.stdout.write("\x1b[31mFAIL\x1b[0m\x1b[0m (return code: " + str(return_value) + ")\n")

# COMPILE step
if action == "SEARCH" or action == "ALL":
	makefile = open("Makefile", "w")
	return_value = subprocess.call(["sed", "s/^DEBUG[ \t]*:= 1/DEBUG := 0/g; s/-DVERBOSE //g; s/SRC_DIR[ \t]*:= src/SRC_DIR := ..\/traceAnalysis\/src/g", MAKEFILE_PATH], stdout = makefile)
	makefile.close()
	if return_value != 0:
		print("ERROR: unable to create the Makefile")
		exit()
	return_value = subprocess.call("make")
	if return_value != 0:
		print("ERROR: unable to build analysis program")
		exit()


# SEARCH step
if action == "SEARCH" or action == "ALL":
	for i in range(recipe_counter):
		if recipe_log[i] == None:
			recipe_log[i] = open(LOG_PATH + recipe_name[i] + ".log", "w")

		recipe_log[i].write("\n\n### SEARCH STDOUT & STDERR ###\n\n")
		recipe_log[i].flush()

		sys.stdout.write("Searching " + str(i+1) + "/" + str(recipe_counter) + " " + recipe_name[i] + " ... ")
		sys.stdout.flush()

		cmd = ["./analysis", TRACE_PATH + "trace" + recipe_name[i]]
		cmd.extend(recipe_arg[i].split(','))

		process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		process.wait()
		if process.returncode == 0:
			output_val = process.stdout.read()
			recipe_log[i].write(output_val)
			recipe_log[i].write(process.stderr.read())
			sys.stdout.write("\x1b[32mOK\x1b[0m\n")

			crypto_list = recipe_crypto[i].split(',')

			nb_found 	= []
			nb_expected = []
			crypto_name = []

			for j in crypto_list:
				nb_found.append(0)
				nb_expected.append(int(j[:j.find(':')]))
				crypto_name.append(j[j.find(':')+1:].strip())

			regex = re.compile("\*\*\* IO match for [a-zA-Z0-9 ]* \*\*\*")
			for j in regex.findall(output_val):
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
			sys.stdout.write("\x1b[31mFAIL\x1b[0m\x1b[0m (return code: " + str(return_value) + ")\n")

# CLEAN step
for i in range(min(len(recipe_name), len(recipe_cmd), len(recipe_arg), len(recipe_crypto))):
	if recipe_log[i] != None:
		recipe_log[i].close()
		recipe_log[i] = None