WAF = backend/tools/waf

all:
	$(WAF) configure all $(PARAMS)

strat:
	$(WAF) configure strat $(PARAMS)

strat_ma:
	$(WAF) configure strat_ma $(PARAMS)

easy_strat:
	$(WAF) configure easy_strat $(PARAMS)

simdata:
	$(WAF) configure simdata $(PARAMS)

simorder:
	$(WAF) configure simorder $(PARAMS)

pricer:
	$(WAF) configure pricer $(PARAMS)

proxy:
	$(WAF) configure proxy $(PARAMS)

mid_data:
	$(WAF) configure mid_data $(PARAMS)

ctpdata:
	$(WAF) configure ctpdata $(PARAMS)

ctporder:
	$(WAF) configure ctporder $(PARAMS)

getins:
	$(WAF) configure getins $(PARAMS)

arbmaker:
	$(WAF) configure arbmaker $(PARAMS)

simplemaker:
	$(WAF) configure simplemaker $(PARAMS)

simplearb:
	$(WAF) configure simplearb $(PARAMS)

demostrat:
	$(WAF) configure demostrat $(PARAMS)

backtest:
	$(WAF) configure backtest $(PARAMS)

order_matcher:
	$(WAF) configure order_matcher $(PARAMS)

teststrat:
	$(WAF) configure teststrat $(PARAMS)

clean:
	rm -rf build
