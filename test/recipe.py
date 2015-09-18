#!/usr/bin/python2.7

import sys
import os
import time
import subprocess
import re
import shutil

class recipe(object):

	def __init__(self, name, build, trace, trace_arg, arg, algo):
		self.name 		= name
		self.build 		= build
		self.trace 		= trace
		self.trace_arg 	= trace_arg
		self.arg 		= arg
		self.algo 		= algo
		self.log  		= None

	def __str__(self):
		string = self.name + "\n\t-BUILD: " + self.build + "\n\t-CMD: " + self.trace + " OPT ARG:"
		for i in self.trace_arg:
			string += " " + i
		string += "\n\t-ARG:\n"
		for i in self.arg:
			string = string + "\t\t-" + i + "\n"
		string += "\n\t-PRIMITIVE(S):\n"
		for i in self.algo:
			string = string + "\t\t-" + i + ": " + str(self.algo.get(i)) + "\n"
		return string

	def build_prog(self, log_path):
		sys.stdout.write("Building " + self.name + " ... ")
		sys.stdout.flush()

		if self.build != "":
			if self.log == None:
				self.log = open(log_path + self.name + ".log", "w")

			self.log.write("\n\n### BULID STDOUT & STDERR ###\n\n")
			self.log.flush()

			time_start = time.time()
			return_value = subprocess.call(self.build.split(' '), stdout = self.log, stderr = self.log)
			time_stop  = time.time()
			if return_value == 0:
				sys.stdout.write("\x1b[32mOK\x1b[0m - "+ str(time_stop - time_start) + "s\n")
			else:
				sys.stdout.write("\x1b[31mFAIL\x1b[0m\x1b[0m (return code: " + str(return_value) + ")\n")
		else:
			sys.stdout.write("no rule\n")

	def trace_prog(self, log_path, pin_path, tool_path, trace_path, whiteList_path):
		sys.stdout.write("Tracing " + self.name + " ... ")
		sys.stdout.flush()
		if self.trace != "":
			if self.log == None:
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
			process = subprocess.Popen(cmd_l, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
			process.wait()
			time_stop = time.time()

			output_val = process.communicate()
			self.log.write(output_val[0])
			self.log.write(output_val[1])

			if process.returncode == 0:
				sys.stdout.write("\x1b[32mOK\x1b[0m - "+ str(time_stop - time_start) + "s\n")
			else:
				sys.stdout.write("\x1b[31mFAIL\x1b[0m\x1b[0m (return code: " + str(process.returncode) + ")\n")
				print(output_val[1])
		else:
			sys.stdout.write("\x1b[36mSKIP\x1b[0m\n")

	def search(self, log_path):
		sys.stdout.write("Searching " + self.name + " ... ")
		sys.stdout.flush()

		signature_file = None
		trace_dir = None

		for a in self.arg:
			if a.startswith("load code signature"):
				signature_file = a[20:]
			elif a.startswith("load trace"):
				trace_dir = a[11:]

		if self.log == None:
			self.log = open(log_path + self.name + ".log", "w")

		self.log.write("\n\n### SEARCH STDOUT & STDERR ###\n\n")
		self.log.flush()

		cmd = ["./signature"]
		cmd.extend(self.arg)

		time_start = time.time()
		process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		output_val = process.communicate()
		process.wait()
		time_stop = time.time()
			
		self.log.write(output_val[0])
		self.log.write(output_val[1])

		if process.returncode == 0:
			sys.stdout.write("\x1b[32mOK\x1b[0m - " + str(time_stop - time_start) + "s\n")

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
				else:
					print("\t" + i + " \x1b[31mFAIL 0/" + str(self.algo.get(i)) + "\x1b[0m")

			for i in detected_signature:
				if i not in self.algo and detected_signature.get(i) > 0:
					print("\t" + i + " \x1b[33mEXTRA " + str(detected_signature.get(i)) + "/0\x1b[0m")

		else:
			sys.stdout.write("\x1b[31mFAIL\x1b[0m\x1b[0m (return code: " + str(process.returncode) + ")\n")
			print(output_val[1])

	def __del__(self):
		if self.log != None:
			self.log.close()
			self.log = None
