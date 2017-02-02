#!/usr/bin/python2.7

import sys
import os
import time
import subprocess
import re
import shutil
import xml.etree.ElementTree as ET

class recipe(object):

	def __init__(self, xml_recipe):
		self.name 		= xml_recipe.attrib.get("name")
		self.build 		= xml_recipe.find("build").attrib.get("cmd")
		self.trace 		= xml_recipe.find("trace").attrib.get("cmd")
		self.active 	= (xml_recipe.attrib.get("active") == "yes")

		self.trace_arg 	= []
		self.search_arg = []
		self.algo 		= {}
		self.comment 	= []
		self.log  		= None

		for xml_arg in xml_recipe.find("trace").findall("arg"):
			self.trace_arg.append(xml_arg.attrib.get("value"))
		for xml_arg in xml_recipe.find("search").findall("arg"):
			self.search_arg.append(xml_arg.attrib.get("value"))
		for xml_algo in xml_recipe.find("result").findall("algo"):
			self.algo[xml_algo.attrib.get("name")] = int(xml_algo.attrib.get("number"))
		for xml_comment in xml_recipe.find("result").findall("comment"):
			self.comment.append(xml_comment.attrib.get("value"))

	def __str__(self):
		string = ""
		for arg in self.search_arg:
			if arg == "exit":
				continue
			if arg.startswith("load trace "):
				arg = "load trace /home/plt/Documents/bacs/test/" + arg[11:]
			if arg.startswith("load code signature ../"):
				arg = "load code signature /home/plt/Documents/bacs/" + arg[23:]
			string += arg + "\n"
		return string

	def create_whl(self, whiteList_path):
		if self.trace != "":
			if not os.path.isdir(whiteList_path):
				os.makedirs(whiteList_path)

			f = open(whiteList_path + self.name + ".lst", "w")
			f.write(self.trace + "\n")
			f.close()
		else:
			sys.stdout.write("Creating whiteList for " + self.name + " ... \x1b[36mSKIP\x1b[0m\n")

	def build_prog(self, log_path):
		sys.stdout.write("Building " + self.name + " ... ")
		sys.stdout.flush()

		if self.build != "":
			if self.log == None:
				if not os.path.isdir(log_path):
					os.makedirs(log_path)
				self.log = open(log_path + self.name + ".log", "w")

			self.log.write("\n\n### BULID STDOUT & STDERR ###\n\n")
			self.log.flush()

			time_start = time.time()
			return_value = subprocess.call(self.build.split(' '), stdout = self.log, stderr = self.log)
			time_stop  = time.time()
			if return_value == 0:
				sys.stdout.write("\x1b[32mOK\x1b[0m - "+ str(round(time_stop - time_start, 2)) + " s\n")
			else:
				sys.stdout.write("\x1b[31mFAIL\x1b[0m\x1b[0m (return code: " + str(return_value) + ")\n")
		else:
			sys.stdout.write("\x1b[36mSKIP\x1b[0m\n")

	def trace_prog(self, log_path, pin_path, tool_path, trace_path, whiteList_path):
		sys.stdout.write("Tracing " + self.name + " ... ")
		sys.stdout.flush()
		if self.trace != "":
			if self.log == None:
				if not os.path.isdir(log_path):
					os.makedirs(log_path)
				self.log = open(log_path + self.name + ".log", "w")

			self.log.write("\n\n### TRACE STDOUT & STDERR ###\n\n")
			self.log.flush()

			folder = trace_path + "trace" + self.name
			if os.path.isdir(folder):
				shutil.rmtree(folder)

			time_start = time.time()
			cmd_l = [pin_path, "-t", tool_path, "-o", folder, "-w", whiteList_path + self.name + ".lst"]
			cmd_l.extend(self.trace_arg)
			cmd_l.extend(["--", self.trace])
			process = subprocess.Popen(cmd_l, stdout = self.log, stderr = subprocess.STDOUT)
			process.wait()
			time_stop = time.time()

			if process.returncode == 0:
				sys.stdout.write("\x1b[32mOK\x1b[0m - "+ str(round(time_stop - time_start, 2)) + " s\n")
			else:
				sys.stdout.write("\x1b[31mFAIL\x1b[0m\x1b[0m (return code: " + str(process.returncode) + ")\n")
		else:
			sys.stdout.write("\x1b[36mSKIP\x1b[0m\n")

	def search(self, log_path):
		sys.stdout.write("Searching " + self.name + " ... ")
		sys.stdout.flush()

		if self.log == None:
			if not os.path.isdir(log_path):
				os.makedirs(log_path)
			self.log = open(log_path + self.name + ".log", "w")

		self.log.write("\n\n### SEARCH STDOUT & STDERR ###\n\n")
		self.log.flush()

		cmd = ["./analysis"]
		cmd.extend(self.search_arg)

		time_start = time.time()
		process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		output_val = process.communicate()
		process.wait()
		time_stop = time.time()
			
		self.log.write(output_val[0])
		self.log.write(output_val[1])

		if process.returncode == 0:
			sys.stdout.write("\x1b[32mOK\x1b[0m - " + str(round(time_stop - time_start, 2)) + " s\n")

			regex = re.compile("[a-zA-Z0-9_]+ +\| [0-9]+ +\| [0-9]+\.[0-9]+ *")
			detected_signature = {}
			for i in regex.findall(output_val[0]):
				name = i[:i.find(' ')].strip()
				occu = int(i[35: 35 + i[35:].find(' ')])

				if name in detected_signature:
					detected_signature[name] = occu + detected_signature.get(name)
				else:
					detected_signature[name] = occu

			for i in self.algo:
				if i in detected_signature:
					nb_expected = self.algo.get(i)
					nb_detected = detected_signature.get(i)

					if nb_expected < nb_detected:
						print("\t" + i + " \x1b[33mEXTRA " + str(detected_signature.get(i)) + "/" + str(self.algo.get(i)) + "\x1b[0m")
					elif nb_expected > nb_detected:
						print("\t" + i + " \x1b[31mFAIL " + str(detected_signature.get(i)) + "/" + str(self.algo.get(i)) + "\x1b[0m")
					else:
						print("\t" + i + " \x1b[32mOK " + str(self.algo.get(i)) + "/" + str(self.algo.get(i)) + "\x1b[0m")
				elif self.algo.get(i) > 0:
					print("\t" + i + " \x1b[31mFAIL 0/" + str(self.algo.get(i)) + "\x1b[0m")

			for i in detected_signature:
				if i not in self.algo and detected_signature.get(i) > 0:
					print("\t" + i + " \x1b[33mEXTRA " + str(detected_signature.get(i)) + "/0\x1b[0m")

			for c in self.comment:
				print("\t\x1b[1mCOMMENT:\x1b[0m " + c)

		else:
			sys.stdout.write("\x1b[31mFAIL\x1b[0m\x1b[0m (return code: " + str(process.returncode) + ")\n")
			print(output_val[1])

	def __del__(self):
		if self.log != None:
			self.log.close()
			self.log = None
