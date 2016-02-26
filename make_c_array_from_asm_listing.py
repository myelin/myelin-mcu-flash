import sys, re

# turn a gcc assembly listing into an array of opcodes that can be inlined into an OpenOCD flash driver

function_name, listing_file, include_file = sys.argv[1:]

def split_bytes(opcodes):
	codes = []
	for i in range(0, len(opcodes), 2):
		codes.append(int(opcodes[i:i+2], 16))
	return codes

def format_bytes(bytes):
	codes = []
	for byte in bytes:
		codes.append("0x%02X," % byte)
	return " ".join(codes)

include_f = open(include_file, 'w')
print>>include_f, "static const uint8_t %s[] = {" % function_name
in_alg = 0
code_size = 0
for line in open(listing_file):
	line = line.rstrip()

	if not in_alg:
		if line.find(".cfi_startproc") != -1:
			in_alg = 1
		continue

	if line.find(".cfi_endproc") != -1: break

	# embedded C code
	m = re.search(r"\*\*\*\* ?(.*?)$", line)
	if m:
		code = m.group(1).rstrip().replace("\t", "    ")
		if code:
			print>>include_f, "// %s" % code
		else:
			print>>include_f
		continue

	# opcodes and data
	#'70 0008 394B     \t\tldr\tr3, .L7\t@ D.4217,'
	m = re.search(r"\s*\d+ ([0-9a-f]{4}) ([0-9A-F]{4,8})\s+(.*?)$", line)
	if m:
		loc, opcodes, code = m.groups()
		bytes = split_bytes(opcodes)
		code_size += len(bytes)
		print>>include_f, "    %s // %04x: %s" % (format_bytes(bytes), int(loc, 16), code)
		continue

	# labels
	m = re.search(r"\s*\d+\s+\.(.*?):$", line)
	if m:
		print>>include_f, "              // .%s:" % m.group(1)
		continue

	# .loc directives
	if re.search(r"\s*\d+\s+(\.code|\.align|\@|\.loc)", line): continue

	print>>sys.stderr, "Can't parse:",`line`

print>>include_f, "// total %d (0x%x) bytes" % (code_size, code_size)
print>>include_f, "};"
print>>include_f
print>>include_f, "#define %s_code ((uint8_t *)%s)" % (function_name, function_name)
print>>include_f, "#define %s_size (sizeof(%s))" % (function_name, function_name)

print "listing processed; found %d (%x) bytes of code" % (code_size, code_size)