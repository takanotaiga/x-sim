module shared_value_locking {
  type Snapshot = {
    hasValue: bool,
    value: int,
    cycle: int,
    sequence: int,
  }

  type Memory = {
    hasValue: bool,
    value: int,
    cycle: int,
    sequence: int,
    history: Set[Snapshot],
  }

  type LockState = {
    readerHeld: bool,
    writerHeld: bool,
  }

  type WriterState = {
    pc: int,
    pendingValue: int,
    pendingCycle: int,
  }

  type ReaderState = {
    pc: int,
    hasValue: bool,
    value: int,
    cycle: int,
    sequence: int,
    completed: Set[Snapshot],
  }

  pure val VALUES = Set(1, 2)
  pure val CYCLES = 0.to(2)
  pure val MAX_SEQUENCE = 3
  pure val NO_VALUE = 0

  pure def snapshot(hasValue: bool, value: int, cycle: int, sequence: int): Snapshot = {
    hasValue: hasValue,
    value: value,
    cycle: cycle,
    sequence: sequence,
  }

  pure val emptySnapshot = snapshot(false, NO_VALUE, 0, 0)

  pure def memorySnapshot(m: Memory): Snapshot =
    snapshot(m.hasValue, m.value, m.cycle, m.sequence)

  pure def readerSnapshot(r: ReaderState): Snapshot =
    snapshot(r.hasValue, r.value, r.cycle, r.sequence)

  var memory: Memory
  var lock: LockState
  var writer: WriterState
  var reader: ReaderState

  action init = all {
    memory' = {
      hasValue: false,
      value: NO_VALUE,
      cycle: 0,
      sequence: 0,
      history: Set(emptySnapshot),
    },
    lock' = {
      readerHeld: false,
      writerHeld: false,
    },
    writer' = {
      pc: 0,
      pendingValue: NO_VALUE,
      pendingCycle: 0,
    },
    reader' = {
      pc: 0,
      hasValue: false,
      value: NO_VALUE,
      cycle: 0,
      sequence: 0,
      completed: Set(),
    },
  }

  action writerAcquire = {
    nondet newValue = VALUES.oneOf()
    nondet taskCycle = CYCLES.oneOf()

    all {
      writer.pc == 0,
      memory.sequence < MAX_SEQUENCE,
      not(lock.readerHeld),
      not(lock.writerHeld),
      memory' = memory,
      lock' = {
        readerHeld: false,
        writerHeld: true,
      },
      writer' = {
        pc: 1,
        pendingValue: newValue,
        pendingCycle: taskCycle,
      },
      reader' = reader,
    }
  }

  action writerStoreValue = all {
    writer.pc == 1,
    lock.writerHeld,
    memory' = {
      hasValue: true,
      value: writer.pendingValue,
      cycle: memory.cycle,
      sequence: memory.sequence,
      history: memory.history,
    },
    lock' = lock,
    writer' = {
      pc: 2,
      pendingValue: writer.pendingValue,
      pendingCycle: writer.pendingCycle,
    },
    reader' = reader,
  }

  action writerStoreCycle = all {
    writer.pc == 2,
    lock.writerHeld,
    memory' = {
      hasValue: memory.hasValue,
      value: memory.value,
      cycle: writer.pendingCycle,
      sequence: memory.sequence,
      history: memory.history,
    },
    lock' = lock,
    writer' = {
      pc: 3,
      pendingValue: writer.pendingValue,
      pendingCycle: writer.pendingCycle,
    },
    reader' = reader,
  }

  action writerStoreSequence = {
    val nextSnapshot = snapshot(true, memory.value, memory.cycle, memory.sequence + 1)

    all {
      writer.pc == 3,
      lock.writerHeld,
      memory' = {
        hasValue: memory.hasValue,
        value: memory.value,
        cycle: memory.cycle,
        sequence: memory.sequence + 1,
        history: memory.history.union(Set(nextSnapshot)),
      },
      lock' = lock,
      writer' = {
        pc: 4,
        pendingValue: writer.pendingValue,
        pendingCycle: writer.pendingCycle,
      },
      reader' = reader,
    }
  }

  action writerRelease = all {
    writer.pc == 4,
    lock.writerHeld,
    memory' = memory,
    lock' = {
      readerHeld: false,
      writerHeld: false,
    },
    writer' = {
      pc: 0,
      pendingValue: NO_VALUE,
      pendingCycle: 0,
    },
    reader' = reader,
  }

  action readerAcquire = all {
    reader.pc == 0,
    not(lock.readerHeld),
    not(lock.writerHeld),
    memory' = memory,
    lock' = {
      readerHeld: true,
      writerHeld: false,
    },
    writer' = writer,
    reader' = {
      pc: 1,
      hasValue: reader.hasValue,
      value: reader.value,
      cycle: reader.cycle,
      sequence: reader.sequence,
      completed: reader.completed,
    },
  }

  action readerLoadValue = all {
    reader.pc == 1,
    lock.readerHeld,
    memory' = memory,
    lock' = lock,
    writer' = writer,
    reader' = {
      pc: 2,
      hasValue: memory.hasValue,
      value: memory.value,
      cycle: reader.cycle,
      sequence: reader.sequence,
      completed: reader.completed,
    },
  }

  action readerLoadCycle = all {
    reader.pc == 2,
    lock.readerHeld,
    memory' = memory,
    lock' = lock,
    writer' = writer,
    reader' = {
      pc: 3,
      hasValue: reader.hasValue,
      value: reader.value,
      cycle: memory.cycle,
      sequence: reader.sequence,
      completed: reader.completed,
    },
  }

  action readerLoadSequence = all {
    reader.pc == 3,
    lock.readerHeld,
    memory' = memory,
    lock' = lock,
    writer' = writer,
    reader' = {
      pc: 4,
      hasValue: reader.hasValue,
      value: reader.value,
      cycle: reader.cycle,
      sequence: memory.sequence,
      completed: reader.completed,
    },
  }

  action readerRelease = all {
    reader.pc == 4,
    lock.readerHeld,
    memory' = memory,
    lock' = {
      readerHeld: false,
      writerHeld: false,
    },
    writer' = writer,
    reader' = {
      pc: 0,
      hasValue: false,
      value: NO_VALUE,
      cycle: 0,
      sequence: 0,
      completed: reader.completed.union(Set(readerSnapshot(reader))),
    },
  }

  action step = any {
    writerAcquire,
    writerStoreValue,
    writerStoreCycle,
    writerStoreSequence,
    writerRelease,
    readerAcquire,
    readerLoadValue,
    readerLoadCycle,
    readerLoadSequence,
    readerRelease,
  }

  val typeOk = all {
    not(lock.readerHeld and lock.writerHeld),
    writer.pc.in(0.to(4)),
    reader.pc.in(0.to(4)),
    memory.sequence >= 0,
    memory.sequence <= MAX_SEQUENCE,
    memory.hasValue == (memory.sequence > 0 or writer.pc.in(Set(2, 3))),
    memory.history.contains(emptySnapshot),
    memory.history.forall(s => s.sequence >= 0),
    memory.history.forall(s => s.sequence <= memory.sequence),
    memory.history.forall(s => s.hasValue == (s.sequence > 0)),
  }

  val lockExclusion = all {
    lock.writerHeld == writer.pc.in(1.to(4)),
    lock.readerHeld == reader.pc.in(1.to(4)),
    not(lock.readerHeld and lock.writerHeld),
  }

  val stableWhenReadable = all {
    not(lock.writerHeld).implies(memorySnapshot(memory).in(memory.history)),
    reader.pc.in(1.to(4)).implies(memorySnapshot(memory).in(memory.history)),
  }

  val completedReadsAreAtomic =
    reader.completed.forall(s => s.in(memory.history))
}
