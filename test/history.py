#!/usr/bin/python2.7

import os.path
import hashlib

class history(object):

	def __init__(self, file_name):
		self.dic_actual = {}
		self.dic_futur = {}
		self.file_name = file_name

		if os.path.isfile(file_name):
			file = open(file_name, "r")
			lines = file.readlines()
			file.close()

			for line in lines:
				entries = line.split(":")
				if len(entries) != 2:
					print "ERROR: incomplete or corrupted file entry"
				else:
					self.dic_actual[entries[0]] = entries[1][:-1]
		return

	def hasFilesChanged(self, file_names):
		result = False;

		for file_name in file_names:
			hash_name = hashlib.md5(file_name).hexdigest()
			hash_file = hashlib.md5(open(file_name, "rb").read()).hexdigest()

			self.dic_futur[hash_name] = hash_file

			if hash_name in self.dic_actual:
				if self.dic_actual[hash_name] != hash_file:
					result = True
			else:
				result = True
		return result

	def hasStringChanged(self, key, value):
		hash_key = hashlib.md5(key).hexdigest()
		hash_value = hashlib.md5(value).hexdigest()

		self.dic_futur[hash_key] = hash_value

		if hash_key in self.dic_actual:
			if self.dic_actual[hash_key] == hash_value:
				return False
			else:
				return True
		else:
			return True

	def clear(self):
		self.dic_actual.clear()
		self.dic_futur.clear()
		return

	def save(self):
		file = open(self.file_name, "w")
		for hash_key in self.dic_actual:
			if hash_key in self.dic_futur:
				file.write(hash_key + ":" +  self.dic_futur.pop(hash_key) + "\n")
			else:
				file.write(hash_key + ":" +  self.dic_actual[hash_key] + "\n")
		for hash_key in self.dic_futur:
			file.write(hash_key + ":" +  self.dic_futur[hash_key] + "\n")
		file.close()
		return
