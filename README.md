# README #

### What is this repository for? ###

* automatic algorithm trading system

### How do I get set up? ###

* System requirement:
  based on centos 6.5 or 7 (other system have not tested)
* IDE recommendation:
  I developed it by vim (using some cool plugin), i also set the neccessary config file for QT, so this can be opened on QT directly.
* Complie Tools
  based on g++, using the complie tools waf, the binary file is included in the repository, path hft/backend/bin/waf, for waf, you can google it
* pre-installed software:
  this project used zeromq to do ipc, version 4.1.2
* complie command:
  in the path of yourhomepath/hft, run "make", you will get binary file in build/bin
* How to run it
  just run it, as a start, you can run ctpdata, if network is good, and time is in the trading session(9:00-11:30 13:30-3:00), you can see marketdata come out in your screen.
* Deployment instructions

### Contribution guidelines ###

* Writing tests
* Code review
* Other guidelines

### Who do I talk to? ###

* Repo owner or admin
* Other community or team contact

Any questions and suggestions are welcome, contact me: huangxy17@fudan.edu.cn
