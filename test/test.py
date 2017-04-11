#!/usr/bin/python2.7

import sys
import subprocess
import xml.etree.ElementTree as ET
from recipe import recipe
import re

PATH 			= "/home/plt/Documents/bacs/"
PIN_PATH 		= "/home/plt/Documents/tools/pin-3.2-81205-gcc-linux/pin"
TOOL_PATH 		= PATH + "lightTracer_pin/obj-ia32/lightTracer.so"
TOOL_SRC_PATH 	= PATH + "lightTracer_pin/"
WHITELIST_PATH 	= PATH + "test/whiteList/"
TRACE_PATH		= PATH + "test/"
LOG_PATH 		= PATH + "test/log/"

if len(sys.argv) < 3:
	sys.stderr.write("ERROR: incorrect number of argument\n")
	sys.stderr.write("- 1 arg: recipe file name\n")
	sys.stderr.write("- 2 arg: action type: PRINT or BUILD or TRACE or SEARCH or ALL or WHL\n")
	sys.stderr.write("- 3 arg: regex to filter the synthetic samples [OPT]\n")
	exit()

file_name = sys.argv[1]
action = sys.argv[2]
if not(action == "PRINT" or action == "BUILD" or action == "TRACE" or action == "SEARCH" or action == "ALL" or action == "WHL"):
	sys.stderr.write("ERROR: incorrect action type\n")
	exit()

recipes = []

if len(sys.argv) >= 4:
	sys.argv[3] = re.compile(sys.argv[3])

# PARSE step
try:
	tree = ET.parse(file_name)
	root = tree.getroot()
	for xml_recipe in root.findall("recipe"):
		r = recipe(xml_recipe)
		if len(sys.argv) >= 4:
			if sys.argv[3].match(r.name) != None:
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
		sys.stderr.write("ERROR: incorrect number of synthetic samples: " + str(len(recipes)) + "\n")

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
	sys.stdout.write("Building Analysis program: ... ")
	sys.stdout.flush()
	return_value = subprocess.call(["make"])
	if return_value != 0:
		sys.stderr.write("ERROR: unable to build Analysis program\n")
		exit()

# SEARCH step
if action == "SEARCH" or action == "ALL":
	for r in recipes:
		r.search(LOG_PATH)

# WHL step
if action == "WHL":
	for r in recipes:
		r.create_whl(WHITELIST_PATH)
