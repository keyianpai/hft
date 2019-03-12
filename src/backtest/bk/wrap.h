#include <string.h>
#include <stdio.h>
#include <zmq.hpp>
#include <libconfig.h++>
#include <order.h>
#include <define.h>
#include <recver.h>
#include <sender.h>
#include <market_snapshot.h>
#include <common_tools.h>
#include <market_snapshot.h>
#include <common_tools.h>
#include <base_strategy.h>
#include <timecontroller.h>
#include <tr1/unordered_map>
#include <vector>

#include <boost/python.hpp>
#include "./strategy.h"

#include <libconfig.h++>
using namespace boost::python;

BaseStrategy * newS(std::string) {
  libconfig::Config cfg;
  std::string config_path = "/root/hft/config/backtest/backtest.config";
  cfg.readFile(config_path.c_str());
  const libconfig::Setting &sleep_time = cfg.lookup("time_controller")["sleep_time"];
  const libconfig::Setting &close_time = cfg.lookup("time_controller")["close_time"];
  std::vector<std::string> sleep_time_v;
  std::vector<std::string> close_time_v;
  for (int i = 0; i < sleep_time.getLength(); i++) {
    sleep_time_v.push_back(sleep_time[i]);
  }
  for (int i = 0; i < close_time.getLength(); i++) {
    close_time_v.push_back(close_time[i]);
  }
  std::tr1::unordered_map<std::string, std::vector<BaseStrategy*> > ticker_strat_map;
  TimeController tc(sleep_time_v, close_time_v, "data");
  std::vector<BaseStrategy*> sv;

  const libconfig::Setting & strategies = cfg.lookup("strategy");
  for (int i = 0; i < strategies.getLength(); i++) {
    const libconfig::Setting& s = strategies[i];
    sv.push_back(new Strategy(s, tc, &ticker_strat_map, "test"));
  }
  return sv.back();
}

BaseStrategy* factoryS() {
  return newS("asd");
}

struct StrategyWrap : BaseStrategy, boost::python::wrapper<BaseStrategy> {
  StrategyWrap(): BaseStrategy() {}
  void Start() { this->get_override("Start")(); }
  void Stop() { this->get_override("Stop")(); }
  void UpdateData(MarketSnapshot shot) { this->get_override("UpdateData")(); }
  void UpdateExchangeInfo(ExchangeInfo info) { this->get_override("UpdateExchangeInfo")(); }
  void RequestQryPos() { this->get_override("RequestQryPos")(); }
  void Print() { this->get_override("Print")(); }
  void InitTicker() { this->get_override("InitTicker")(); }
  void InitTimer() { this->get_override("InitTimer")(); }
  void SendPlainText(std::string s) { this->get_override("SendPlainText")(); }
  void Clear() { this->get_override("Clear")(); }
  void debug() { this->get_override("debug")(); }
  void UpdateAvgCost(std::string contract, double trade_price, int size) {this->get_override("UpdateAvgCost")();}
  std::string GenOrderRef() {this->get_override("GenOrderRef");}
  Order* NewOrder(std::string contract, OrderSide::Enum side, int size, bool control_price, bool sleep_order, std::string tbd = "null") {this->get_override("NewOrder");}
  void ModOrder(Order* o, bool sleep=false);
  void CancelAll(std::string contract) {this->get_override("CancelAll");}
  void CancelAll() {this->get_override("CancelAll");}
  void CancelOrder(Order* o) {this->get_override("CancelOrder");}
  void DelOrder(std::string ref) {this->get_override("DelOrder");}
  void DelSleepOrder(std::string ref) {this->get_override("DelSleepOrder");}
  void ClearAll() {this->get_override("ClearAll");}
  void Wakeup() {this->get_override("Wakeup");}
  void Wakeup(Order* o) {this->get_override("Wakeup");}
  void CheckStatus(MarketSnapshot shot) {this->get_override("CheckStatus");}
  void Run() {this->get_override("Run");}
  void Resume() {this->get_override("Resume");}
  void Pause() {this->get_override("Pause");}
  void Train() {this->get_override("Train");}
  void Flatting() {this->get_override("Flatting");}
  void ForceFlat() {this->get_override("ForceFlat");}
  bool Ready() {this->get_override("Ready");}

  void UpdatePos(Order* o, ExchangeInfo info) {this->get_override("UpdatePos");}
  void ModerateOrders(std::string contract) {this->get_override("ModerateOrders");}
  bool PriceChange(double current_price, double reasonable_price, OrderSide::Enum side, double edurance) {this->get_override("PriceChange");}
  void DoOperationAfterUpdatePos(Order* o, ExchangeInfo info) {this->get_override("DoOperationAfterUpdatePos");}
  void DoOperationAfterUpdateData(MarketSnapshot shot) {this->get_override("DoOperationAfterUpdateData");}
  void DoOperationAfterFilled(Order* o, ExchangeInfo info) {this->get_override("DoOperationAfterFilled");}
  void DoOperationAfterCancelled(Order* o) {this->get_override("DoOperationAfterCancelled");}

  double OrderPrice(std::string contract, OrderSide::Enum side, bool control_price) {this->get_override("OrderPrice");}
};

BOOST_PYTHON_MODULE(Strategy) {
  class_<StrategyWrap, boost::noncopyable>("StrategyWrap", init<>())
    .def("Start", pure_virtual(&BaseStrategy::Start))
    .def("debug", &BaseStrategy::debug)
    .def("Clear", &BaseStrategy::Clear)
    .def("SendPlainText", &BaseStrategy::SendPlainText)
    .def("Print", &BaseStrategy::Print)
    .def("RequestQryPos", &BaseStrategy::RequestQryPos)
    .def("UpdateData", &BaseStrategy::UpdateData)
    .def("UpdateExchangeInfo", &BaseStrategy::UpdateExchangeInfo)
    .def("InitTicker", pure_virtual(&BaseStrategy::InitTicker))
    .def("InitTimer", pure_virtual(&BaseStrategy::InitTimer))
    .def("Stop", pure_virtual(&BaseStrategy::Stop));

  class_<Strategy, bases<BaseStrategy> >("Strategy", init<const libconfig::Setting&, TimeController, std::tr1::unordered_map<std::string, std::vector<BaseStrategy*> >*, std::string>())
    .def("debug", &Strategy::debug);

  def("factoryS", factoryS, return_value_policy<manage_new_object>());
}
