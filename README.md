# High Frequency Trading Solution #

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

### Author:

* XinYu Huang

any questions or suggestions are welcome, please contract me with:huangxy17@fudan.edu.cn, i will list your name here to thanks for your contribution.

### Thanks list :
* ShengRui zhao: most active products getting tools on futures market, fill the detail config for all contracts.
* Jian Sun: professor of school of Economics, Fudan university. Fund-supporter and strategy idea consultant.
* Wei Sun: systems consultant.
