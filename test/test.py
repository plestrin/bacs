#!/usr/bin/python2.7

import sys
import subprocess
import xml.etree.ElementTree as ET
from recipe import recipe

# How to improve this script:
# 	- print a report in HTML
#	- archive previous reports

PIN_PATH 				= "/home/pierre/Documents/tool/pin-3.0-76991-gcc-linux/pin"
TOOL_PATH 				= "/home/pierre/Documents/bacs/lightTracer_pin/obj-ia32/lightTracer.so"
TOOL_SRC_PATH 			= "/home/pierre/Documents/bacs/lightTracer_pin/"
WHITELIST_PATH 			= "/home/pierre/Documents/bacs/test/whiteList/"
MAKEFILE_ANAL_PATH 		= "/home/pierre/Documents/bacs/traceAnalysis/Makefile"
MAKEFILE_SIG_PATH 		= "/home/pierre/Documents/bacs/staticSignature/Makefile"
TRACE_PATH				= "/home/pierre/Documents/bacs/test/"
LOG_PATH 				= "/home/pierre/Documents/bacs/test/log/"

if len(sys.argv) < 3:
	print("ERROR: incorrect number of argument")
	print("- 1 arg: recipe file name")
	print("- 2 arg: action type: PRINT or BUILD or TRACE or SEARCH or ALL")
	print("- 3 arg: name of specific test case [OPT]")
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
		name 		= None
		kind 		= None
		build 		= None
		trace 		= None
		trace_arg 	= []
		search_arg 	= []
		algo 		= {}

		name 		= xml_recipe.attrib.get("name")
		build 		= xml_recipe.find("build").attrib.get("cmd")
		trace 		= xml_recipe.find("trace").attrib.get("cmd")

		for xml_arg in xml_recipe.find("trace").findall("arg"):
			trace_arg.append(xml_arg.attrib.get("value"))
		for xml_arg in xml_recipe.find("search").findall("arg"):
			search_arg.append(xml_arg.attrib.get("value"))
		for xml_algo in xml_recipe.find("result").findall("algo"):
			algo[xml_algo.attrib.get("name")] = int(xml_algo.attrib.get("number"))

		if len(sys.argv) == 4:
			if name == sys.argv[3]:
				recipes.append(recipe(name, build, trace, trace_arg, search_arg, algo))
		else:
			if xml_recipe.attrib.get("active") == "yes":
				recipes.append(recipe(name, build, trace, trace_arg, search_arg, algo))
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

# COMPILE TOOL step
if action == "TRACE" or action == "ALL":
	sys.stdout.write("Building Trace program: ... ")
	sys.stdout.flush()
	return_value = subprocess.call(["make", "-C", TOOL_SRC_PATH])
	if return_value != 0:
		print("ERROR: unable to build Trace program")
		exit()

# TRACE step
if action == "TRACE" or action == "ALL":
	for r in recipes:
		r.trace_prog(LOG_PATH, PIN_PATH, TOOL_PATH, TRACE_PATH, WHITELIST_PATH)

# COMPILE SEARCH step
if action == "SEARCH" or action == "ALL":
	sys.stdout.write("Building Signature program: ... ")
	sys.stdout.flush()
	return_value = subprocess.call(["make"])
	if return_value != 0:
		print("ERROR: unable to build Signature program")
		exit()

# SEARCH step
if action == "SEARCH" or action == "ALL":
	for r in recipes:
		r.search(LOG_PATH)