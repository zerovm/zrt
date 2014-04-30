#!/usr/bin/env python

import os, glob, sys

def strip_zeros(s):
	while len(s) > 0 and s[0] == '0':
		s = s[1:]
	return s

def load_symbols(prog):
	f = os.popen('nm ' + prog)
	sym = {}

	for line in f.readlines():
		s = line.split()
		if len(s) > 2 :
			val = int('0x'+s[0], 16)
			name = s[2]
			sym[val] = name
	f.close()

	return sym

def parse_file(finname, foutname):
	fr = file(finname, 'r')
	fw = file(foutname, 'w')

	s = fr.readline()
	while s != '':
		words = s.rstrip().split(' ')
		for i in range(len(words)):
			if words[i].startswith('0x'):
				try:
					words[i] = sym_hash[int(words[i], 16)]
				except KeyError:
					words[i] = 'unknown'

		fw.write(' '.join(words) + '\n')

		s = fr.readline()

	fr.close()
	fw.close()

def parse_file_hierarchy(finname, foutname):
	level = 0
	prevname = ""
	count=0

	fr = file(finname, 'r')
	fw = file(foutname, 'w')

	s = fr.readline()
	while s != '':
		words = s.rstrip().split(' ')

		for i in range(len(words)):
			if words[i].startswith('0x'):
				try:
					name = sym_hash[int(words[i], 16)]
				except KeyError:
					name = 'unknown'

		if words[0]=="enter":
			# Check if new name compared to previous input
			if prevname==name:
				# was the counter counting?
				if count==0:
					# print function name
					fw.write('\n    '+level*'|   '+name,)
				count=count+1
			else:
				# New name received. Was the counter counting?
				if count>0:
					fw.write("(total: %d times)" % (count+1),)
					# re-initialize counter
					count=0
				fw.write('\n    '+level*'|   '+name,)
			level=level+1
			prevname = name
		else:
			level=level-1

		s = fr.readline()

	fr.close()
	fw.close()


sym_hash = load_symbols(sys.argv[1])
finname = sys.argv[2]
assert finname.endswith('.unparsed')
foutname = finname.replace('.unparsed', '')
foutname2 = finname.replace('.unparsed', '.hierarchy')
parse_file(finname, foutname)
parse_file_hierarchy(finname, foutname2)
os.unlink(finname)
