#ifndef TIME_STATUS_H_
#define TIME_STATUS_H_

struct TimeStatus {
  enum Enum {
    Valid,
    Pause,
    Close,
    Training,
    ForceClose
  };

  static inline const char* ToString(Enum status) {
    switch (status) {
     case Valid:
       return "Valid";
       break;

     case Pause:
       return "Pause";
       break;

     case Close:
       return "Close";
       break;

     case Training:
       return "Training";
       break;

     case ForceClose:
       return "ForceClose";
       break;

     default:
       return "Unknown";
       break;
    }
  }
};

#endif  // TIME_STATUS_H_
