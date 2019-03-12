import time

class MarketSnapshot:
  def __init__(self, time_check = False, depth = 5):
    self.ticker = ""
    self.bids = [-1.0] * depth
    self.asks = [-1.0] * depth
    self.bid_sizes = [-1.0] * depth
    self.ask_sizes = [-1.0] * depth
    self.last_trade = -1.0
    self.last_trade_size = -1
    self.volume = -1
    self.turnover = -1.0
    self.open_interest = -1.0
    self.time = -1
    self.is_initialized = False
    self.depth = depth
    self.time_str = ''
    self.time_check = time_check

  def construct(self, string):
    space_count = 0 
    #dash_count = 0 
    content = string.split(' ')
    for i in range(len(content)):
      if content[i] == '': 
        space_count += 1
      #if dash_count == '|':
        #dash_count += 1
    for i in range(space_count):
      content.remove('')
    #for i in range(dash_count):
      #content.remove('|')
    if len(content) != 46:
      print "wrong data for line"
      print string
      return False
    time_sec = int(content[0])
    time_usec = float('0.'+content[1])
    self.time = time_sec+time_usec
    topic = content[2]
    self.ticker = content[3]
    for i in range(5):
      self.bids[i] = float(content[5+7*i])
      self.asks[i] = float(content[6+7*i])
      self.bid_sizes[i] = float(content[8+7*i])
      self.ask_sizes[i] = float(content[10+7*i])
    self.last_trade = float(content[40])
    self.last_trade_size = float(content[41])
    self.volume = float(content[42])
    self.turnover = float(content[44])
    self.open_interest = float(content[45])
    return self.Check()

  def s_construct(self, string):
    content = string.split(' ')
    if len(content) != 8:
      return False
    ticker, time, bids, asks, bsize, asize, last_trade, volume = content
    '''
    if ticker == 'ticker':
      return
    if float(bids) <= 0.1:
      return
    '''
    time = int(time)
    bids = float(bids)
    asks = float(asks)
    bsize = float(bsize)
    asize = float(asize)
    last_trade = float(last_trade)
    volume = float(volume)

    self.time = time
    self.ticker = ticker
    self.bids[0] = bids
    self.asks[0] = asks
    self.bid_sizes[0] = bsize
    self.ask_sizes[0] = asize
    self.last_trade = last_trade
    self.volume = volume
    return self.Check()

  def sql_construct(self, string):
    temp = string.split(' ')
    temp = filter(None, temp)
    if len(temp) != 6:
      return False
    ticker, bids, asks, volume, time1, time2 = temp
    if ticker == 'CODE':
      return False
    bids = float(bids)
    asks = float(asks)
    volume = float(volume)

    self.time_str = time1+"@"+time2
    self.ticker = ticker
    self.bids[0] = bids
    self.asks[0] = asks
    self.bid_sizes[0] = 5
    self.ask_sizes[0] = 5
    self.volume = volume
    self.last_trade = (self.bids[0]+self.asks[0])/2
    return self.Check()

  def Check(self):
    if self.bid_sizes[0] < 0.1 or self.ask_sizes[0] < 0.1 or self.last_trade < 0.1 or self.bids[0] < 0.1 or self.asks[0] < 0.1:
      return False
    if self.ticker == 'ticker':
      return False
    if self.ticker == 'CODE':
      return False
    if self.time_check == True:
      return self.CheckTime()
    return True

  def CheckTime(self):
    valid_time = [(9*3600, 10*3600+15*60), (10.5*3600, 11.5*3600), (13.5*3600, 15*3600)]
    time_sec = -1
    if self.time == -1:
      time_s = self.time_str.split('@')[-1]
      hour = int(time_s[0:2])
      mit = int(time_s[3:5])
      sec = int(time_s[6:8])
      time_sec = hour*3600+mit*60+sec
      #print time_s + '->' + str(time_sec)
    else:
      time_sec = (self.time+8*3600)%(24*2600)
    for pair in valid_time:
      low = pair[0]
      high = pair[1]
      if time_sec > high or time_sec < low:
        continue
      return True

  def Show(self):
    split_char = ' '
    show_content = ""
    if self.time == -1:
      show_content += self.time_str.replace(' ', '*')
    else:
      show_content += repr(self.time).replace('.', ' ')
    show_content += split_char
    show_content += "SNAPSHOT"
    show_content += split_char
    show_content += self.ticker
    show_content += split_char
    show_content += '|'
    show_content += split_char
    for i in range(self.depth):
      show_content += str(self.bids[i]) + split_char + str(self.asks[i]) + '|   ' + str(self.bid_sizes[i]) + ' x   ' + str(self.ask_sizes[i]) + ' |'
      show_content += split_char
    show_content += str(self.last_trade)
    show_content += split_char
    show_content += str(self.last_trade_size)
    show_content += split_char
    show_content += str(self.volume)
    show_content += split_char
    show_content += 'M'
    show_content += split_char
    show_content += str(self.turnover)
    show_content += split_char
    show_content += str(self.open_interest)
    show_content += split_char
    print(show_content)

'''
def main():
  shot = MarketSnapshot()
  shot.ticker = "HAHA"
  #shot.Show()
  s = '1501542787 621478 SNAPSHOT OKC/BTCCNY/ | 19309.01 19391.17 | 27861 x 1612 | 19301 19397.58 | 100 x 285 | 19300.1 19398 | 500 x 1283 | 19300.04 19399 | 114 x 800 | 19300 19400 | 1956 x 6180 | 19309.01 0 8445877 M 0 0'
  shot.construct(s)
  shot.Show()

main()
'''
