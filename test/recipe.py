#!/usr/bin/python2.7

import sys
import time
import subprocess
import re

class recipe(object):

	def __init__(self, name, build, cmd, arg, result):
		self.name 	= name
		self.build 	= build
		self.cmd 	= cmd
		self.arg 	= arg
		self.result = result
		self.log  	= None

	def __str__(self):
		return self.name + "\n\t-BUILD: " + self.build + "\n\t-CMD: " + self.cmd + "\n\t-ARG: " + self.arg + "\n\t-RESULT: " + self.result

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

	def trace_prog(self, hist, log_path, pin_path, tool_path, white_list_path, trace_path):
		sys.stdout.write("Tracing " + self.name + " ... ")
		sys.stdout.flush()

		if self.cmd != "":
			if hist.hasFilesChanged([pin_path, tool_path, white_list_path, self.cmd]):
				if self.log == None:
					self.log = open(log_path + self.name + ".log", "w")

				self.log.write("\n\n### TRACE STDOUT & STDERR ###\n\n")
				self.log.flush()

				time_start = time.time()
				process = subprocess.Popen([pin_path, "-t", tool_path, "-o", trace_path + "trace" + self.name, "-w", white_list_path, "--", self.cmd], stdout = subprocess.PIPE, stderr = subprocess.PIPE)
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

				regex = re.compile("ERROR: [a-zA-Z0-9 _,():]*")
				for j in regex.findall(output_val[0]):
					print j.replace("ERROR", "\x1b[35mERROR\x1b[0m")
			else:
				sys.stdout.write("\x1b[36mPASS\x1b[0m\n")
		else:
			sys.stdout.write("no rule\n")

	def search(self, hist, log_path):
		return

	def __del__(self):
		if self.log != None:
			self.log.close()
			self.log = None


class ioRecipe(recipe):

	def search(self, hist, log_path):
		sys.stdout.write("Searching " + self.name + " ... ")
		sys.stdout.flush()

		ioChecker_file = self.arg[self.arg.find("load ioChecker") + 15:]
		ioChecker_file = ioChecker_file[:ioChecker_file.find(",")]
		trace_dir = self.arg[self.arg.find("load trace") + 11:]
		trace_dir = trace_dir[:trace_dir.find(",")]

		condition1 = hist.hasFilesChanged(["./analysis", ioChecker_file, trace_dir + "/ins.bin", trace_dir + "/op.bin", trace_dir + "/data.bin"])
		condition2 = hist.hasStringChanged("analysis" + self.name, self.arg)

		if condition1 or condition2:
			if self.log == None:
				self.log = open(log_path + self.name + ".log", "w")

			self.log.write("\n\n### SEARCH STDOUT & STDERR ###\n\n")
			self.log.flush()

			cmd = ["./analysis"]
			cmd.extend(self.arg.split(','))

			time_start = time.time()
			process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
			output_val = process.communicate()
			process.wait()
			time_stop = time.time()
			
			self.log.write(output_val[0])
			self.log.write(output_val[1])

			if process.returncode == 0:
				sys.stdout.write("\x1b[32mOK\x1b[0m - " + str(time_stop - time_start) + "s\n")

				crypto_list = self.result.split(',')

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


class sigRecipe(recipe):

	def __init__(self, name, build, cmd, arg, result):
		super(sigRecipe, self).__init__(name, build, cmd, arg, result)
		self.signature = {}

		signature_list = result.split(',')
		for i in signature_list:
			self.signature[i[i.find(':') + 1:].strip()] = int(i[:i.find(':')])

	def __str__(self):
		string = self.name + "\n\t-BUILD: " + self.build + "\n\t-CMD: " + self.cmd + "\n\t-ARG: " + self.arg + "\n\t-SIGNATURE(S):\n"
		for i in self.signature:
			string = string + "\t\t-" + i + ": " + str(self.signature.get(i)) + "\n"
		return string


	def search(self, hist, log_path):
		sys.stdout.write("Searching " + self.name + " ... ")
		sys.stdout.flush()

		codeSignature_file = self.arg[self.arg.find("load code signature") + 20:]
		codeSignature_file = codeSignature_file[:codeSignature_file.find(",")]
		trace_dir = self.arg[self.arg.find("load trace") + 11:]
		trace_dir = trace_dir[:trace_dir.find(",")]

		condition1 = hist.hasFilesChanged(["./signature", codeSignature_file, trace_dir + "/ins.bin", trace_dir + "/op.bin", trace_dir + "/data.bin"])
		condition2 = hist.hasStringChanged("signature" + self.name, self.arg)

		if condition1 or condition2:
			if self.log == None:
				self.log = open(log_path + self.name + ".log", "w")

			self.log.write("\n\n### SEARCH STDOUT & STDERR ###\n\n")
			self.log.flush()

			cmd = ["./signature"]
			cmd.extend(self.arg.split(','))

			time_start = time.time()
			process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
			output_val = process.communicate()
			process.wait()
			time_stop = time.time()
			
			self.log.write(output_val[0])
			self.log.write(output_val[1])

			if process.returncode == 0:
				sys.stdout.write("\x1b[32mOK\x1b[0m - " + str(time_stop - time_start) + "s\n")

				regex = re.compile("Found [0-9]+ subgraph\(s\) instance for signature: [a-zA-Z0-9_]+")
				detected_signature = {}
				for i in regex.findall(output_val[0]):
					name = i[i.find(':') + 2:].strip()
					occu = int(i[6: 6 + i[6:].find(' ')])

					if name in detected_signature:
						detected_signature[name] = occu + detected_signature.get(name)
					else:
						detected_signature[name] = occu

				for i in self.signature:
					if i in detected_signature:
						nb_expected = self.signature.get(i)
						nb_detected = detected_signature.get(i)

						if nb_expected < nb_detected:
							print("\t" + i + " \x1b[33mEXTRA " + str(detected_signature.get(i)) + "/" + str(self.signature.get(i)) + "\x1b[0m")
						elif nb_expected > nb_detected:
							print("\t" + i + " \x1b[31mFAIL " + str(detected_signature.get(i)) + "/" + str(self.signature.get(i)) + "\x1b[0m")
						else:
							print("\t" + i + " \x1b[32mOK " + str(self.signature.get(i)) + "/" + str(self.signature.get(i)) + "\x1b[0m")
					else:
						print("\t" + i + " \x1b[31mFAIL 0/" + str(self.signature.get(i)) + "\x1b[0m")

				for i in detected_signature:
					if i not in self.signature:
						print("\t" + i + " \x1b[33mEXTRA " + str(detected_signature.get(i)) + "/0\x1b[0m")

			else:
				sys.stdout.write("\x1b[31mFAIL\x1b[0m\x1b[0m (return code: " + str(process.returncode) + ")\n")
				print(output_val[1])
		else:
			sys.stdout.write("\x1b[36mPASS\x1b[0m\n")

