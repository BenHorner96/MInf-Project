import threading
import sys
import time
import subprocess
cacheLoc = "/home/ben/Documents/Project/camera"
interval = int(sys.argv[1])
nameTime = False # bool(sys.argv[2])

def capture():
	
	cameraCMD = ["./cap"]
	t = time.gmtime()
	if(nameTime):
		cameraCMD.append("{}-{:02d}-{:02d}_{:02d}:{:02d}:{:02d}".format(t[0],t[1],t[2],t[3],t[4],t[5]))
	else:
		cameraCMD.append("frame")
	subprocess.Popen(cameraCMD,cwd=cacheLoc)

while True:
	t = time.clock()
	ct = threading.Thread(target=capture)
	ct.start()

	ct.join()

	time.sleep(interval - time.clock() + t)
	
