#!/usr/bin/python2.7

import sys
import subprocess
import xml.etree.ElementTree as ET
from recipe import recipe

# How to improve this script:
# 	- print a report in HTML
#	- archive previous reports

PIN_PATH 				= "/home/pierre/Documents/tool/pin-2.14-71313-gcc.4.4.7-linux/pin"
WHITE_LIST_PATH 		= "/home/pierre/Documents/bacs/tracer/linux_lib.lst"
MAKEFILE_ANAL_PATH 		= "/home/pierre/Documents/bacs/traceAnalysis/Makefile"
MAKEFILE_SIG_PATH 		= "/home/pierre/Documents/bacs/staticSignature/Makefile"
TRACE_PATH				= "/home/pierre/Documents/bacs/test/"
LOG_PATH 				= "/home/pierre/Documents/bacs/test/log/"

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

recipes = []

# VERIFY step
if action == "TRACE" or action == "ALL":
	file = open("/proc/sys/kernel/yama/ptrace_scope", "r")
	if int(file.read()) == 1:
		print("ERROR: the Operating System configuration prevents Pin from using the default (parent) injection mode")
		file.close()
		exit()
	file.close()

# PARSE step
try:
	tree = ET.parse(file_name)
	root = tree.getroot()
	for xml_recipe in root.findall("recipe"):
		name 	= None
		kind 	= None
		build 	= None
		trace 	= None
		arg 	= []
		algo 	= {}

		name 	= xml_recipe.attrib.get("name")
		build 	= xml_recipe.find("build").attrib.get("cmd")
		trace 	= xml_recipe.find("trace").attrib.get("cmd")

		for xml_arg in xml_recipe.find("search").findall("arg"):
			arg.append(xml_arg.attrib.get("value"))
		for xml_algo in xml_recipe.find("result").findall("algo"):
			algo[xml_algo.attrib.get("name")] = int(xml_algo.attrib.get("number"))

		if xml_recipe.attrib.get("active") == "yes":
			recipes.append(recipe(name, build, trace, arg, algo))

		else:
			print("\x1b[35mWARNING: " + name + " is desactivated\x1b[0m")
except IOError:
	print("ERROR: unable to access recipe file: \"" + file_name + "\"")
	exit()


# PRINT step
if action == "PRINT":
	for r in recipes:
		print r

# BUILD step
if action == "BUILD" or action == "ALL":
	for r in recipes:
		r.build_prog(LOG_PATH)

# TRACE step
if action == "TRACE" or action == "ALL":
	for r in recipes:
		r.trace_prog(LOG_PATH, PIN_PATH, WHITE_LIST_PATH, TRACE_PATH)

# COMPILE SEARCH step
if action == "SEARCH" or action == "ALL":
	makefile = open("makefile", "w")
	return_value = subprocess.call(["sed", "s/^DEBUG[ \t]*:= 1/DEBUG := 0/g; s/^VERBOSE[ \t]*:= 1/VERBOSE := 0/g; s/SRC_DIR[ \t]*:= src/SRC_DIR := ..\/staticSignature\/src/g", MAKEFILE_SIG_PATH], stdout = makefile)
	makefile.close()
	if return_value != 0:
		print("ERROR: unable to create the Makefile")
		exit()
	sys.stdout.write("Building Signature program: ... ")
	sys.stdout.flush()
	return_value = subprocess.call(["make", "signature"])
	if return_value != 0:
		print("ERROR: unable to build Signature program")
		exit()

# SEARCH step
if action == "SEARCH" or action == "ALL":
	for r in recipes:
		r.search(LOG_PATH)