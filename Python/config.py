config = ".config"

def create(t):
	cf = open(config,"w+")

	cf.write("Time\t{}\n".format(t))

	cf.close()

	return

def edit(condition, value):
	cf = open(config,"r")

	lines = cf.readlines()

	cf.close()

	found = 0
	i = 0

	for line in lines:
		l = line.split()
		if(l[0] == condition):
			line = "{}\t{}\n".format(condition,value)
			found = 1
			lines[i] = line
			break
		i += 1

	cf = open(config,"w")
	cf.writelines(lines)
	cf.close()

	if(not found):
		raise ValueError

	return

def add(condition, value):
	cf = open(config,"a")

	cf.write("{}\t{}\n".format(condition,value))

	return

def remove(condition):
	cf = open(config,"r")

	lines = cf.readlines()

	cf.close()

	found = 0
	i = 0

	for line in lines:
		l = line.split()
		if(l[0] == condition):
			found = 1
			del(lines[i])
			break
		i += 1

	cf = open(config,"w")
	cf.writelines(lines)
	cf.close()

	if(not found):
		raise ValueError

	return

def read(condition):
	cf = open(config,"r")

	for line in cf:
		l = line.split()
		if(l[0] == condition):
			cf.close()
			return l[1]

	cf.close()

	raise ValueError

	return
