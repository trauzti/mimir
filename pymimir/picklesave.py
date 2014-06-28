import pickle
import urllib2

filename = "data.pickle"

text = None
try:
    file_read = open(filename, 'rb')
    text = pickle.load(file_read)
    file_read.close()
    print "Loading from pickled file succeeded :)"
except:
    print "Loading from pickled file failed :("
    w = urllib2.urlopen('http://www.mbl.is/')
    text = w.read()
    file_write = open(filename, 'wb')
    pickle.dump(text, file_write, -1) # -1 is the highest protocol number
    file_write.close()

