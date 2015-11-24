#!/usr/bin/python2.7

import sys
import os
import re
import subprocess
import difflib

ckeck_list = [
	# TEA a XTEA (tomCrypt is not included since it only implements XTEA)
	[
		r'test/build/test_tea_[gc]o[0-9sz]',
		r'botan/build/bot_tea',
		r'cryptopp/build/cryptopp_tea'
	],

	# TEA ECB
	[
		r'test/build/test_ecb_tea_[gc]o[0-9sz]',
		r'tomCrypt/build/tom_ecb_tea',
		r'botan/build/bot_ecb_tea'
	],

	# TEA CBC
	[
		r'test/build/test_cbc_tea_[gc]o[0-9sz]',
		r'tomCrypt/build/tom_cbc_tea',
		r'cryptopp/build/cryptopp_cbc_tea',
		r'botan/build/bot_cbc_tea'
	],

	# TEA OFB
	[
		r'test/build/test_ofb_tea_[gc]o[0-9sz]',
		r'tomCrypt/build/tom_ofb_tea',
		r'botan/build/bot_ofb_tea'
	],

	# TEA CFB
	[
		r'test/build/test_cfb_tea_[gc]o[0-9sz]',
		r'tomCrypt/build/tom_cfb_tea',
		r'botan/build/bot_cfb_tea'
	],

	# TEA CTR
	[
		r'test/build/test_ctr_tea_[gc]o[0-9sz]',
		r'tomCrypt/build/tom_ctr_tea'
	],

	# MD5
	[
		r'test/build/test_md5_[gc]o[0-9sz]',
		r'openssl/build/openssl_md5',
		r'cryptopp/build/cryptopp_md5',
		r'tomCrypt/build/tom_md5',
		r'go/build/go_md5',
		r'nettle/build/net_md5'
	],

	# MD5 HMAC
	[
		r'test/build/test_hmac_md5_[gc]o[0-9sz]',
		r'tomCrypt/build/tom_hmac_md5',
		r'botan/build/bot_hmac_md5',
		r'openssl/build/openssl_hmac_md5',
		r'nettle/build/net_hmac_md5',
		r'cryptopp/build/cryptopp_hmac_md5'
	],
	
	# RC4
	[
		r'test/build/test_rc4_[gc]o[0-9sz]',
		r'openssl/build/openssl_rc4',
		r'cryptopp/build/cryptopp_rc4',
		r'tomCrypt/build/tom_rc4',
		r'botan/build/bot_rc4',
		r'go/build/go_rc4'
	],

	# AES
	[
		r'test/build/test_aes_[gc]o[0-9sz]',
		r'openssl/build/openssl_aes',
		r'cryptopp/build/cryptopp_aes',
		r'tomCrypt/build/tom_aes',
		r'botan/build/bot_aes',
		r'nettle/build/net_aes'
	],

	# AES ECB
	[
		r'test/build/test_ecb_aes_[gc]o[0-9sz]',
		r'tomCrypt/build/tom_ecb_aes',
		r'botan/build/bot_ecb_aes'
	],

	# AES CBC
	[
		r'test/build/test_cbc_aes_[gc]o[0-9sz]',
		r'tomCrypt/build/tom_cbc_aes',
		r'botan/build/bot_cbc_aes',
		r'nettle/build/net_cbc_aes'
	],

	# AES OFB
	[
		r'test/build/test_ofb_aes_[gc]o[0-9sz]',
		r'tomCrypt/build/tom_ofb_aes',
		r'botan/build/bot_ofb_aes'
	],

	# AES CFB
	[
		r'test/build/test_cfb_aes_[gc]o[0-9sz]',
		r'tomCrypt/build/tom_cfb_aes',
		r'botan/build/bot_cfb_aes'
	],

	# AES CTR
	[
		r'test/build/test_ctr_aes_[gc]o[0-9sz]',
		r'tomCrypt/build/tom_ctr_aes'
	],

	# SHA1
	[
		r'test/build/test_sha1_[gc]o[0-9sz]',
		r'openssl/build/openssl_sha1',
		r'cryptopp/build/cryptopp_sha1',
		r'tomCrypt/build/tom_sha1',
		r'botan/build/bot_sha1',
		r'go/build/go_sha1'
	],

	# SHA1 HMAC
	[
		r'test/build/test_hmac_sha1_[gc]o[0-9sz]',
		r'tomCrypt/build/tom_hmac_sha1',
		r'botan/build/bot_hmac_sha1'
	],

	# SERPENT
	[
		r'test/build/test_serpent_[gc]o[0-9sz]',
		r'cryptopp/build/cryptopp_serpent',
		r'botan/build/bot_serpent'
	],
	
	# DES
	[
		r'test/build/test_des_[gc]o[0-9sz]',
		r'openssl/build/openssl_des',
		r'cryptopp/build/cryptopp_des',
		r'tomCrypt/build/tom_des',
		r'botan/build/bot_des'
	],
	
	# TWOFISH
	[
		r'test/build/test_twofish_[gc]o[0-9sz]',
		r'cryptopp/build/cryptopp_twofish',
		r'tomCrypt/build/tom_twofish',
		r'botan/build/bot_twofish'
	],

]

def search_files(regex, top):
	result = []
	sep = os.path.sep
	matcher = re.compile(regex)
	pieces = regex.split(sep)
	partial_matchers = map(re.compile, (sep.join(pieces[:i+1]) for i in range(len(pieces))))
	for root, dirs, files in os.walk(top,topdown=True):
		for i in reversed(range(len(dirs))):
			dirname = os.path.relpath(os.path.join(root,dirs[i]), top)
			dirlevel = dirname.count(sep)
			if not partial_matchers[dirlevel].match(dirname):
				del dirs[i]

		for file_name in files:
			file_name = os.path.relpath(os.path.join(root, file_name))
			if matcher.match(file_name) and os.access(file_name, os.X_OK):
				result.append(file_name)
	return result

def execute_file(file_name):
	process = subprocess.Popen(file_name, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	output = process.communicate()
	process.wait()

	if process.returncode != 0:
		sys.stdout.write('\x1b[31mFAIL\x1b[0m\x1b[0m (return code: " + str(process.returncode) + ")\n')
	return output[0]

if __name__=='__main__':
	current_dir = os.path.dirname(os.path.abspath(__file__))

	for entry in ckeck_list:
		file_list = []
		output_list = []

		for regex in entry:
			nb = len(file_list)
			file_list += search_files(regex, current_dir)
			if len(file_list) == nb:
				sys.stdout.write("\x1b[33mWARNING\x1b[0m no match for regex " + regex + "\n")

		for file_name in file_list:
			output_list += [execute_file(current_dir + os.path.sep + file_name)]

		for output, file_name in zip(output_list[1:], file_list[1:]):
			diff = difflib.unified_diff(output_list[0].split('\n'), output.split('\n'), fromfile=file_list[0], tofile=file_name)
			sys.stdout.writelines('\n'.join(diff))
		
