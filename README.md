mimir
=====

The cache profiling framework from my masters thesis

    M       M  I  M       M  I M       M  I  RRRRRR
    MM     MM  I  MM     MM  I MM     MM  I  R     R     
    M M   M M  I  M M   M M  I M M   M M  I  RRRRRR
    M  M M  M  I  M  M M  M  I M  M M  M  I  R    R 
    M   M   M  I  M   M   M  I M   M   M  I  R     R
    

Paper
======
Available at www.mimircache.com


pymimir
======

This python server supports the set/get commands from the memcached protocol.
Several replacement policies are available and MIMIR profiles the running
replacement policy.

Note that this server is much slower than real memcached and I will not
guarantee that this works flawlessly in a production system.
It supports the stats hrc command so the memcache-monitor can connect to pymimir.


memcached-1.4.15 with statistics
======
This server is just regular memcached-1.4.15 with additional statistics collection
to predict what would happen if the server used less/more memory.
It supports the stats cdf command


memcache-monitor
======
Web interface to collect HRCs via the stats hrc command.
