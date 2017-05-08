// Long-term locks for processes
struct sleeplock {
  uint locked;       // Is the lock held?
  // For debugging:
  char *name;        // Name of lock.
  int pid;           // Process holding lock
};

