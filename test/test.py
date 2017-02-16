#!/usr/bin/python2.7

import sys
import subprocess
import xml.etree.ElementTree as ET
from recipe import recipe

# How to improve this script:
# 	- print a report in HTML
#	- archive previous reports

PIN_PATH 				= "/home/plt/Documents/tools/pin-3.0-76991-gcc-linux/pin"
TOOL_PATH 				= "../lightTracer_pin/obj-ia32/lightTracer.so"
TOOL_SRC_PATH 			= "../lightTracer_pin/"
WHITELIST_PATH 			= "./whiteList/"
TRACE_PATH				= ""
LOG_PATH 				= "./log/"

if len(sys.argv) < 3:
	sys.stderr.write("ERROR: incorrect number of argument\n")
	sys.stderr.write("- 1 arg: recipe file name\n")
	sys.stderr.write("- 2 arg: action type: PRINT or BUILD or TRACE or SEARCH or ALL or WHL\n")
	sys.stderr.write("- 3 arg: name of specific test case [OPT]\n")
	exit()

file_name = sys.argv[1]
action = sys.argv[2]
if not(action == "PRINT" or action == "BUILD" or action == "TRACE" or action == "SEARCH" or action == "ALL" or action == "WHL"):
	sys.stderr.write("ERROR: incorrect action type\n")
	exit()

recipes = []

# PARSE step
try:
	tree = ET.parse(file_name)
	root = tree.getroot()
	for xml_recipe in root.findall("recipe"):
		r = recipe(xml_recipe)
		if len(sys.argv) == 4:
			if r.name == sys.argv[3]:
				recipes.append(r)
		elif r.active:
			recipes.append(r)
		else:
			print("\x1b[35mWARNING: " + r.name + " is desactivated\x1b[0m")
except IOError:
	sys.stderr.write("ERROR: unable to access recipe file: \"" + file_name + "\"\n")
	exit()


# PRINT step
if action == "PRINT":
	if len(sys.argv) == 4:
		if len(recipes) == 1:
			sys.stdout.write(str(recipes[0]))
			sys.stdout.flush()
			while True:
				cmd = sys.stdin.readline()
				sys.stdout.write(cmd)
				if cmd[:-1] == "exit":
					break
				else:
					sys.stdout.flush()
		else:
			sys.stderr.write("ERROR: incorrect test case name: " + sys.argv[3] + "\n")
	else:
		sys.stderr.write("ERROR: a third argument is expected (name of a specific test case)\n")

# BUILD step
if action == "BUILD" or action == "ALL":
	for r in recipes:
		r.build_prog(LOG_PATH)

# COMPILE TOOL step
if action == "TRACE" or action == "ALL":
	sys.stdout.write("Building Trace program: ... ")
	sys.stdout.flush()
	return_value = subprocess.call(["make", "-C", TOOL_SRC_PATH])
	if return_value != 0:
		sys.stderr.write("ERROR: unable to build Trace program\n")
		exit()

# TRACE step
if action == "TRACE" or action == "ALL":
	for r in recipes:
		r.trace_prog(LOG_PATH, PIN_PATH, TOOL_PATH, TRACE_PATH, WHITELIST_PATH)

# COMPILE SEARCH step
if action == "SEARCH" or action == "ALL":
	sys.stdout.write("Building analysis program: ... ")
	sys.stdout.flush()
	return_value = subprocess.call(["make"])
	if return_value != 0:
		sys.stderr.write("ERROR: unable to build analysis program\n")
		exit()

# SEARCH step
if action == "SEARCH" or action == "ALL":
	for r in recipes:
		r.search(LOG_PATH)

# WHL step
if action == "WHL":
	for r in recipes:
		r.create_whl(WHITELIST_PATH)
