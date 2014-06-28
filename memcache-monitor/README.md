==== Setup ====

	$ virtualenv2 --no-site-packages env
	$ . env/bin/activate
	$ pip2.7 install -r requirements.txt


	#... Start the memcached server before running the next command!

	$ python2 collect_graphs.py -d

