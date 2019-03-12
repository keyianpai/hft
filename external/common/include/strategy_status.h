#ifndef STRATEGY_STATUS_H_
#define STRATEGY_STATUS_H_

struct StrategyStatus {
  enum Enum {
    Init,
    Training,
    Running,
    Pause,
    Stopped,
    Flatting,
    ForceFlat
  };

  static inline const char* ToString(Enum status) {
    switch (status) {
     case Init:
       return "init";
       break;

     case Training:
       return "Training";
       break;

     case Running:
       return "Running";
       break;

     case Pause:
       return "Pause";
       break;

     case Stopped:
       return "Stopped";
       break;

     case Flatting:
       return "Flatting";
       break;

     case ForceFlat:
       return "ForceFlat";
       break;

     default:
       return "Unknown";
       break;
    }
  }
};

#endif  // STRATEGY_STATUS_H_
